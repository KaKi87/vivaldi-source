// Copyright 2023 Vivaldi Technologies. All rights reserved.

#import "ios/ui/ad_tracker_blocker/manager/vivaldi_atb_manager_bridge.h"

#import <Foundation/Foundation.h>

#import "base/check.h"
#import "base/notreached.h"
#import "ios/chrome/browser/ad_blocker/adblock_rule_service_factory.h"

namespace vivaldi_adblocker {

VivaldiATBManagerBridge::VivaldiATBManagerBridge(
    id<VivaldiATBConsumer> observer,
    RuleService* ruleService)
    : observer_(observer), rule_service_(ruleService) {
  DCHECK(observer_);
  DCHECK(rule_service_);
  rule_service_->AddObserver(this);
  if (rule_service_->IsLoaded()) {
    this->StartObservingRuleSourceManager();
  }
}

VivaldiATBManagerBridge::~VivaldiATBManagerBridge() {
  if (rule_service_) {
    if (rule_service_->IsLoaded()) {
      rule_service_->GetRuleManager()->RemoveObserver(this);
      rule_service_->GetKnownSourcesHandler()->RemoveObserver(this);
    }
    rule_service_->RemoveObserver(this);
  }
}

void VivaldiATBManagerBridge::OnRuleServiceStateLoaded(
    RuleService* rule_service) {
  this->StartObservingRuleSourceManager();
}

void VivaldiATBManagerBridge::OnStartApplyingRules(RuleGroup group) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(rulesListDidStartApplying:)]) {
    [observer rulesListDidStartApplying:group];
  }
}

void VivaldiATBManagerBridge::OnDoneApplyingRules(RuleGroup group) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(rulesListDidEndApplying:)]) {
    [observer rulesListDidEndApplying:group];
  }
}

void VivaldiATBManagerBridge::OnRuleSourceUpdated(
    RuleGroup group,
    const ActiveRuleSource& rule_source) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector
                (ruleSourceDidUpdate:group:fetchResult:)]) {
    ATBFetchResult fetchResult =
        this->FlattenFetchResult(rule_source.last_download_result,
            rule_source.last_read_result);
    [observer ruleSourceDidUpdate:rule_source.core.id()
                            group:group
                      fetchResult:fetchResult];
  }
}

void VivaldiATBManagerBridge::OnRuleSourceDeleted(uint32_t source_id,
                                                  RuleGroup group) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(ruleSourceDidRemove:group:)]) {
    [observer ruleSourceDidRemove:source_id group:group];
  }
}

void VivaldiATBManagerBridge::OnExceptionListStateChanged(RuleGroup group) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(exceptionListStateDidChange:)]) {
    [observer exceptionListStateDidChange:group];
  }
}

void VivaldiATBManagerBridge::OnExceptionListChanged(
    RuleGroup group,
    RuleManager::ExceptionsList list) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(exceptionListDidChange:list:)]) {
    [observer exceptionListDidChange:group list:list];
  }
}

void VivaldiATBManagerBridge::OnKnownSourceAdded(
    RuleGroup group,
    const KnownRuleSource& rule_source) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(knownSourceDidAdd:key:)]) {
    [observer knownSourceDidAdd:group key:rule_source.core.id()];
  }
}

void VivaldiATBManagerBridge::OnKnownSourceRemoved(RuleGroup group,
                                                   uint32_t source_id) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(knownSourceDidRemove:key:)]) {
    [observer knownSourceDidRemove:group key:source_id];
  }
}

void VivaldiATBManagerBridge::OnKnownSourceEnabled(RuleGroup group,
                                                   uint32_t source_id) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(knownSourceDidEnable:key:)]) {
    [observer knownSourceDidEnable:group key:source_id];
  }
}

void VivaldiATBManagerBridge::OnKnownSourceDisabled(RuleGroup group,
                                                    uint32_t source_id) {
  id<VivaldiATBConsumer> observer = observer_;
  if (!observer)
    return;

  if ([observer respondsToSelector:@selector(knownSourceDidDisable:key:)]) {
    [observer knownSourceDidDisable:group key:source_id];
  }
}

void VivaldiATBManagerBridge::StartObservingRuleSourceManager() {
  if (rule_service_) {
    rule_service_->GetRuleManager()->AddObserver(this);
    rule_service_->GetKnownSourcesHandler()->AddObserver(this);

    id<VivaldiATBConsumer> observer = observer_;
    if (!observer)
      return;

    if ([observer respondsToSelector:@selector(ruleServiceStateDidLoad)]) {
      [observer ruleServiceStateDidLoad];
    }
  }
}

ATBFetchResult VivaldiATBManagerBridge::FlattenFetchResult(
    std::optional<DownloadResult> download_result,
    std::optional<ReadResult> read_result) {
  if (!read_result || !download_result) {
      return FetchResultUnknown;
  }

  if (*download_result != DownloadResult::kSuccess) {
      return FetchResultDownloadFailed;
  }

  switch (*read_result) {
    case ReadResult::kSuccess:
      return FetchResultSuccess;
    case ReadResult::kFileNotFound:
      return FetchResultFileNotFound;
    case ReadResult::kFileReadError:
      return FetchResultFileReadError;
    case ReadResult::kFileUnsupported:
      return FetchResultFileUnsupported;
    case ReadResult::kFailedSavingParsedRules:
      return FetchResultFailedSavingParsedRules;
  }
}

}  // namespace vivaldi_adblocker
