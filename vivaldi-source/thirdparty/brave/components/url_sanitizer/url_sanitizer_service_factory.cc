// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#include "brave/components/url_sanitizer/url_sanitizer_service_factory.h"

#include "brave/components/url_sanitizer/url_sanitizer_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace url_sanitizer {

// static
URLSanitizerService* URLSanitizerServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<URLSanitizerService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
URLSanitizerServiceFactory* URLSanitizerServiceFactory::GetInstance() {
  return base::Singleton<URLSanitizerServiceFactory>::get();
}

URLSanitizerServiceFactory::URLSanitizerServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "URLSanitizerService",
          BrowserContextDependencyManager::GetInstance()) {}

URLSanitizerServiceFactory::~URLSanitizerServiceFactory() = default;

content::BrowserContext* URLSanitizerServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

std::unique_ptr<KeyedService>
URLSanitizerServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  auto service = std::make_unique<URLSanitizerService>();
  service->Load(profile);
  return service;
}

bool URLSanitizerServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

bool URLSanitizerServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace url_sanitizer
