// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#import "ios/panel/panel_prefs.h"

#import "components/prefs/pref_registry_simple.h"
#import "components/prefs/pref_service.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"

@implementation PanelPrefs

+ (void)registerLocalStatePrefs:(PrefRegistrySimple*)registry {
  registry->RegisterBooleanPref(vivaldiprefs::kVivaldiPreferTranslatePanel,
                                false);
}

@end
