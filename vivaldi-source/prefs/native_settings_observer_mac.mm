// Copyright (c) 2016 Vivaldi Technologies. All Rights Reserved.

#include "prefs/native_settings_observer_mac.h"

#import <CoreFoundation/CoreFoundation.h>

#include "components/prefs/pref_service.h"
#include "prefs/appkit_settings_observer.h"
#include "prefs/native_settings_helper_mac.h"
#include "vivaldi/prefs/vivaldi_gen_pref_enums.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

namespace vivaldi {

// static
NativeSettingsObserver* NativeSettingsObserver::Create(Profile* profile) {
  return new NativeSettingsObserverMac(profile);
}

void SwipeDirectionChanged(CFNotificationCenterRef center,
                           void* observer,
                           CFStringRef name,
                           const void* object,
                           CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)->SetPref(
      vivaldiprefs::kSystemMacSwipeScrollDirection, getSwipeDirection());
}

void KeyboardUIModeChanged(CFNotificationCenterRef center,
                           void* observer,
                           CFStringRef name,
                           const void* object,
                           CFDictionaryRef userInfo) {
  reinterpret_cast<NativeSettingsObserver*>(observer)->SetPref(
      vivaldiprefs::kSystemMacKeyboardUiMode, getKeyboardUIMode());
}

void NativeSettingsObserverMac::OnSettingChanged(AppkitSettingsType type) {
  switch (type) {
    case AppkitSettingsType::EffectiveAppearance:
      SetPref(vivaldiprefs::kSystemDesktopThemeColor,
              vivaldi::getSystemDarkMode());
      break;
    case AppkitSettingsType::ActionOnDoubleClick:
      SetPref(vivaldiprefs::kSystemMacActionOnDoubleClick,
              vivaldi::getActionOnDoubleClick());
      break;
    case AppkitSettingsType::ScreenParametersChanged:
      SetPref(vivaldiprefs::kSystemMacMenubarVisibleInFullscreen,
              getMenubarVisibleInFullscreen());
      break;
    case AppkitSettingsType::SystemColorsDidChange:
      SetPref(vivaldiprefs::kSystemAccentColor,
              vivaldi::getSystemAccentColor());
      SetPref(vivaldiprefs::kSystemHighlightColor,
              vivaldi::getSystemHighlightColor());
      break;
  }
}

NativeSettingsObserverMac::NativeSettingsObserverMac(Profile* profile)
    : NativeSettingsObserver(profile) {
  // Initialize, in case the values are changed while Vivaldi is not running.
  SetPref(vivaldiprefs::kSystemAccentColor, vivaldi::getSystemAccentColor());
  SetPref(vivaldiprefs::kSystemHighlightColor,
          vivaldi::getSystemHighlightColor());
  SetPref(vivaldiprefs::kSystemMacMenubarVisibleInFullscreen,
          vivaldi::getMenubarVisibleInFullscreen());
  SetPref(vivaldiprefs::kSystemDesktopThemeColor, vivaldi::getSystemDarkMode());
  SetPref(vivaldiprefs::kSystemMacActionOnDoubleClick,
          vivaldi::getActionOnDoubleClick());
  SetPref(vivaldiprefs::kSystemMacSwipeScrollDirection, getSwipeDirection());
  SetPref(vivaldiprefs::kSystemMacKeyboardUiMode, getKeyboardUIMode());

  // NS Observers
  __weak auto weak_this = this;
  appkitObserver = [[AppkitSettingsObserver alloc]
      initWithCallback:^(AppkitSettingsType type) {
        if (weak_this) {
          weak_this->OnSettingChanged(type);
        }
      }];

  // NOTE(tomas@vivaldi.com): fix for VB-39486
  CFNotificationCenterRemoveEveryObserver(
      CFNotificationCenterGetDistributedCenter(), this);

  // CF observers
  CFNotificationCenterAddObserver(
      CFNotificationCenterGetDistributedCenter(), this, SwipeDirectionChanged,
      CFSTR("SwipeScrollDirectionDidChangeNotification"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);

  CFNotificationCenterAddObserver(
      CFNotificationCenterGetDistributedCenter(), this, KeyboardUIModeChanged,
      CFSTR("KeyboardUIModeDidChangeNotification"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);
}

NativeSettingsObserverMac::~NativeSettingsObserverMac() {
  [appkitObserver stopObserving];
  appkitObserver = nil;
  CFNotificationCenterRemoveEveryObserver(
      CFNotificationCenterGetDistributedCenter(), this);
}

}  // namespace vivaldi
