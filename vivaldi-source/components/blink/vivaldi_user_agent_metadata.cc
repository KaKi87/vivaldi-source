// Copyright (c) 2023 Vivaldi Technologies AS. All rights reserved
// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"

#include <string_view>

#include "base/no_destructor.h"
#include "components/google/core/common/google_util.h"
#include "url/gurl.h"

namespace blink {

namespace {
base::NoDestructor<base::flat_map<std::string, UserAgentMetadata>>
                               main_domain_ua_metadata_override;
}

UserAgentOverride::UserAgentOverride() {
  domain_ua_metadata_override = *main_domain_ua_metadata_override.get();
}

UserAgentOverride::UserAgentOverride(const UserAgentOverride& old) = default;

std::optional<UserAgentMetadata> UserAgentOverride::GetUaMetaDataOverride(
    const std::string& hostname,
    bool return_main_metadata) const {
  using namespace google_util;

  if (!hostname.empty() && domain_ua_metadata_override.size()) {
    std::string_view name(hostname);
    while (name.find('.') != std::string_view::npos) {
      auto it = domain_ua_metadata_override.find(name);
      if (it != domain_ua_metadata_override.end()) {
        return it->second;
      }
      name.remove_prefix(name.find('.') + 1);
    }
  }

  if (IsGoogleDomainUrl((GURL("https://" + hostname)),
                        SubdomainPermission::DISALLOW_SUBDOMAIN,
                        PortPermission::DISALLOW_NON_STANDARD_PORTS)) {
    auto it = domain_ua_metadata_override.find("@@GOOGLE.DOMAIN@@");
    if (it != domain_ua_metadata_override.end()) {
      return it->second;
    }
  } else {
    auto it = domain_ua_metadata_override.find("@@OTHER.DOMAINS@@");
    if (it != domain_ua_metadata_override.end()) {
      return it->second;
    }
  }


  if (!return_main_metadata) {
    return std::nullopt;
  }
  return ua_metadata_override;
}

/* static */
void UserAgentOverride::AddGetUaMetaDataOverride(
    const std::string& domainname,
    const UserAgentMetadata& metadata) {
  main_domain_ua_metadata_override.get()->emplace(domainname, metadata);
}
}  // namespace blink
