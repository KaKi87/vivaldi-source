// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/chrome/browser/ui/tab_switcher/tab_grid/vivaldi_tab_grid_swipe_helper.h"

#import <objc/message.h>
#import <objc/runtime.h>

#import "ios/ui/settings/tabs/vivaldi_tab_setting_prefs.h"

namespace {

constexpr CGFloat kVivaldiSwipeDismissThresholdFraction = 0.4f;
constexpr CGFloat kVivaldiSwipeDismissVelocity = 800.0f;
constexpr CGFloat kVivaldiSwipeMaxRotation = 0.2f;
constexpr CGFloat kVivaldiSwipeMaxFade = 0.3f;
constexpr CGFloat kVivaldiSwipeDismissRotation = 0.3f;
constexpr CGFloat kVivaldiSwipeDismissScale = 0.8f;
constexpr CGFloat kVivaldiSwipeDismissOvershoot = 100.0f;
constexpr CGFloat kVivaldiSwipeMaxDistanceFraction = 0.8f;
constexpr NSTimeInterval kVivaldiSwipeDismissAnimationDuration = 0.3;
constexpr NSTimeInterval kVivaldiSwipeReturnAnimationDuration = 0.4;
constexpr CGFloat kVivaldiSwipeReturnSpringDamping = 0.7f;
constexpr CGFloat kVivaldiSwipeReturnSpringVelocity = 0.5f;
// Selector defined on UICollectionViewCell (UIKit) for drag-and-drop state.
NSString* const kCollectionViewCellDragStateSelectorName = @"dragState";
// Selector implemented by GridCell / GroupGridCell for tab grid selection mode.
// See
// chromium/ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/grid/grid_cell.mm
// and
// chromium/ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/grid/group_grid_cell.mm.
NSString* const kTabGridCellSelectionModeSelectorName = @"isInSelectionMode";
// Selector implemented by GridCell / GroupGridCell to trigger close behavior.
// See
// chromium/ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/grid/grid_cell.mm
// and
// chromium/ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/grid/group_grid_cell.mm.
NSString* const kTabGridCellCloseButtonSelectorName = @"closeButtonTapped:";

const void* kVivaldiSwipeHandlerKey = &kVivaldiSwipeHandlerKey;

BOOL SwipeToCloseEnabled() {
  return [VivaldiTabSettingPrefs swipeToCloseTabEnabled];
}

UICollectionViewCellDragState DragStateForCellIfAvailable(
    UICollectionViewCell* cell) {
  SEL selector = NSSelectorFromString(kCollectionViewCellDragStateSelectorName);
  if ([cell respondsToSelector:selector]) {
    return ((UICollectionViewCellDragState (*)(id, SEL))objc_msgSend)(cell,
                                                                      selector);
  }
  return UICollectionViewCellDragStateNone;
}

BOOL CellIsInSelectionMode(UICollectionViewCell* cell) {
  SEL selector = NSSelectorFromString(kTabGridCellSelectionModeSelectorName);
  if ([cell respondsToSelector:selector]) {
    return ((BOOL (*)(id, SEL))objc_msgSend)(cell, selector);
  }
  return NO;
}

}  // namespace

@interface VivaldiTabGridSwipeHandler : NSObject <UIGestureRecognizerDelegate>

@property(nonatomic, weak) UICollectionViewCell* cell;
@property(nonatomic, strong) UIPanGestureRecognizer* panGesture;
@property(nonatomic, assign) BOOL interactive;

@end

@interface VivaldiTabGridSwipeHandler ()

- (void)installGestureIfNeeded;
- (void)updateTransformWithTranslation:(CGFloat)translationX;
- (void)finishSwipeWithTranslation:(CGFloat)translationX
                          velocity:(CGFloat)velocityX;
- (void)animateDismissInDirection:(BOOL)isRightDirection;
- (void)animateBackToNormal;
- (void)resetState;

@end

@implementation VivaldiTabGridSwipeHandler

- (instancetype)initWithCell:(UICollectionViewCell*)cell {
  self = [super init];
  if (self) {
    _cell = cell;
    [self installGestureIfNeeded];
  }
  return self;
}

- (void)attachToCell:(UICollectionViewCell*)cell {
  self.cell = cell;
  [self installGestureIfNeeded];
  [self resetState];
}

- (void)installGestureIfNeeded {
  UICollectionViewCell* cell = self.cell;
  if (!cell) {
    return;
  }
  if (!self.panGesture) {
    self.panGesture =
        [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                action:@selector(handlePan:)];
    self.panGesture.delegate = self;
  }
  if (![cell.gestureRecognizers containsObject:self.panGesture]) {
    [cell addGestureRecognizer:self.panGesture];
  }
}

- (void)handlePan:(UIPanGestureRecognizer*)gesture {
  if (!SwipeToCloseEnabled()) {
    [self resetState];
    return;
  }
  UICollectionViewCell* cell = self.cell;
  if (!cell) {
    return;
  }
  UIView* referenceView = cell.superview ?: cell;
  CGPoint translation = [gesture translationInView:referenceView];
  CGPoint velocity = [gesture velocityInView:referenceView];

  switch (gesture.state) {
    case UIGestureRecognizerStateBegan:
      self.interactive = YES;
      break;
    case UIGestureRecognizerStateChanged:
      [self updateTransformWithTranslation:translation.x];
      break;
    case UIGestureRecognizerStateEnded:
    case UIGestureRecognizerStateCancelled:
      [self finishSwipeWithTranslation:translation.x velocity:velocity.x];
      break;
    default:
      break;
  }
}

- (void)updateTransformWithTranslation:(CGFloat)translationX {
  if (!self.interactive) {
    return;
  }
  UICollectionViewCell* cell = self.cell;
  if (!cell) {
    return;
  }
  CGFloat width = MAX(CGRectGetWidth(cell.bounds), 1.0);
  CGFloat distance = fabs(translationX);
  CGFloat maxDistance = width * kVivaldiSwipeMaxDistanceFraction;
  CGFloat progress = MIN(distance / maxDistance, 1.0f);
  CGFloat rotation = (translationX / width) * kVivaldiSwipeMaxRotation;
  CGFloat alpha = 1.0f - (progress * kVivaldiSwipeMaxFade);

  cell.transform =
      CGAffineTransformConcat(CGAffineTransformMakeTranslation(translationX, 0),
                              CGAffineTransformMakeRotation(rotation));
  cell.alpha = alpha;
}

- (void)finishSwipeWithTranslation:(CGFloat)translationX
                          velocity:(CGFloat)velocityX {
  if (!self.interactive) {
    [self resetState];
    return;
  }
  UICollectionViewCell* cell = self.cell;
  if (!cell) {
    return;
  }
  CGFloat width = MAX(CGRectGetWidth(cell.bounds), 1.0);
  CGFloat distance = fabs(translationX);
  CGFloat threshold = width * kVivaldiSwipeDismissThresholdFraction;
  BOOL hasVelocity = fabs(velocityX) > kVivaldiSwipeDismissVelocity;
  BOOL shouldDismiss = (distance > threshold) || hasVelocity;

  if (shouldDismiss) {
    [self animateDismissInDirection:(translationX > 0)];
  } else {
    [self animateBackToNormal];
  }
}

- (void)animateDismissInDirection:(BOOL)isRightDirection {
  UICollectionViewCell* cell = self.cell;
  if (!cell) {
    return;
  }
  CGFloat width = MAX(CGRectGetWidth(cell.bounds), 1.0);
  CGFloat targetX = isRightDirection ? (width + kVivaldiSwipeDismissOvershoot)
                                     : -(width + kVivaldiSwipeDismissOvershoot);
  CGFloat targetRotation = isRightDirection ? kVivaldiSwipeDismissRotation
                                            : -kVivaldiSwipeDismissRotation;

  __weak __typeof(self) weakSelf = self;
  [UIView animateWithDuration:kVivaldiSwipeDismissAnimationDuration
      delay:0
      options:UIViewAnimationOptionCurveEaseOut
      animations:^{
        VivaldiTabGridSwipeHandler* strongSelf = weakSelf;
        UICollectionViewCell* strongCell = strongSelf.cell;
        if (!strongSelf || !strongCell) {
          return;
        }
        strongCell.transform = CGAffineTransformConcat(
            CGAffineTransformMakeTranslation(targetX, 0),
            CGAffineTransformConcat(
                CGAffineTransformMakeRotation(targetRotation),
                CGAffineTransformMakeScale(kVivaldiSwipeDismissScale,
                                           kVivaldiSwipeDismissScale)));
        strongCell.alpha = 0.0f;
      }
      completion:^(BOOL finished) {
        VivaldiTabGridSwipeHandler* strongSelf = weakSelf;
        UICollectionViewCell* strongCell = strongSelf.cell;
        if (!strongSelf || !strongCell) {
          return;
        }
        strongSelf.interactive = NO;
        SEL selector =
            NSSelectorFromString(kTabGridCellCloseButtonSelectorName);
        if ([strongCell respondsToSelector:selector]) {
          ((void (*)(id, SEL, id))objc_msgSend)(strongCell, selector, nil);
        }
      }];
}

- (void)animateBackToNormal {
  __weak __typeof(self) weakSelf = self;
  [UIView animateWithDuration:kVivaldiSwipeReturnAnimationDuration
      delay:0
      usingSpringWithDamping:kVivaldiSwipeReturnSpringDamping
      initialSpringVelocity:kVivaldiSwipeReturnSpringVelocity
      options:UIViewAnimationOptionCurveEaseOut
      animations:^{
        VivaldiTabGridSwipeHandler* strongSelf = weakSelf;
        UICollectionViewCell* strongCell = strongSelf.cell;
        if (!strongSelf || !strongCell) {
          return;
        }
        strongCell.transform = CGAffineTransformIdentity;
        strongCell.alpha = 1.0f;
      }
      completion:^(BOOL finished) {
        VivaldiTabGridSwipeHandler* strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }
        strongSelf.interactive = NO;
      }];
}

- (void)resetState {
  UICollectionViewCell* cell = self.cell;
  if (!cell) {
    return;
  }
  self.interactive = NO;
  cell.transform = CGAffineTransformIdentity;
  cell.alpha = 1.0f;
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer*)gestureRecognizer {
  if (gestureRecognizer != self.panGesture) {
    return YES;
  }
  if (!SwipeToCloseEnabled()) {
    return NO;
  }
  UICollectionViewCell* cell = self.cell;
  if (!cell || self.interactive) {
    return NO;
  }
  if (CellIsInSelectionMode(cell)) {
    return NO;
  }
  if (DragStateForCellIfAvailable(cell) != UICollectionViewCellDragStateNone) {
    return NO;
  }
  UIPanGestureRecognizer* pan = (UIPanGestureRecognizer*)gestureRecognizer;
  CGPoint velocity = [pan velocityInView:cell];
  return fabs(velocity.x) > fabs(velocity.y);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  if (gestureRecognizer == self.panGesture) {
    return !self.interactive;
  }
  return NO;
}

@end

@implementation VivaldiTabGridSwipeHelper

+ (void)attachToCell:(UICollectionViewCell*)cell {
  if (!cell) {
    return;
  }
  VivaldiTabGridSwipeHandler* handler =
      objc_getAssociatedObject(cell, kVivaldiSwipeHandlerKey);
  if (!handler) {
    handler = [[VivaldiTabGridSwipeHandler alloc] initWithCell:cell];
    objc_setAssociatedObject(cell, kVivaldiSwipeHandlerKey, handler,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  [handler attachToCell:cell];
}

@end
