// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "brave/common/cached_rule_service.h"

#include <memory>
#include <string>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

#if !BUILDFLAG(IS_IOS)
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/storage_partition.h"
#endif

namespace {

// Base URL for all rule-set downloads.
constexpr char kBaseUrl[] = "https://downloads.vivaldi.com/lists/brave/";

constexpr int64_t kWaitMs = 5 * 1000; // Wait till starting download.

// Backoff policy for download retries.
constexpr net::BackoffEntry::Policy kBackoffPolicy = {
    // Number of initial errors to ignore before backoff.
    0,
    // Initial delay in ms.
    5000,
    // Multiplier.
    2,
    // Fuzzing percentage.
    0.1,
    // Maximum delay in ms (5 minutes).
    1000 * 60 * 5,
    // Never discard entries.
    -1,
    // Don't use initial delay unless last request failed.
    false,
};

// Update interval after successful download (24 hours).
constexpr base::TimeDelta kUpdateInterval = base::Hours(24);

// Maximum response size for a rule-set download.
constexpr int kMaxResponseSize = 1 * 1024 * 1024;  // 1 MB.

// Thread-safe helper to read a file from disk.
std::string ReadFileFromDisk(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    LOG(WARNING) << "CachedRuleService: Failed reading cached rules from "
                 << path.value();
    return {};
  }
  return contents;
}

// Atomically move |temp_download| to |destination|, creating the parent
// directory if needed.
bool ReplaceDownload(const base::FilePath& temp_download,
                     const base::FilePath& destination) {
  if (!base::CreateDirectoryAndGetError(destination.DirName(), nullptr))
    return false;
  return base::Move(temp_download, destination);
}

}  // namespace

CachedRuleService::CachedRuleService() : backoff_entry_(&kBackoffPolicy) {}

CachedRuleService::~CachedRuleService() = default;

void CachedRuleService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void CachedRuleService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

#if BUILDFLAG(IS_IOS)
void CachedRuleService::Load(
    const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
    PrefService* prefs) {
  // iOS-specific initialization if needed in the future.
  LOG(ERROR) << "CachedRuleService: Load not implemented for iOS";
}
#else
void CachedRuleService::Load(Profile* profile) {
  url_loader_factory_ = profile->GetDefaultStoragePartition()
                            ->GetURLLoaderFactoryForBrowserProcess();

  cache_dir_ = profile->GetPath().AppendASCII("query_filter");
  cache_file_path_ = cache_dir_.AppendASCII(CacheFileName());

  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(base::IgnoreResult(&base::CreateDirectory), cache_dir_));

  // Try loading from cache first so we have data available immediately.
  LoadFromCache();

  // Delay the first download to allow the network service to fully initialize.
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CachedRuleService::StartDownload,
                     weak_ptr_factory_.GetWeakPtr()),
      base::Milliseconds(kWaitMs));
}
#endif  // IS_IOS

void CachedRuleService::StartDownload() {
  if (!url_loader_factory_) {
    LOG(WARNING) << "CachedRuleService: No URL loader factory available";
    return;
  }

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("vivaldi_cached_rule_fetcher", R"(
        semantics {
          sender: "Vivaldi Cached Rule Fetcher"
          description:
            "This request is used to fetch signed rule-set data."
          trigger:
            "This request is triggered at browser startup and periodically
             after to check for updated rules."
          data:
            "Rule set data in JSON format."
          destination: OTHER
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled in settings."
          chrome_policy {
          }
        })");

  const GURL url(kBaseUrl + RelativeUrlPath());
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = "GET";
  resource_request->load_flags = net::LOAD_BYPASS_CACHE;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  VLOG(1) << "CachedRuleService: Downloading rule from " << url;

  download_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);

  download_loader_->SetRetryOptions(
      2, network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);

  download_loader_->DownloadToTempFile(
      url_loader_factory_.get(),
      base::BindOnce(&CachedRuleService::OnDownloadDone,
                     weak_ptr_factory_.GetWeakPtr()),
      kMaxResponseSize);
}

void CachedRuleService::OnDownloadDone(base::FilePath file) {
  std::unique_ptr<network::SimpleURLLoader> loader;
  download_loader_.swap(loader);

  if (file.empty()) {
    backoff_entry_.InformOfRequest(false);

    // Log detailed diagnostic info.
    int net_error = loader->NetError();
    LOG(WARNING) << "CachedRuleService: Download failed: net_error="
                 << net_error << " (" << net::ErrorToString(net_error) << ")";

    // Retry after backoff delay.
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&CachedRuleService::StartDownload,
                       weak_ptr_factory_.GetWeakPtr()),
        backoff_entry_.GetTimeUntilRelease());
    return;
  }

  // Atomically move the temp file to the cache location.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReplaceDownload, std::move(file), cache_file_path_),
      base::BindOnce(&CachedRuleService::OnDownloadReplaced,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CachedRuleService::OnDownloadReplaced(bool success) {
  if (!success) {
    backoff_entry_.InformOfRequest(false);

    LOG(ERROR) << "CachedRuleService: Failed to move downloaded file to cache";

    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&CachedRuleService::StartDownload,
                       weak_ptr_factory_.GetWeakPtr()),
        backoff_entry_.GetTimeUntilRelease());
    return;
  }

  LOG(INFO) << "CachedRuleService: Downloaded and replaced rule file "
            << cache_file_path_;

  // Read the downloaded file from disk and parse.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadFileFromDisk, cache_file_path_),
      base::BindOnce(&CachedRuleService::OnDownloadLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CachedRuleService::OnDownloadLoaded(std::string downloaded_data) {
  ParseAndNotify(downloaded_data);
  ScheduleNextDownload();
}

void CachedRuleService::LoadFromCache() {
  if (!base::PathExists(cache_file_path_)) {
    VLOG(1) << "CachedRuleService: No cached file found at "
            << cache_file_path_.value();
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadFileFromDisk, cache_file_path_),
      base::BindOnce(&CachedRuleService::OnCacheLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CachedRuleService::OnCacheLoaded(std::string cached_data) {
  if (cached_data.empty()) {
    VLOG(1) << "CachedRuleService: Cached data is empty";
    return;
  }

  ParseAndNotify(cached_data);
}


void CachedRuleService::ParseAndNotify(const std::string &data) {
  if (data.empty()) {
    LOG(WARNING) << "CachedRuleService: Downloaded file is empty: "
                 << CacheFileName();
    return;
  }

  // Parse and apply rules via derived-class override.
  if (ParseAndApplyRules(data)) {
    is_loaded_ = true;
    for (Observer& observer : observers_) {
      observer.OnRulesLoaded(this);
    }
  } else {
    LOG(WARNING) << "CachedRuleService: Failed to parse downloaded rule data "
                 << CacheFileName();
  }
}

void CachedRuleService::ScheduleNextDownload() {
  backoff_entry_.InformOfRequest(true);
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CachedRuleService::StartDownload,
                     weak_ptr_factory_.GetWeakPtr()),
      kUpdateInterval);
}
