// Copyright (c) 2026 Vivaldi Technologies. All Rights Reserved.

#include "prefs/appkit_settings_observer.h"

#import <CoreFoundation/CoreFoundation.h>

@interface AppkitSettingsObserver ()
@property(nonatomic, copy) void (^settingChangedCallback)(AppkitSettingsType);
@end

@implementation AppkitSettingsObserver

@synthesize settingChangedCallback = _settingChangedCallback;

- (instancetype)initWithCallback:(void (^)(AppkitSettingsType))callback {
  if (self = [super init]) {
    _settingChangedCallback = [callback copy];

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

  self.settingChangedCallback = nil;
}

- (void)colorsChanged {
  if (!_settingChangedCallback)
    return;

  _settingChangedCallback(AppkitSettingsType::SystemColorsDidChange);
}

- (void)screenParametersChanged {
  if (!_settingChangedCallback)
    return;

  _settingChangedCallback(AppkitSettingsType::ScreenParametersChanged);
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
  if ([keyPath isEqualToString:@"effectiveAppearance"]) {
    if (_settingChangedCallback)
      _settingChangedCallback(AppkitSettingsType::EffectiveAppearance);
    return;
  }

  if ([keyPath isEqualToString:@"AppleActionOnDoubleClick"]) {
    if (_settingChangedCallback)
      _settingChangedCallback(AppkitSettingsType::ActionOnDoubleClick);
    return;
  }

  [super observeValueForKeyPath:keyPath
                       ofObject:object
                         change:change
                        context:context];
}

@end
