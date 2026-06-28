// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.
// Adapted from brave/components/url_sanitizer/core/browser/url_sanitizer_service.cc:
// stripped Brave-specific component installer, mojom bindings, JS permission
// checks, and Android remote support; converted to inherit CachedRuleService
// for download/cache management.

#include "brave/components/url_sanitizer/url_sanitizer_service.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

namespace url_sanitizer {

namespace {

std::optional<extensions::URLPatternSet> CreateURLPatternSetFromList(
    const base::ListValue* value) {
  if (!value) {
    return std::nullopt;
  }
  extensions::URLPatternSet result;
  std::string error;
  bool valid = result.Populate(*value,
                               URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
                               false, &error);
  if (!valid) {
    VLOG(1) << "URLSanitizer: Unable to create url pattern: " << error;
    return std::nullopt;
  }
  return result;
}

std::optional<base::flat_set<std::string>> CreateParamsList(
    const base::ListValue* value) {
  if (!value) {
    return std::nullopt;
  }
  base::flat_set<std::string> result;
  for (const auto& param : *value) {
    DCHECK(param.is_string());
    result.insert(param.GetString());
  }
  return result;
}

// Remove tracking query parameters from a query string, leaving all other
// parts untouched.
// Original: brave/components/url_sanitizer/core/browser/url_sanitizer_service.cc
std::string StripQueryParameter(
    std::string_view query,
    const base::flat_set<std::string>& trackers) {
  // We are using custom query string parsing code here. See
  // https://github.com/brave/brave-core/pull/13726#discussion_r897712350
  // for more information on why this approach was selected.
  //
  // Split query string by ampersands, remove tracking parameters,
  // then join the remaining query parameters, untouched, back into
  // a single query string.
  const std::vector<std::string_view> input_kv_strings =
      base::SplitStringPiece(query, "&", base::KEEP_WHITESPACE,
                             base::SPLIT_WANT_ALL);
  std::vector<std::string> output_kv_strings;
  int disallowed_count = 0;
  for (const std::string_view kv_string : input_kv_strings) {
    const std::vector<std::string_view> pieces =
        base::SplitStringPiece(kv_string, "=", base::KEEP_WHITESPACE,
                               base::SPLIT_WANT_NONEMPTY);
    std::string_view key = pieces.empty() ? std::string_view() : pieces[0];
    if (pieces.size() >= 2 && trackers.count(key) == 1) {
      ++disallowed_count;
    } else {
      output_kv_strings.emplace_back(std::string(kv_string));
    }
  }
  if (disallowed_count > 0) {
    return base::JoinString(output_kv_strings, "&");
  }
  return std::string(query);
}

}  // namespace

URLSanitizerService::URLSanitizerService() = default;
URLSanitizerService::~URLSanitizerService() = default;

std::string URLSanitizerService::CacheFileName() const {
  return "clean-urls.json";
}

std::string URLSanitizerService::RelativeUrlPath() const {
  return "clean-urls-current.json";
}

bool URLSanitizerService::ParseAndApplyRules(const std::string& json_data) {
  auto parsed_json = base::JSONReader::ReadAndReturnValueWithError(
      json_data, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed_json.has_value()) {
    LOG(WARNING) << "URLSanitizer: Error parsing JSON: "
                 << parsed_json.error().message;
    return false;
  }

  const auto* matchers_list = parsed_json->GetIfList();
  if (!matchers_list) {
    LOG(WARNING) << "URLSanitizer: Root is not a list";
    return false;
  }

  matchers_.clear();

  // Parse "matchers" array.
  for (const auto& item : *matchers_list) {
    const auto* dict = item.GetIfDict();
    if (!dict) {
      continue;
    }

    auto include = CreateURLPatternSetFromList(dict->FindList("include"));
    if (!include) {
      continue;
    }

    auto params = CreateParamsList(dict->FindList("params"));
    if (!params) {
      continue;
    }

    auto exclude = CreateURLPatternSetFromList(dict->FindList("exclude"))
                       .value_or(extensions::URLPatternSet());

    matchers_.emplace_back(std::move(*include), std::move(exclude),
                           std::move(*params));
  }

  return true;
}

GURL URLSanitizerService::SanitizeURL(const GURL& initial_url) const {
  if (matchers_.empty() || !initial_url.SchemeIsHTTPOrHTTPS()) {
    return initial_url;
  }

  GURL url = initial_url;
  for (const auto& matcher : matchers_) {
    if (!matcher.include.MatchesURL(url) || matcher.exclude.MatchesURL(url)) {
      continue;
    }

    const std::string sanitized_query =
        StripQueryParameter(url.query(), matcher.params);
    GURL::Replacements replacements;
    if (!sanitized_query.empty()) {
      replacements.SetQueryStr(sanitized_query);
    } else {
      replacements.ClearQuery();
    }
    url = url.ReplaceComponents(replacements);
  }

  return url;
}

}  // namespace url_sanitizer
