// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#import "ios/chrome/browser/browser_view/ui_bundled/browser_view_controller.h"
#import "ios/panel/sidebar_panel_presentation_controller.h"

@interface BrowserViewController (PanelToolbarOffsetProvider) <
    PanelToolbarOffsetProvider>
@end

@interface BrowserViewController (PanelToolbarOffsetProviderPrivate)
- (CGFloat)expandedTopToolbarHeight;
- (CGFloat)secondaryToolbarHeightWithInset;
@end

@implementation BrowserViewController (PanelToolbarOffsetProvider)

- (CGFloat)panelTopToolbarOffset {
  if (!self.isViewLoaded) {
    return 0;
  }

  [self.view layoutIfNeeded];
  return [self expandedTopToolbarHeight];
}

- (CGFloat)panelBottomToolbarOffset {
  if (!self.isViewLoaded) {
    return 0;
  }

  [self.view layoutIfNeeded];
  return MAX(0, [self secondaryToolbarHeightWithInset]);
}

@end
