// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef VIVALDI_QUERY_FILTER_SERVICE_FACTORY_H_
#define VIVALDI_QUERY_FILTER_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace query_filter {

class QueryFilterService;

class QueryFilterServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static QueryFilterService* GetForBrowserContext(
      content::BrowserContext* context);
  static QueryFilterServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<QueryFilterServiceFactory>;

  QueryFilterServiceFactory();
  ~QueryFilterServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace query_filter

#endif  // VIVALDI_QUERY_FILTER_SERVICE_FACTORY_H_
