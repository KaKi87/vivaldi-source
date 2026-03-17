// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/chrome/browser/ui/tab_switcher/tab_grid/vivaldi_tab_grid_inactive_tabs_pinned_helper.h"

#import <objc/message.h>
#import <objc/runtime.h>

#import "ios/chrome/browser/shared/ui/util/layout_guide_names.h"
#import "ios/chrome/browser/tab_switcher/tab_grid/base_grid/ui/base_grid_view_controller+subclassing.h"
#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/grid/grid_constants.h"
#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/grid/regular/inactive_tabs_button_cell.h"
#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_utils.h"
#import "ios/chrome/common/ui/elements/gradient_view.h"
#import "ios/chrome/common/ui/util/constraints_ui_util.h"
#import "ios/chrome/common/ui/util/ui_util.h"
#import "ios/ui/ntp/vivaldi_ntp_constants.h"

namespace {
constexpr CGFloat kSpacingPercentage = 0.12;
constexpr CGFloat kIPhonePortraitSpacing = 16.0;
constexpr CGFloat kMinimumSpacing = kIPhonePortraitSpacing;
constexpr CGFloat kInactiveTabsSectionBottomInset = 10.0;
constexpr NSTimeInterval kFadeAnimationDuration = 0.25;
constexpr CGFloat kContainerZPosition = 1.0;
constexpr CGFloat kFadeZPosition = 0.5;
constexpr CGFloat kFadeHiddenAlpha = 0.0;
constexpr CGFloat kFadeVisibleAlpha = 1.0;
constexpr CGFloat kButtonSizingHeightMultiplier = 10.0;
constexpr NSInteger kExtraSpacingSlots = 1;
constexpr NSTimeInterval kPressGestureMinimumDuration = 0.0;

const void* kPinnedHelperKey = &kPinnedHelperKey;

CGFloat SpacingForSize(CGSize size,
                       UIContentSizeCategory preferredContentSizeCategory) {
  const CGFloat width = size.width;
  const CGFloat total_spacing = width * kSpacingPercentage;
  const NSInteger spaces_count =
      TabGridColumnsCount(size, preferredContentSizeCategory) +
      kExtraSpacingSlots;
  const CGFloat spacing = AlignValueToPixel(total_spacing / spaces_count);
  return MAX(spacing, kMinimumSpacing);
}

}  // namespace

@interface VivaldiTabGridInactiveTabsPinnedHelper ()

@property(nonatomic, weak) BaseGridViewController* gridViewController;
@property(nonatomic, strong) UIControl* containerView;
@property(nonatomic, strong) InactiveTabsButtonCell* buttonCell;
@property(nonatomic, strong) GradientView* fadeView;
@property(nonatomic, assign) BOOL fadeEnabled;
@property(nonatomic, assign) BOOL fadeVisible;
@property(nonatomic, strong) NSLayoutConstraint* fadeHeightConstraint;
@property(nonatomic, strong) NSLayoutConstraint* leadingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* trailingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* topConstraint;
@property(nonatomic, strong) NSLayoutConstraint* heightConstraint;
@property(nonatomic, assign) BOOL visible;
@property(nonatomic, assign) CGFloat extraTopInset;

@end

@implementation VivaldiTabGridInactiveTabsPinnedHelper

+ (void)updateForGridViewController:(BaseGridViewController*)gridViewController
                            visible:(BOOL)visible
                              count:(NSInteger)count
                      daysThreshold:(NSInteger)daysThreshold {
  if (!gridViewController) {
    return;
  }
  VivaldiTabGridInactiveTabsPinnedHelper* helper =
      objc_getAssociatedObject(gridViewController, kPinnedHelperKey);
  if (!helper) {
    helper = [[VivaldiTabGridInactiveTabsPinnedHelper alloc] init];
    helper.gridViewController = gridViewController;
    helper.fadeEnabled = YES;
    objc_setAssociatedObject(gridViewController, kPinnedHelperKey, helper,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }

  helper.visible = visible;
  [helper ensurePinnedView];
  helper.buttonCell.count = count;
  helper.buttonCell.daysThreshold = daysThreshold;
  helper.containerView.hidden = !visible;
  [helper updateLayout];
}

+ (void)setFadeEnabled:(BOOL)enabled
    forGridViewController:(BaseGridViewController*)gridViewController {
  if (!gridViewController) {
    return;
  }
  VivaldiTabGridInactiveTabsPinnedHelper* helper =
      objc_getAssociatedObject(gridViewController, kPinnedHelperKey);
  if (!helper) {
    return;
  }
  if (helper.fadeEnabled == enabled) {
    return;
  }
  helper.fadeEnabled = enabled;
  [helper updateLayout];
}

- (void)updateFadeVisibility:(BOOL)visible animated:(BOOL)animated {
  if (!self.fadeView) {
    return;
  }
  BOOL wasVisible = self.fadeVisible;
  self.fadeVisible = visible;

  if (visible) {
    self.fadeView.hidden = NO;
    if (animated && !wasVisible) {
      self.fadeView.alpha = kFadeHiddenAlpha;
      [UIView animateWithDuration:kFadeAnimationDuration
                       animations:^{
                         self.fadeView.alpha = kFadeVisibleAlpha;
                       }];
    } else {
      self.fadeView.alpha = kFadeVisibleAlpha;
    }
  } else {
    if (animated && wasVisible) {
      [UIView animateWithDuration:kFadeAnimationDuration
          animations:^{
            self.fadeView.alpha = kFadeHiddenAlpha;
          }
          completion:^(BOOL finished) {
            if (!self.fadeVisible) {
              self.fadeView.hidden = YES;
            }
          }];
    } else {
      self.fadeView.alpha = kFadeHiddenAlpha;
      self.fadeView.hidden = YES;
    }
  }
}

+ (void)updateLayoutForGridViewController:
    (BaseGridViewController*)gridViewController {
  if (!gridViewController) {
    return;
  }
  VivaldiTabGridInactiveTabsPinnedHelper* helper =
      objc_getAssociatedObject(gridViewController, kPinnedHelperKey);
  if (!helper || !helper.visible) {
    return;
  }
  [helper updateLayout];
}

- (void)ensurePinnedView {
  if (self.containerView) {
    return;
  }

  UICollectionView* collectionView = self.gridViewController.collectionView;
  if (!collectionView) {
    return;
  }

  UIControl* container = [[UIControl alloc] init];
  container.translatesAutoresizingMaskIntoConstraints = NO;
  container.backgroundColor = [UIColor clearColor];
  container.clipsToBounds = YES;
  container.layer.zPosition = kContainerZPosition;

  GradientView* fadeView = [[GradientView alloc]
      initWithTopColor:[self fadeTopColor]
           bottomColor:[[self fadeTopColor] colorWithAlphaComponent:0.0f]];
  fadeView.translatesAutoresizingMaskIntoConstraints = NO;
  fadeView.layer.zPosition = kFadeZPosition;
  fadeView.userInteractionEnabled = NO;
  fadeView.alpha = kFadeHiddenAlpha;
  fadeView.hidden = YES;
  self.fadeVisible = NO;
  [collectionView addSubview:fadeView];
  [collectionView registerForTraitChanges:@[ UITraitUserInterfaceStyle.class ]
                               withTarget:self
                                   action:@selector(updateFadeColors)];

  self.fadeHeightConstraint =
      [fadeView.heightAnchor constraintEqualToConstant:0];

  [NSLayoutConstraint activateConstraints:@[
    [fadeView.topAnchor
        constraintEqualToAnchor:collectionView.frameLayoutGuide.topAnchor],
    [fadeView.leadingAnchor
        constraintEqualToAnchor:collectionView.frameLayoutGuide.leadingAnchor],
    [fadeView.trailingAnchor
        constraintEqualToAnchor:collectionView.frameLayoutGuide.trailingAnchor],
    self.fadeHeightConstraint,
  ]];

  InactiveTabsButtonCell* buttonCell =
      [[InactiveTabsButtonCell alloc] initWithFrame:CGRectZero];
  buttonCell.translatesAutoresizingMaskIntoConstraints = NO;
  buttonCell.backgroundColor = UIColor.clearColor;
  buttonCell.contentView.backgroundColor = UIColor.clearColor;
  container.layer.cornerRadius = buttonCell.layer.cornerRadius;
  container.layer.cornerCurve = kCACornerCurveContinuous;

  UIBlurEffect* blurEffect =
      [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemThinMaterial];
  UIVisualEffectView* effectView =
      [[UIVisualEffectView alloc] initWithEffect:blurEffect];
  effectView.translatesAutoresizingMaskIntoConstraints = NO;
  effectView.userInteractionEnabled = NO;
  effectView.clipsToBounds = YES;
  effectView.layer.cornerRadius = container.layer.cornerRadius;
  [container addSubview:effectView];

  [container addSubview:buttonCell];
  AddSameConstraints(effectView, container);
  AddSameConstraints(buttonCell, container);

  UILongPressGestureRecognizer* pressGesture =
      [[UILongPressGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(handlePress:)];
  pressGesture.minimumPressDuration = kPressGestureMinimumDuration;
  pressGesture.cancelsTouchesInView = YES;
  [container addGestureRecognizer:pressGesture];

  [collectionView addSubview:container];

  self.leadingConstraint = [container.leadingAnchor
      constraintEqualToAnchor:collectionView.frameLayoutGuide.leadingAnchor];
  self.trailingConstraint = [container.trailingAnchor
      constraintEqualToAnchor:collectionView.frameLayoutGuide.trailingAnchor];
  self.topConstraint = [container.topAnchor
      constraintEqualToAnchor:collectionView.frameLayoutGuide.topAnchor];
  self.heightConstraint = [container.heightAnchor constraintEqualToConstant:0];

  [NSLayoutConstraint activateConstraints:@[
    self.leadingConstraint,
    self.trailingConstraint,
    self.topConstraint,
    self.heightConstraint,
  ]];

  self.fadeView = fadeView;
  self.containerView = container;
  self.buttonCell = buttonCell;
}

- (void)updateLayout {
  if (!self.visible || !self.containerView) {
    self.extraTopInset = 0;
    if (self.fadeHeightConstraint) {
      self.fadeHeightConstraint.constant = 0;
    }
    [self updateFadeVisibility:NO animated:NO];
    [self applyInsets];
    return;
  }

  UICollectionView* collectionView = self.gridViewController.collectionView;
  if (!collectionView) {
    return;
  }

  const CGSize size = collectionView.bounds.size;
  if (size.width <= 0) {
    return;
  }

  const CGFloat spacing = SpacingForSize(
      size, collectionView.traitCollection.preferredContentSizeCategory);
  UIEdgeInsets baseInsets = self.gridViewController.contentInsets;
  const CGFloat leadingInset = baseInsets.left + spacing;
  const CGFloat trailingInset = baseInsets.right + spacing;
  self.leadingConstraint.constant = leadingInset;
  self.trailingConstraint.constant = -trailingInset;
  CGFloat pinnedTopOffset = baseInsets.top;
  const CGFloat pageControlBottom =
      [self pageControlBottomOffsetInCollectionView:collectionView];
  if (pageControlBottom > 0) {
    pinnedTopOffset = MAX(pinnedTopOffset, pageControlBottom);
  }
  self.topConstraint.constant = pinnedTopOffset + spacing;

  const CGFloat buttonWidth = size.width - leadingInset - trailingInset;
  const CGFloat buttonHeight = [self buttonHeightForWidth:buttonWidth];
  self.heightConstraint.constant = buttonHeight;

  BOOL shouldShowFade =
      (self.fadeView && self.fadeHeightConstraint && self.fadeEnabled);
  if (shouldShowFade) {
    self.fadeHeightConstraint.constant =
        pinnedTopOffset + spacing + buttonHeight;
  } else if (self.fadeHeightConstraint) {
    self.fadeHeightConstraint.constant = 0;
  }
  [self updateFadeVisibility:shouldShowFade
                    animated:(self.fadeVisible != shouldShowFade)];

  self.extraTopInset = (pinnedTopOffset - baseInsets.top) + spacing +
                       buttonHeight + kInactiveTabsSectionBottomInset;
  [self applyInsets];
}

- (void)updateFadeColors {
  if (!self.fadeView) {
    return;
  }
  [self.fadeView
      setStartColor:[self fadeTopColor]
           endColor:[[self fadeTopColor] colorWithAlphaComponent:0.0f]];
}

- (void)applyInsets {
  BaseGridViewController* controller = self.gridViewController;
  if (!controller || !controller.collectionView) {
    return;
  }
  UIEdgeInsets baseInsets = controller.contentInsets;
  UIEdgeInsets newInsets = baseInsets;
  newInsets.top += self.extraTopInset;
  controller.collectionView.contentInset =
      UIEdgeInsetsMake(newInsets.top, 0, newInsets.bottom, 0);
  controller.collectionView.scrollIndicatorInsets = UIEdgeInsetsMake(
      newInsets.top, baseInsets.left, newInsets.bottom, baseInsets.right);
}

- (CGFloat)buttonHeightForWidth:(CGFloat)width {
  if (width <= 0 || !self.buttonCell) {
    return 0;
  }
  self.buttonCell.bounds = CGRectMake(
      0, 0, width, kIPhonePortraitSpacing * kButtonSizingHeightMultiplier);
  [self.buttonCell setNeedsLayout];
  [self.buttonCell layoutIfNeeded];
  CGSize targetSize = CGSizeMake(width, UILayoutFittingCompressedSize.height);
  CGSize size = [self.buttonCell.contentView
        systemLayoutSizeFittingSize:targetSize
      withHorizontalFittingPriority:UILayoutPriorityRequired
            verticalFittingPriority:UILayoutPriorityFittingSizeLevel];
  return ceil(size.height);
}

- (CGFloat)pageControlBottomOffsetInCollectionView:
    (UICollectionView*)collectionView {
  if (!collectionView) {
    return 0;
  }

  id layoutGuideCenter = self.gridViewController.layoutGuideCenter;
  if (!layoutGuideCenter) {
    return 0;
  }

  SEL selector = NSSelectorFromString(@"referencedViewUnderName:");
  if (![layoutGuideCenter respondsToSelector:selector]) {
    return 0;
  }

  UIView* pageControl = ((UIView * (*)(id, SEL, id)) objc_msgSend)(
      layoutGuideCenter, selector, kTabGridPageControlGuide);
  if (!pageControl || !pageControl.window || !collectionView.window) {
    return 0;
  }

  CGRect frame = [pageControl convertRect:pageControl.bounds
                                   toView:collectionView];
  return CGRectGetMaxY(frame);
}

- (UIColor*)fadeTopColor {
  UIColor* baseColor = self.gridViewController.isIncognito
                           ? [UIColor colorNamed:vPrivateNTPBackgroundColor]
                           : [UIColor colorNamed:kGridBackgroundColor];
  return [baseColor
      resolvedColorWithTraitCollection:self.gridViewController.traitCollection];
}

- (void)handlePress:(UILongPressGestureRecognizer*)gesture {
  if (!self.visible || !self.containerView || !self.buttonCell) {
    return;
  }
  CGPoint location = [gesture locationInView:self.containerView];
  BOOL inside = CGRectContainsPoint(self.containerView.bounds, location);

  switch (gesture.state) {
    case UIGestureRecognizerStateBegan:
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      if (inside) {
        BaseGridViewController* controller = self.gridViewController;
        id delegate = controller.delegate;
        SEL selector = NSSelectorFromString(
            @"didTapInactiveTabsButtonInGridViewController:");
        if ([delegate respondsToSelector:selector]) {
          ((void (*)(id, SEL, id))objc_msgSend)(delegate, selector, controller);
        }
      }
      break;
    default:
      break;
  }
}

@end
