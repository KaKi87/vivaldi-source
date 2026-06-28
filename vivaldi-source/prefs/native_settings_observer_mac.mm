// Copyright (c) 2016 Vivaldi Technologies. All Rights Reserverd.

#include "prefs/native_settings_observer_mac.h"

#import <CoreFoundation/CoreFoundation.h>
#import <Cocoa/Cocoa.h>

#include "components/prefs/pref_service.h"
#include "prefs/native_settings_helper_mac.h"
#include "vivaldi/prefs/vivaldi_gen_pref_enums.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

@interface AppkitSettingsObserver : NSObject {
  raw_ptr<vivaldi::NativeSettingsObserverMac> _owner;
}
- (instancetype)initWithOwner:(vivaldi::NativeSettingsObserverMac*)owner;
- (void)stopObserving;
@end

@implementation AppkitSettingsObserver
- (instancetype)initWithOwner:(vivaldi::NativeSettingsObserverMac*)owner {
  if (self = [super init]) {
    _owner = owner;

    // System Accent/Highlight Color
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(colorsChanged)
               name:NSSystemColorsDidChangeNotification
             object:nil];

    // SystemMacMenubarVisibleInFullscreen
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(screenParametersChanged)
               name:NSApplicationDidChangeScreenParametersNotification
             object:nil];

    // Dark/Light mode observer
    [NSApp addObserver:self
            forKeyPath:@"effectiveAppearance"
               options:0
               context:nullptr];

    // Window title bar double-click action
    [[NSUserDefaults standardUserDefaults]
        addObserver:self
         forKeyPath:@"AppleActionOnDoubleClick"
            options:NSKeyValueObservingOptionNew
            context:nullptr];
  }
  return self;
}

- (void)stopObserving {
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSSystemColorsDidChangeNotification
              object:nil];

  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSApplicationDidChangeScreenParametersNotification
              object:nil];

  [NSApp removeObserver:self forKeyPath:@"effectiveAppearance"];

  [[NSUserDefaults standardUserDefaults]
      removeObserver:self
          forKeyPath:@"AppleActionOnDoubleClick"];

  _owner = nullptr;
}

- (void)colorsChanged {
  if (!_owner)
    return;

  _owner->SetPref(vivaldiprefs::kSystemAccentColor,
                  vivaldi::getSystemAccentColor());
  _owner->SetPref(vivaldiprefs::kSystemHighlightColor,
                  vivaldi::getSystemHighlightColor());
}

- (void)screenParametersChanged {
  if (!_owner)
    return;

  _owner->SetPref(vivaldiprefs::kSystemMacMenubarVisibleInFullscreen,
                  vivaldi::getMenubarVisibleInFullscreen());
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
  if ([keyPath isEqualToString:@"effectiveAppearance"]) {
    if (_owner)
      _owner->SetPref(vivaldiprefs::kSystemDesktopThemeColor,
                      vivaldi::getSystemDarkMode());
  } else if ([keyPath isEqualToString:@"AppleActionOnDoubleClick"]) {
    if (_owner)
      _owner->SetPref(vivaldiprefs::kSystemMacActionOnDoubleClick,
                      vivaldi::getActionOnDoubleClick());
  } else {
    [super observeValueForKeyPath:keyPath
                         ofObject:object
                           change:change
                          context:context];
  }
}

@end

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

void MenubarSettingChanged(CFNotificationCenterRef center,
                           void* observer,
                           CFStringRef name,
                           const void* object,
                           CFDictionaryRef userInfo) {
  if (!observer) {
    return;
  }

  reinterpret_cast<NativeSettingsObserver*>(observer)->SetPref(
      vivaldiprefs::kSystemMacMenubarVisibleInFullscreen,
      getMenubarVisibleInFullscreen());
}

NativeSettingsObserverMac::NativeSettingsObserverMac(Profile* profile)
    : NativeSettingsObserver(profile) {
  // Initialize, in case the values are changed while Vivaldi is not running.
  SetPref(vivaldiprefs::kSystemAccentColor, vivaldi::getSystemAccentColor());
  SetPref(vivaldiprefs::kSystemHighlightColor,
          vivaldi::getSystemHighlightColor());
  SetPref(vivaldiprefs::kSystemMacMenubarVisibleInFullscreen,
          vivaldi::getMenubarVisibleInFullscreen());
  SetPref(vivaldiprefs::kSystemMacMenubarVisibleInFullscreen,
          vivaldi::getMenubarVisibleInFullscreen());
  SetPref(vivaldiprefs::kSystemDesktopThemeColor, vivaldi::getSystemDarkMode());
  SetPref(vivaldiprefs::kSystemMacActionOnDoubleClick,
          vivaldi::getActionOnDoubleClick());
  SetPref(vivaldiprefs::kSystemMacSwipeScrollDirection, getSwipeDirection());
  SetPref(vivaldiprefs::kSystemMacKeyboardUiMode, getKeyboardUIMode());
  SetPref(vivaldiprefs::kSystemMacMenubarVisibleInFullscreen,
          getMenubarVisibleInFullscreen());

  // NS Observers
  appkitObserver = [[AppkitSettingsObserver alloc] initWithOwner:this];

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

  CFNotificationCenterRemoveEveryObserver(CFNotificationCenterGetLocalCenter(),
                                          this);
  CFNotificationCenterAddObserver(
      CFNotificationCenterGetLocalCenter(), this, MenubarSettingChanged,
      CFSTR("NSApplicationDidChangeSafeVisibleFrameNotification"), NULL,
      CFNotificationSuspensionBehaviorDeliverImmediately);
}

NativeSettingsObserverMac::~NativeSettingsObserverMac() {
  [appkitObserver stopObserving];
  CFNotificationCenterRemoveEveryObserver(
      CFNotificationCenterGetDistributedCenter(), this);
  CFNotificationCenterRemoveEveryObserver(CFNotificationCenterGetLocalCenter(),
                                          this);
}

}  // namespace vivaldi
