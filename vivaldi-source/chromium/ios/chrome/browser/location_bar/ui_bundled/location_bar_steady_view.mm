// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/location_bar/ui_bundled/location_bar_steady_view.h"

#import "base/check.h"
#import "base/check_op.h"
#import "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/badges/ui_bundled/badge_view_visibility_delegate.h"
#import "ios/chrome/browser/content_suggestions/ui/content_suggestions_collection_utils.h"
#import "ios/chrome/browser/contextual_panel/entrypoint/ui/contextual_panel_entrypoint_visibility_delegate.h"
#import "ios/chrome/browser/intelligence/features/features.h"
#import "ios/chrome/browser/location_bar/ui_bundled/badges_container_view.h"
#import "ios/chrome/browser/location_bar/ui_bundled/location_bar_constants.h"
#import "ios/chrome/browser/omnibox/public/omnibox_constants.h"
#import "ios/chrome/browser/shared/public/commands/page_action_menu_commands.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/ui/elements/extended_touch_target_button.h"
#import "ios/chrome/browser/shared/ui/symbols/symbols.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/chrome/common/ui/util/constraints_ui_util.h"
#import "ios/chrome/common/ui/util/pointer_interaction_util.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ui/base/l10n/l10n_util.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/chrome/browser/popup_menu/public/popup_menu_constants.h"
#import "ios/chrome/browser/ui/location_bar/location_bar_constants+vivaldi.h"
#import "ios/chrome/browser/ui/location_bar/vivaldi_location_bar_steady_view_container.h"
#import "ios/ui/ad_tracker_blocker/vivaldi_atb_constants.h"
#import "ios/ui/helpers/vivaldi_global_helpers.h"
#import "ios/ui/ntp/vivaldi_ntp_constants.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using vivaldi::IsVivaldiRunning;
// End Vivaldi

namespace {

// Length of the trailing button side.
const CGFloat kButtonSize = 24;
// The offset to be applied to the centerig constraints when in incognito.
const CGFloat kIncognitoCenteringOffset = 3;
// Space between the incognito image and the location icon or label.
const CGFloat kIncognitoImageToLocationSpacing = 8;
// The size of the incognito image.
const CGFloat kIncognitoImageSize = 15;
// Space between the location icon and the location label.
const CGFloat kLocationImageToLabelSpacing = -2.0;
// Minimal horizontal padding between the leading edge of the location bar and
// the content of the location bar.
const CGFloat kLocationBarLeadingPadding = 8.0;
// Trailing space between the trailing button and the trailing edge of the
// location bar.
const CGFloat kShareButtonTrailingSpacing = -11;
const CGFloat kVoiceSearchButtonTrailingSpacing = -7;
// Location label vertical offset.
const CGFloat kLocationLabelVerticalOffset = -1;
// The margin from the leading side when not centered.
const CGFloat kLeadingMargin = 20;
// The multiplier for the smaller location label font, used when animating in
// the large Contextual Panel entrypoint.
const CGFloat kSmallerLocationLabelFontMultiplier = 0.75;
// The duration of the custom leading view fade animation.
const CGFloat kCustomLeadingViewAnimationDuration = 0.3;
}  // namespace

@interface LocationBarSteadyView ()

// The image view displaying the current location icon (i.e. http[s] status).
@property(nonatomic, strong) UIImageView* locationIconImageView;

#if !defined(VIVALDI_BUILD)
// The view containing the location label, and (sometimes) the location image
// view.
@property(nonatomic, strong) UIView* locationContainerView;
#endif // End Vivaldi

// Leading constraint for locationContainerView when there is no BadgeView to
// its left.
@property(nonatomic, strong)
    NSLayoutConstraint* locationContainerViewLeadingAnchorConstraint;

// The constraint that pins the trailingButton to the trailing edge of the
// location bar.
@property(nonatomic, strong)
    NSLayoutConstraint* trailingButtonTrailingAnchorConstraint;

// The trailing spacing to be used for the trailingButton. This property is
// based on the type of trailing button in use (i.e. share or voice search).
@property(nonatomic, readonly) CGFloat trailingButtonTrailingSpacing;

// Constraints to pin the badges container stackview to the right next to the
// `locationContainerView`.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* badgesViewFullScreenEnabledConstraints;

// Constraints to pin the badges container stackview to the left side of the
// LocationBar.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* badgesViewFullScreenDisabledConstraints;

// Elements to surface in accessibility.
@property(nonatomic, strong) NSMutableArray* accessibleElements;

// Vivaldi
// Constraints to hide the location image view.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* hideLocationImageConstraints;

// Constraints to show the location image view.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* showLocationImageConstraints;

// The image view displaying the current site connection security status.
@property(nonatomic, strong) UIImageView* connectionIconImageView;

// Constraints to hide the site connection status image view.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* hideConnectionStatusImageConstraints;

// Constraints to show the site connection status image view.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* showConnectionStatusImageConstraints;

// Constraints to hide the location image view and shield button.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* hideLocationAndConnectionImageConstraints;

// Constraints to preserve the leading button slot for real-page text when the
// leading button/icon state has not settled yet.
@property(nonatomic, strong) NSArray<NSLayoutConstraint*>*
    hideLocationAndConnectionImageReservedSpaceConstraints;

// Constraints to show the location image view and shield button.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* showLocationAndConnectionImageConstraints;
// End Vivaldi

@end

#pragma mark - LocationBarSteadyViewColorScheme

@implementation LocationBarSteadyViewColorScheme

+ (instancetype)standardScheme {
  LocationBarSteadyViewColorScheme* scheme =
      [[LocationBarSteadyViewColorScheme alloc] init];

  scheme.fontColor = [UIColor colorNamed:kTextPrimaryColor];
  scheme.placeholderColor = content_suggestions::SearchHintLabelColor();
  scheme.trailingButtonColor = [UIColor colorNamed:kGrey600Color];

  return scheme;
}

@end

#pragma mark - LocationBarSteadyButton

// Buttons with a darker background in highlighted state.
@interface LocationBarSteadyButton : UIButton
@end

@implementation LocationBarSteadyButton

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.pointerInteractionEnabled = YES;
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];

  if (IsVivaldiRunning()) {
    self.layer.cornerRadius = vNTPSearchBarCornerRadius;
  } else {
  self.layer.cornerRadius = self.bounds.size.height / 2.0;
  } // End Vivaldi

}

- (void)setHighlighted:(BOOL)highlighted {
  [super setHighlighted:highlighted];

  // Note: (prio@vivaldi.com) We don't want to highlight the steady view since
  // that ends up showing a visible glitch in transition as the UI begins to
  // move at the same time.
  if (IsVivaldiRunning())
    return;

  CGFloat duration = highlighted ? 0.1 : 0.2;
  [UIView animateWithDuration:duration
                        delay:0
                      options:UIViewAnimationOptionBeginFromCurrentState
                   animations:^{
                     CGFloat alpha = highlighted ? 0.07 : 0;
                     self.backgroundColor = [UIColor colorWithWhite:0
                                                              alpha:alpha];
                   }
                   completion:nil];
}

@end

#pragma mark - LocationBarSteadyView

@implementation LocationBarSteadyView {
  // The different X anchor constraints that can apply to the location label at
  // a given time.
  NSLayoutConstraint* _xStickToLeadingSideConstraint;
  NSLayoutConstraint* _xAbsoluteCenteredConstraint;
  NSLayoutConstraint* _xRelativeToContentCenteredConstraint;

  // LayoutGuide centered between the contents at the edges of the location bar.
  // (i.e. the layout guide will push towards the trailing side when the
  // entrypoint is present on the leading edge.)
  UILayoutGuide* _centeredBetweenLocationBarContentsLayoutGuide;

  // The trailing view that is hidden by default, shown for highlight mode.
  UIView* _trailingButtonSpotlightView;

  // The image view displaying the incognito icon.
  UIImageView* _incognitoImageView;

  // Whether the current text is a placeholder.
  BOOL _isShowingPlaceholder;

  // Spacing between the custom leading view and the URL label.
  CGFloat _customLeadingViewSpacing;

  // Target width for the custom leading view when visible.
  CGFloat _customLeadingViewTargetWidth;

  // Custom view added to the left of the location label.
  UIView* _customLeadingView;
  UIView* _customLeadingViewContainer;

  // Width constraint for custom leading view container (used for animation).
  NSLayoutConstraint* _customLeadingViewWidthConstraint;

  // Leading constraint for custom leading view (used for animation).
  NSLayoutConstraint* _customLeadingViewLeadingConstraint;

  // Array of active constraints for the content views inside
  // `locationContainerView`.
  NSArray<NSLayoutConstraint*>* _containerActiveConstraints;
}

- (instancetype)init {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    [self setUpViews];
    [self setUpLayout];
  }
  [self setUpAccessibility];
  return self;
}

- (void)updateCustomLeadingViewVisibility:(BOOL)visible
                                 animated:(BOOL)animated {
  CGFloat targetAlpha = visible ? 1.0 : 0.0;
  if (_customLeadingView.hidden == !visible &&
      _customLeadingView.alpha == targetAlpha) {
    return;
  }

  CGFloat priorSpacing =
      [self shouldShowIncognitoBadge] ? kIncognitoImageToLocationSpacing : 0.0;
  CGFloat targetWidth = visible ? _customLeadingViewTargetWidth : 0.0;
  CGFloat targetSpacing =
      visible ? priorSpacing : (priorSpacing - _customLeadingViewSpacing);
  CGAffineTransform targetTransform =
      visible ? CGAffineTransformIdentity
              : CGAffineTransformMakeScale(0.01, 0.01);

  if (!animated) {
    _customLeadingView.hidden = !visible;
    [self updateContainerConstraints];

    _customLeadingViewWidthConstraint.constant = targetWidth;
    _customLeadingViewLeadingConstraint.constant = targetSpacing;
    _customLeadingView.transform = targetTransform;
    _customLeadingView.alpha = targetAlpha;
    [self updateAccessibility];
    [self layoutIfNeeded];
    return;
  }

  if (visible) {
    _customLeadingView.hidden = NO;
    [self updateAccessibility];
    [self updateContainerConstraints];
  }

  NSLayoutConstraint* widthConstraint = _customLeadingViewWidthConstraint;
  NSLayoutConstraint* leadingConstraint = _customLeadingViewLeadingConstraint;
  UIView* customLeadingView = _customLeadingView;
  __weak LocationBarSteadyView* weakSelf = self;

  [UIView animateWithDuration:kCustomLeadingViewAnimationDuration
      animations:^{
        widthConstraint.constant = targetWidth;
        leadingConstraint.constant = targetSpacing;
        customLeadingView.transform = targetTransform;
        customLeadingView.alpha = targetAlpha;
        [weakSelf layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        if (!visible && finished) {
          customLeadingView.hidden = YES;
          [weakSelf updateAccessibility];
          [weakSelf updateContainerConstraints];
        }
      }];
}

- (void)setUpViews {
  _locationLabel = [[UILabel alloc] init];
  _locationIconImageView = [[UIImageView alloc] init];
  _locationIconImageView.translatesAutoresizingMaskIntoConstraints = NO;
  [_locationIconImageView
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];
  SetA11yLabelAndUiAutomationName(
      _locationIconImageView,
      IDS_IOS_PAGE_INFO_SECURITY_BUTTON_ACCESSIBILITY_LABEL,
      @"Page Security Info");
  _locationIconImageView.isAccessibilityElement = YES;

  // Setup trailing button.
  _trailingButton =
      [ExtendedTouchTargetButton buttonWithType:UIButtonTypeSystem];
  _trailingButton.hidden = IsChromeNextIaEnabled();
  _trailingButton.translatesAutoresizingMaskIntoConstraints = NO;
  _trailingButton.pointerInteractionEnabled = YES;
  // Make the pointer shape fit the location bar's semi-circle end shape.
  _trailingButton.pointerStyleProvider =
      CreateLiftEffectCirclePointerStyleProvider();

  // Setup label.
  _locationLabel.lineBreakMode = NSLineBreakByTruncatingHead;
  _locationLabel.translatesAutoresizingMaskIntoConstraints = NO;
  [_locationLabel
      setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                      forAxis:UILayoutConstraintAxisVertical];
  if (IsChromeNextIaEnabled()) {
    _locationLabel.font =
        PreferredFontForTextStyle(UIFontTextStyleCallout, UIFontWeightMedium);
  } else {

    if (IsVivaldiRunning()) {
      _locationLabel.font =
          [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
    } else {
    _locationLabel.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    } // End Vivaldi

  }
  _locationLabel.adjustsFontForContentSizeCategory = YES;
  _locationLabel.maximumContentSizeCategory =
      IsChromeNextIaEnabled() ? LocationBarSteadyViewMaxSizeCategory()
                              : LegacyLocationBarSteadyViewMaxSizeCategory();

  // Container for location label and icon.
  _locationContainerView = [[UIView alloc] init];
  _locationContainerView.translatesAutoresizingMaskIntoConstraints = NO;
  _locationContainerView.userInteractionEnabled = NO;
  [_locationContainerView addSubview:_locationLabel];

  _trailingButtonSpotlightView = [[UIView alloc] init];
  _trailingButtonSpotlightView.translatesAutoresizingMaskIntoConstraints = NO;
  _trailingButtonSpotlightView.hidden = YES;
  _trailingButtonSpotlightView.userInteractionEnabled = NO;
  _trailingButtonSpotlightView.backgroundColor =
      [UIColor colorNamed:kBlueColor];

  _locationButton = [[LocationBarSteadyButton alloc] init];
  _locationButton.translatesAutoresizingMaskIntoConstraints = NO;
  [_locationButton addSubview:_trailingButton];
  [_locationButton insertSubview:_trailingButtonSpotlightView
                    belowSubview:_trailingButton];
  [_locationButton addSubview:_locationContainerView];
  AddSameCenterConstraints(_trailingButton, _trailingButtonSpotlightView);

  [self addSubview:_locationButton];

  AddSameConstraints(self, _locationButton);

  // Badges (infobar badge , Contextual Panel & Lens Overlay entypoints)
  // container view.
  _badgesContainerView = [[LocationBarBadgesContainerView alloc] init];
  _badgesContainerView.translatesAutoresizingMaskIntoConstraints = NO;
  [_locationButton addSubview:_badgesContainerView];
}

- (void)setUpLayout {
    if (IsVivaldiRunning()) {
      // Hide location icon image view always as its replaced with site info.
      _locationIconImageView.hidden = YES;

      // Connection status icon
      _connectionIconImageView = [[UIImageView alloc] init];
      _connectionIconImageView.translatesAutoresizingMaskIntoConstraints = NO;
      [_connectionIconImageView
          setContentCompressionResistancePriority:UILayoutPriorityRequired
              forAxis:UILayoutConstraintAxisHorizontal];
      SetA11yLabelAndUiAutomationName(
            _connectionIconImageView,
            IDS_IOS_PAGE_INFO_SECURITY_BUTTON_ACCESSIBILITY_LABEL,
            @"Page Security Info");
      _connectionIconImageView.isAccessibilityElement = YES;

      // Setup leading button.
      _leadingButton =
          [ExtendedTouchTargetButton buttonWithType:UIButtonTypeSystem];
      _leadingButton.translatesAutoresizingMaskIntoConstraints = NO;
      _leadingButton.pointerInteractionEnabled = YES;
      // Make the pointer shape fit the location bar's semi-circle end shape.
      _leadingButton.pointerStyleProvider =
          CreateLiftEffectCirclePointerStyleProvider();
      [_leadingButton setImage:[UIImage imageNamed:vATBShieldNone]
                      forState:UIControlStateNormal];
      _leadingButton.accessibilityLabel =
          l10n_util::GetNSString(IDS_ACCNAME_ATB);
      _leadingButton.accessibilityIdentifier =
          kToolsMenuSiteInformation;
      [_locationContainerView addSubview:_leadingButton];
      AttachVivaldiLocationBarLeadingButtonContainer(_locationButton,
                                                     _leadingButton);

      _locationContainerView.userInteractionEnabled = YES;

      // Resuse the location image contraints property from chromium for leading
      // button since the name is only different, and the functional behaviour
      // is same for the leading button and the image which is not present on
      // the UI.
      _showLocationImageConstraints = @[
        [_locationContainerView.leadingAnchor
            constraintEqualToAnchor:_leadingButton.leadingAnchor],
        [_leadingButton.trailingAnchor
            constraintEqualToAnchor:_locationLabel.leadingAnchor
                constant:vLocationBarSteadyViewLocationImageToLabelSpacing],
        [_locationLabel.trailingAnchor
            constraintLessThanOrEqualToAnchor:
              _locationContainerView.trailingAnchor],
        [_leadingButton.centerYAnchor
            constraintEqualToAnchor:_locationContainerView.centerYAnchor],
        [_leadingButton.widthAnchor constraintEqualToConstant:kButtonSize],
        [_leadingButton.heightAnchor constraintEqualToConstant:kButtonSize],
      ];

      _showConnectionStatusImageConstraints = @[
        [_connectionIconImageView.leadingAnchor
            constraintEqualToAnchor:_locationContainerView.leadingAnchor],
        [_connectionIconImageView.trailingAnchor
            constraintEqualToAnchor:_locationLabel.leadingAnchor
                  constant:vLocationBarSteadyViewLocationImageToLabelSpacing],
        [_connectionIconImageView.centerYAnchor
            constraintEqualToAnchor:_locationContainerView.centerYAnchor],
        [_connectionIconImageView.widthAnchor
            constraintEqualToConstant:vLocationBarSiteConnectionStatusIconSize],
        [_connectionIconImageView.heightAnchor
            constraintEqualToConstant:vLocationBarSiteConnectionStatusIconSize],
      ];

      _showLocationAndConnectionImageConstraints = @[
        [_locationContainerView.leadingAnchor
            constraintEqualToAnchor:_leadingButton.leadingAnchor],
        [_locationLabel.trailingAnchor
            constraintLessThanOrEqualToAnchor:
              _locationContainerView.trailingAnchor],
        [_leadingButton.centerYAnchor
            constraintEqualToAnchor:_locationContainerView.centerYAnchor],
        [_leadingButton.widthAnchor constraintEqualToConstant:kButtonSize],
        [_leadingButton.heightAnchor constraintEqualToConstant:kButtonSize],

        [_connectionIconImageView.leadingAnchor
            constraintEqualToAnchor:_leadingButton.trailingAnchor
                constant:-vLocationBarSteadyViewLocationImageToLabelSpacing],
        [_connectionIconImageView.trailingAnchor
            constraintEqualToAnchor:_locationLabel.leadingAnchor
                  constant:vLocationBarSteadyViewLocationImageToLabelSpacing],
        [_connectionIconImageView.centerYAnchor
            constraintEqualToAnchor:_locationContainerView.centerYAnchor],
        [_connectionIconImageView.widthAnchor
            constraintEqualToConstant:vLocationBarSiteConnectionStatusIconSize],
        [_connectionIconImageView.heightAnchor
            constraintEqualToConstant:vLocationBarSiteConnectionStatusIconSize],
      ];

      _hideConnectionStatusImageConstraints = @[
        [_leadingButton.trailingAnchor
            constraintEqualToAnchor:_locationLabel.leadingAnchor
                constant:vLocationBarSteadyViewLocationImageToLabelSpacing],
      ];

      _hideLocationImageConstraints = @[
        [_locationLabel.trailingAnchor
            constraintEqualToAnchor:_locationContainerView.trailingAnchor],
      ];

      _hideLocationAndConnectionImageConstraints = @[
        [_locationContainerView.leadingAnchor
            constraintEqualToAnchor:_locationLabel.leadingAnchor],
        [_locationLabel.trailingAnchor
            constraintEqualToAnchor:_locationContainerView.trailingAnchor],
      ];

      _hideLocationAndConnectionImageReservedSpaceConstraints = @[
        [_locationContainerView.leadingAnchor
            constraintEqualToAnchor:_leadingButton.leadingAnchor],
        [_leadingButton.trailingAnchor
            constraintEqualToAnchor:_locationLabel.leadingAnchor
                constant:vLocationBarSteadyViewLocationImageToLabelSpacing],
        [_locationLabel.trailingAnchor
            constraintEqualToAnchor:_locationContainerView.trailingAnchor],
        [_leadingButton.centerYAnchor
            constraintEqualToAnchor:_locationContainerView.centerYAnchor],
        [_leadingButton.widthAnchor constraintEqualToConstant:kButtonSize],
        [_leadingButton.heightAnchor constraintEqualToConstant:kButtonSize],
      ];

    } else {
    _showLocationImageConstraints = @[
    [_locationContainerView.leadingAnchor
        constraintEqualToAnchor:_locationIconImageView.leadingAnchor],
    [_locationIconImageView.trailingAnchor
        constraintEqualToAnchor:_locationLabel.leadingAnchor
                       constant:kLocationImageToLabelSpacing],
    [_locationLabel.trailingAnchor
        constraintEqualToAnchor:_locationContainerView.trailingAnchor],
    [_locationIconImageView.centerYAnchor
        constraintEqualToAnchor:_locationContainerView.centerYAnchor],
  ];

  _hideLocationImageConstraints = @[
    [_locationContainerView.leadingAnchor
        constraintEqualToAnchor:_locationLabel.leadingAnchor],
    [_locationLabel.trailingAnchor
        constraintEqualToAnchor:_locationContainerView.trailingAnchor],
  ];
  } // End Vivaldi

  [self updateContainerConstraints];

  // Setup constraints for badge view that shows translate and other dynamic
  // buttons needed based on right context.
  if (IsVivaldiRunning()) {
    [NSLayoutConstraint activateConstraints:@[
      [_badgesContainerView.leadingAnchor
       constraintEqualToAnchor:_locationContainerView.trailingAnchor],
      [_badgesContainerView.trailingAnchor
       constraintEqualToAnchor:_trailingButton.leadingAnchor
       constant:vLocationBarSteadyViewShareButtonTrailingSpacing],
      [_badgesContainerView.centerYAnchor
       constraintEqualToAnchor:self.centerYAnchor],
      // Set width and height to match trailingButton
      [_badgesContainerView.widthAnchor
       constraintEqualToAnchor:_trailingButton.widthAnchor],
      [_badgesContainerView.heightAnchor
       constraintEqualToAnchor:_trailingButton.heightAnchor],
    ]];
  } else {
  self.badgesViewFullScreenEnabledConstraints = @[
    [_badgesContainerView.leadingAnchor
        constraintGreaterThanOrEqualToAnchor:self.leadingAnchor],
    [_badgesContainerView.trailingAnchor
        constraintEqualToAnchor:self.locationContainerView.leadingAnchor],
  ];

  self.badgesViewFullScreenDisabledConstraints = @[
    [_badgesContainerView.leadingAnchor
        constraintEqualToAnchor:self.leadingAnchor],
    [_badgesContainerView.trailingAnchor
        constraintLessThanOrEqualToAnchor:self.locationContainerView
                                              .leadingAnchor],
  ];

  // This low-priority, 0 width constraint is necessary for the stackview to
  // return to its 0 size when empty and exiting fullscreen.
  NSLayoutConstraint* badgesContainerViewWidthConstraint =
      [_badgesContainerView.widthAnchor constraintEqualToConstant:0];
  badgesContainerViewWidthConstraint.priority = UILayoutPriorityDefaultLow - 1;

  [NSLayoutConstraint
      activateConstraints:[self.badgesViewFullScreenDisabledConstraints
                              arrayByAddingObjectsFromArray:@[
                                [_badgesContainerView.topAnchor
                                    constraintEqualToAnchor:self.topAnchor],
                                [_badgesContainerView.bottomAnchor
                                    constraintEqualToAnchor:self.bottomAnchor],
                                badgesContainerViewWidthConstraint,
                              ]]];
  } // End Vivaldi

  // Different possible X anchors for the location label container.
  _xStickToLeadingSideConstraint = [_locationContainerView.leadingAnchor
      constraintEqualToAnchor:self.leadingAnchor
                     constant:kLeadingMargin];
  _xStickToLeadingSideConstraint.priority = UILayoutPriorityDefaultHigh;

  if (IsVivaldiRunning()) {
    _locationContainerViewLeadingAnchorConstraint =
        [_locationContainerView.leadingAnchor
            constraintEqualToAnchor:self.leadingAnchor
                           constant:kLocationBarLeadingPadding];

    _trailingButtonTrailingAnchorConstraint =
        [self.trailingButton.trailingAnchor
            constraintEqualToAnchor:self.trailingAnchor
                           constant:self.trailingButtonTrailingSpacing];

    // Setup and activate constraints.
    [NSLayoutConstraint activateConstraints:@[
      [_locationLabel.centerYAnchor
          constraintEqualToAnchor:_locationContainerView.centerYAnchor],
      [_locationLabel.heightAnchor
          constraintLessThanOrEqualToAnchor:_locationContainerView.heightAnchor],
      [_trailingButton.centerYAnchor
          constraintEqualToAnchor:self.centerYAnchor],
      [_locationContainerView.centerYAnchor
          constraintEqualToAnchor:self.centerYAnchor],
      [_trailingButton.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:_locationContainerView
                                                   .trailingAnchor],
      [_trailingButton.widthAnchor constraintEqualToConstant:kButtonSize],
      [_trailingButton.heightAnchor constraintEqualToConstant:kButtonSize],
      _trailingButtonTrailingAnchorConstraint,
      _locationContainerViewLeadingAnchorConstraint,
    ]];
  } else { // Vivaldi
  _xAbsoluteCenteredConstraint = [_locationContainerView.centerXAnchor
      constraintEqualToAnchor:self.centerXAnchor];
  _xAbsoluteCenteredConstraint.priority = UILayoutPriorityDefaultHigh;

  _locationContainerViewLeadingAnchorConstraint =
      [_locationContainerView.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:self.leadingAnchor
                                      constant:kLocationBarLeadingPadding];

  // Setup the layout guide centered between the contents of the location
  // bar.
  _centeredBetweenLocationBarContentsLayoutGuide = [[UILayoutGuide alloc] init];
  [_locationButton
      addLayoutGuide:_centeredBetweenLocationBarContentsLayoutGuide];
  [NSLayoutConstraint activateConstraints:@[
    [_centeredBetweenLocationBarContentsLayoutGuide.leadingAnchor
        constraintEqualToAnchor:_badgesContainerView.trailingAnchor],
    [_centeredBetweenLocationBarContentsLayoutGuide.trailingAnchor
        constraintEqualToAnchor:_trailingButton.leadingAnchor],
  ]];

  _xRelativeToContentCenteredConstraint = [_locationContainerView.centerXAnchor
      constraintEqualToAnchor:_centeredBetweenLocationBarContentsLayoutGuide
                                  .centerXAnchor];
  _xRelativeToContentCenteredConstraint.priority =
      UILayoutPriorityDefaultHigh - 1;

  _trailingButtonTrailingAnchorConstraint = [self.trailingButton.trailingAnchor
      constraintEqualToAnchor:self.trailingAnchor
                     constant:self.trailingButtonTrailingSpacing];

  // Setup and activate constraints.
  [NSLayoutConstraint activateConstraints:@[
    [_locationLabel.centerYAnchor
        constraintEqualToAnchor:_locationContainerView.centerYAnchor
                       constant:kLocationLabelVerticalOffset],
    [_locationLabel.heightAnchor
        constraintLessThanOrEqualToAnchor:_locationContainerView.heightAnchor
                                 constant:2 * kLocationLabelVerticalOffset],
    [_trailingButton.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    [_locationContainerView.centerYAnchor
        constraintEqualToAnchor:self.centerYAnchor],
    [_trailingButton.leadingAnchor
        constraintGreaterThanOrEqualToAnchor:_locationContainerView
                                                 .trailingAnchor],
    [_trailingButton.widthAnchor constraintEqualToConstant:kButtonSize],
    [_trailingButton.heightAnchor constraintEqualToConstant:kButtonSize],
    _trailingButtonTrailingAnchorConstraint,
    _xAbsoluteCenteredConstraint,
    _locationContainerViewLeadingAnchorConstraint,
    [_trailingButtonSpotlightView.trailingAnchor
        constraintEqualToAnchor:self.trailingAnchor],
    [_trailingButtonSpotlightView.heightAnchor
        constraintEqualToAnchor:self.heightAnchor],
  ]];
  } // End Vivaldi
}

- (void)setUpAccessibility {
  // Setup accessibility.
  _trailingButton.isAccessibilityElement = YES;
  _locationButton.isAccessibilityElement = YES;
  _locationButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_ACCNAME_LOCATION);

  // These two elements must remain accessible for egtests, but will not be
  // included in accessibility navigation as they are not added to the
  // accessibleElements array.
  _locationIconImageView.isAccessibilityElement = YES;
  _locationLabel.isAccessibilityElement = YES;

  _accessibleElements = [[NSMutableArray alloc] init];
  [self updateAccessibility];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _trailingButtonSpotlightView.layer.cornerRadius =
      _trailingButtonSpotlightView.bounds.size.height / 2;
}

- (CGFloat)trailingButtonTrailingSpacing {
  if (IsSplitToolbarMode(self)) {

    if (IsVivaldiRunning())
      return vLocationBarSteadyViewShareButtonTrailingSpacing;
    // End Vivaldi

    return kShareButtonTrailingSpacing;
  } else {
    return kVoiceSearchButtonTrailingSpacing;
  }
}

- (void)setColorScheme:(LocationBarSteadyViewColorScheme*)colorScheme {
  _colorScheme = colorScheme;
  self.trailingButton.tintColor = self.colorScheme.trailingButtonColor;
  // The text color is set in -setLocationLabelText: and
  // -setLocationLabelPlaceholderText: because the two text styles have
  // different colors. The icon should be the same color as the text, but it
  // only appears with the regular label, so its color can be set here.
  self.locationIconImageView.tintColor = self.colorScheme.fontColor;
  _incognitoImageView.tintColor = self.colorScheme.fontColor;

  // Vivaldi
  self.locationLabel.textColor = self.colorScheme.fontColor;
  self.leadingButton.tintColor = self.colorScheme.trailingButtonColor;
  self.connectionIconImageView.tintColor = self.colorScheme.trailingButtonColor;
  // End Vivaldi
}

- (void)setLocationImage:(UIImage*)locationImage {

  if (IsVivaldiRunning()) {
    BOOL hadImage = self.connectionIconImageView.image != nil;
    BOOL hasImage = locationImage != nil;
    self.connectionIconImageView.image = locationImage;
    if (hadImage == hasImage) {
      return;
    }
    if (hasImage) {
      [self.locationContainerView addSubview:self.connectionIconImageView];
    } else {
      [self.connectionIconImageView removeFromSuperview];
    }
    [self activateConstraintsForLeadingButtonEnabled:self.leadingButton.enabled
                                    hasLocationImage:hasImage];
    [self updateAccessibility];
  } else {
  BOOL hadImage = self.locationIconImageView.image != nil;
  BOOL hasImage = locationImage != nil;
  self.locationIconImageView.image = locationImage;
  if (hadImage == hasImage) {
    return;
  }
  } // End Vivaldi

  [self updateContainerConstraints];
}

- (void)addCustomLeadingView:(UIView*)view
                 targetWidth:(CGFloat)targetWidth
                     spacing:(CGFloat)spacing {
  // Clean up if the icon is already set.
  if (_customLeadingViewContainer &&
      [_customLeadingViewContainer isDescendantOfView:self]) {
    [_customLeadingViewContainer removeFromSuperview];
  }
  _customLeadingView = view;
  _customLeadingViewSpacing = spacing;
  _customLeadingViewTargetWidth = targetWidth;

  // Ensure accessibility is correctly configured on the custom leading view.
  _customLeadingView.isAccessibilityElement = YES;
  if (!_customLeadingView.accessibilityLabel) {
    _customLeadingView.accessibilityLabel =
        l10n_util::GetNSString(IDS_IOS_GEMINI_LIVE_ACCESSIBILITY_LABEL);
  }

  _customLeadingViewContainer = [[UIView alloc] init];
  _customLeadingViewContainer.translatesAutoresizingMaskIntoConstraints = NO;
  _customLeadingViewContainer.clipsToBounds = NO;

  [_customLeadingViewContainer addSubview:_customLeadingView];
  _customLeadingView.translatesAutoresizingMaskIntoConstraints = NO;

  // Constrain the child view tightly inside its container.
  [NSLayoutConstraint activateConstraints:@[
    [_customLeadingView.leadingAnchor
        constraintEqualToAnchor:_customLeadingViewContainer.leadingAnchor],
    [_customLeadingView.centerYAnchor
        constraintEqualToAnchor:_customLeadingViewContainer.centerYAnchor],
    [_customLeadingView.widthAnchor
        constraintEqualToConstant:_customLeadingViewTargetWidth],
    [_customLeadingViewContainer.heightAnchor
        constraintEqualToAnchor:_customLeadingView.heightAnchor],
  ]];

  _customLeadingViewWidthConstraint =
      [_customLeadingViewContainer.widthAnchor constraintEqualToConstant:0.0];
  _customLeadingViewWidthConstraint.active = YES;

  _customLeadingView.transform = CGAffineTransformMakeScale(0.01, 0.01);
  _customLeadingView.alpha = 0.0;
  _customLeadingView.hidden = YES;

  [self.locationContainerView addSubview:_customLeadingViewContainer];
  [self updateContainerConstraints];
  [self updateAccessibility];
}

- (void)setLocationLabelText:(NSString*)string {
  [self setLocationLabelText:string clipTail:NO];
}

- (void)setLocationLabelText:(NSString*)string clipTail:(BOOL)clipTail {
  _isShowingPlaceholder = NO;
  // Use attributed text to force LTR direction for URLs, preventing RTL
  // characters from messing up the visual order (e.g. IDN with RTL scripts).
  NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
  // https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/url_display_guidelines/url_display_guidelines.md#rtl
  [style setBaseWritingDirection:NSWritingDirectionLeftToRight];
  [style setLineBreakMode:clipTail ? NSLineBreakByTruncatingTail
                                   : NSLineBreakByTruncatingHead];

  NSDictionary* attributes = @{NSParagraphStyleAttributeName : style};

  self.locationLabel.attributedText =
      [[NSAttributedString alloc] initWithString:string attributes:attributes];
  self.locationLabel.textColor = self.colorScheme.fontColor;
  [self updateAccessibility];
  [self updateContainerConstraints];
}

- (void)setLocationLabelPlaceholderText:(NSString*)string {
  _isShowingPlaceholder = YES;
  self.locationLabel.lineBreakMode = NSLineBreakByTruncatingTail;
  self.locationLabel.textColor = self.colorScheme.placeholderColor;
  self.locationLabel.text = string;
  [self updateContainerConstraints];
}

- (void)setSecurityLevelAccessibilityString:(NSString*)string {
  if ([_securityLevelAccessibilityString isEqualToString:string]) {
    return;
  }
  _securityLevelAccessibilityString = [string copy];
  [self updateAccessibility];
}

- (void)setIncognitoBadgeView:(UIView*)incognitoBadgeView {
  BOOL hadBadgeView = _badgesContainerView.incognitoBadgeView != nil;
  if (!hadBadgeView && incognitoBadgeView) {
    _badgesContainerView.incognitoBadgeView = incognitoBadgeView;
  }
  [self updateAccessibility];
}

- (void)setBadgeView:(UIView*)badgeView {
  BOOL hadBadgeView = _badgesContainerView.badgeView != nil;
  if (!hadBadgeView && badgeView) {
    _badgesContainerView.badgeView = badgeView;
  }
  [self updateAccessibility];
}

- (void)setContextualPanelEntrypointView:
    (UIView*)contextualPanelEntrypointView {
  BOOL hadEntrypointView =
      _badgesContainerView.contextualPanelEntrypointView != nil;
  if (!hadEntrypointView && contextualPanelEntrypointView) {
    _badgesContainerView.contextualPanelEntrypointView =
        contextualPanelEntrypointView;
  }
  [self updateAccessibility];
}

- (void)setReaderModeChipView:(UIView*)readerModeChipView {
  if (!_badgesContainerView.readerModeChipView && readerModeChipView) {
    _badgesContainerView.readerModeChipView = readerModeChipView;
  }
  [self updateAccessibility];
}

- (void)setPlaceholderView:(UIView*)placeholderView
                      type:(LocationBarPlaceholderType)placeholderType {
  if (_badgesContainerView.placeholderView != placeholderView) {
    _badgesContainerView.placeholderType = placeholderType;
    _badgesContainerView.placeholderView = placeholderView;
  }
  [self updateAccessibility];
}

- (void)setPageActionMenuHandler:
    (id<PageActionMenuCommands>)pageActionMenuHandler {
  if (IsProactiveSuggestionsFrameworkEnabled()) {
    _pageActionMenuHandler = pageActionMenuHandler;
    _badgesContainerView.pageActionMenuHandler = pageActionMenuHandler;
  }
}

- (void)setFullScreenCollapsedMode:(BOOL)isFullScreenCollapsed {

  // Vivaldi: Since we don't show badge, we can skip this.
  if (IsVivaldiRunning())
    return; // End Vivaldi

  if (!self.badgesContainerView.badgeView ||
      self.badgesContainerView.badgeView.hidden) {
    return;
  }

  if (isFullScreenCollapsed) {
    [NSLayoutConstraint
        activateConstraints:self.badgesViewFullScreenEnabledConstraints];
    [NSLayoutConstraint
        deactivateConstraints:self.badgesViewFullScreenDisabledConstraints];
  } else {
    [NSLayoutConstraint
        deactivateConstraints:self.badgesViewFullScreenEnabledConstraints];
    [NSLayoutConstraint
        activateConstraints:self.badgesViewFullScreenDisabledConstraints];
  }
}

- (void)enableTrailingButton:(BOOL)enabled {
  self.trailingButton.enabled = enabled;
  [self updateAccessibility];
}

- (void)setTrailingButtonHidden:(BOOL)hidden {
  self.trailingButton.hidden = hidden;
  if (IsChromeNextIaEnabled()) {
    [self updateAccessibility];
  }
}

- (void)setCentered:(BOOL)centered {
  if (centered) {
    _xStickToLeadingSideConstraint.active = NO;
    // If the location label is currently being centered relative to content
    // around it, don't activate the following constraint (absolute centering).
    _xAbsoluteCenteredConstraint.active =
        !_xRelativeToContentCenteredConstraint.active;
  } else {
    _xAbsoluteCenteredConstraint.active = NO;
    _xStickToLeadingSideConstraint.active = YES;
  }

  // Call this in case the font was previously made smaller by the large
  // Contextual Panel entrypoint.
  _locationContainerView.transform = CGAffineTransformIdentity;
}

- (void)setLocationBarLabelCenteredBetweenContent:(BOOL)centered {
  // Early return if the label is already justified to the leading edge, or if
  // the Contextual Panel entrypoint is not being shown.
  if (_xStickToLeadingSideConstraint.active ||
      (centered &&
       self.badgesContainerView.contextualPanelEntrypointView.hidden)) {
    _locationContainerView.transform = CGAffineTransformIdentity;
    return;
  }

  if (centered) {
    _xAbsoluteCenteredConstraint.active = NO;
    _xRelativeToContentCenteredConstraint.active = YES;

    // Make the location container smaller via transform to 1. allow animating
    // the "font" change and 2. make the entire location label container package
    // (label + image) become smaller momentarily.
    _locationContainerView.transform =
        CGAffineTransformMakeScale(kSmallerLocationLabelFontMultiplier,
                                   kSmallerLocationLabelFontMultiplier);
  } else {
    _xRelativeToContentCenteredConstraint.active = NO;
    _xAbsoluteCenteredConstraint.active = YES;
    _locationContainerView.transform = CGAffineTransformIdentity;
  }

  // This method is called as part of an animation, so layout here if needed.
  [self layoutIfNeeded];
}

- (id<ContextualPanelEntrypointVisibilityDelegate>)
    contextualEntrypointVisibilityDelegate {
  return self.badgesContainerView;
}

- (id<ReaderModeChipVisibilityDelegate>)readerModeChipVisibilityDelegate {
  return self.badgesContainerView;
}

- (id<BadgeViewVisibilityDelegate>)badgeViewVisibilityDelegate {
  return self.badgesContainerView;
}

- (id<IncognitoBadgeViewVisibilityDelegate>)
    incognitoBadgeViewVisibilityDelegate {
  return self.badgesContainerView;
}

#pragma mark - UIResponder

// This is needed for UIMenu
- (BOOL)canBecomeFirstResponder {
  return true;
}

#pragma mark - UIAccessibilityContainer

- (NSArray*)accessibilityElements {
  return self.accessibleElements;
}

- (NSInteger)accessibilityElementCount {
  return self.accessibleElements.count;
}

- (id)accessibilityElementAtIndex:(NSInteger)index {
  return self.accessibleElements[index];
}

- (NSInteger)indexOfAccessibilityElement:(id)element {
  return [self.accessibleElements indexOfObject:element];
}

#pragma mark - private

// Updates the location accessibility label and adds the correct views to
// accessible elements depending on their current displayed state.
- (void)updateAccessibility {
  [self.accessibleElements removeAllObjects];

  if (_customLeadingView && !_customLeadingView.hidden) {
    [self.accessibleElements addObject:_customLeadingView];
  }

  [_accessibleElements addObject:_locationButton];

  if ([self shouldShowIncognitoBadge]) {
    [self.accessibleElements addObject:_incognitoImageView];
  }

  if (self.securityLevelAccessibilityString.length > 0) {
    self.locationButton.accessibilityValue =
        [NSString stringWithFormat:@"%@ %@", self.locationLabel.text,
                                   self.securityLevelAccessibilityString];
  } else {
    self.locationButton.accessibilityValue =
        [NSString stringWithFormat:@"%@", self.locationLabel.text];
  }

  [self.accessibleElements
      addObjectsFromArray:self.badgesContainerView.accessibilityElements];

  if (self.trailingButton && self.trailingButton.enabled) {
    if (!IsChromeNextIaEnabled() || !self.trailingButton.hidden) {
      [self.accessibleElements addObject:self.trailingButton];
    }
  }

  // Vivaldi
  if (self.leadingButton.enabled) {
    if ([self.accessibleElements indexOfObject:self.leadingButton] ==
        NSNotFound) {
      [self.accessibleElements addObject:self.leadingButton];
    }
  } else {
    [self.accessibleElements removeObject:self.leadingButton];
  }
  // End Vivaldi

}

// Propagates the incognito state to the badges container view.
- (void)setIncognito:(BOOL)incognito {
  _incognito = incognito;
  if (_incognito && !_incognitoImageView) {
    _incognitoImageView = [[UIImageView alloc] init];
    _incognitoImageView.translatesAutoresizingMaskIntoConstraints = NO;
    [_incognitoImageView
        setContentCompressionResistancePriority:UILayoutPriorityRequired
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];
    _incognitoImageView.isAccessibilityElement = YES;
    _incognitoImageView.accessibilityLabel =
        l10n_util::GetNSString(IDS_IOS_BADGE_INCOGNITO_HINT);
    UIImageConfiguration* configuration = [UIImageSymbolConfiguration
        configurationWithPointSize:kIncognitoImageSize
                            weight:UIImageSymbolWeightBold
                             scale:UIImageSymbolScaleMedium];
    _incognitoImageView.image =
        SymbolWithConfiguration(SymbolIncognito, configuration);
    _incognitoImageView.tintColor = self.colorScheme.fontColor;
  }

  self.badgesContainerView.incognito = incognito;
  [self updateContainerConstraints];
}

// Updates the current constraints.
- (void)updateContainerConstraints {
  [NSLayoutConstraint deactivateConstraints:_containerActiveConstraints];

  if (IsVivaldiRunning()) {
    BOOL hasConnectionImage = self.connectionIconImageView.image != nil;
    [self.locationIconImageView removeFromSuperview];
    [_incognitoImageView removeFromSuperview];
    _containerActiveConstraints = @[ [self.locationLabel.trailingAnchor
        constraintEqualToAnchor:self.locationContainerView.trailingAnchor] ];
    [NSLayoutConstraint activateConstraints:_containerActiveConstraints];
    [self activateConstraintsForLeadingButtonEnabled:self.leadingButton.enabled
                                    hasLocationImage:hasConnectionImage];
    return;
  } // End Vivaldi

  BOOL hasIncognito = [self shouldShowIncognitoBadge];
  BOOL hasLocationImage = self.locationIconImageView.image != nil;

  if (hasIncognito) {
    [self.locationButton addSubview:_incognitoImageView];
  } else {
    [_incognitoImageView removeFromSuperview];
  }

  if (hasLocationImage) {
    [self.locationContainerView addSubview:self.locationIconImageView];
  } else {
    [self.locationIconImageView removeFromSuperview];
  }

  NSMutableArray* constraints = [[NSMutableArray alloc] init];

  if (hasIncognito) {
    [constraints addObjectsFromArray:@[
      [_incognitoImageView.centerYAnchor
          constraintEqualToAnchor:_locationContainerView.centerYAnchor],
      [_locationContainerView.leadingAnchor
          constraintEqualToAnchor:_incognitoImageView.leadingAnchor]
    ]];
    _xAbsoluteCenteredConstraint.constant = -kIncognitoCenteringOffset;
  } else {
    _xAbsoluteCenteredConstraint.constant = 0;
  }

  // Pin label to trailing edge.
  [constraints addObject:[_locationLabel.trailingAnchor
                             constraintEqualToAnchor:_locationContainerView
                                                         .trailingAnchor]];

  NSLayoutXAxisAnchor* leadingTargetAnchor =
      _locationContainerView.leadingAnchor;
  CGFloat currentSpacing = 0.0;

  if (hasIncognito) {
    leadingTargetAnchor = _incognitoImageView.trailingAnchor;
    currentSpacing = kIncognitoImageToLocationSpacing;
  }

  if (_customLeadingViewContainer && !_customLeadingView.hidden) {
    _customLeadingViewLeadingConstraint =
        [_customLeadingViewContainer.leadingAnchor
            constraintEqualToAnchor:leadingTargetAnchor
                           constant:currentSpacing];
    [constraints addObjectsFromArray:@[
      _customLeadingViewLeadingConstraint,
      [_customLeadingViewContainer.centerYAnchor
          constraintEqualToAnchor:_locationContainerView.centerYAnchor],
    ]];
    leadingTargetAnchor = _customLeadingViewContainer.trailingAnchor;
    currentSpacing = _customLeadingViewSpacing;
  }

  if (hasLocationImage) {
    [constraints addObjectsFromArray:@[
      [self.locationIconImageView.leadingAnchor
          constraintEqualToAnchor:leadingTargetAnchor
                         constant:currentSpacing],
      [self.locationIconImageView.centerYAnchor
          constraintEqualToAnchor:_locationContainerView.centerYAnchor],
    ]];
    leadingTargetAnchor = self.locationIconImageView.trailingAnchor;
    currentSpacing = -kLocationImageToLabelSpacing;
  }

  [constraints addObject:[_locationLabel.leadingAnchor
                             constraintEqualToAnchor:leadingTargetAnchor
                                            constant:currentSpacing]];

  _containerActiveConstraints = constraints;
  [NSLayoutConstraint activateConstraints:_containerActiveConstraints];
}

// Whether the incognito badge should be visible or not.
- (BOOL)shouldShowIncognitoBadge {
  return self.isIncognito && IsChromeNextIaEnabled() && !_isShowingPlaceholder;
}


#pragma mark - VIVALDI
#pragma mark - SharingPositioner

- (UIView*)sourceView {
  return self.locationLabel;
}

- (CGRect)sourceRect {
  return self.locationLabel.bounds;
}

- (void)updateLocationText:(NSString*)text
                    domain:(NSString*)domain
                  showFull:(BOOL)showFull
                  clipTail:(BOOL)clipTail {
  if (self.showFullAddress != showFull)
    self.showFullAddress = showFull;

  // Chromium 148+ updates the container constraints whenever the label content
  // changes. Vivaldi uses this custom entry point for label updates, so keep
  // the placeholder state and constraints in sync here as well to avoid layout
  // jumps (e.g. NTP -> tab switch).
  _isShowingPlaceholder = (text.length == 0);

  if (text.length <= 0) {
    self.locationLabel.textColor = self.colorScheme.placeholderColor;
    [self updateContainerConstraints];
    return;
  }

  self.locationLabel.lineBreakMode =
      clipTail ? NSLineBreakByTruncatingTail : NSLineBreakByTruncatingHead;

  if (showFull && ![text isEqualToString:domain] && [text length] > 0) {
    UIColor *fullTextColor =
        [self.colorScheme.fontColor
            colorWithAlphaComponent:vLocationBarSteadyViewFullAddressOpacity];
    UIColor *domainColor =
        [self.colorScheme.fontColor
            colorWithAlphaComponent:vLocationBarSteadyViewDomainOpacity];
    NSAttributedString *attributedString =
        [VivaldiGlobalHelpers attributedStringWithText:text
                                             highlight:domain
                                             textColor:fullTextColor
                                        highlightColor:domainColor];
    self.locationLabel.attributedText = attributedString;
    [self updateAccessibility];
    [self updateContainerConstraints];
    return;
  } else {
    [self setLocationLabelText:text];
    self.locationLabel.textColor = self.colorScheme.trailingButtonColor;
  }
}

- (void)fadeSteadyViewContentsWithAlpha:(CGFloat)alpha {
  _locationButton.alpha = alpha;
}

- (void)setLeadingButtonEnabled:(BOOL)enabled {
  self.leadingButton.enabled = enabled;
  self.leadingButton.hidden = !enabled;
  BOOL hasConnectionImage = self.connectionIconImageView.image != nil;
  [self activateConstraintsForLeadingButtonEnabled:enabled
                                  hasLocationImage:hasConnectionImage];
  [self updateAccessibility];
}

- (void)setLeadingButtonIconFromATBSetting:(ATBSettingType)setting {
  [_leadingButton setImage:[self shieldIconForSetting:setting]
                  forState:UIControlStateNormal];
}

#pragma mark - Helper

- (UIImage*)shieldIconForSetting:(ATBSettingType)setting {
  UIImage* iconImage;

  switch (setting) {
    case ATBSettingNoBlocking:
      iconImage = [UIImage imageNamed:vATBShieldNone];
      break;
    case ATBSettingBlockTrackers:
      iconImage = [UIImage imageNamed:vATBShieldTrackers];
      break;
    case ATBSettingBlockTrackersAndAds:
      iconImage = [UIImage imageNamed:vATBShieldTrackesAndAds];
      break;
    default:
      iconImage = [UIImage imageNamed:vATBShieldNone];
      break;
  }

  return [iconImage imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
}

- (void)activateConstraintsForLeadingButtonEnabled:(BOOL)leadingButtonEnabled
                                  hasLocationImage:(BOOL)hasLocationImage {

  // Disable all the constraints first and enabled the one we want
  // based on state.
  [NSLayoutConstraint
      deactivateConstraints:self.showLocationImageConstraints];
  [NSLayoutConstraint
      deactivateConstraints:self.showConnectionStatusImageConstraints];
  [NSLayoutConstraint
      deactivateConstraints:self.showLocationAndConnectionImageConstraints];
  [NSLayoutConstraint
      deactivateConstraints:self.hideLocationImageConstraints];
  [NSLayoutConstraint
      deactivateConstraints:self.hideConnectionStatusImageConstraints];
  [NSLayoutConstraint
      deactivateConstraints:self.hideLocationAndConnectionImageConstraints];
  [NSLayoutConstraint
      deactivateConstraints:
          self.hideLocationAndConnectionImageReservedSpaceConstraints];

  if (leadingButtonEnabled && hasLocationImage) {
    [NSLayoutConstraint
        activateConstraints:self.showLocationAndConnectionImageConstraints];
  } else if (leadingButtonEnabled && !hasLocationImage) {
    [NSLayoutConstraint
        activateConstraints:self.showLocationImageConstraints];
  } else if (!leadingButtonEnabled && hasLocationImage) {
    [NSLayoutConstraint
        activateConstraints:self.showConnectionStatusImageConstraints];
  } else {
    NSArray<NSLayoutConstraint*>* constraints =
        _isShowingPlaceholder
            ? self.hideLocationAndConnectionImageConstraints
            : self.hideLocationAndConnectionImageReservedSpaceConstraints;
    [NSLayoutConstraint activateConstraints:constraints];
  }
}
// End Vivaldi

@end
