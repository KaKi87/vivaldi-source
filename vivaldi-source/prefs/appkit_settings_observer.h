// Copyright (c) 2026 Vivaldi Technologies. All Rights Reserved.

#ifndef PREFS_APPKIT_SETTINGS_OBSERVER_H_
#define PREFS_APPKIT_SETTINGS_OBSERVER_H_

#import <Cocoa/Cocoa.h>

#include "base/component_export.h"
#include "prefs/appkit_settings_types.h"

using vivaldi::AppkitSettingsType;

COMPONENT_EXPORT(APPKIT_SETTINGS_OBSERVER)
@interface AppkitSettingsObserver : NSObject
- (instancetype)initWithCallback:(void (^)(AppkitSettingsType))callback;
- (void)stopObserving;
@end

#endif  // PREFS_APPKIT_SETTINGS_OBSERVER_H_
