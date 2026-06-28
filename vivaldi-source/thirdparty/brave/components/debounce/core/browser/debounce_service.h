// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_DEBOUNCE_CORE_BROWSER_DEBOUNCE_SERVICE_H_
#define COMPONENTS_DEBOUNCE_CORE_BROWSER_DEBOUNCE_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "brave/common/cached_rule_service.h"
#include "brave/components/debounce/core/browser/debounce_rule.h"

class GURL;

namespace debounce {

// Debounce service that downloads and caches signed debounce rules,
// then applies them to URLs to remove tracking parameters.
class DebounceService : public CachedRuleService {
 public:
  explicit DebounceService(PrefService* prefs);

  DebounceService(const DebounceService&) = delete;
  DebounceService& operator=(const DebounceService&) = delete;
  ~DebounceService() override;

  // The main routine - applies debounce rules to a URL.
  bool Debounce(const GURL& original_url, GURL* final_url) const;

 private:
  // CachedRuleService overrides.
  std::string CacheFileName() const override;
  std::string RelativeUrlPath() const override;
  bool ParseAndApplyRules(const std::string& json_data) override;

  raw_ptr<PrefService> prefs_ = nullptr;

  std::vector<std::unique_ptr<DebounceRule>> rules_;
  base::flat_set<std::string> host_cache_;
};

}  // namespace debounce

#endif  // COMPONENTS_DEBOUNCE_CORE_BROWSER_DEBOUNCE_SERVICE_H_
