// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_DEBOUNCE_CORE_BROWSER_DEBOUNCE_SERVICE_FACTORY_H_
#define COMPONENTS_DEBOUNCE_CORE_BROWSER_DEBOUNCE_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace debounce {

class DebounceService;

class DebounceServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static DebounceService* GetForBrowserContext(
      content::BrowserContext* context);
  static DebounceServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<DebounceServiceFactory>;

  DebounceServiceFactory();
  ~DebounceServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace debounce

#endif  // COMPONENTS_DEBOUNCE_CORE_BROWSER_DEBOUNCE_SERVICE_FACTORY_H_
