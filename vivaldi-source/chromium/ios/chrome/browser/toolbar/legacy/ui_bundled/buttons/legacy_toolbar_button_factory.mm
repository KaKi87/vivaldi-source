// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/legacy_toolbar_button_factory.h"

#import "base/ios/ios_util.h"
#import "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/intelligence/features/features.h"
#import "ios/chrome/browser/omnibox/public/omnibox_constants.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/ui/symbols/symbols.h"
#import "ios/chrome/browser/shared/ui/util/rtl_geometry.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/legacy_toolbar_button.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/toolbar_button_actions_handler.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/toolbar_button_visibility_configuration.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/toolbar_configuration.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/toolbar_tab_grid_button.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/toolbar_tab_group_state.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_constants.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/chrome/common/ui/util/constraints_ui_util.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/grit/ios_theme_resources.h"
#import "ios/public/provider/chrome/browser/raccoon/raccoon_api.h"
#import "ui/base/l10n/l10n_util.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/chrome/browser/shared/ui/symbols/symbols.h"
#import "ios/ui/helpers/vivaldi_colors_helper.h"
#import "ios/ui/settings/vivaldi_settings_constants.h"
#import "ios/ui/toolbar/vivaldi_toolbar_constants.h"
#import "ios/ui/vivaldi_symbols/vivaldi_symbol_names.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using l10n_util::GetNSString;
using vivaldi::IsVivaldiRunning;
// End Vivaldi

namespace {

// The size of the symbol image.
const CGFloat kSymbolToolbarPointSize = 24;

// The padding to be added to the bottom of the system share icon to balance
// the white space on top.
const CGFloat kShareIconBalancingHeightPadding = 1;

}  // namespace

@implementation LegacyToolbarButtonFactory

- (instancetype)initWithStyle:(ToolbarStyle)style {
  self = [super init];
  if (self) {
    _style = style;
    _toolbarConfiguration = [[ToolbarConfiguration alloc] initWithStyle:style];
  }
  return self;
}

#pragma mark - Buttons

- (LegacyToolbarButton*)backButton {
  auto loadImageBlock = ^UIImage* {
    UIImage* backImage =
        SymbolWithPointSize(SymbolBack, kSymbolToolbarPointSize);

    if (IsVivaldiRunning())
      backImage =
          CustomSymbolWithPointSize(
              vToolbarBackButtonIcon, kSymbolToolbarPointSize); // End Vivaldi

    return [backImage imageFlippedForRightToLeftLayoutDirection];
  };

  LegacyToolbarButton* backButton =
      [[LegacyToolbarButton alloc] initWithImageLoader:loadImageBlock];

  [self configureButton:backButton width:kAdaptiveToolbarButtonWidth];
  backButton.accessibilityLabel = l10n_util::GetNSString(IDS_ACCNAME_BACK);
  backButton.accessibilityHint =
      l10n_util::GetNSString(IDS_IOS_TOOLBAR_ACCESSIBILITY_HINT_BACK);
  [backButton addTarget:self.actionHandler
                 action:@selector(backAction)
       forControlEvents:UIControlEventTouchUpInside];
  backButton.visibilityMask = self.visibilityConfiguration.backButtonVisibility;
  return backButton;
}

// Returns a forward button without visibility mask configured.
- (LegacyToolbarButton*)forwardButton {
  auto loadImageBlock = ^UIImage* {
    UIImage* forwardImage =
        SymbolWithPointSize(SymbolForward, kSymbolToolbarPointSize);

    if (IsVivaldiRunning())
      forwardImage =
        CustomSymbolWithPointSize(
            vToolbarForwardButtonIcon, kSymbolToolbarPointSize); // End Vivaldi

    return [forwardImage imageFlippedForRightToLeftLayoutDirection];
  };

  LegacyToolbarButton* forwardButton =
      [[LegacyToolbarButton alloc] initWithImageLoader:loadImageBlock];

  [self configureButton:forwardButton width:kAdaptiveToolbarButtonWidth];
  forwardButton.visibilityMask =
      self.visibilityConfiguration.forwardButtonVisibility;
  forwardButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_ACCNAME_FORWARD);
  forwardButton.accessibilityHint =
      l10n_util::GetNSString(IDS_IOS_TOOLBAR_ACCESSIBILITY_HINT_FORWARD);
  [forwardButton addTarget:self.actionHandler
                    action:@selector(forwardAction)
          forControlEvents:UIControlEventTouchUpInside];
  return forwardButton;
}

- (ToolbarTabGridButton*)tabGridButton {
  auto imageBlock = ^UIImage*(ToolbarTabGroupState tabGroupState) {

    if (IsVivaldiRunning()) {
      switch (tabGroupState) {
        case ToolbarTabGroupState::kNormal:
          return CustomSymbolWithPointSize(vToolbarTabSwitcherButtonIcon,
                                           kSymbolToolbarPointSize);
        case ToolbarTabGroupState::kTabGroup:
          return CustomSymbolWithPointSize(vToolbarTabSwitcherStackButtonIcon,
                                           kSymbolToolbarPointSize);
      }
    }  // End Vivaldi

    switch (tabGroupState) {
      case ToolbarTabGroupState::kNormal:
        return SymbolWithPointSize(SymbolSquareNumber, kSymbolToolbarPointSize);
      case ToolbarTabGroupState::kTabGroup:
        return SymbolWithPointSize(SymbolSquareFilledOnSquare,
                                   kSymbolToolbarPointSize);
    }
  };

  ToolbarTabGridButton* tabGridButton = [[ToolbarTabGridButton alloc]
      initWithTabGroupStateImageLoader:imageBlock];

  tabGridButton.accessibilityHint =
      l10n_util::GetNSString(IDS_IOS_TOOLBAR_ACCESSIBILITY_HINT_TAB_GRID);
  tabGridButton.blueDotAccessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_TAB_GROUP_NEW_ACTIVITY_LABEL_TEXT);

  [self configureButton:tabGridButton width:kAdaptiveToolbarButtonWidth];
  [tabGridButton addTarget:self.actionHandler
                    action:@selector(tabGridTouchDown)
          forControlEvents:UIControlEventTouchDown];
  [tabGridButton addTarget:self.actionHandler
                    action:@selector(tabGridTouchUp)
          forControlEvents:UIControlEventTouchUpInside];
  tabGridButton.visibilityMask =
      self.visibilityConfiguration.tabGridButtonVisibility;
  return tabGridButton;
}

- (LegacyToolbarButton*)toolsMenuButton {
  auto loadImageBlock = ^UIImage* {
    return SymbolWithPointSize(SymbolMenu, kSymbolToolbarPointSize);
  };
  UIColor* locationBarBackgroundColor =
      [self.toolbarConfiguration locationBarBackgroundColorWithVisibility:1];

  auto loadIPHHighlightedImageBlock = ^UIImage* {
    return SymbolWithPalette(
        SymbolWithPointSize(SymbolEllipsisSquareFill, kSymbolToolbarPointSize),
        @[ [UIColor colorNamed:kGrey600Color], locationBarBackgroundColor ]);
  };
  LegacyToolbarButton* toolsMenuButton = [[LegacyToolbarButton alloc]
            initWithImageLoader:loadImageBlock
      IPHHighlightedImageLoader:loadIPHHighlightedImageBlock];

  if (IsVivaldiRunning()) {
    auto iconImageBlock = ^UIImage* {
      return [UIImage imageNamed:vToolbarMenu];
    };
    toolsMenuButton =
        [[LegacyToolbarButton alloc] initWithImageLoader:iconImageBlock];
  } // End Vivaldi

  SetA11yLabelAndUiAutomationName(toolsMenuButton, IDS_IOS_TOOLBAR_SETTINGS,
                                  kLegacyToolbarToolsMenuButtonIdentifier);
  toolsMenuButton.blueDotAccessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_NEW_ITEM_ACCESSIBILITY_HINT);
  [self configureButton:toolsMenuButton width:kAdaptiveToolbarButtonWidth];
  [toolsMenuButton.heightAnchor
      constraintEqualToConstant:kAdaptiveToolbarButtonWidth]
      .active = YES;
  [toolsMenuButton addTarget:self.actionHandler
                      action:@selector(toolsMenuAction)
            forControlEvents:UIControlEventTouchUpInside];
  toolsMenuButton.visibilityMask =
      self.visibilityConfiguration.toolsMenuButtonVisibility;
  return toolsMenuButton;
}

- (LegacyToolbarButton*)shareButton {
  auto loadImageBlock = ^UIImage* {
    UIImage* image = SymbolWithPointSize(SymbolShare, kSymbolToolbarPointSize);

    // The system share image has uneven vertical padding. Add a small bottom
    // padding to balance it.
    UIGraphicsImageRendererFormat* format =
        [UIGraphicsImageRendererFormat preferredFormat];
    format.scale = 0.0;
    format.opaque = NO;
    UIGraphicsImageRenderer* renderer = [[UIGraphicsImageRenderer alloc]
        initWithSize:CGSizeMake(
                         image.size.width,
                         image.size.height + kShareIconBalancingHeightPadding)
              format:format];

    return
        [renderer imageWithActions:^(UIGraphicsImageRendererContext* context) {
          [image
              drawInRect:CGRectMake(0, 0, image.size.width, image.size.height)];
        }];
  };

  LegacyToolbarButton* shareButton =
      [[LegacyToolbarButton alloc] initWithImageLoader:loadImageBlock];

  [self configureButton:shareButton width:kAdaptiveToolbarButtonWidth];
  SetA11yLabelAndUiAutomationName(shareButton, IDS_IOS_TOOLS_MENU_SHARE,
                                  kLegacyToolbarShareButtonIdentifier);
  shareButton.titleLabel.text = @"Share";
  [shareButton addTarget:self.actionHandler
                  action:@selector(shareAction:)
        forControlEvents:UIControlEventTouchUpInside];
  shareButton.visibilityMask =
      self.visibilityConfiguration.shareButtonVisibility;
  return shareButton;
}

- (LegacyToolbarButton*)reloadButton {
  auto loadImageBlock = ^UIImage* {
    return SymbolWithPointSize(SymbolArrowClockWise, kSymbolToolbarPointSize);
  };

  LegacyToolbarButton* reloadButton =
      [[LegacyToolbarButton alloc] initWithImageLoader:loadImageBlock];

  [self configureButton:reloadButton width:kAdaptiveToolbarButtonWidth];
  reloadButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_ACCNAME_RELOAD);
  [reloadButton addTarget:self.actionHandler
                   action:@selector(reloadAction)
         forControlEvents:UIControlEventTouchUpInside];
  reloadButton.visibilityMask =
      self.visibilityConfiguration.reloadButtonVisibility;
  return reloadButton;
}

- (LegacyToolbarButton*)stopButton {
  auto loadImageBlock = ^UIImage* {
    return SymbolWithPointSize(SymbolXMark, kSymbolToolbarPointSize);
  };

  LegacyToolbarButton* stopButton =
      [[LegacyToolbarButton alloc] initWithImageLoader:loadImageBlock];

  [self configureButton:stopButton width:kAdaptiveToolbarButtonWidth];
  stopButton.accessibilityLabel = l10n_util::GetNSString(IDS_IOS_ACCNAME_STOP);
  [stopButton addTarget:self.actionHandler
                 action:@selector(stopAction)
       forControlEvents:UIControlEventTouchUpInside];
  stopButton.visibilityMask = self.visibilityConfiguration.stopButtonVisibility;
  return stopButton;
}

- (LegacyToolbarButton*)openNewTabButton {
  UIColor* locationBarBackgroundColor =
      [self.toolbarConfiguration locationBarBackgroundColorWithVisibility:1];
  UIColor* buttonsTintColorIPHHighlighted =
      self.toolbarConfiguration.buttonsTintColorIPHHighlighted;
  UIColor* buttonsIPHHighlightColor =
      self.toolbarConfiguration.buttonsIPHHighlightColor;

  auto loadImageBlock = ^UIImage* {
    return SymbolWithPalette(
        SymbolWithPointSize(SymbolPlusCircleFill, kSymbolToolbarPointSize),
        @[ [UIColor colorNamed:kGrey600Color], locationBarBackgroundColor ]);
  };

  auto loadIPHHighlightedImageBlock = ^UIImage* {
    return SymbolWithPalette(
        SymbolWithPointSize(SymbolPlusCircleFill, kSymbolToolbarPointSize), @[
          // The color of the 'plus'.
          buttonsTintColorIPHHighlighted,
          // The filling color of the circle.
          buttonsIPHHighlightColor,
        ]);
  };

  LegacyToolbarButton* newTabButton = nil;
  if (IsVivaldiRunning()) {
    auto iconImageBlock = ^UIImage* {
      UIImage* newTabButtonImage =
          [CustomSymbolWithPointSize(vMenuNewTab, kSymbolToolbarPointSize)
              imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
      return [newTabButtonImage imageFlippedForRightToLeftLayoutDirection];
    };
    newTabButton =
        [[LegacyToolbarButton alloc] initWithImageLoader:iconImageBlock];
  } else
  newTabButton = [[LegacyToolbarButton alloc]
            initWithImageLoader:loadImageBlock
      IPHHighlightedImageLoader:loadIPHHighlightedImageBlock];

  [newTabButton addTarget:self.actionHandler
                   action:@selector(newTabAction:)
         forControlEvents:UIControlEventTouchUpInside];

  [self configureButton:newTabButton width:kAdaptiveToolbarButtonWidth];

  newTabButton.accessibilityLabel = [self.toolbarConfiguration
      accessibilityLabelForOpenNewTabButtonInGroup:NO];
  newTabButton.accessibilityIdentifier = kToolbarNewTabButtonIdentifier;
  newTabButton.accessibilityHint =
      l10n_util::GetNSString(IDS_IOS_TOOLBAR_ACCESSIBILITY_HINT_NEW_TAB);

  newTabButton.visibilityMask =
      self.visibilityConfiguration.newTabButtonVisibility;
  return newTabButton;
}

- (UIButton*)cancelButton {
  UIButton* cancelButton = [UIButton buttonWithType:UIButtonTypeSystem];
  cancelButton.tintColor = [UIColor colorNamed:kBlueColor];
  [cancelButton setContentHuggingPriority:UILayoutPriorityRequired
                                  forAxis:UILayoutConstraintAxisHorizontal];
  [cancelButton
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];

  UIButtonConfiguration* buttonConfiguration =
      [UIButtonConfiguration plainButtonConfiguration];
  buttonConfiguration.contentInsets = NSDirectionalEdgeInsetsMake(
      0, kCancelButtonHorizontalInset, 0, kCancelButtonHorizontalInset);
  UIFont* font = [UIFont systemFontOfSize:kLocationBarFontSize];
  NSDictionary* attributes = @{NSFontAttributeName : font};
  NSMutableAttributedString* attributedString =
      [[NSMutableAttributedString alloc]
          initWithString:l10n_util::GetNSString(IDS_CANCEL)
              attributes:attributes];
  buttonConfiguration.attributedTitle = attributedString;
  cancelButton.configuration = buttonConfiguration;

  cancelButton.hidden = YES;
  [cancelButton addTarget:self.actionHandler
                   action:@selector(cancelOmniboxFocusAction)
         forControlEvents:UIControlEventTouchUpInside];
  cancelButton.accessibilityIdentifier =
      kOmniboxCancelButtonAccessibilityIdentifier;
  return cancelButton;
}

#pragma mark: - VIVALDI
- (LegacyToolbarButton*)panelButton {
  auto iconImageBlock = ^UIImage* {
    UIImage* panelImage =
        [CustomSymbolWithPointSize(vToolbarPanelButtonIcon,
                                   kSymbolToolbarPointSize)
            imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    return [panelImage imageFlippedForRightToLeftLayoutDirection];
  };
  LegacyToolbarButton* panelButton =
    [[LegacyToolbarButton alloc] initWithImageLoader:iconImageBlock];
  [self configureButton:panelButton width:kAdaptiveToolbarButtonWidth];
  panelButton.accessibilityLabel = GetNSString(IDS_ACCNAME_PANEL);
  [panelButton addTarget:self.actionHandler
                 action:@selector(panelAction)
       forControlEvents:UIControlEventTouchUpInside];
  panelButton.visibilityMask =
      self.visibilityConfiguration.toolsMenuButtonVisibility;
  return panelButton;
}

// Vivaldi search button -> Visible only on new tab page.
- (LegacyToolbarButton*)vivaldiSearchButton {
  auto iconImageBlock = ^UIImage* {
    UIImage* searchImage =
        CustomSymbolWithPointSize(vSearch, kSymbolToolbarPointSize);
    return [searchImage imageFlippedForRightToLeftLayoutDirection];
  };

  LegacyToolbarButton* searchButton =
      [[LegacyToolbarButton alloc] initWithImageLoader:iconImageBlock];
  [self configureButton:searchButton width:kAdaptiveToolbarButtonWidth];
  searchButton.accessibilityLabel = GetNSString(IDS_ACCNAME_SEARCH);
  [searchButton addTarget:self.actionHandler
                 action:@selector(vivaldiSearchAction)
       forControlEvents:UIControlEventTouchUpInside];
  searchButton.visibilityMask =
      self.visibilityConfiguration.toolsMenuButtonVisibility;
  return searchButton;
}

  // Vivaldi Homepage Buttton
- (LegacyToolbarButton*)vivaldiHomeButton {
  auto iconImageBlock = ^UIImage* {
    UIImage* homeImage =
        [CustomSymbolWithPointSize(vToolbarHomeButtonIcon,
                                   kSymbolToolbarPointSize)
            imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    return [homeImage imageFlippedForRightToLeftLayoutDirection];
  };

  LegacyToolbarButton* homeButton =
    [[LegacyToolbarButton alloc] initWithImageLoader:iconImageBlock];
  [self configureButton:homeButton width:kAdaptiveToolbarButtonWidth];
  [homeButton addTarget:self.actionHandler
                 action:@selector(vivaldiHomeAction)
       forControlEvents:UIControlEventTouchUpInside];
  homeButton.visibilityMask =
      self.visibilityConfiguration.toolsMenuButtonVisibility;
  return homeButton;
}

// Visible only in iPhone portrait + Tab bar enabled + bottom omnibox enabled
// state.
- (LegacyToolbarButton*)vivaldiMoreButton {
  auto iconImageBlock = ^UIImage* {
    UIImage* moreImage =
        [CustomSymbolWithPointSize(vToolbarMoreButtonIcon,
                                   kSymbolToolbarPointSize)
            imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    return [moreImage imageFlippedForRightToLeftLayoutDirection];
  };

  LegacyToolbarButton* moreButton =
      [[LegacyToolbarButton alloc] initWithImageLoader:iconImageBlock];
  [self configureButton:moreButton width:kAdaptiveToolbarButtonWidth];
  moreButton.accessibilityLabel = GetNSString(IDS_ACCNAME_MORE);
  moreButton.visibilityMask =
    self.visibilityConfiguration.toolsMenuButtonVisibility;
  moreButton.showsMenuAsPrimaryAction = YES;
  return moreButton;
}

- (UIMenu*)overflowMenuWithNavForwardEnabled:(BOOL)navigationForwardEnabled
                          navBackwordEnabled:(BOOL)navigationBackwordEnabled {

  NSMutableArray* overflowActions = [NSMutableArray array];

  // Add common actions
  [overflowActions addObject:self.tabSwitcherAction];
  [overflowActions addObject:self.panelAction];

  // Conditionally add navigation actions
  if (navigationBackwordEnabled) {
    [overflowActions addObject:self.navigationBackwordAction];
  }

  if (navigationForwardEnabled) {
    [overflowActions addObject:self.navigationForwardAction];
  }

  // Create and return the menu with the actions
  UIMenu* menu = [UIMenu menuWithTitle:@"" children:overflowActions];
  return menu;
}

#pragma mark - Private

- (UIAction*)panelAction {
  NSString* buttonTitle = GetNSString(IDS_IOS_TOOLBAR_VIVALDI_PANEL);
  UIImage* buttonIcon =
      [CustomSymbolWithPointSize(vToolbarPanelButtonIcon,
                                 kSymbolToolbarPointSize)
          imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  UIAction* panelAction =
      [UIAction actionWithTitle:buttonTitle
                          image:buttonIcon
                     identifier:nil
                        handler:^(__kindof UIAction*_Nonnull
                                  action) {
        [self.actionHandler panelAction];
      }];
  panelAction.accessibilityLabel = buttonTitle;
  return panelAction;
}

- (UIAction*)navigationForwardAction {
  NSString* buttonTitle = GetNSString(IDS_IOS_TOOLBAR_OVERFLOW_FORWARD);
  UIImage* buttonIcon =
      [CustomSymbolWithPointSize(vToolbarForwardButtonIcon,
                                 kSymbolToolbarPointSize)
          imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  UIAction* forwardAction =
      [UIAction actionWithTitle:buttonTitle
                          image:buttonIcon
                     identifier:nil
                        handler:^(__kindof UIAction*_Nonnull
                                  action) {
        [self.actionHandler forwardAction];
      }];
  forwardAction.accessibilityLabel = buttonTitle;
  return forwardAction;
}

- (UIAction*)navigationBackwordAction {
  NSString* buttonTitle = GetNSString(IDS_IOS_TOOLBAR_OVERFLOW_BACK);
  UIImage* buttonIcon =
      [CustomSymbolWithPointSize(vToolbarBackButtonIcon,
                                 kSymbolToolbarPointSize)
          imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  UIAction* backAction =
      [UIAction actionWithTitle:buttonTitle
                          image:buttonIcon
                     identifier:nil
                        handler:^(__kindof UIAction*_Nonnull
                                  action) {
        [self.actionHandler backAction];
      }];
  backAction.accessibilityLabel = buttonTitle;
  return backAction;
}

- (UIAction*)tabSwitcherAction {
  NSString* buttonTitle = GetNSString(IDS_IOS_TOOLBAR_OVERFLOW_TAB_SWITCHER);
  UIImage* buttonIcon =
      [CustomSymbolWithPointSize(vToolbarTabSwitcherOveflowButtonIcon,
                                 kSymbolToolbarPointSize)
          imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  UIAction* tabSwitcherAction =
      [UIAction actionWithTitle:buttonTitle
                          image:buttonIcon
                     identifier:nil
                        handler:^(__kindof UIAction*_Nonnull
                                  action) {
        [self.actionHandler tabGridTouchDown];
        [self.actionHandler tabGridTouchUp];
      }];
  tabSwitcherAction.accessibilityLabel = buttonTitle;
  return tabSwitcherAction;
}

// Apply default tint color to the image and return.
- (UIImage*)toolbarButtonWithImage:(NSString*)image {
  UIColor* tintColor = [UIColor colorNamed:vToolbarButtonColor];
  UIImage* buttonIcon =
      [CustomSymbolWithPointSize(image, kSymbolToolbarPointSize)
          imageWithTintColor:tintColor
               renderingMode:UIImageRenderingModeAlwaysTemplate];
  return buttonIcon;
}

// End Vivaldi

#pragma mark - Helpers

// Sets the `button` width to `width` with a priority of
// UILayoutPriorityRequired - 1. If the priority is `UILayoutPriorityRequired`,
// there is a conflict when the buttons are hidden as the stack view is setting
// their width to 0. Setting the priority to UILayoutPriorityDefaultHigh doesn't
// work as they would have a lower priority than other elements.
- (void)configureButton:(LegacyToolbarButton*)button width:(CGFloat)width {
  NSLayoutConstraint* constraint =
      [button.widthAnchor constraintEqualToConstant:width];
  constraint.priority = UILayoutPriorityRequired - 1;
  constraint.active = YES;
  button.toolbarConfiguration = self.toolbarConfiguration;
  button.exclusiveTouch = YES;
  button.pointerInteractionEnabled = YES;
  button.geminiHandler = self.geminiHandler;
  if (ios::provider::IsRaccoonEnabled()) {
    button.hoverStyle = [UIHoverStyle
        styleWithShape:[UIShape rectShapeWithCornerRadius:width / 4]];
  }
  button.pointerStyleProvider =
      ^UIPointerStyle*(UIButton* uiButton, UIPointerEffect* proposedEffect,
                       UIPointerShape* proposedShape) {
        // This gets rid of a thin border on a spotlighted bookmarks button.
        // This is applied to all toolbar buttons for consistency.
        CGRect rect = CGRectInset(uiButton.frame, 1, 1);
        UIPointerShape* shape = [UIPointerShape shapeWithRoundedRect:rect];
        return [UIPointerStyle styleWithEffect:proposedEffect shape:shape];
      };
}

@end
