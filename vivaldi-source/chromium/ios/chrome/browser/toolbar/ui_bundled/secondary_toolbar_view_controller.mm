// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/toolbar/ui_bundled/secondary_toolbar_view_controller.h"

#import "base/check.h"
#import "ios/chrome/browser/fullscreen/ui_bundled/fullscreen_controller.h"
#import "ios/chrome/browser/fullscreen/ui_bundled/scoped_fullscreen_disabler.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/public/prototypes/diamond/utils.h"
#import "ios/chrome/browser/shared/ui/util/layout_guide_names.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/shared/ui/util/util_swift.h"
#import "ios/chrome/browser/toolbar/ui_bundled/adaptive_toolbar_view_controller+subclassing.h"
#import "ios/chrome/browser/toolbar/ui_bundled/buttons/toolbar_button.h"
#import "ios/chrome/browser/toolbar/ui_bundled/buttons/toolbar_button_factory.h"
#import "ios/chrome/browser/toolbar/ui_bundled/buttons/toolbar_configuration.h"
#import "ios/chrome/browser/toolbar/ui_bundled/public/omnibox_position_util.h"
#import "ios/chrome/browser/toolbar/ui_bundled/public/toolbar_constants.h"
#import "ios/chrome/browser/toolbar/ui_bundled/public/toolbar_height_delegate.h"
#import "ios/chrome/browser/toolbar/ui_bundled/public/toolbar_utils.h"
#import "ios/chrome/browser/toolbar/ui_bundled/secondary_toolbar_keyboard_state_provider.h"
#import "ios/chrome/browser/toolbar/ui_bundled/secondary_toolbar_view.h"
#import "ios/chrome/browser/toolbar/ui_bundled/toolbar_progress_bar.h"
#import "ios/chrome/common/ui/util/ui_util.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/chrome/browser/toolbar/ui_bundled/buttons/toolbar_button.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/ui/toolbar/vivaldi_toolbar_constants.h"

using vivaldi::IsVivaldiRunning;
// End Vivaldi

@interface SecondaryToolbarViewController ()

/// Redefined to be a `SecondaryToolbarView`.
@property(nonatomic, strong) SecondaryToolbarView* view;

/// Whether the location indicator is currently active.
@property(nonatomic) BOOL locationIndicatorActive;

// Vivaldi
@property(nonatomic, assign) BOOL isNTP;
// End Vivaldi

@end

@implementation SecondaryToolbarViewController

@dynamic view;

- (void)loadView {
  self.view =
      [[SecondaryToolbarView alloc] initWithButtonFactory:self.buttonFactory];
  DCHECK(self.layoutGuideCenter);
  [self.layoutGuideCenter referenceView:self.view
                              underName:kSecondaryToolbarGuide];

  if (IsBottomOmniboxAvailable()) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(keyboardWillHide:)
               name:UIKeyboardWillHideNotification
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(keyboardWillShow:)
               name:UIKeyboardWillShowNotification
             object:nil];
  }

  if (IsDiamondPrototypeEnabled()) {
    UIButton* button = self.view.diamondPrototypeButton;
    UIMenu* emptyMenu = [UIMenu menuWithChildren:@[]];
    button.menu = emptyMenu;
    UIAction* action = [UIAction
        actionWithTitle:@""
                  image:nil
             identifier:nil
                handler:^(UIAction* uiAction) {
                  TriggerHapticFeedbackForImpact(UIImpactFeedbackStyleHeavy);
                  [[NSNotificationCenter defaultCenter]
                      postNotificationName:kDiamondLongPressButton
                                    object:button];
                }];
    [button addAction:action
        forControlEvents:UIControlEventMenuActionTriggered];
  }

  if (IsVivaldiRunning()) {
    if (@available(iOS 17, *)) {
      NSArray<UITrait>* traits = TraitCollectionSetForTraits(@[
        UITraitVerticalSizeClass.class, UITraitHorizontalSizeClass.class
      ]);
      [self registerForTraitChanges:traits
                         withAction:@selector(updateToolbarButtonsTintColor)];
    }
  } // End Vivaldi

}

- (void)disconnect {
  _fullscreenController = nullptr;
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Setters

// TODO(crbug.com/429955447): Remove when diamond prototype is cleaned.
- (void)setUsedAsPrimaryToolbar:(BOOL)usedAsPrimaryToolbar {
  _usedAsPrimaryToolbar = usedAsPrimaryToolbar;
  self.view.usedAsPrimaryToolbar = usedAsPrimaryToolbar;
}

- (void)setLocationIndicatorActive:(BOOL)locationIndicatorActive {
  if (locationIndicatorActive == _locationIndicatorActive) {
    return;
  }

  _locationIndicatorActive = locationIndicatorActive;

  if (locationIndicatorActive) {
    if (_fullscreenController) {
      _fullscreenController->EnterForceFullscreenMode(
          /* insets_update_enabled */ false);
    }
    self.view.locationBarTopConstraint.constant = 0;
    self.view.bottomSeparator.alpha = 1.0;
    [self.toolbarHeightDelegate secondaryToolbarMovedAboveKeyboard];
  } else {
    if (_fullscreenController) {
      _fullscreenController->ExitForceFullscreenMode();
    }
    self.view.bottomSeparator.alpha = 0.0;
    [self.toolbarHeightDelegate secondaryToolbarRemovedFromKeyboard];
  }
}

#pragma mark - AdaptiveToolbarViewController

- (void)collapsedToolbarButtonTapped {
  [super collapsedToolbarButtonTapped];

  if (self.locationIndicatorActive) {
    // When the bottom omnibox is collapsed above the keyboard, it's positioned
    // behind an `omniboxTypingShield` (transparent view) in the
    // `formInputAccessoryView`. This allow the keyboard to know about the size
    // of the omnibox (crbug.com/1490601).
    // When voice over is off, tapping the collapsed bottom omnibox interacts
    // with the `omniboxTypingShield`. The logic to dismiss the keyboard is
    // handled in `formInputAccessoryViewHandler`. However, the typing shield
    // has `isAccessibilityElement` equals NO to let the user interact with the
    // omnibox on voice over. In this mode, logic to dismiss the keyboard is
    // handled here in `SecondaryToolbarViewController`.
    CHECK([self hasOmnibox]);
    UIResponder* responder = GetFirstResponder();
    [responder resignFirstResponder];
  }
}

#pragma mark - FullscreenUIElement

- (void)updateForFullscreenProgress:(CGFloat)progress {
  [super updateForFullscreenProgress:progress];

  CGFloat alphaValue = fmax(progress * 1.1 - 0.1, 0);
  if (IsBottomOmniboxAvailable()) {
    self.view.buttonStackView.alpha = alphaValue;
  }

  if (IsDiamondPrototypeEnabled()) {
    self.view.toolsMenuButton.alpha = alphaValue;
    self.view.diamondPrototypeButton.alpha = alphaValue;
    self.view.backButton.alpha = alphaValue;
    self.view.forwardButton.alpha = alphaValue;
  }

  self.view.locationBarTopConstraint.constant =
      [self verticalMarginForLocationBarForFullscreenProgress:progress];
}

#pragma mark - SecondaryToolbarConsumer

- (void)makeTranslucent {
  [self.view makeTranslucent];
}

- (void)makeOpaque {
  [self.view makeOpaque];
}

#pragma mark - UIKeyboardNotification

- (void)keyboardWillShow:(NSNotification*)notification {
  [self constraintToKeyboard:YES withNotification:notification];
}

- (void)keyboardWillHide:(NSNotification*)notification {
  [self constraintToKeyboard:NO withNotification:notification];
}

#pragma mark - Private

/// Returns the vertical margin to the location bar based on fullscreen
/// `progress`, aligned to the nearest pixel.
- (CGFloat)verticalMarginForLocationBarForFullscreenProgress:(CGFloat)progress {

  if (IsVivaldiRunning() && !self.hasOmnibox)
    return 0; // End Vivaldi

  const CGFloat clampedFontSizeMultiplier = ToolbarClampedFontSizeMultiplier(
      self.traitCollection.preferredContentSizeCategory);

  if (IsVivaldiRunning()) {
    return AlignValueToPixel(
        (vBottomAdaptiveLocationBarTopMargin * progress +
         kBottomAdaptiveLocationBarVerticalMarginFullscreen * (1 - progress)) *
            clampedFontSizeMultiplier +
        (clampedFontSizeMultiplier - 1) * kLocationBarVerticalMarginDynamicType);
  } // End Vivaldi

  const BOOL hasBottomSafeArea = self.view.window.safeAreaInsets.bottom;
  const CGFloat fullscreenMargin =
      hasBottomSafeArea ? kBottomAdaptiveLocationBarVerticalMarginFullscreen
                        : 0;

  if (IsDiamondPrototypeEnabled()) {
    return AlignValueToPixel(kBottomAdaptiveLocationBarTopMargin * progress);
  }

  return AlignValueToPixel((kBottomAdaptiveLocationBarTopMargin * progress +
                            fullscreenMargin * (1 - progress)) *
                               clampedFontSizeMultiplier +
                           (clampedFontSizeMultiplier - 1) *
                               kLocationBarVerticalMarginDynamicType);
}

/// Updates keyboard constraints with `notification`. When
/// `constraintToKeyboard`, the toolbar is collapsed above the keyboard.
- (void)constraintToKeyboard:(BOOL)shouldConstraintToKeyboard
            withNotification:(NSNotification*)notification {
  if (!self.hasOmnibox) {
    return;
  }

  // Whether to cleanup the location indication previously shown for web
  // content.
  BOOL hideLocationIndicator =
      !shouldConstraintToKeyboard && self.locationIndicatorActive;

  // Whether to show the secondary toolbar as a location indicator when keyboard
  // is active for web content. Bottom omnibox exclusive.
  BOOL keyboardActiveForWebContent =
      [self.keyboardStateProvider keyboardIsActiveForWebContent];
  BOOL showLocationIndicator = shouldConstraintToKeyboard &&
                               keyboardActiveForWebContent &&
                               !hideLocationIndicator;

  // Whether the toolbar containing the omnibox should follow the keyboard.
  BOOL followSteadyStateEnabled =
      omnibox::ShouldFocusedOmniboxFollowSteadyStatePosition();
  BOOL forceBottomOmniboxInEditState = omnibox::ForceBottomOmniboxInEditState();
  BOOL omniboxAttachedInEditState =
      !keyboardActiveForWebContent &&
      (followSteadyStateEnabled || forceBottomOmniboxInEditState);

  BOOL shouldAnimateOmniboxMovement = showLocationIndicator ||
                                      hideLocationIndicator ||
                                      omniboxAttachedInEditState;
  if (!shouldAnimateOmniboxMovement) {
    return;
  }

  if (showLocationIndicator) {
    self.locationIndicatorActive = YES;
    [self.view layoutIfNeeded];
  } else if (hideLocationIndicator) {
    [GetFirstResponder() resignFirstResponder];
    self.locationIndicatorActive = NO;
  }

  [self.view layoutIfNeeded];

  NSDictionary* userInfo = notification.userInfo;
  NSTimeInterval duration =
      [userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];
  UIViewAnimationCurve curve = (UIViewAnimationCurve)
      [userInfo[UIKeyboardAnimationCurveUserInfoKey] integerValue];

  CGFloat visibleKeyboardHeight = 0;
  if (shouldConstraintToKeyboard) {
    if ([self useAccessoryViewPosition]) {
      visibleKeyboardHeight = [self inputAccessoryHeightInWindow];
    } else {
      visibleKeyboardHeight =
          [self keyboardHeightInWindowFromNotification:notification];
    }
  }

  if (IsVivaldiRunning()) {
    // When tab bar disabled the bottom toolbar buttons shows up below
    // keyboard when omnibox is moved above. Hide the control buttons
    // during transition.
    if (!self.view.tabBarEnabled) {
      if (shouldConstraintToKeyboard && IsSplitToolbarMode(self)) {
        [self hideControlButtons];
      } else if (!shouldConstraintToKeyboard && IsSplitToolbarMode(self)) {
        [self showControlButtons];
      }
    }
  } // End Vivaldi

  [self.toolbarHeightDelegate
      adjustSecondaryToolbarForKeyboardHeight:visibleKeyboardHeight
                                  isCollapsed:self.locationIndicatorActive
                                     duration:duration
                                        curve:curve];
}

- (BOOL)useAccessoryViewPosition {
  UIView* inputAccessory = [self.layoutGuideCenter
      referencedViewUnderName:kInputAccessoryViewLayoutGuide];
  return inputAccessory != nil;
}

- (CGFloat)inputAccessoryHeightInWindow {
  UIView* inputAccessory = [self.layoutGuideCenter
      referencedViewUnderName:kInputAccessoryViewLayoutGuide];
  CGRect rectInWindowIA =
      [inputAccessory convertRect:inputAccessory.layer.presentationLayer.frame
                           toView:self.view.window];
  return self.view.window.frame.size.height - rectInWindowIA.origin.y;
}

// Returns the user visible height of the keyboard.
- (CGFloat)keyboardHeightInWindowFromNotification:
    (NSNotification*)notification {
  NSDictionary* userInfo = notification.userInfo;
  // Part of the keyboard might be hidden. Keep only the visible area.
  CGRect keyboardFrame = [userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
  id<UICoordinateSpace> fromCoordinateSpace =
      ((UIScreen*)notification.object).coordinateSpace;
  id<UICoordinateSpace> toCoordinateSpace = self.view.window;
  CGRect keyboardFrameInWindow =
      [fromCoordinateSpace convertRect:keyboardFrame
                     toCoordinateSpace:toCoordinateSpace];
  return CGRectIntersection(keyboardFrameInWindow, self.view.window.bounds)
      .size.height;
}

#pragma mark - ToolbarAnimatee

- (void)expandLocationBar {
  // NO-OP
}

- (void)contractLocationBar {
  // NO-OP
}

- (void)showCancelButton {
  // NO-OP
}

- (void)hideCancelButton {
  // NO-OP
}

- (void)showControlButtons {
  self.view.progressBar.alpha = 1;
  self.view.buttonStackView.hidden = NO;
}

- (void)hideControlButtons {
  self.view.progressBar.alpha = 0;
  self.view.buttonStackView.hidden = YES;
}

- (void)setLocationBarHeightToMatchFakeOmnibox {
  // NO-OP
}

- (void)setLocationBarHeightExpanded {
  // With multine omnibox the location bar edit state height is managed by the
  // toolbar coordinator. This will only update the expanded corner radius.
  if (IsMultilineBrowserOmniboxEnabled()) {
    self.view.locationBarContainer.layer.cornerRadius =
        LocationBarHeight(self.traitCollection.preferredContentSizeCategory) /
        2;
  }
}

// Changes related to the toolbar itself.
- (void)setToolbarFaded:(BOOL)faded {
  self.view.alpha = faded ? 0 : 1;
}

#pragma mark - Vivaldi
- (UIStackView*)toolbarButtonStackView {
  return self.view.buttonStackView;
}

- (void)updateToolbarButtonsTintColor {
  UIColor* accentColor =
      [self toolbarBackgroundColorForType:ToolbarType::kSecondary];
  UIColor* buttonsTintColor = self.isTabBarEnabled ||
      (!self.isTabBarEnabled && !self.isBottomOmniboxEnabled) ?
        [UIColor colorNamed:kToolbarButtonColor] :
            [self.buttonFactory.toolbarConfiguration
                buttonsTintColorForAccentColor:accentColor];
  self.buttonFactory.toolbarConfiguration.buttonsTintColor = buttonsTintColor;
  for (ToolbarButton *button in self.view.buttonStackView.arrangedSubviews) {
    [button updateTintColor];
  }
  [self.view.openNewTabButton updateTintColor];
}

- (void)setIsNTP:(BOOL)isNTP {
  if (_isNTP == isNTP)
    return;
  [super setIsNTP:isNTP];
  _isNTP = isNTP;

  [self updateBackgroundColor];
}

- (void)updateBackgroundColor {
  [UIView animateWithDuration:0.2 animations:^{
    self.view.backgroundColor =
        [self toolbarBackgroundColorForType:ToolbarType::kSecondary];
    [self updateToolbarButtonsTintColor];
  }];
}

- (void)updateLocationBarBackgroundColor {
  // Update omnibox background color. When tab bar is enabled its not modified
  // with accent color and rather follows the prefixed color. However, when tab
  // bar is disabled the color is calculated from the accent color so that its
  // visible regardless of the accent color.
  if (self.isTabBarEnabled || self.isOmniboxFocused) {
    self.view.locationBarContainer.backgroundColor =
        [self.buttonFactory.toolbarConfiguration
         locationBarBackgroundColorWithVisibility:1.0];
  } else {
    UIColor* accentColor =
        [self toolbarBackgroundColorForType:ToolbarType::kSecondary];
    self.view.locationBarContainer.backgroundColor =
        [self.buttonFactory.toolbarConfiguration
            locationBarBackgroundColorForAccentColor:accentColor];
  }
}

#pragma mark - AdaptiveToolbarViewController (Subclassing)
- (void)setLocationBarViewController:
    (UIViewController*)locationBarViewController {
  [super setLocationBarViewController:locationBarViewController];
  [self updateForFullscreenProgress:1];
}
// End Vivaldi

@end
