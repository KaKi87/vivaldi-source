// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/core/adblock_rule_source_handler.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "components/ad_blocker/core/parser/adblock_ruleset_file_parser.h"
#include "components/ad_blocker/core/parser/ddg_rules_parser.h"
#include "components/ad_blocker/core/parser/parse_result.h"
#include "components/ad_blocker/core/utils.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/functional/overload.h"

namespace adblock_filter {

namespace {
constexpr base::TimeDelta kMinTimeBetweenUpdates = base::Hours(1);
constexpr base::TimeDelta kMaxTimeBetweenUpdates = base::Days(14);
constexpr base::TimeDelta kDefaultTimeBetweenUpdates = base::Days(1);
constexpr base::TimeDelta kUpdateTimeJitter = base::Minutes(30);
constexpr base::TimeDelta kInitialUpdateDelay = base::Minutes(1);

constexpr char kDownloadedSuffix[] = "_raw.txt";

base::Time CalculateNextUpdateTime(const ActiveRuleSource& source) {
  return source.last_update +
         std::min(std::max(source.parsed_metadata.expires.value_or(
                               kDefaultTimeBetweenUpdates),
                           kMinTimeBetweenUpdates),
                  kMaxTimeBetweenUpdates) +
         base::RandDouble() * kUpdateTimeJitter;
}

base::Time GetNextUpdateTimeAfterFailUpdate(base::Time last_update_time) {
  return last_update_time + kMinTimeBetweenUpdates +
         base::RandDouble() * kUpdateTimeJitter;
}

void ParseContent(const std::string& file_contents,
                  RuleSourceSettings source_settings,
                  ParseResult* parse_result) {
  JSONStringValueDeserializer serializer(file_contents);
  std::unique_ptr<base::Value> root(serializer.Deserialize(nullptr, nullptr));
  if (root.get()) {
    DuckDuckGoRulesParser(parse_result).Parse(*root);
    return;
  }

  RulesetFileParser(parse_result, source_settings).Parse(file_contents);
  return;
}

bool ReplaceDownload(const base::FilePath& temp_download,
                     const base::FilePath& destination) {
  if (!base::CreateDirectoryAndGetError(destination.DirName(), nullptr))
    return false;

  return base::Move(temp_download, destination);
}

}  // namespace

struct RuleSourceHandler::RulesReadResult {
 public:
  RulesReadResult() = default;
  ~RulesReadResult() = default;
  RulesReadResult(RulesReadResult&&) = default;
  RulesReadResult& operator=(RulesReadResult&&) = default;

  ReadResult result_type = ReadResult::kSuccess;
  ParsedMetadata metadata;
  std::string checksum;
};

/*static*/
RuleSourceHandler::RulesReadResult RuleSourceHandler::ReadRules(
    const base::FilePath& source_path,
    const base::FilePath& output_path,
    RulesCompiler rules_compiler,
    RuleSourceSettings source_settings) {
  RulesReadResult read_result;
  if (!base::PathExists(source_path)) {
    read_result.result_type = ReadResult::kFileNotFound;
    return read_result;
  }

  std::string file_contents;
  if (!base::ReadFileToString(source_path, &file_contents)) {
    read_result.result_type = ReadResult::kFileReadError;
    return read_result;
  }
  ParseResult parse_result;
  ParseContent(file_contents, source_settings, &parse_result);
  read_result.result_type = parse_result.empty() ? ReadResult::kFileUnsupported
                                                 : ReadResult::kSuccess;
  read_result.metadata = parse_result.metadata;

  if (read_result.result_type == ReadResult::kFileUnsupported) {
    // If the file used to have supported rules in a previous version, our
    // compiled copy of it is now obsolete, remove it.
    base::DeleteFile(output_path);
    // We want to return an empty checksum and expect it hasn't been set to
    // anything by this point.
    CHECK(read_result.checksum.empty());
  }

  if (read_result.result_type != ReadResult::kSuccess)
    return read_result;

  if (!rules_compiler.Run(parse_result, source_settings, output_path,
                          read_result.checksum))
    read_result.result_type = ReadResult::kFailedSavingParsedRules;

  return read_result;
}

RuleSourceHandler::RuleSourceHandler(
    RuleGroup group,
    ActiveRuleSource rule_source,
    const base::FilePath& profile_path,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    scoped_refptr<base::SequencedTaskRunner> file_task_runner,
    RulesCompiler rules_compiler,
    OnUpdateCallback on_update_callback)
    : url_loader_factory_(std::move(url_loader_factory)),
      rules_compiler_(rules_compiler),
      on_update_callback_(on_update_callback),
      rule_source_(rule_source),
      rules_list_path_(
          profile_path.Append(GetRulesFolderName())
              .Append(GetGroupFolderName(group))
              .AppendASCII(base::NumberToString(rule_source_.core.id()))),
      file_task_runner_(file_task_runner),
      weak_factory_(this) {
  if (std::holds_alternative<GURL>(rule_source_.core.source_location())) {
    download_path_ =
        profile_path.Append(GetRulesFolderName())
            .Append(GetGroupFolderName(group))
            .AppendASCII(base::NumberToString(rule_source_.core.id()) +
                         kDownloadedSuffix);
  }

  if (rule_source_.next_fetch == base::Time()) {
    rule_source_.next_fetch = CalculateNextUpdateTime(rule_source_);
  }

  StartUpdateTimer();
}

RuleSourceHandler::~RuleSourceHandler() = default;

void RuleSourceHandler::FetchNow(bool recompile_needed) {
  try_recompile_from_previous_download_ = recompile_needed;

  // We already have an update in progress.
  if (!update_timer_.IsRunning())
    return;

  update_timer_.FireNow();
}

// Specifies a new compiler, if the compilation parameters have changed. This
// will call FetchNow.
void RuleSourceHandler::ResetCompiler(RulesCompiler rules_compiler) {
  rules_compiler_ = rules_compiler;
  FetchNow(true);
}

void RuleSourceHandler::Clear() {
  // We'll probably get deleted soon at this point, but it's worth
  // making sure the file won't get re-created by then.
  update_timer_.Stop();

  file_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(base::GetDeleteFileCallback(rules_list_path_)));
  if (download_path_) {
    file_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(base::GetDeleteFileCallback(*download_path_)));
  }
}

void RuleSourceHandler::StartUpdateTimer() {
  update_timer_.Start(
      FROM_HERE,
      rule_source_.next_fetch > base::Time::Now()
          ? rule_source_.next_fetch - base::Time::Now()
          : kInitialUpdateDelay + base::RandDouble() * kUpdateTimeJitter,
      base::BindOnce(&RuleSourceHandler::DoFetch, base::Unretained(this)));
}

void RuleSourceHandler::DoFetch() {
  rule_source_.is_fetching = true;
  on_update_callback_.Run(this);

  std::visit(absl::Overload{
                 [&](GURL) { DownloadRules(); },
                 [&](base::FilePath path) { ReadRulesFromFile(false, path); }},
             rule_source_.core.source_location());
}

void RuleSourceHandler::DownloadRules() {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = std::get<GURL>(rule_source_.core.source_location());
  resource_request->method = "GET";
  resource_request->load_flags = net::LOAD_BYPASS_CACHE;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  // See
  // https://chromium.googlesource.com/chromium/src/+/lkgr/docs/network_traffic_annotations.md
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("vivaldi_adblock_rules", R"(
        semantics {
          sender: "Vivaldi Adblock Rules"
          description: "Download new or updated rules in the EasyList/AdBlock format."
          trigger: "Triggered when a new list is added or when an existing list is about to be out of date."
          data: "Adblock filter list in one of the Adblock format variants"
          destination: OTHER
        }
        policy {
          cookies_allowed: NO
          setting:
            "You can enable or disable this feature via the ad blcoker settings."
          chrome_policy {
            }
          }
        })");

  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 traffic_annotation);

  url_loader_->SetRetryOptions(
      2, network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);

  url_loader_->DownloadToTempFile(
      url_loader_factory_.get(),
      base::BindOnce(&RuleSourceHandler::OnRulesDownloaded,
                     base::Unretained(this)));
}

void RuleSourceHandler::OnRulesDownloaded(base::FilePath file) {
  std::unique_ptr<network::SimpleURLLoader> url_loader;
  url_loader_.swap(url_loader);

  if (file.empty()) {
    rule_source_.last_download_result = DownloadResult::kDownloadFailed;
    LOG(WARNING) << "Downloading rule source:"
                 << std::get<GURL>(rule_source_.core.source_location())
                 << " failed with error " << url_loader->NetError();

    rule_source_.is_fetching = false;
    rule_source_.next_fetch =
        GetNextUpdateTimeAfterFailUpdate(base::Time::Now());
    StartUpdateTimer();
    on_update_callback_.Run(this);

    if (try_recompile_from_previous_download_) {
      ReadRulesFromFile(true, *download_path_);
    }
    return;
  }

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReplaceDownload, std::move(file), *download_path_),
      base::BindOnce(&RuleSourceHandler::OnDownloadReplaced,
                     weak_factory_.GetWeakPtr()));
}

void RuleSourceHandler::OnDownloadReplaced(bool success) {
  if (!success) {
    rule_source_.last_download_result = DownloadResult::kReplaceFailed;
    rule_source_.next_fetch =
        GetNextUpdateTimeAfterFailUpdate(base::Time::Now());
    StartUpdateTimer();
    return;
  }
  rule_source_.last_download_result = DownloadResult::kSuccess;
  ReadRulesFromFile(false, *download_path_);
}
void RuleSourceHandler::ReadRulesFromFile(bool from_previous_download,
                                          const base::FilePath& file) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&RuleSourceHandler::ReadRules, file, rules_list_path_,
                     rules_compiler_, rule_source_.core.settings()),
      base::BindOnce(&RuleSourceHandler::OnRulesRead,
                     weak_factory_.GetWeakPtr(), from_previous_download));
}

void RuleSourceHandler::OnRulesRead(bool from_previous_download,
                                    RulesReadResult result) {
  try_recompile_from_previous_download_ = false;

  rule_source_.last_read_result = result.result_type;
  rule_source_.is_fetching = false;
  bool read_success =
      rule_source_.last_read_result == ReadResult::kSuccess ||
      rule_source_.last_read_result == ReadResult::kFileUnsupported;

  if (read_success) {
    rule_source_.parsed_metadata = result.metadata;
    rule_source_.rules_list_checksum = result.checksum;
    rule_source_.last_update = base::Time::Now();
  }

  // If we are recompiling from a previous download, there was a download
  // failure. Set the next update time accordingly.
  if (!read_success || from_previous_download) {
    rule_source_.next_fetch =
        GetNextUpdateTimeAfterFailUpdate(base::Time::Now());

  } else {
    rule_source_.next_fetch = CalculateNextUpdateTime(rule_source_);
  }
  StartUpdateTimer();
  on_update_callback_.Run(this);
}
}  // namespace adblock_filter
