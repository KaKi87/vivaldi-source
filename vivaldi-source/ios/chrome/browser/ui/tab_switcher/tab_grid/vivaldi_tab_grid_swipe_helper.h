// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_VIVALDI_TAB_GRID_SWIPE_HELPER_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_VIVALDI_TAB_GRID_SWIPE_HELPER_H_

#import <UIKit/UIKit.h>

// Attaches Vivaldi swipe-to-close behavior to tab grid cells.
@interface VivaldiTabGridSwipeHelper : NSObject

// Installs the swipe recognizer (if needed) and resets any swipe state.
+ (void)attachToCell:(UICollectionViewCell*)cell;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_VIVALDI_TAB_GRID_SWIPE_HELPER_H_
