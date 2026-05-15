// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#import "ios/ui/settings/addressbar/vivaldi_addressbar_settings_prefs.h"

#import "components/pref_registry/pref_registry_syncable.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"
#import "vivaldi/prefs/vivaldi_gen_prefs.h"

namespace {
NSString* const kShouldMigrateSearchSuggestionsPref =
    @"kShouldMigrateSearchSuggestionsPref";
}  // namespace

@implementation VivaldiAddressBarSettingsPrefs

+ (void)registerBrowserStatePrefs:(user_prefs::PrefRegistrySyncable*)registry {
  // Register the iOS specific prefs here.
  // The prefs common to all three platforms could be already registered in the
  // backend. So double check before registering it here.

  // Keep old swipe key registered for one-time migration only.
  registry->RegisterBooleanPref(
      vivaldiprefs::kVivaldiAddressBarSwipeGestureEnabledLegacy, false);
  registry->RegisterBooleanPref(
      vivaldiprefs::kVivaldiAddressBarSwipeGestureEnabled, false);
}

+ (void)registerLocalStatePrefs:(PrefRegistrySimple*)registry {
  registry->RegisterBooleanPref(vivaldiprefs::kVivaldiShowFullAddressEnabled,
                                NO);
  registry->RegisterBooleanPref(vivaldiprefs::kVivaldiShowXForSuggestionEnabled,
                                NO);
}

+ (void)migratePrefsIfNeeded:(PrefService*)prefs
                isNewProfile:(BOOL)isNewProfile {
  // Added 03/2026:
  // Rename legacy address bar swipe preference to the new iOS key.
  const auto* old_swipe_pref = prefs->FindPreference(
      vivaldiprefs::kVivaldiAddressBarSwipeGestureEnabledLegacy);
  const auto* new_swipe_pref = prefs->FindPreference(
      vivaldiprefs::kVivaldiAddressBarSwipeGestureEnabled);

  if (old_swipe_pref && new_swipe_pref && new_swipe_pref->IsDefaultValue() &&
      !old_swipe_pref->IsDefaultValue()) {
    prefs->SetBoolean(
        vivaldiprefs::kVivaldiAddressBarSwipeGestureEnabled,
        prefs->GetBoolean(
            vivaldiprefs::kVivaldiAddressBarSwipeGestureEnabledLegacy));
  }
  if (old_swipe_pref) {
    prefs->ClearPref(vivaldiprefs::kVivaldiAddressBarSwipeGestureEnabledLegacy);
  }

  // Search Suggestions pref should be migrated in a way that:
  // 1: New users should have it disabled by default to match our other clients.
  // This part is handled in the PrefRegistry.
  // 2: The old users should have it enabled because that was enabled by default
  // before this update. So, the migration should only happen if this is not a
  // newly-created profile, and it is not already migrated.

  // Check if migration has already been done. If UserDefaults has object for
  // this key, that means migration is already completed once.
  const bool migrationKeyExists =
      [[NSUserDefaults standardUserDefaults]
          objectForKey:kShouldMigrateSearchSuggestionsPref] != nil;

  if (migrationKeyExists) {
    return;
  }

  const auto* search_suggest_pref =
      prefs->FindPreference(prefs::kSearchSuggestEnabled);
  if (!isNewProfile && search_suggest_pref &&
      search_suggest_pref->IsDefaultValue()) {
    prefs->SetBoolean(prefs::kSearchSuggestEnabled, YES);
  }

  [[NSUserDefaults standardUserDefaults]
      setBool:NO
       forKey:kShouldMigrateSearchSuggestionsPref];
}

@end
