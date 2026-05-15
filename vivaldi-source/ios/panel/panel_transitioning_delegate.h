// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#ifndef IOS_PANEL_PANEL_TRANSITIONING_DELEGATE_H_
#define IOS_PANEL_PANEL_TRANSITIONING_DELEGATE_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_type.h"
#import "ios/panel/sidebar_panel_presentation_controller.h"

@interface PanelTransitioningDelegate
    : NSObject <UIViewControllerTransitioningDelegate>

@property(nonatomic, assign) ToolbarType toolbarType;
@property(nonatomic, weak) id<PanelToolbarOffsetProvider> toolbarOffsetProvider;

@end

#endif  // IOS_PANEL_PANEL_TRANSITIONING_DELEGATE_H_
