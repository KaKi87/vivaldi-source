// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#import "components/user_agent/vivaldi_ios_user_agent_override.h"

#import "base/strings/stringprintf.h"
#import "components/version_info/version_info_values.h"
#import "ios/web/common/user_agent.h"
#import "url/gurl.h"

namespace vivaldi_ios_user_agent {

namespace {

// List of domains allowed to receive a modified Vivaldi iOS User-Agent.
// If the requested URL matches one of these domains, the override logic will
// apply.
constexpr const char* kVivaiOSAllowedDomains[] = {
    "cartaidentita.interno.gov.it",  // Italian digital identity site
    "digid.nl",                      // Dutch government digital identity site
    "mitid.dk",                      // Danish government digital identity site
    "posteid.poste.it",              // Poste Italiane digital identity site
    "spid.gov.it",  // Italian Public System for Digital Identity
};

// Product tag for the Vivaldi iOS User-Agent.
constexpr char kVivaiOSProductTagWithPlaceholder[] = "VivaiOS/%s";

bool IsURLAllowed(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }

  for (const auto& domain : kVivaiOSAllowedDomains) {
    if (url.DomainIs(domain)) {
      return true;
    }
  }
  return false;
}

std::string GetVivaiOSUserAgent(web::UserAgentType type) {
  std::string product =
      base::StringPrintf(kVivaiOSProductTagWithPlaceholder, VIVALDI_UA_VERSION);
  return type == web::UserAgentType::DESKTOP
             ? web::BuildDesktopUserAgent(product)
             : web::BuildMobileUserAgent(product);
}

}  // namespace

std::string GetUserAgentForURL(web::UserAgentType type,
                               const GURL& url,
                               std::string default_user_agent) {
  if (!IsURLAllowed(url)) {
    return default_user_agent;
  }
  return GetVivaiOSUserAgent(type);
}

}  // namespace vivaldi_ios_user_agent
