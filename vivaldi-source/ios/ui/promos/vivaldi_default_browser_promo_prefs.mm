// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/ui/promos/vivaldi_default_browser_promo_prefs.h"

#import "base/time/time.h"
#import "components/prefs/pref_registry_simple.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"

@implementation VivaldiDefaultBrowserPromoPrefs

+ (void)registerLocalStatePrefs:(PrefRegistrySimple*)registry {
  registry->RegisterTimePref(
      vivaldiprefs::kVivaldiDefaultBrowserStartupPromoLastActiveTime,
      base::Time());
}

@end
