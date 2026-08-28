// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/user_agent/vivaldi_user_agent_config_service_factory.h"

#include "chrome/browser/browser_process.h"
#include "components/browser/vivaldi_brand_select.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_agent/vivaldi_user_agent_config_service.h"
#include "prefs/vivaldi_pref_names.h"

namespace vivaldi_user_agent {

// static
VivaldiUserAgentConfigService*
VivaldiUserAgentConfigServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<VivaldiUserAgentConfigService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
VivaldiUserAgentConfigServiceFactory*
VivaldiUserAgentConfigServiceFactory::GetInstance() {
  return base::Singleton<VivaldiUserAgentConfigServiceFactory>::get();
}

VivaldiUserAgentConfigServiceFactory::VivaldiUserAgentConfigServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "VivaldiUserAgentConfigService",
          BrowserContextDependencyManager::GetInstance()) {}

VivaldiUserAgentConfigServiceFactory::~VivaldiUserAgentConfigServiceFactory() {}

std::unique_ptr<KeyedService>
VivaldiUserAgentConfigServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<VivaldiUserAgentConfigService>(
      g_browser_process->local_state());
}

content::BrowserContext*
VivaldiUserAgentConfigServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Make sure the service exist in incognito mode.
  return context;
}

void VivaldiUserAgentConfigServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {

  registry->RegisterIntegerPref(vivaldiprefs::kVivaldiClientHintsBrand,
                                int(vivaldi::BrandSelection::kChromeBrand));
  registry->RegisterBooleanPref(
      vivaldiprefs::kVivaldiClientHintsBrandAppendVivaldi, false);
  registry->RegisterStringPref(
      vivaldiprefs::kVivaldiClientHintsBrandCustomBrand, "");
  registry->RegisterStringPref(
      vivaldiprefs::kVivaldiClientHintsBrandCustomBrandVersion, "");

}
}  // namespace vivaldi
