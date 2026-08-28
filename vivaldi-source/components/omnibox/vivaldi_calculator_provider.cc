// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/omnibox/vivaldi_calculator_provider.h"

#include <stddef.h>
#include <cmath>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/calculator/calculator.h"
#include "components/omnibox/browser/autocomplete_match.h"

#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/titled_url_match_utils.h"

VivaldiCalculatorProvider::VivaldiCalculatorProvider(
    AutocompleteProviderClient* client)
    : AutocompleteProvider(AutocompleteProvider::TYPE_VIVALDI_CALCULATOR),
      client_(client) {}

VivaldiCalculatorProvider::~VivaldiCalculatorProvider() = default;

void VivaldiCalculatorProvider::Start(const AutocompleteInput& input,
                                      bool minimal_changes) {
  matches_.clear();

  if (input.text().empty())
    return;

  std::u16string trimmed_input = base::CollapseWhitespace(input.text(), false);

  std::string utf8_query = base::UTF16ToUTF8(trimmed_input);

  if (!vivaldi::calculator::IsMathQuery(utf8_query)) {
    return;
  }

  std::optional<double> result = vivaldi::calculator::Evaluate(utf8_query);

  // If parsing fails. (syntax error), abort silently.
  if (!result.has_value()) {
    return;
  }

  std::u16string result_formatted;
  if (std::isnan(result.value())) {
    result_formatted = u"undefined";
  } else {
    result_formatted = base::NumberToString16(result.value());
  }

  AutocompleteMatch match(this, 3105, false,
                          AutocompleteMatchType::VIVALDI_CALCULATOR);

  match.contents_class.push_back(
      {0, AutocompleteMatch::ACMatchClassification::DIM});

  match.contents_class.push_back(AutocompleteMatch::ACMatchClassification(
      trimmed_input.length(), AutocompleteMatch::ACMatchClassification::MATCH));

  match.fill_into_edit = result_formatted;

  size_t end_pos = trimmed_input.find_last_not_of(u'=');
  if (end_pos != std::u16string::npos) {
    trimmed_input.erase(end_pos + 1);
  } else {
    trimmed_input.clear();
  }

  match.contents = base::StrCat({trimmed_input, u" = ", result_formatted});

  match.description_class.push_back(
      {0, AutocompleteMatch::ACMatchClassification::NONE});

  match.allowed_to_be_default_match = true;

  matches_.push_back(match);
}
