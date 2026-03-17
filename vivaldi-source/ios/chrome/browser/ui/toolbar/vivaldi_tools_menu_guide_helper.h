// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_VIVALDI_TOOLS_MENU_GUIDE_HELPER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_VIVALDI_TOOLS_MENU_GUIDE_HELPER_H_

#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_type.h"

@class AdaptiveToolbarViewController;
@class LayoutGuideCenter;

// Updates Vivaldi tools menu guide references for the currently active toolbar.
void VivaldiUpdateToolsMenuButtonGuides(
    LayoutGuideCenter* layoutGuideCenter,
    ToolbarType omniboxPosition,
    AdaptiveToolbarViewController* primaryToolsController,
    AdaptiveToolbarViewController* vivaldiTopToolsController);

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_VIVALDI_TOOLS_MENU_GUIDE_HELPER_H_
