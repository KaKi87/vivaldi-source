// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#import "ios/chrome/browser/sharing/ui_bundled/vivaldi_sharing_utils.h"

#import <objc/message.h>

namespace {

constexpr NSTimeInterval kDismissAnimationFallbackDelay = 0.15;

BOOL IsOverflowMenuHostingController(UIViewController* viewController) {
  if (!viewController) {
    return NO;
  }
  return [NSStringFromClass([viewController class])
      containsString:@"OverflowMenuHostingController"];
}

UIView* ReferencedView(id layoutGuideCenter, NSString* guideName) {
  if (!layoutGuideCenter || !guideName.length) {
    return nil;
  }
  SEL selector = NSSelectorFromString(@"referencedViewUnderName:");
  if (![layoutGuideCenter respondsToSelector:selector]) {
    return nil;
  }
  return ((UIView * (*)(id, SEL, NSString*)) objc_msgSend)(layoutGuideCenter,
                                                           selector, guideName);
}

UILayoutGuide* MakeLayoutGuide(id layoutGuideCenter, NSString* guideName) {
  if (!layoutGuideCenter || !guideName.length) {
    return nil;
  }
  SEL selector = NSSelectorFromString(@"makeLayoutGuideNamed:");
  if (![layoutGuideCenter respondsToSelector:selector]) {
    return nil;
  }
  return ((UILayoutGuide * (*)(id, SEL, NSString*)) objc_msgSend)(
      layoutGuideCenter, selector, guideName);
}

BOOL IsValidAnchorView(UIView* view, UIView* rootView) {
  return view && view.window && view != rootView;
}

void RunCompletionOnMainQueue(dispatch_block_t completion, BOOL animated) {
  if (!completion) {
    return;
  }
  if (animated) {
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW,
                      (int64_t)(kDismissAnimationFallbackDelay * NSEC_PER_SEC)),
        dispatch_get_main_queue(), completion);
  } else {
    dispatch_async(dispatch_get_main_queue(), completion);
  }
}

CGRect LayoutFrameForGuide(UIViewController* baseViewController,
                           id layoutGuideCenter,
                           NSString* guideName) {
  UILayoutGuide* guide = MakeLayoutGuide(layoutGuideCenter, guideName);
  UIView* baseView = baseViewController.view;
  if (!guide || !baseView) {
    return CGRectZero;
  }
  if (guide.owningView != baseView) {
    [baseView addLayoutGuide:guide];
  }
  return guide.layoutFrame;
}

}  // namespace

UIViewController* VivaldiFindPresentedOverflowMenuController(
    UIViewController* rootViewController) {
  UIViewController* presentedViewController =
      rootViewController.presentedViewController;
  while (presentedViewController) {
    if (IsOverflowMenuHostingController(presentedViewController)) {
      return presentedViewController;
    }
    presentedViewController = presentedViewController.presentedViewController;
  }
  return nil;
}

BOOL VivaldiDismissPopupMenuAndRunCompletion(id popupMenuHandler,
                                             BOOL animated,
                                             dispatch_block_t completion) {
  if (!popupMenuHandler) {
    RunCompletionOnMainQueue(completion, NO);
    return NO;
  }

  SEL dismissWithCompletionSelector =
      NSSelectorFromString(@"dismissPopupMenuAnimated:completion:");
  if ([popupMenuHandler respondsToSelector:dismissWithCompletionSelector]) {
    ((void (*)(id, SEL, BOOL, dispatch_block_t))objc_msgSend)(
        popupMenuHandler, dismissWithCompletionSelector, animated, completion);
    return YES;
  }

  SEL dismissSelector = NSSelectorFromString(@"dismissPopupMenuAnimated:");
  if ([popupMenuHandler respondsToSelector:dismissSelector]) {
    ((void (*)(id, SEL, BOOL))objc_msgSend)(popupMenuHandler, dismissSelector,
                                            animated);
    RunCompletionOnMainQueue(completion, animated);
    return YES;
  }

  RunCompletionOnMainQueue(completion, NO);
  return NO;
}

UIView* VivaldiFindToolsMenuAnchorView(id layoutGuideCenter,
                                       NSString* preferredGuideName,
                                       NSString* fallbackGuideName) {
  UIView* anchorView = ReferencedView(layoutGuideCenter, preferredGuideName);
  if (anchorView) {
    return anchorView;
  }
  if (!fallbackGuideName.length ||
      [fallbackGuideName isEqualToString:preferredGuideName]) {
    return nil;
  }
  return ReferencedView(layoutGuideCenter, fallbackGuideName);
}

void VivaldiResolveShareAnchor(UIViewController* baseViewController,
                               id layoutGuideCenter,
                               UIView* preferredAnchorView,
                               NSString* preferredGuideName,
                               NSString* fallbackGuideName,
                               UIView** sourceViewOut,
                               CGRect* sourceRectOut) {
  UIView* baseView = baseViewController.view;
  UIView* sourceView = baseView ?: preferredAnchorView;
  CGRect sourceRect = sourceView ? sourceView.bounds : CGRectZero;

  UIView* anchorView = preferredAnchorView;
  if (!IsValidAnchorView(anchorView, baseView)) {
    anchorView = VivaldiFindToolsMenuAnchorView(
        layoutGuideCenter, preferredGuideName, fallbackGuideName);
  }

  if (IsValidAnchorView(anchorView, baseView)) {
    if (sourceViewOut) {
      *sourceViewOut = anchorView;
    }
    if (sourceRectOut) {
      *sourceRectOut = anchorView.bounds;
    }
    return;
  }

  CGRect guideRect = LayoutFrameForGuide(baseViewController, layoutGuideCenter,
                                         preferredGuideName);
  if (CGRectIsEmpty(guideRect) && fallbackGuideName.length &&
      ![fallbackGuideName isEqualToString:preferredGuideName]) {
    guideRect = LayoutFrameForGuide(baseViewController, layoutGuideCenter,
                                    fallbackGuideName);
  }
  if (!CGRectIsEmpty(guideRect)) {
    sourceRect = guideRect;
  }

  if (sourceViewOut) {
    *sourceViewOut = sourceView;
  }
  if (sourceRectOut) {
    *sourceRectOut = sourceRect;
  }
}
