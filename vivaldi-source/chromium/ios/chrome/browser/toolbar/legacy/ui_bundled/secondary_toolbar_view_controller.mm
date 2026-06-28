// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/toolbar/legacy/ui_bundled/secondary_toolbar_view_controller.h"

#import "base/check.h"
#import "ios/chrome/browser/fullscreen/model/fullscreen_browser_agent.h"
#import "ios/chrome/browser/fullscreen/public/fullscreen_metrics.h"
#import "ios/chrome/browser/fullscreen/ui_bundled/fullscreen_controller.h"
#import "ios/chrome/browser/fullscreen/ui_bundled/scoped_fullscreen_disabler.h"
#import "ios/chrome/browser/shared/public/commands/fullscreen_commands.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/ui/util/layout_guide_names.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/shared/ui/util/util_swift.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/adaptive_toolbar_view_controller+subclassing.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/legacy_toolbar_button.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/legacy_toolbar_button_factory.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/toolbar_configuration.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_constants.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_utils.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/secondary_toolbar_keyboard_state_provider.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/secondary_toolbar_view.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/toolbar_progress_bar.h"
#import "ios/chrome/browser/toolbar/ui/toolbar_height_delegate.h"
#import "ios/chrome/common/ui/util/ui_util.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/chrome/browser/toolbar/ui/buttons/toolbar_button.h"
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

  if (IsVivaldiRunning()) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(handleApplicationWillEnterForeground)
               name:UIApplicationWillEnterForegroundNotification
             object:nil];

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
  [super disconnect];
  _fullscreenController = nullptr;
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Setters

- (void)setLocationIndicatorActive:(BOOL)locationIndicatorActive {
  if (locationIndicatorActive == _locationIndicatorActive) {
    return;
  }

  _locationIndicatorActive = locationIndicatorActive;

  FullscreenModeTransitionTrigger trigger =
      FullscreenModeTransitionTrigger::kForcedByCode;

  if (IsFullscreenRefactoringEnabled()) {
    if (locationIndicatorActive) {
      [self.fullscreenCommands enterFullscreenWithTrigger:trigger animated:YES];
    } else {
      [self.fullscreenCommands exitFullscreenWithTrigger:trigger animated:YES];
    }
  } else if (_fullscreenController) {
    if (locationIndicatorActive) {
      _fullscreenController->EnterForceFullscreenMode(
          /* insets_update_enabled */ false, trigger);
    } else {
      _fullscreenController->ExitForceFullscreenMode(trigger);
    }
  }

  if (locationIndicatorActive) {
    self.view.locationBarTopConstraint.constant = 0;
    self.view.bottomSeparator.alpha = 1.0;
    [self.toolbarHeightDelegate secondaryToolbarMovedAboveKeyboard];
  } else {
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
    // When switching to landscape, the bottom omnibox is not available. If
    // the location indicator was previously active (e.g. Find in Page was open
    // in portrait), clean up the state so that ExitForceFullscreenMode is
    // called to balance the earlier EnterForceFullscreenMode.
    // See crbug.com/498378084 for more context.
    if (self.locationIndicatorActive) {
      self.locationIndicatorActive = NO;
    }
    return;
  }

  if (IsVivaldiRunning()) {
    // Ref: VIB-1830
    // Vivaldi can resume with `keyboardIsActiveForWebContent` still set after
    // the keyboard is gone. Keep that stale-state handling in a Vivaldi-only
    // path instead of patching Chromium's transition logic inline.
    [self vivaldiConstraintToKeyboard:shouldConstraintToKeyboard
                     withNotification:notification];
    return;
  } // End Vivaldi

  // Whether to cleanup the location indication previously shown for web
  // content.
  BOOL hideLocationIndicator =
      !shouldConstraintToKeyboard && self.locationIndicatorActive;

  // Whether to show the secondary toolbar as a location indicator when keyboard
  // is active for web content or the Find navigator is visible. Bottom omnibox
  // exclusive.
  BOOL keyboardActiveForWebContent =
      [self.keyboardStateProvider keyboardIsActiveForWebContent];
  BOOL findNavigatorVisible =
      [self.keyboardStateProvider isFindNavigatorVisibleForWebContent];
  BOOL showLocationIndicator =
      shouldConstraintToKeyboard &&
      (keyboardActiveForWebContent || findNavigatorVisible) &&
      !hideLocationIndicator;

  BOOL shouldAnimateOmniboxMovement =
      showLocationIndicator || hideLocationIndicator;
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
          VisibleKeyboardHeightFromNotification(notification, self.view.window);
      // If the Find navigator is visible and the toolbar is constrained to the
      // keyboard, then add room for the collapsed toolbar to be visible above
      // the keyboard.
      if (findNavigatorVisible) {
        visibleKeyboardHeight += ToolbarCollapsedHeight(
            self.traitCollection.preferredContentSizeCategory);
      }
    }
  }

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

// The minimum height of this toolbar.
- (CGFloat)minHeight {
  UIContentSizeCategory category =
      self.traitCollection.preferredContentSizeCategory;
  return self.hasOmnibox ? ToolbarCollapsedHeight(category) : 0;
}

// The maximum height of this toolbar.
- (CGFloat)maxHeight {
  UIContentSizeCategory category =
      self.traitCollection.preferredContentSizeCategory;
  CGFloat maxHeight = self.view.intrinsicContentSize.height;
  if (self.hasOmnibox) {
    maxHeight += ToolbarExpandedHeight(category);
  }
  return maxHeight;
}

#pragma mark - FullscreenBrowserAgentObserving

- (void)fullscreenWillUpdateObscuredInsetRange:(FullscreenBrowserAgent*)agent {
  if (!IsSplitToolbarMode(self)) {
    return;
  }
  agent->AddObscuredInsetRange(UIRectEdgeBottom, [self minHeight],
                               [self maxHeight]);
}

- (void)fullscreenWillUpdateState:(FullscreenBrowserAgent*)agent {
  if (!IsSplitToolbarMode(self)) {
    return;
  }
  [self updateForFullscreenProgress:agent->bottom_progress()];
  [self.view layoutIfNeeded];
  CGFloat minHeight = [self minHeight];
  CGFloat maxHeight = [self maxHeight];
  CGFloat currentHeight =
      minHeight + (maxHeight - minHeight) * agent->bottom_progress();
  agent->AddObscuredInset(UIRectEdgeBottom, currentHeight);
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
  // NO OP.
}

// Changes related to the toolbar itself.
- (void)setToolbarFaded:(BOOL)faded {
  self.view.alpha = faded ? 0 : 1;
}

#pragma mark - Properties

- (void)setLocationBarViewController:
    (UIViewController*)locationBarViewController {
  // Resets `locationIndicatorActive` when the location bar is removed. This
  // prevents an inconsistent active indicator state (and potential crash) when
  // the secondary toolbar no longer has an omnibox.
  [super setLocationBarViewController:locationBarViewController];
  if (!self.hasOmnibox && self.locationIndicatorActive) {
    self.locationIndicatorActive = NO;
  }

  if (IsVivaldiRunning()) {
    [self updateForFullscreenProgress:1];
  } // End Vivaldi
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
  for (LegacyToolbarButton *button in
       self.view.buttonStackView.arrangedSubviews) {
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

- (void)handleApplicationWillEnterForeground { // Ref: VIB-1830
  BOOL findNavigatorVisible =
      [self.keyboardStateProvider isFindNavigatorVisibleForWebContent];
  BOOL keyboardAnchorVisible = NO;
  if ([self useAccessoryViewPosition]) {
    keyboardAnchorVisible = [self inputAccessoryHeightInWindow] > 0;
  }

  // Resume can miss the keyboard-hide teardown on iPad. If there is no visible
  // keyboard anchor anymore, clear the stale keyboard layout before the
  // secondary toolbar keeps force fullscreen latched.
  if (!findNavigatorVisible && !keyboardAnchorVisible) {
    [self resetKeyboardDrivenToolbarState];
  }
}

- (void)resetKeyboardDrivenToolbarState {
  BOOL keyboardActiveForWebContent =
      [self.keyboardStateProvider keyboardIsActiveForWebContent];
  if (!self.locationIndicatorActive && !keyboardActiveForWebContent) {
    return;
  }

  // Resign any stale "toolbar above keyboard" state after resume.
  [GetFirstResponder() resignFirstResponder];
  if (self.locationIndicatorActive) {
    self.locationIndicatorActive = NO;
  } else {
    [self.toolbarHeightDelegate secondaryToolbarRemovedFromKeyboard];
  }

  if (!self.view.tabBarEnabled && IsSplitToolbarMode(self)) {
    [self showControlButtons];
  }

  [self.toolbarHeightDelegate
      adjustSecondaryToolbarForKeyboardHeight:0
                                  isCollapsed:NO
                                     duration:0
                                        curve:UIViewAnimationCurveEaseInOut];
}

- (void)vivaldiConstraintToKeyboard:(BOOL)shouldConstraintToKeyboard
                   withNotification:(NSNotification*)notification {
  // Vivaldi keeps the stale resume handling separate from Chromium's keyboard
  // transition logic.
  CGFloat visibleKeyboardHeight = 0;
  BOOL keyboardAnchorVisible = NO;
  BOOL keyboardActiveForWebContent =
      [self.keyboardStateProvider keyboardIsActiveForWebContent];
  BOOL findNavigatorVisible =
      [self.keyboardStateProvider isFindNavigatorVisibleForWebContent];
  if (shouldConstraintToKeyboard) {
    if ([self useAccessoryViewPosition]) {
      visibleKeyboardHeight = [self inputAccessoryHeightInWindow];
      keyboardAnchorVisible = visibleKeyboardHeight > 0;
    } else {
      visibleKeyboardHeight =
          VisibleKeyboardHeightFromNotification(notification, self.view.window);
      keyboardAnchorVisible = visibleKeyboardHeight > 0;
      if (findNavigatorVisible) {
        visibleKeyboardHeight += ToolbarCollapsedHeight(
            self.traitCollection.preferredContentSizeCategory);
      }
    }
  }

  BOOL hideLocationIndicator =
      !shouldConstraintToKeyboard && self.locationIndicatorActive;
  // Only trust the web-content keyboard flag when there is a real visible
  // anchor. Resume can otherwise re-enter keyboard mode with a zero-height
  // keyboard and keep the toolbar in forced fullscreen.
  BOOL staleKeyboardActivation =
      shouldConstraintToKeyboard && keyboardActiveForWebContent &&
      !findNavigatorVisible && !keyboardAnchorVisible;
  if (staleKeyboardActivation && self.locationIndicatorActive) {
    hideLocationIndicator = YES;
  }

  BOOL showLocationIndicator =
      shouldConstraintToKeyboard &&
      (findNavigatorVisible ||
       (keyboardActiveForWebContent && keyboardAnchorVisible)) &&
      !hideLocationIndicator;
  BOOL omniboxAttachedInEditState =
      self.locationBarFocused && !keyboardActiveForWebContent;
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
    self.locationIndicatorActive = NO;
  }

  [self.view layoutIfNeeded];

  NSDictionary* userInfo = notification.userInfo;
  NSTimeInterval duration =
      [userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];
  UIViewAnimationCurve curve = (UIViewAnimationCurve)
      [userInfo[UIKeyboardAnimationCurveUserInfoKey] integerValue];

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

  [self.toolbarHeightDelegate
      adjustSecondaryToolbarForKeyboardHeight:visibleKeyboardHeight
                                  isCollapsed:self.locationIndicatorActive
                                     duration:duration
                                        curve:curve];
}

// End Vivaldi

@end
