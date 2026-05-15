// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#ifndef IOS_UI_SETTINGS_TABS_VIVALDI_TAB_SETTING_PREFS_H_
#define IOS_UI_SETTINGS_TABS_VIVALDI_TAB_SETTING_PREFS_H_

#import <UIKit/UIKit.h>

#import "ios/ui/settings/tabs/vivaldi_ntp_type.h"
#import "ios/ui/settings/tabs/vivaldi_tab_stack_style.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

class PrefService;

// Stores and retrieves the prefs for the tab settings.
@interface VivaldiTabSettingPrefs : NSObject

/// Static variable to store prefService for Vivaldi UI usage.
+ (PrefService*)prefService;

/// Static method to set the PrefService, must be set before calling any method
/// of this class.
+ (void)setPrefService:(PrefService*)prefService;

/// Registers the feature preferences.
+ (void)registerBrowserStatePrefs:(user_prefs::PrefRegistrySyncable*)registry;

/// Applies one-time migrations for tab settings prefs.
+ (void)migratePrefsIfNeeded:(PrefService*)prefs
                isNewProfile:(BOOL)isNewProfile;

/// Returns the desktop style tab status
+ (BOOL)getDesktopTabsModeWithPrefService:(PrefService*)prefService;
/// Returns the setting for tab stack
+ (BOOL)getUseTabStackWithPrefService:(PrefService*)prefService;

/// Returns the tab stack style.
+ (VivaldiTabStackStyle)getTabStackStyleWithPrefService:
    (PrefService*)prefService;

/// Returns Homepage Url
+ (NSString*)getHomepageUrlWithPrefService:(PrefService*)prefService;

/// Get new tab settings
+ (VivaldiNTPType)getNewTabSettingWithPrefService:(PrefService*)prefService;

/// Returns Newtab Url
+ (NSString*)getNewTabUrlWithPrefService:(PrefService*)prefService;

/// Returns YES when swipe-to-close is enabled in tab switcher.
+ (BOOL)swipeToCloseTabEnabled;

/// Returns the tab stack style (uses shared PrefService).
+ (VivaldiTabStackStyle)tabStackStyle;

/// Sets the desktop style tab mode.
+ (void)setDesktopTabsMode:(BOOL)enabled
            inPrefServices:(PrefService*)prefService;
/// Sets the bottom omnibox.
+ (void)setBottomOmniboxEnabled:(BOOL)enabled
                 inPrefServices:(PrefService*)prefService;
/// Sets the reverse search suggestion for bottom omnibox.
+ (void)setReverseSearchSuggestionsEnabled:(BOOL)enabled
                            inPrefServices:(PrefService*)prefService;
/// Sets the setting for tab stack
+ (void)setUseTabStack:(BOOL)enabled inPrefServices:(PrefService*)prefService;
/// Sets the tab stack style
+ (void)setTabStackStyle:(VivaldiTabStackStyle)style
          inPrefServices:(PrefService*)prefService;

/// Sets Homepage Url
+ (void)setHomepageUrlWithPrefService:(NSString*)url
                       inPrefServices:(PrefService*)prefService;

/// Save the new tab settings
+ (void)setNewTabSettingWithPrefService:(PrefService*)prefService
                             andSetting:(VivaldiNTPType)setting
                                withURL:(NSString*)url;

@end

#endif  // IOS_UI_SETTINGS_TABS_VIVALDI_TAB_SETTING_PREFS_H_
