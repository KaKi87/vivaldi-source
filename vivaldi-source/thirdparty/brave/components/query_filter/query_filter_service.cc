// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "brave/components/query_filter/query_filter_service.h"

#include <string>

#include "base/logging.h"
#include "base/version.h"
#include "brave/components/query_filter/browser/query_filter_data.h"

namespace query_filter {

QueryFilterService::QueryFilterService() {}

QueryFilterService::~QueryFilterService() = default;

std::string QueryFilterService::CacheFileName() const {
  return "query_filter.json";
}

std::string QueryFilterService::RelativeUrlPath() const {
  return "query-filter-current.json";
}

const std::vector<schema::Rule>& QueryFilterService::rules() const {
  static const std::vector<schema::Rule> empty_rules;
  auto* data = QueryFilterData::GetInstance();
  return data ? data->rules() : empty_rules;
}

std::string QueryFilterService::GetVersion() const {
  auto* data = QueryFilterData::GetInstance();
  return data ? data->GetVersion() : std::string();
}

bool QueryFilterService::ParseAndApplyRules(const std::string& json_data) {
  auto* data = QueryFilterData::GetInstance();
  if (data && data->PopulateDataFromComponent(json_data)) {
    VLOG(1) << "QueryFilter: Query filter rules loaded";
    return true;
  }
  LOG(WARNING) << "QueryFilter: Failed to populate query filter data";
  return false;
}

}  // namespace query_filter
