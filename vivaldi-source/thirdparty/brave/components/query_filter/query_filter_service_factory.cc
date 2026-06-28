// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#include "brave/components/query_filter/query_filter_service_factory.h"
#include "brave/components/query_filter/query_filter_service.h"

namespace query_filter {

// static
QueryFilterService* QueryFilterServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<QueryFilterService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
QueryFilterServiceFactory* QueryFilterServiceFactory::GetInstance() {
  return base::Singleton<QueryFilterServiceFactory>::get();
}

QueryFilterServiceFactory::QueryFilterServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "QueryFilterBoombasticService",
          BrowserContextDependencyManager::GetInstance()) {
}

QueryFilterServiceFactory::~QueryFilterServiceFactory() = default;

content::BrowserContext* QueryFilterServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

std::unique_ptr<KeyedService>
QueryFilterServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  auto service = std::make_unique<QueryFilterService>();
  service->Load(profile);
  return service;
}

bool QueryFilterServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

bool QueryFilterServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace query_filter
