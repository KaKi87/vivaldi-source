// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_AD_BLOCKER_CORE_ADBLOCK_RULE_SOURCE_HANDLER_H_
#define COMPONENTS_AD_BLOCKER_CORE_ADBLOCK_RULE_SOURCE_HANDLER_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/timer/timer.h"
#include "components/ad_blocker/core/parser/parse_result.h"
#include "components/ad_blocker/public/core/adblock_types.h"

namespace content {
class BrowserContext;
}

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace adblock_filter {

class RuleSourceHandler {
 public:
  using OnUpdateCallback = base::RepeatingCallback<void(RuleSourceHandler*)>;
  using RulesCompiler =
      base::RepeatingCallback<bool(const ParseResult& parse_result,
                                   const RuleSourceSettings& source_settings,
                                   const base::FilePath& output_path,
                                   std::string& checksum)>;

  RuleSourceHandler(
      RuleGroup group,
      ActiveRuleSource rule_source,
      const base::FilePath& profile_path,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      scoped_refptr<base::SequencedTaskRunner> file_task_runner,
      RulesCompiler rules_compiler,
      OnUpdateCallback on_update_callback);
  ~RuleSourceHandler();
  RuleSourceHandler(const RuleSourceHandler&) = delete;
  RuleSourceHandler& operator=(const RuleSourceHandler&) = delete;

  const ActiveRuleSource& rule_source() const { return rule_source_; }

  // Trigger an immediate attempt to download an updated version of the list. If
  // recompile needed is set and downloading fail, the last successful download
  // will be used for recompiling instead, to ensure continuity in situation
  // where compiled rules are obsoleted by an update. If a download was already
  // in progress, its result will be used and a new one won't be triggered.
  void FetchNow(bool recompile_needed);

  // Specifies a new compiler, if the compilation parameters have changed. This
  // will call FetchNow(true).
  void ResetCompiler(RulesCompiler rules_compiler);

  // Remove the rules list file associated with this data source.
  void Clear();

 private:
  struct RulesReadResult;

  void StartUpdateTimer();

  void DoFetch();
  void DownloadRules();
  void OnRulesDownloaded(base::FilePath file);
  void OnDownloadReplaced(bool success);
  void ReadRulesFromFile(bool from_previous_download,
                         const base::FilePath& file);
  void OnRulesRead(bool from_previous_download, RulesReadResult result);

  static RulesReadResult ReadRules(const base::FilePath& source_path,
                                   const base::FilePath& output_path,
                                   RulesCompiler rules_compiler,
                                   RuleSourceSettings source_settings);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  RulesCompiler rules_compiler_;
  OnUpdateCallback on_update_callback_;
  ActiveRuleSource rule_source_;
  base::FilePath rules_list_path_;
  std::optional<base::FilePath> download_path_;

  bool try_recompile_from_previous_download_;

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  base::OneShotTimer update_timer_;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  base::WeakPtrFactory<RuleSourceHandler> weak_factory_;
};

}  // namespace adblock_filter

#endif  // COMPONENTS_AD_BLOCKER_CORE_ADBLOCK_RULE_SOURCE_HANDLER_H_
