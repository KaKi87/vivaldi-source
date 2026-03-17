// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef IOS_CHROME_BROWSER_SHARING_UI_BUNDLED_VIVALDI_SHARING_UTILS_H_
#define IOS_CHROME_BROWSER_SHARING_UI_BUNDLED_VIVALDI_SHARING_UTILS_H_

#import <UIKit/UIKit.h>

// Finds the presented overflow menu hosting controller in the current
// presentation chain, if any.
UIViewController* VivaldiFindPresentedOverflowMenuController(
    UIViewController* rootViewController);

// Dismisses the popup menu handler if possible and invokes `completion` on the
// main queue once dismissal is expected to be completed.
BOOL VivaldiDismissPopupMenuAndRunCompletion(id popupMenuHandler,
                                             BOOL animated,
                                             dispatch_block_t completion);

// Returns the referenced tools menu anchor view for `preferredGuideName`,
// falling back to `fallbackGuideName` if needed.
UIView* VivaldiFindToolsMenuAnchorView(id layoutGuideCenter,
                                       NSString* preferredGuideName,
                                       NSString* fallbackGuideName);

// Resolves a robust share sheet anchor using `preferredAnchorView`, layout
// guide references, and layout guide frames.
void VivaldiResolveShareAnchor(UIViewController* baseViewController,
                               id layoutGuideCenter,
                               UIView* preferredAnchorView,
                               NSString* preferredGuideName,
                               NSString* fallbackGuideName,
                               UIView** sourceViewOut,
                               CGRect* sourceRectOut);

#endif  // IOS_CHROME_BROWSER_SHARING_UI_BUNDLED_VIVALDI_SHARING_UTILS_H_
