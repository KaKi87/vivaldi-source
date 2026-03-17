// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_VIVALDI_TAB_GRID_INACTIVE_TABS_PINNED_HELPER_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_VIVALDI_TAB_GRID_INACTIVE_TABS_PINNED_HELPER_H_

#import <UIKit/UIKit.h>

@class BaseGridViewController;

// Helper to pin the inactive tabs button above the regular grid.
@interface VivaldiTabGridInactiveTabsPinnedHelper : NSObject

// Updates visibility and contents of the pinned inactive tabs button.
// This allows us to move most of the patches from Chromium to Vivaldi.
+ (void)updateForGridViewController:(BaseGridViewController*)gridViewController
                            visible:(BOOL)visible
                              count:(NSInteger)count
                      daysThreshold:(NSInteger)daysThreshold;

// Recomputes layout/insets after size or insets changes.
+ (void)updateLayoutForGridViewController:
    (BaseGridViewController*)gridViewController;

// Toggles the top fade overlay for the inactive tabs header.
+ (void)setFadeEnabled:(BOOL)enabled
    forGridViewController:(BaseGridViewController*)gridViewController;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_VIVALDI_TAB_GRID_INACTIVE_TABS_PINNED_HELPER_H_
