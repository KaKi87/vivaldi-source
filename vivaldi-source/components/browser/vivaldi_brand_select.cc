// Copyright (c) 2023 Vivaldi Technologies AS. All rights reserved

#include "components/browser/vivaldi_brand_select.h"

#include "app/vivaldi_apptools.h"
#include "app/vivaldi_version_info.h"

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/version.h"
#include "components/embedder_support/user_agent_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_agent/vivaldi_user_agent.h"

#include "components/user_agent/vivaldi_ua_config_holder.h"

#include "components/version_info/version_info.h"
#include "components/version_info/version_info_values.h"

#include "prefs/vivaldi_pref_names.h"
#include "vivaldi/base/base/chrome_spoof_version.h"
#include "vivaldi/base/base/edge_version.h"

namespace vivaldi {

namespace {

const BrandConfiguration* g_brand_override(nullptr);

std::string GetVivaldiReleaseVersion() {
  return base::NumberToString(GetVivaldiVersion().components()[0]) + "." +
         base::NumberToString(GetVivaldiVersion().components()[1]);
}
}  // namespace

BrandOverride::BrandOverride(const BrandConfiguration brand_config)
    : brand_config_(std::move(brand_config)) {
  DCHECK(g_brand_override == nullptr);
  g_brand_override = &brand_config_;
}

BrandOverride::~BrandOverride() {
  DCHECK(g_brand_override == &brand_config_);
  g_brand_override = nullptr;
}

void SelectClientHintsBrand(std::optional<std::string>& brand,
                            std::string& major_version,
                            std::string& full_version) {
  if (!IsVivaldiRunning())
    return;

  ::vivaldi_user_agent::mojom::BrandConfiguration config =
      vivaldi_user_agent::UAConfigHolder::GetInstance()->GetConfig();

  BrandSelection brand_selection =
      g_brand_override ? g_brand_override->brand : BrandSelection(config.brand);

  switch (brand_selection) {
    case BrandSelection::kChromeBrand:
      brand.emplace("Google Chrome");
      break;

    case BrandSelection::kEdgeBrand:
      brand.emplace("Microsoft Edge");
      full_version = EDGE_FULL_VERSION;
      break;

    case BrandSelection::kVivaldiBrand:
      brand.emplace("Vivaldi");
      major_version = GetVivaldiReleaseVersion();
      full_version = GetVivaldiVersionString();
      break;

    case BrandSelection::kCustomBrand: {
      std::string custom_brand =
          g_brand_override
              ? g_brand_override->custom_brand
              : config.custom_brand;
      std::string custom_brand_version =
          g_brand_override
              ? g_brand_override->custom_brand_version
              : config.custom_brand_version;

      if (!custom_brand.empty() && !custom_brand_version.empty()) {
        brand.emplace(custom_brand);
        major_version = custom_brand_version;
        full_version = custom_brand_version;
      }
    } break;

    case BrandSelection::kNoBrand:
      break;
  }
}

void UpdateBrands(
    std::optional<blink::UserAgentBrandVersion>& additional_brand_version) {
  if (additional_brand_version.has_value())
    return;

  if (!IsVivaldiRunning())
    return;

  ::vivaldi_user_agent::mojom::BrandConfiguration config =
      vivaldi_user_agent::UAConfigHolder::GetInstance()->GetConfig();

  BrandSelection brand_selection =
      g_brand_override ? g_brand_override->brand : BrandSelection(config.brand);

  if (brand_selection == BrandSelection::kVivaldiBrand)
    return;

  if (!(g_brand_override ? g_brand_override->specify_vivaldi_brand
                         : config.specify_vivaldi_brand))
    return;

  additional_brand_version =
      blink::UserAgentBrandVersion("Vivaldi", GetVivaldiReleaseVersion());
}

std::string GetBrandFullVersion() {
  if (!IsVivaldiRunning())
    return std::string(version_info::GetVersionNumber());

  ::vivaldi_user_agent::mojom::BrandConfiguration config =
      vivaldi_user_agent::UAConfigHolder::GetInstance()->GetConfig();

  BrandSelection brand_selection =
      g_brand_override ? g_brand_override->brand
                       : BrandSelection(config.brand);

  switch (brand_selection) {
    case BrandSelection::kEdgeBrand:
      return EDGE_FULL_VERSION;
    case BrandSelection::kVivaldiBrand:
      return std::string(GetVivaldiVersionString());

    case BrandSelection::kChromeBrand:
    case BrandSelection::kCustomBrand:
    case BrandSelection::kNoBrand:
      return std::string(version_info::GetVersionNumber());
  }
}

void ConfigureClientHintsOverrides() {
  {
    BrandOverride brand_override(
        BrandConfiguration({BrandSelection::kVivaldiBrand, false, {}, {}}));

    for (auto domain : vivaldi_user_agent::GetVivaldiAllowlist()) {
      {
        vivaldi_user_agent::ScopedVivaldiThreadURL site(
            (GURL("https://" + domain)));
        blink::UserAgentOverride::AddGetUaMetaDataOverride(
            std::string(domain), embedder_support::GetUserAgentMetadata());
      }
    }
  }

  {
    BrandOverride brand_override(
        BrandConfiguration({BrandSelection::kEdgeBrand, false, {}, {}}));

    for (auto domain : vivaldi_user_agent::GetVivaldiEdgeList()) {
      {
        vivaldi_user_agent::ScopedVivaldiThreadURL site(
            (GURL("https://" + domain)));
        blink::UserAgentOverride::AddGetUaMetaDataOverride(
            domain, embedder_support::GetUserAgentMetadata());
      }
    }
  }

  if (SPOOF_CHROME_ACTIVE) {
    {
      vivaldi_user_agent::ScopedVivaldiThreadURL site(
          (GURL("https://google.com")));
      blink::UserAgentOverride::AddGetUaMetaDataOverride(
          "@@GOOGLE.DOMAIN@@", embedder_support::GetUserAgentMetadata());
    }

    {
      vivaldi_user_agent::ScopedVivaldiThreadURL site(
          (GURL("https://example.net")));
      blink::UserAgentOverride::AddGetUaMetaDataOverride(
          "@@OTHER.DOMAINS@@", embedder_support::GetUserAgentMetadata());
    }
  }
}

}  // namespace vivaldi
