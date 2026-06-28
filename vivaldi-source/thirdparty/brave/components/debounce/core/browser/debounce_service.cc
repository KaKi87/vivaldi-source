// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "brave/components/debounce/core/browser/debounce_service.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "url/gurl.h"

namespace debounce {

DebounceService::DebounceService(PrefService* prefs) : prefs_{prefs} {}
DebounceService::~DebounceService() = default;

std::string DebounceService::CacheFileName() const { return "debounce.json"; }

std::string DebounceService::RelativeUrlPath() const {
  return "debounce-current.json";
}

bool DebounceService::ParseAndApplyRules(const std::string& json_data) {
  auto parsed_rules = DebounceRule::ParseRules(json_data);
  if (!parsed_rules.has_value()) {
    LOG(WARNING) << "Debounce: Failed parsing rules: " << parsed_rules.error();
    return false;
  }

  rules_ = std::move(parsed_rules.value().first);
  host_cache_ = parsed_rules.value().second;
  return true;
}

bool DebounceService::Debounce(const GURL& original_url,
                               GURL* final_url) const {
  // Check host cache to see if this URL needs to have any debounce rules
  // applied.
  const std::string etldp1 =
      DebounceRule::GetETLDForDebounce(original_url.host());
  if (!host_cache_.contains(etldp1)) {
    return false;
  }

  for (const std::unique_ptr<DebounceRule>& rule : rules_) {
    if (rule->Apply(original_url, final_url, prefs_)) {
      if (original_url != *final_url) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace debounce
