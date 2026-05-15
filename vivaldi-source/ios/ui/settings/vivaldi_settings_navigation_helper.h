// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SETTINGS_VIVALDI_SETTINGS_NAVIGATION_HELPER_H_
#define IOS_UI_SETTINGS_VIVALDI_SETTINGS_NAVIGATION_HELPER_H_

#import <UIKit/UIKit.h>

@protocol VivaldiSettingsNavigationCommands <NSObject>
- (void)closeSettings;
@end

// Returns YES when `navigationController` is the settings navigation
// controller and the close request has been forwarded to it.
static inline BOOL VivaldiCloseSettingsIfPossible(
    UINavigationController* navigationController) {
  if (!navigationController ||
      ![navigationController respondsToSelector:@selector(closeSettings)]) {
    return NO;
  }

  id<VivaldiSettingsNavigationCommands> settingsNavigationController =
      (id<VivaldiSettingsNavigationCommands>)navigationController;
  [settingsNavigationController closeSettings];
  return YES;
}

#endif  // IOS_UI_SETTINGS_VIVALDI_SETTINGS_NAVIGATION_HELPER_H_
