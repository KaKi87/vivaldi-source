// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved
#include "components/search_engines/vivaldi_template_url_helper.h"

#include "components/search_engines/template_url.h"

namespace vivaldi {

void TemplateURLHelper::ParseParameters(
    const std::string_view parameter,
    size_t start,
    size_t end,
    std::string* url,
    TemplateURLRef::Replacements* replacements) {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  constexpr bool is_mobile = true;
#else
  constexpr bool is_mobile = false;
#endif

  if (parameter == "ddg:Referral") {
    url->insert(start, is_mobile ? "t=vivaldim" : "t=vivaldi");
  } else if (parameter == "yandex:vivaldiReferralID") {
    url->insert(start, is_mobile ? "clid=2358106" : "clid=2207714");
  } else if (parameter == "vivaldi:kagiToken") {
    replacements->push_back(TemplateURLRef::Replacement(
        TemplateURLRef::ReplacementType::VIVALDI_KAGI_TOKEN, start));
  }
}

void TemplateURLHelper::HandleKagiTokenReplacement(
    const TemplateURLRef* template_url_ref,
    const SearchTermsData& search_terms_data,
    const TemplateURLRef::Replacement& replacement,
    std::string* url) {
  const std::string_view kagi_token = search_terms_data.VivaldiGetKagiToken();
  if (!kagi_token.empty()) {
    template_url_ref->HandleReplacement("token", std::string(kagi_token),
                                        replacement, url);
  }
}
}  // namespace vivaldi
