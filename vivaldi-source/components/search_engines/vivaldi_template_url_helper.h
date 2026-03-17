// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_SEARCH_ENGINES_VIVALDI_TEMPLATE_URL_HELPER_H_
#define COMPONENTS_SEARCH_ENGINES_VIVALDI_TEMPLATE_URL_HELPER_H_

#include <string>

#include "components/search_engines/template_url.h"

#define VIVALDI_TEMPLATE_URL_HANDLE_REPLACEMENTS            \
  case VIVALDI_KAGI_TOKEN:                                  \
    vivaldi::TemplateURLHelper::HandleKagiTokenReplacement( \
        this, search_terms_data, replacement, &url);        \
    break;

namespace vivaldi {

class TemplateURLHelper {
 public:
  static void ParseParameters(const std::string_view parameter,
                              size_t start,
                              size_t end,
                              std::string* url,
                              TemplateURLRef::Replacements* replacements);

  static void HandleKagiTokenReplacement(
      const TemplateURLRef* template_url_ref,
      const SearchTermsData& search_terms_data,
      const TemplateURLRef::Replacement& replacement,
      std::string* url);
};

}  // namespace vivaldi

#endif  // COMPONENTS_SEARCH_ENGINES_VIVALDI_TEMPLATE_URL_HELPER_H_
