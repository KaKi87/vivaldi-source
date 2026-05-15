// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_AD_BLOCKER_CORE_PARSER_PARSE_RESULT_H_
#define COMPONENTS_AD_BLOCKER_CORE_PARSER_PARSE_RESULT_H_

#include "components/ad_blocker/core/parser/adblock_content_injection_rule.h"
#include "components/ad_blocker/core/parser/adblock_request_filter_rule.h"
#include "components/ad_blocker/public/core/adblock_types.h"

namespace adblock_filter {
struct ParseResult {
 public:
  ParseResult();
  ~ParseResult();
  ParseResult(ParseResult&&);
  ParseResult& operator=(ParseResult&&);

  bool empty() {
    return request_filter_rules.empty() && cosmetic_rules.empty() &&
           scriptlet_injection_rules.empty();
  }

  ParsedMetadata metadata;
  RequestFilterRules request_filter_rules;
  CosmeticRules cosmetic_rules;
  ScriptletInjectionRules scriptlet_injection_rules;
};

}  // namespace adblock_filter
#endif  // COMPONENTS_AD_BLOCKER_CORE_PARSER_PARSE_RESULT_H_
