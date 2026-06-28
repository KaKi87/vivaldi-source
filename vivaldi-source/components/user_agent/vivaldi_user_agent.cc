// Copyright (c) 2017 Vivaldi Technologies AS. All rights reserved

#include "components/user_agent/vivaldi_user_agent.h"

#include "app/vivaldi_apptools.h"
#include "base/command_line.h"
#include "base/containers/fixed_flat_set.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_local.h"
#include "components/browser/vivaldi_brand_select.h"
#include "components/google/core/common/google_util.h"
#include "components/version_info/version_info_values.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "url/gurl.h"
#include "vivaldi/base/base/chrome_spoof_version.h"
#include "vivaldi/base/base/edge_version.h"

namespace vivaldi_user_agent {

#if BUILDFLAG(IS_IOS)
const char kVivaldiSuffix[] = " VivaiOS/" VIVALDI_UA_VERSION;
const char kVivaldiSuffixReduced[] = " VivaiOS/" VIVALDI_UA_VERSION_REDUCED;
#else
const char kVivaldiSuffix[] = " Vivaldi/" VIVALDI_UA_VERSION;
const char kVivaldiSuffixReduced[] = " Vivaldi/" VIVALDI_UA_VERSION_REDUCED;
#endif

namespace {

constexpr auto kVivaldiAllowedDomains =
    base::MakeFixedFlatSet<std::string_view>({
#include "components/user_agent/vivaldi_user_agent_allow_list.inc"
    });

// VB-113889: Disabled Edge overrides for the time being
// constexpr auto kVivaldiEdgeDomains =
//    base::MakeFixedFlatSet<std::string_view>({"bing.com"});

const char kEdgeSuffix[] = " Edg/" EDGE_FULL_VERSION;
const char kEdgeSuffixReduced[] = " Edg/" CHROME_PRODUCT_VERSION_REDUCED;

bool g_user_agent_switch_checked = false;
bool g_user_agent_switch_present = false;

#if BUILDFLAG(IS_ANDROID)
constexpr bool g_google_is_vivaldi_partner = false;
#else
constexpr bool g_google_is_vivaldi_partner = false;
#endif

template <typename Container>
bool MatchHost(std::string_view host, const Container& container) {
  const auto get_parent_host =
      [](const std::string_view host) -> std::string_view {
    const size_t dot_pos = host.find('.');
    return (dot_pos != std::string::npos && dot_pos + 1 < host.length())
               ? host.substr(dot_pos + 1)
               : "";
  };

  do {
    if (container.contains(host)) {
      return true;
    }
    host = get_parent_host(host);
  } while (!host.empty());
  return false;
}

bool HasUserAgentSwitch() {
  // If we have --user-agent switch, always respect it as if the allow-list
  // was cleared.
  if (!g_user_agent_switch_checked) {
    g_user_agent_switch_checked = true;
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    // Cannot use switches::kUserAgent from Chromium as that is in a wrong
    // library.
    if (command_line->HasSwitch("user-agent")) {
      g_user_agent_switch_present = true;
    }
  }
  return g_user_agent_switch_present;
}

bool IsGooglePartnerUrl(const GURL& url) {
  using namespace google_util;

  const auto is_google_disallowed_path = [](const auto& path) {
    // Try to keep the list short
    static const char* kGoogleDisallowedPaths[] = {
        "/travel",  // VB-108684
    };
    if (!path.empty()) {
      for (const auto& disallowed_path : kGoogleDisallowedPaths) {
        if (base::StartsWith(path, disallowed_path,
                             base::CompareCase::INSENSITIVE_ASCII)) {
          return true;
        }
      }
    }
    return false;
  };

  // Allow "[www.]google.<TLD>" domains if Google is our partner.
  // Disallow subdomains to not acidentally break e.g. Google Docs with our UA.
  // Disallow specific paths that are known to break.
  return g_google_is_vivaldi_partner &&
         IsGoogleDomainUrl(url, SubdomainPermission::DISALLOW_SUBDOMAIN,
                           PortPermission::DISALLOW_NON_STANDARD_PORTS) &&
         !is_google_disallowed_path(url.path());
}

bool InternaSpoofStableChromiumVersion(
    std::optional<GURL> direct_url = std::nullopt) {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  return false;  // TODO (VB-124387): At present don't spoof stable for
                 // Android/IOS
#else            // Desktop
  using namespace google_util;

  if (!vivaldi::IsVivaldiRunning()) {
    return false;
  }

  if (!SPOOF_CHROME_ACTIVE) {
    return false;
  }

  GURL test_url;

  if (direct_url.has_value()) {
    test_url = direct_url.value();
  } else {
    std::optional<GURL> scoped_url = ScopedVivaldiThreadURL::GetURLForThread();
    if (!scoped_url.has_value()) {
      // Default spoofing as we might have frames that are created with renderer
      // prefs that has no navigation to decide the spoof value. Better to only
      // not spoof on the sites we specify in isAllowed and isGoogle.
      return true;
    }
    test_url = scoped_url.value();
  }

  if (IsUrlAllowed(test_url)) {
    return false;
  }

  if (IsGoogleDomainUrl(test_url, SubdomainPermission::DISALLOW_SUBDOMAIN,
                        PortPermission::DISALLOW_NON_STANDARD_PORTS)) {
    return false;
  }

  return true;
#endif           // Desktop
}

}  // namespace

bool SpoofStableChromiumVersion(GURL url) {
  return InternaSpoofStableChromiumVersion(url);
}

std::optional<GURL>& ScopedVivaldiThreadURL::GetInstanceForThread() {
  static base::NoDestructor<base::ThreadLocalOwnedPointer<std::optional<GURL>>>
      instance;

  if (!instance->Get()) {
    instance->Set(base::WrapUnique(new std::optional<GURL>));
  }
  return *(instance->Get());
}

std::optional<GURL> ScopedVivaldiThreadURL::GetURLForThread() {
  return GetInstanceForThread();
}

ScopedVivaldiThreadURL::ScopedVivaldiThreadURL(GURL url) : url_(url) {
  std::optional<GURL> spoof;
  if (url_.is_valid() && url_.SchemeIsHTTPOrHTTPS()) {
    spoof = url_;
  }
  old_status = GetInstanceForThread();
  GetInstanceForThread() = spoof;
}

ScopedVivaldiThreadURL::~ScopedVivaldiThreadURL() {
  GetInstanceForThread() = old_status;
}

bool IsUrlAllowed(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS())
    return false;

  // If we have --user-agent switch, always respect it as if the allow-list
  // was cleared.
  if (HasUserAgentSwitch())
    return false;

  if (IsGooglePartnerUrl(url)) {
    return true;
  }

  return MatchHost(url.host(), kVivaldiAllowedDomains);
}

bool IsBingHost(std::string_view host) {
  if (host.length() == 0)
    return false;

  // If we have --user-agent switch, always respect it as if the allow-list
  // was cleared.
  if (HasUserAgentSwitch())
    return false;

  return false;  // MatchHost(host, kVivaldiEdgeDomains);
}

void UpdateAgentString(bool reduced, std::string& user_agent) {
  if (!vivaldi::IsVivaldiRunning())
    return;

  std::optional<GURL> scoped_url = ScopedVivaldiThreadURL::GetURLForThread();
  if (!scoped_url.has_value())
    return;

  GURL test_url = scoped_url.value();
  if (!test_url.is_valid() || !test_url.SchemeIsHTTPOrHTTPS())
    return;

  if (IsBingHost(test_url.host())) {
    user_agent += (reduced ? kEdgeSuffixReduced : kEdgeSuffix);
  }

  if (!IsUrlAllowed(test_url))
    return;

  user_agent += (reduced ? kVivaldiSuffixReduced : kVivaldiSuffix);
}

std::vector<std::string> GetVivaldiAllowlist() {
  std::vector<std::string> domain_allowlist;
  for (std::string_view domain : kVivaldiAllowedDomains) {
    domain_allowlist.emplace_back(std::string(domain));
  }

  return domain_allowlist;
}

std::vector<std::string> GetVivaldiEdgeList() {
  std::vector<std::string> edge_domain_list;
  /*
  for (std::string_view domain : kVivaldiEdgeDomains) {
    edge_domain_list.emplace_back(std::string(domain));
  }
  */

  return edge_domain_list;
}

std::string_view UpdateChromeProductString(std::string_view actual_product) {
  if (!InternaSpoofStableChromiumVersion())
    return actual_product;

  return "Chrome/" SPOOF_CHROME_VERSION_STRING_FULL;
}

std::string UpdateReducedChromeProductString(std::string actual_product) {
  if (!InternaSpoofStableChromiumVersion())
    return actual_product;

  return "Chrome/" SPOOF_CHROME_VERSION_STRING_REDUCED;
}

std::string UpdateChromeMajorVersionString(std::string_view actual_product) {
  if (!InternaSpoofStableChromiumVersion())
    return std::string(actual_product);

  return SPOOF_CHROME_MAJOR_VERSION_STRING;
}

std::string UpdateChromeFullVersionString(std::string_view actual_product) {
  if (!InternaSpoofStableChromiumVersion())
    return std::string(actual_product);

  return SPOOF_CHROME_VERSION_STRING_FULL;
}

}  // namespace vivaldi_user_agent
