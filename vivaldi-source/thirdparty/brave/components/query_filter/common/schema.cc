// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "brave/components/query_filter/common/schema.h"

#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

namespace query_filter {
namespace schema {

Rule::Rule() = default;
Rule::~Rule() = default;
Rule::Rule(Rule&& rhs) noexcept = default;
Rule& Rule::operator=(Rule&& rhs) noexcept = default;

bool Rule::Populate(const base::DictValue& dict,
                    Rule& out,
                    std::u16string& error) {
  // Populate include.
  const base::Value* include_value = dict.Find("include");
  if (include_value) {
    if (!include_value->is_list()) {
      error = u"'include': expected list, got " +
              base::UTF8ToUTF16(base::Value::GetTypeName(include_value->type()));
      return false;
    }
    for (const auto& it : include_value->GetList()) {
      const std::string* temp = it.GetIfString();
      if (!temp) {
        error = u"'include': expected string, got " +
                base::UTF8ToUTF16(base::Value::GetTypeName(it.type()));
        return false;
      }
      out.include.push_back(*temp);
    }
  }

  // Populate exclude.
  const base::Value* exclude_value = dict.Find("exclude");
  if (exclude_value) {
    if (!exclude_value->is_list()) {
      error = u"'exclude': expected list, got " +
              base::UTF8ToUTF16(base::Value::GetTypeName(exclude_value->type()));
      return false;
    }
    for (const auto& it : exclude_value->GetList()) {
      const std::string* temp = it.GetIfString();
      if (!temp) {
        error = u"'exclude': expected string, got " +
                base::UTF8ToUTF16(base::Value::GetTypeName(it.type()));
        return false;
      }
      out.exclude.push_back(*temp);
    }
  }

  // Populate params.
  const base::Value* params_value = dict.Find("params");
  if (params_value) {
    if (!params_value->is_list()) {
      error = u"'params': expected list, got " +
              base::UTF8ToUTF16(base::Value::GetTypeName(params_value->type()));
      return false;
    }
    for (const auto& it : params_value->GetList()) {
      const std::string* temp = it.GetIfString();
      if (!temp) {
        error = u"'params': expected string, got " +
                base::UTF8ToUTF16(base::Value::GetTypeName(it.type()));
        return false;
      }
      out.params.push_back(*temp);
    }
  }

  return true;
}

bool Rule::Populate(const base::Value& value,
                    Rule& out,
                    std::u16string& error) {
  if (!value.is_dict()) {
    error = u"expected dictionary, got " +
            base::UTF8ToUTF16(base::Value::GetTypeName(value.type()));
    return false;
  }
  return Populate(value.GetDict(), out, error);
}

Rule Rule::Clone() const {
  Rule out;
  out.include = include;
  out.exclude = exclude;
  out.params = params;
  return out;
}

base::expected<Rule, std::u16string> Rule::FromValue(
    const base::DictValue& value) {
  std::u16string error;
  Rule out;
  bool result = Populate(value, out, error);
  if (!result) {
    DCHECK(!error.empty());
    return base::unexpected(std::move(error));
  }
  return out;
}

base::expected<Rule, std::u16string> Rule::FromValue(
    const base::Value& value) {
  std::u16string error;
  Rule out;
  bool result = Populate(value, out, error);
  if (!result) {
    DCHECK(!error.empty());
    return base::unexpected(std::move(error));
  }
  return out;
}

}  // namespace schema
}  // namespace query_filter
