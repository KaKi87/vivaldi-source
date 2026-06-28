// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include <memory>
#include <string>

#include "brave/components/debounce/core/browser/debounce_service_factory.h"

#include "brave/components/debounce/core/browser/debounce_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "chrome/browser/profiles/incognito_helpers.h"

namespace debounce {

// static
DebounceService* DebounceServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<DebounceService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
DebounceServiceFactory* DebounceServiceFactory::GetInstance() {
  return base::Singleton<DebounceServiceFactory>::get();
}

DebounceServiceFactory::DebounceServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "DebounceService",
          BrowserContextDependencyManager::GetInstance()) {}

DebounceServiceFactory::~DebounceServiceFactory() = default;

content::BrowserContext* DebounceServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

std::unique_ptr<KeyedService>
DebounceServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  auto service = std::make_unique<DebounceService>(profile->GetPrefs());
  service->Load(profile);
  return service;
}

bool DebounceServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

bool DebounceServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace debounce
