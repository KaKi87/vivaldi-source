// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.
// Adapted from brave/components/url_sanitizer/core/browser/url_sanitizer_service.h:
// stripped Brave-specific component installer, mojom bindings, and JS permission
// checks; converted to inherit CachedRuleService for download/cache management.

#ifndef COMPONENTS_URL_SANITIZER_URL_SANITIZER_SERVICE_H_
#define COMPONENTS_URL_SANITIZER_URL_SANITIZER_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "brave/common/cached_rule_service.h"
#include "extensions/common/url_pattern_set.h"

class GURL;

namespace url_sanitizer {

// Service that downloads and caches clean-urls rules, then strips tracking
// parameters from URLs via the SanitizeURL call.
class URLSanitizerService : public CachedRuleService {
 public:
  URLSanitizerService();
  ~URLSanitizerService() override;

  URLSanitizerService(const URLSanitizerService&) = delete;
  URLSanitizerService& operator=(const URLSanitizerService&) = delete;

  // Strip tracking query parameters from |url| and return the sanitized GURL.
  GURL SanitizeURL(const GURL& url) const;

 private:
  // CachedRuleService overrides.
  std::string CacheFileName() const override;
  std::string RelativeUrlPath() const override;
  bool ParseAndApplyRules(const std::string& json_data) override;

  struct MatchItem {
    extensions::URLPatternSet include;
    extensions::URLPatternSet exclude;
    base::flat_set<std::string> params;
  };

  // Ordered list of matchers to apply.
  std::vector<MatchItem> matchers_;
};

}  // namespace url_sanitizer

#endif  // COMPONENTS_URL_SANITIZER_URL_SANITIZER_SERVICE_H_
