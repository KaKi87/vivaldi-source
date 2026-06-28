// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef VIVALDI_QUERY_FILTER_SERVICE_H
#define VIVALDI_QUERY_FILTER_SERVICE_H

#include <string>
#include <vector>

#include "brave/common/cached_rule_service.h"
#include "brave/components/query_filter/common/features.h"
#include "brave/components/query_filter/common/schema.h"

namespace query_filter {

class QueryFilterService : public CachedRuleService {
 public:
  QueryFilterService();
  ~QueryFilterService() override;

  QueryFilterService(const QueryFilterService&) = delete;
  QueryFilterService& operator=(const QueryFilterService&) = delete;

  // Returns the current set of query filter rules.
  const std::vector<schema::Rule>& rules() const;

  // Returns the version string of the currently loaded query filter data.
  std::string GetVersion() const;

 private:
  // CachedRuleService overrides.
  std::string CacheFileName() const override;
  std::string RelativeUrlPath() const override;
  bool ParseAndApplyRules(const std::string& json_data) override;
};

}  // namespace query_filter

#endif  // VIVALDI_QUERY_FILTER_SERVICE_H
