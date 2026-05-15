// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TOOLBAR_LEGACY_UI_BUNDLED_BUTTONS_LEGACY_TOOLBAR_BUTTON_FACTORY_H_
#define IOS_CHROME_BROWSER_TOOLBAR_LEGACY_UI_BUNDLED_BUTTONS_LEGACY_TOOLBAR_BUTTON_FACTORY_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/toolbar_style.h"

@protocol BWGCommands;
@class LegacyToolbarButton;
@class ToolbarButtonActionsHandler;
@class ToolbarButtonVisibilityConfiguration;
@class ToolbarConfiguration;
@class ToolbarTabGridButton;
@class ToolbarToolsMenuButton;

// LegacyToolbarButton Factory protocol to create LegacyToolbarButton objects
// with certain style and configuration, depending of the implementation. A
// dispatcher is used to send the commands associated with the buttons.
@interface LegacyToolbarButtonFactory : NSObject

- (instancetype)initWithStyle:(ToolbarStyle)style NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@property(nonatomic, assign, readonly) ToolbarStyle style;
// Configuration object for styling. It is used by the factory to set the style
// of the buttons title.
@property(nonatomic, strong, readonly)
    ToolbarConfiguration* toolbarConfiguration;
// Handler for the actions.
@property(nonatomic, weak) ToolbarButtonActionsHandler* actionHandler;
// Handler for gemini commands.
@property(nonatomic, weak) id<BWGCommands> geminiHandler;
// Configuration object for the visibility of the buttons.
@property(nonatomic, strong)
    ToolbarButtonVisibilityConfiguration* visibilityConfiguration;

// Back LegacyToolbarButton.
- (LegacyToolbarButton*)backButton;
// Forward LegacyToolbarButton.
- (LegacyToolbarButton*)forwardButton;
// Tab Grid LegacyToolbarButton.
- (ToolbarTabGridButton*)tabGridButton;
// Tools Menu LegacyToolbarButton.
- (LegacyToolbarButton*)toolsMenuButton;
// Share LegacyToolbarButton.
- (LegacyToolbarButton*)shareButton;
// Reload LegacyToolbarButton.
- (LegacyToolbarButton*)reloadButton;
// Stop LegacyToolbarButton.
- (LegacyToolbarButton*)stopButton;
// LegacyToolbarButton to create a new tab.
- (LegacyToolbarButton*)openNewTabButton;
// Button to cancel the edit of the location bar.
- (UIButton*)cancelButton;

// Vivaldi
// Panel toolbar button.
- (LegacyToolbarButton*)panelButton;
// Visible only in iPhone portrait + Tab bar enabled + bottom omnibox enabled
// state.
- (LegacyToolbarButton*)vivaldiMoreButton;
// Vivaldi search button -> Visible only on new tab page.
- (LegacyToolbarButton*)vivaldiSearchButton;
/// Vivaldi home button, Visible only on web page if iOS topbar enabled
/// It will be always be visible on iPad, landscape mode & if iOS bottom bar enabled
- (LegacyToolbarButton*)vivaldiHomeButton;

/// Returns the context menu for overflow action for
/// tab bar enabled + bottom omnibox + iPhone portrait state.
/// For NTP only panel and tab switcher button is visible.
/// For valid browsing state navigation buttons
/// are present too.
- (UIMenu*)overflowMenuWithNavForwardEnabled:(BOOL)navigationForwardEnabled
                         navBackwordEnabled:(BOOL)navigationBackwordEnabled;
// End Vivaldi
@end

#endif  // IOS_CHROME_BROWSER_TOOLBAR_LEGACY_UI_BUNDLED_BUTTONS_LEGACY_TOOLBAR_BUTTON_FACTORY_H_
