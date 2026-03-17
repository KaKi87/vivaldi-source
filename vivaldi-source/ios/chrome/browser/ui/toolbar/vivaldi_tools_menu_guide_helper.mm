// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#import "ios/chrome/browser/ui/toolbar/vivaldi_tools_menu_guide_helper.h"

#import "ios/chrome/browser/shared/ui/util/layout_guide_names.h"
#import "ios/chrome/browser/shared/ui/util/util_swift.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/adaptive_toolbar_view_controller.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/legacy_toolbar_button.h"

namespace {

void SetToolsMenuGuideEnabled(BOOL enabled,
                              AdaptiveToolbarViewController* controller) {
  if (!controller || ![controller isViewLoaded]) {
    return;
  }
  controller.toolsMenuButton.guideName = enabled ? vToolsMenuGuide : nil;
  [controller refreshToolbarButtonsGuide];
}

}  // namespace

void VivaldiUpdateToolsMenuButtonGuides(
    LayoutGuideCenter* layoutGuideCenter,
    ToolbarType omniboxPosition,
    AdaptiveToolbarViewController* primaryToolsController,
    AdaptiveToolbarViewController* vivaldiTopToolsController) {
  AdaptiveToolbarViewController* activeToolsController =
      omniboxPosition == ToolbarType::kPrimary ? vivaldiTopToolsController
                                               : primaryToolsController;
  AdaptiveToolbarViewController* inactiveToolsController =
      activeToolsController == vivaldiTopToolsController
          ? primaryToolsController
          : vivaldiTopToolsController;

  [layoutGuideCenter referenceView:nil underName:vToolsMenuGuide];
  [layoutGuideCenter referenceView:nil underName:kToolsMenuGuide];

  SetToolsMenuGuideEnabled(NO, inactiveToolsController);
  SetToolsMenuGuideEnabled(YES, activeToolsController);

  if (activeToolsController && [activeToolsController isViewLoaded]) {
    UIView* toolsMenuButtonView = activeToolsController.toolsMenuButton;
    [layoutGuideCenter referenceView:toolsMenuButtonView
                           underName:vToolsMenuGuide];
    [layoutGuideCenter referenceView:toolsMenuButtonView
                           underName:kToolsMenuGuide];
  }
}
