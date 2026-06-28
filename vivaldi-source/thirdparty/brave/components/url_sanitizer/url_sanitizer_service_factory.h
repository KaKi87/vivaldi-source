// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_
#define COMPONENTS_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace url_sanitizer {

class URLSanitizerService;

class URLSanitizerServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static URLSanitizerService* GetForBrowserContext(
      content::BrowserContext* context);
  static URLSanitizerServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<URLSanitizerServiceFactory>;

  URLSanitizerServiceFactory();
  ~URLSanitizerServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace url_sanitizer

#endif  // COMPONENTS_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_
