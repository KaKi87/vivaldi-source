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
  return GetUaMetaDataOverrideCommon(
      domain_ua_metadata_override, hostname,
      return_main_metadata ? &ua_metadata_override : nullptr);
}

/* static */
std::optional<UserAgentMetadata> UserAgentOverride::GetUaMetaDataOverrideGlobal(
        const std::string& hostname) {
  return GetUaMetaDataOverrideCommon(*main_domain_ua_metadata_override, hostname);
}

/* static */
std::optional<UserAgentMetadata> UserAgentOverride::GetUaMetaDataOverrideCommon(
      const base::flat_map<std::string, UserAgentMetadata>&
          use_domain_ua_metadata_override,
    const std::string& hostname,
    const std::optional<UserAgentMetadata>* return_main_metadata) {
  using namespace google_util;

  if (!hostname.empty() && use_domain_ua_metadata_override.size()) {
    std::string_view name(hostname);
    while (name.find('.') != std::string_view::npos) {
      auto it = use_domain_ua_metadata_override.find(name);
      if (it != use_domain_ua_metadata_override.end()) {
        return it->second;
      }
      name.remove_prefix(name.find('.') + 1);
    }
  }

  // If an explicit per-WebContents override is set (e.g. Android desktop mode,
  // DevTools UA spoof), honor it instead of falling back to the global
  // @@OTHER.DOMAINS@@ / @@GOOGLE.DOMAIN@@ entries below. Ref. VAB-12975.
  if (return_main_metadata) {
    return *return_main_metadata;
  }

  if (IsGoogleDomainUrl((GURL("https://" + hostname)),
                        SubdomainPermission::DISALLOW_SUBDOMAIN,
                        PortPermission::DISALLOW_NON_STANDARD_PORTS)) {
    auto it = use_domain_ua_metadata_override.find("@@GOOGLE.DOMAIN@@");
    if (it != use_domain_ua_metadata_override.end()) {
      return it->second;
    }
  } else {
    auto it = use_domain_ua_metadata_override.find("@@OTHER.DOMAINS@@");
    if (it != use_domain_ua_metadata_override.end()) {
      return it->second;
    }
  }

  return std::nullopt;
}

/* static */
void UserAgentOverride::AddGetUaMetaDataOverride(
    const std::string& domainname,
    const UserAgentMetadata& metadata) {
  main_domain_ua_metadata_override.get()->emplace(domainname, metadata);
}
}  // namespace blink
