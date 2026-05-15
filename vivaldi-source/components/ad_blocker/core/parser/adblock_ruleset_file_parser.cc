// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/core/parser/adblock_ruleset_file_parser.h"

#include "base/strings/string_split.h"
#include "components/ad_blocker/core/parser/parse_result.h"

namespace adblock_filter {

RulesetFileParser::RulesetFileParser(ParseResult* parse_result,
                                     RuleSourceSettings source_settings)
    : parse_result_(parse_result), parser_(parse_result, source_settings) {
  DCHECK_NE(nullptr, parse_result);
}
RulesetFileParser::~RulesetFileParser() = default;

void RulesetFileParser::Parse(std::string_view file_contents) {
  for (auto rule_string :
       base::SplitStringPiece(file_contents, "\r\n", base::TRIM_WHITESPACE,
                              base::SPLIT_WANT_NONEMPTY)) {
    ParseLine(rule_string);
  }
}

void RulesetFileParser::ParseLine(std::string_view string_piece) {
  switch (parser_.Parse(string_piece)) {
    case RuleParser::kMetadata:
    case RuleParser::kComment:
      break;
    case RuleParser::kUnsupported:
      parse_result_->metadata.unsupported_rules++;
      break;
    case RuleParser::kError:
      parse_result_->metadata.invalid_rules++;
      break;
    case RuleParser::kRequestFilterRule:
    case RuleParser::kCosmeticRule:
    case RuleParser::kScriptletInjectionRule:
      parse_result_->metadata.valid_rules++;
      break;
  }
}

}  // namespace adblock_filter
