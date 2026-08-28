// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/user_agent/vivaldi_user_agent_config_service.h"

#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/user_agent/vivaldi_ua_config_holder.h"
#include "components/user_agent/vivaldi_user_agent_config_service_factory.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "prefs/vivaldi_pref_names.h"

namespace vivaldi_user_agent {

// static
VivaldiUserAgentConfigService* VivaldiUserAgentConfigService::Get(
    content::BrowserContext* context) {
  return VivaldiUserAgentConfigServiceFactory::GetForBrowserContext(context);
}

VivaldiUserAgentConfigService::VivaldiUserAgentConfigService(
    PrefService* prefs) {
  prefs_ = prefs;
  pref_registrar_.Init(prefs_);
  pref_registrar_.Add(
      vivaldiprefs::kVivaldiClientHintsBrandAppendVivaldi,
      base::BindRepeating(&VivaldiUserAgentConfigService::OnPrefChanged,
                          base::Unretained(this)));

  pref_registrar_.Add(
      vivaldiprefs::kVivaldiClientHintsBrand,
      base::BindRepeating(&VivaldiUserAgentConfigService::OnPrefChanged,
                          base::Unretained(this)));

  pref_registrar_.Add(
      vivaldiprefs::kVivaldiClientHintsBrandCustomBrand,
      base::BindRepeating(&VivaldiUserAgentConfigService::OnPrefChanged,
                          base::Unretained(this)));

  pref_registrar_.Add(
      vivaldiprefs::kVivaldiClientHintsBrandCustomBrandVersion,
      base::BindRepeating(&VivaldiUserAgentConfigService::OnPrefChanged,
                          base::Unretained(this)));

  // Update now.
  UpdatePrefs();
}

VivaldiUserAgentConfigService::~VivaldiUserAgentConfigService() {
  pref_registrar_.RemoveAll();
}

void VivaldiUserAgentConfigService::UpdatePrefs() {
  mojom::BrandSelection brand = static_cast<mojom::BrandSelection>(
      prefs_->GetInteger(vivaldiprefs::kVivaldiClientHintsBrand));

  std::string custom_brand =
      prefs_->GetString(vivaldiprefs::kVivaldiClientHintsBrandCustomBrand);

  std::string custom_brand_version = prefs_->GetString(
      vivaldiprefs::kVivaldiClientHintsBrandCustomBrandVersion);
  current_config_.reset(new mojom::BrandConfiguration(
      brand,
      prefs_->GetBoolean(vivaldiprefs::kVivaldiClientHintsBrandAppendVivaldi),
      custom_brand, custom_brand_version));

  // Broadcast the new value to all connected renderers.
  for (auto& remote : remotes_) {
    remote->SetPreferences(current_config_->Clone());
  }
  // Update the browser-process instance.
    vivaldi_user_agent::UAConfigHolder::GetInstance()->SetConfig(
        *current_config_);
}

void VivaldiUserAgentConfigService::OnPrefChanged(const std::string& path) {
  UpdatePrefs();

  // Broadcast the new value to all connected renderers
  for (auto& remote : remotes_) {
    remote->SetPreferences(current_config_->Clone());
  }
  // Update the browser-process instance.
  vivaldi_user_agent::UAConfigHolder::GetInstance()->SetConfig(
      *current_config_);
}

void VivaldiUserAgentConfigService::OnRenderProcessHostCreated(
    content::RenderProcessHost* process_host) {

  mojo::Remote<mojom::VivaldiUserAgentConfig> remote;
  process_host->BindReceiver(remote.BindNewPipeAndPassReceiver());

  remote->SetPreferences(current_config_->Clone());
  // Note that this is removed on |Disconnect()|.
  remotes_.Add(std::move(remote));

}

}  // namespace vivaldi_user_agent
