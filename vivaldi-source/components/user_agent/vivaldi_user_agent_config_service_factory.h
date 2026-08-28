// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENT_VIVALDI_USER_AGENT_CONFIG_FACTORY_H_
#define COMPONENT_VIVALDI_USER_AGENT_CONFIG_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace vivaldi_user_agent {

class VivaldiUserAgentConfigService;

class VivaldiUserAgentConfigServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static VivaldiUserAgentConfigService* GetForBrowserContext(
      content::BrowserContext* context);
  static VivaldiUserAgentConfigServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      VivaldiUserAgentConfigServiceFactory>;

  VivaldiUserAgentConfigServiceFactory();
  ~VivaldiUserAgentConfigServiceFactory() override;
  VivaldiUserAgentConfigServiceFactory(
      const VivaldiUserAgentConfigServiceFactory&) =
      delete;
  VivaldiUserAgentConfigServiceFactory& operator=(
      const VivaldiUserAgentConfigServiceFactory&) =
      delete;

  // BrowserContextKeyedBaseFactory methods:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

}  // namespace vivaldi_user_agent

#endif  // COMPONENT_VIVALDI_USER_AGENT_CONFIG_FACTORY_H_
