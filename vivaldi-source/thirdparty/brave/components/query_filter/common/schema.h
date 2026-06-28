// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_COMMON_SCHEMA_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_COMMON_SCHEMA_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "base/types/expected.h"

namespace query_filter {
namespace schema {

// Schema for the query filter rule, written to match what would be generated from schema.idl
// Keep in sync with schema.idl and adblock-lists/brave-lists/query-filter.json.
struct Rule {
  Rule();
  ~Rule();
  Rule(const Rule&) = delete;
  Rule& operator=(const Rule&) = delete;
  Rule(Rule&& rhs) noexcept;
  Rule& operator=(Rule&& rhs) noexcept;

  // Populates a Rule object from a DictValue. Returns whether |out| was
  // successfully populated. On failure, |error| is populated.
  static bool Populate(const base::DictValue& value,
                       Rule& out,
                       std::u16string& error);

  // Populates a Rule object from a Value. Returns whether |out| was
  // successfully populated. On failure, |error| is populated.
  static bool Populate(const base::Value& value,
                       Rule& out,
                       std::u16string& error);

  // Creates a deep copy of this Rule.
  Rule Clone() const;

  // Creates a Rule from a DictValue, or unexpected on failure.
  static base::expected<Rule, std::u16string> FromValue(
      const base::DictValue& value);

  // Creates a Rule from a Value, or unexpected on failure.
  static base::expected<Rule, std::u16string> FromValue(
      const base::Value& value);

  // The list of domains (wildcard supported) to consider to remove the query
  // params |params|.
  std::vector<std::string> include;
  // The list of domains (wildcard supported) on which NOT to exclude the query
  // params |params|.
  std::vector<std::string> exclude;
  // The list of query parameters which are subjected to query filtering.
  std::vector<std::string> params;
};

}  // namespace schema
}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_COMMON_SCHEMA_H_
