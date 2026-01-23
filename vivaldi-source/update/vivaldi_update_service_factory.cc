// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vivaldi_update_service_factory.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/prefs/pref_service.h"

#include "prefs/vivaldi_pref_names.h"
#include "update/vivaldi_update_service.h"

namespace update {

VivaldiUpdateServiceFactory::VivaldiUpdateServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "VivaldiUpdateService",
          BrowserContextDependencyManager::GetInstance()) {}

VivaldiUpdateServiceFactory::~VivaldiUpdateServiceFactory() {}

// static
VivaldiUpdateService* VivaldiUpdateServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<VivaldiUpdateService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
VivaldiUpdateService* VivaldiUpdateServiceFactory::GetForProfileIfExists(
    Profile* profile,
    ServiceAccessType sat) {
  return static_cast<VivaldiUpdateService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

// static
VivaldiUpdateService* VivaldiUpdateServiceFactory::GetForProfileWithoutCreating(
    Profile* profile) {
  return static_cast<VivaldiUpdateService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

// static
VivaldiUpdateServiceFactory* VivaldiUpdateServiceFactory::GetInstance() {
  return base::Singleton<VivaldiUpdateServiceFactory>::get();
}

// static
void VivaldiUpdateServiceFactory::ShutdownForProfile(Profile* profile) {
  VivaldiUpdateServiceFactory* factory = GetInstance();
  factory->BrowserContextDestroyed(profile);
}

content::BrowserContext* VivaldiUpdateServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

std::unique_ptr<KeyedService>
VivaldiUpdateServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  std::unique_ptr<update::VivaldiUpdateService> update_service(
      new update::VivaldiUpdateService());

  if (!update_service->Init()) {
    return nullptr;
  }
  return update_service;
}

bool VivaldiUpdateServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace update
