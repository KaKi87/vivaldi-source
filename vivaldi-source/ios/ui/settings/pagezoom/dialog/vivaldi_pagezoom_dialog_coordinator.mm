// Copyright 2024-25 Vivaldi Technologies. All rights reserved.

#import "ios/ui/settings/pagezoom/dialog/vivaldi_pagezoom_dialog_coordinator.h"

#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#import "ios/chrome/browser/shared/public/commands/text_zoom_commands.h"
#import "ios/ui/settings/pagezoom/dialog/vivaldi_pagezoom_dialog_mediator.h"
#import "ios/ui/settings/pagezoom/vivaldi_pagezoom_settings_coordinator.h"
#import "ui/base/l10n/l10n_util_mac.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

namespace {
const CGFloat kAnimationDuration = 0.3;
}

@interface VivaldiPageZoomDialogCoordinator () <
    VivaldiPageZoomViewControllerLayoutDelegate>
// View controller for the page zoom setting.
@property(nonatomic, strong, readwrite)
    VivaldiPageZoomViewController* viewController;
// Page zoom preference mediator.
@property(nonatomic, strong) VivaldiPageZoomDialogMediator* mediator;
// Allows simplified access to the TextZoomCommands handler.
@property(nonatomic) id<TextZoomCommands> textZoomCommandHandler;
// The coordinator showing the view for page zoom setting
@property(nonatomic, strong)
    VivaldiPageZoomSettingsCoordinator* vivaldiPageZoomSettingsCoordinator;
@property(nonatomic, strong) NSLayoutConstraint* panelTopConstraint;
@property(nonatomic, strong) NSLayoutConstraint* panelHeightConstraint;
@property(nonatomic, assign) CGFloat panelHeight;
@property(nonatomic, assign, getter=isUpdatingPanelHeight)
    BOOL updatingPanelHeight;
@end

@implementation VivaldiPageZoomDialogCoordinator

#pragma mark - ChromeCoordinator

- (void)start {
  DCHECK(self.browser);

  self.textZoomCommandHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), TextZoomCommands);
  self.mediator = [[VivaldiPageZoomDialogMediator alloc]
      initWithWebStateList:self.browser->GetWebStateList()
            commandHandler:self.textZoomCommandHandler];
  self.mediator.prefService = self.browser->GetProfile()->GetPrefs();
  self.viewController = [[VivaldiPageZoomViewController alloc] init];
  self.viewController.commandHandler = self.textZoomCommandHandler;
  self.viewController.zoomHandler = self.mediator;
  self.viewController.settingsDelegate = self;
  self.viewController.layoutDelegate = self;
  self.mediator.consumer = self.viewController;

  if (!self.baseViewController) {
    [self stop];
    return;
  }

  [self showPanelAnimated:YES];
}

- (void)stop {
  [self stopAnimated:NO completion:nil];
}

- (void)stopAnimated:(BOOL)animated completion:(void (^)(void))completion {
  __weak __typeof(self) weakSelf = self;
  [self hidePanelAnimated:animated
               completion:^{
                 __strong __typeof(weakSelf) strongSelf = weakSelf;
                 [strongSelf.mediator disconnect];
                 strongSelf.mediator.consumer = nil;
                 strongSelf.mediator = nil;
                 strongSelf.viewController.layoutDelegate = nil;
                 strongSelf.viewController = nil;
                 if (completion) {
                   completion();
                 }
               }];
}

#pragma mark - VivaldiPageZoomSettingsDelegate

- (void)showVivaldiPageZoomSettings {
  __weak __typeof(self) weakSelf = self;
  [self stopAnimated:YES
          completion:^{
            __strong __typeof(weakSelf) strongSelf = weakSelf;
            if (!strongSelf) {
              return;
            }
            strongSelf.vivaldiPageZoomSettingsCoordinator =
                [[VivaldiPageZoomSettingsCoordinator alloc]
                    initWithBaseViewController:strongSelf.baseViewController
                                       browser:strongSelf.browser];
            strongSelf.vivaldiPageZoomSettingsCoordinator.isFromDialog = YES;
            // Start the coordinator to set up the zoom settings view
            [strongSelf.vivaldiPageZoomSettingsCoordinator start];
          }];
}

#pragma mark - VivaldiPageZoomViewControllerLayoutDelegate

- (void)pageZoomViewControllerDidUpdateLayout:
    (VivaldiPageZoomViewController*)viewController {
  [self updatePanelHeightIfNeeded];
}

#pragma mark - Private

- (CGFloat)panelHeightForBaseView:(UIView*)baseView {
  UIView* panelView = self.viewController.view;
  CGFloat targetWidth = CGRectGetWidth(baseView.bounds);
  if (targetWidth <= 0) {
    [baseView layoutIfNeeded];
    targetWidth = CGRectGetWidth(baseView.bounds);
  }

  CGSize targetSize =
      CGSizeMake(targetWidth, UILayoutFittingCompressedSize.height);
  BOOL heightConstraintWasActive = self.panelHeightConstraint.active;
  if (heightConstraintWasActive) {
    self.panelHeightConstraint.active = NO;
  }
  CGSize fittingSize =
      [panelView systemLayoutSizeFittingSize:targetSize
               withHorizontalFittingPriority:UILayoutPriorityRequired
                     verticalFittingPriority:UILayoutPriorityFittingSizeLevel];
  if (heightConstraintWasActive) {
    self.panelHeightConstraint.active = YES;
  }
  return fittingSize.height;
}

- (void)updatePanelHeightIfNeeded {
  if (self.isUpdatingPanelHeight || !self.panelHeightConstraint ||
      !self.viewController.view.superview || !self.baseViewController) {
    return;
  }

  self.updatingPanelHeight = YES;
  UIView* baseView = self.baseViewController.view;
  CGFloat panelHeight = [self panelHeightForBaseView:baseView];
  if (panelHeight > 0) {
    self.panelHeight = panelHeight;
    self.panelHeightConstraint.constant = panelHeight;
    if (self.panelTopConstraint.constant < 0) {
      self.panelTopConstraint.constant = -panelHeight;
    }
    [baseView setNeedsLayout];
  }
  self.updatingPanelHeight = NO;
}

- (void)showPanelAnimated:(BOOL)animated {
  UIViewController* baseViewController = self.baseViewController;
  if (!baseViewController || !self.viewController) {
    return;
  }

  UIView* baseView = baseViewController.view;
  UIView* panelView = self.viewController.view;

  if (panelView.superview) {
    return;
  }

  [baseViewController addChildViewController:self.viewController];
  panelView.translatesAutoresizingMaskIntoConstraints = NO;
  [baseView addSubview:panelView];

  self.panelTopConstraint =
      [panelView.topAnchor constraintEqualToAnchor:baseView.topAnchor];
  NSLayoutConstraint* leadingConstraint =
      [panelView.leadingAnchor constraintEqualToAnchor:baseView.leadingAnchor];
  NSLayoutConstraint* trailingConstraint = [panelView.trailingAnchor
      constraintEqualToAnchor:baseView.trailingAnchor];

  [NSLayoutConstraint activateConstraints:@[
    self.panelTopConstraint,
    leadingConstraint,
    trailingConstraint,
  ]];

  [baseView layoutIfNeeded];

  self.panelHeight = [self panelHeightForBaseView:baseView];
  self.panelHeightConstraint =
      [panelView.heightAnchor constraintEqualToConstant:self.panelHeight];
  self.panelHeightConstraint.active = YES;
  self.panelTopConstraint.constant = -self.panelHeight;
  [baseView layoutIfNeeded];

  self.panelTopConstraint.constant = 0;

  __weak __typeof(self) weakSelf = self;
  void (^completion)(BOOL) = ^(BOOL) {
    [weakSelf.viewController didMoveToParentViewController:baseViewController];
  };

  if (animated) {
    [UIView animateWithDuration:kAnimationDuration
                     animations:^{
                       [baseView layoutIfNeeded];
                     }
                     completion:completion];
  } else {
    [baseView layoutIfNeeded];
    completion(YES);
  }
}

- (void)hidePanelAnimated:(BOOL)animated {
  [self hidePanelAnimated:animated completion:nil];
}

- (void)hidePanelAnimated:(BOOL)animated completion:(void (^)(void))completion {
  if (!self.panelTopConstraint || !self.panelHeightConstraint ||
      !self.viewController || !self.baseViewController) {
    if (completion) {
      completion();
    }
    return;
  }

  UIViewController* baseViewController = self.baseViewController;
  UIView* baseView = baseViewController.view;
  UIView* panelView = self.viewController.view;
  [self.viewController willMoveToParentViewController:nil];

  CGFloat panelHeight = [self panelHeightForBaseView:baseView];
  if (panelHeight > 0) {
    self.panelHeight = panelHeight;
    self.panelHeightConstraint.constant = panelHeight;
  } else if (self.panelHeight <= 0) {
    self.panelHeight = [self panelHeightForBaseView:baseView];
  }
  self.panelTopConstraint.constant = -self.panelHeight;

  __weak __typeof(self) weakSelf = self;
  void (^finalCompletion)(BOOL) = ^(BOOL) {
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    [panelView removeFromSuperview];
    if (strongSelf) {
      [strongSelf.viewController removeFromParentViewController];
      NSMutableArray<NSLayoutConstraint*>* constraints =
          [NSMutableArray arrayWithCapacity:2];
      if (strongSelf.panelTopConstraint) {
        [constraints addObject:strongSelf.panelTopConstraint];
      }
      if (strongSelf.panelHeightConstraint) {
        [constraints addObject:strongSelf.panelHeightConstraint];
      }
      if (constraints.count > 0) {
        [NSLayoutConstraint deactivateConstraints:constraints];
      }
      strongSelf.panelTopConstraint = nil;
      strongSelf.panelHeightConstraint = nil;
      strongSelf.panelHeight = 0;
    }
    if (completion) {
      completion();
    }
  };

  if (animated) {
    [UIView animateWithDuration:kAnimationDuration
                     animations:^{
                       [baseView layoutIfNeeded];
                     }
                     completion:finalCompletion];
  } else {
    [baseView layoutIfNeeded];
    finalCompletion(YES);
  }
}

@end
