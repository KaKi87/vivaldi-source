// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef IOS_CHROME_BROWSER_SHARING_UI_BUNDLED_SHARING_POSITIONER_H_
#define IOS_CHROME_BROWSER_SHARING_UI_BUNDLED_SHARING_POSITIONER_H_

#import <UIKit/UIKit.h>

// Protocol for providing a source view and rect for share UI positioning.
@protocol SharingPositioner <NSObject>
- (UIView*)sourceView;
- (CGRect)sourceRect;
@end

#endif  // IOS_CHROME_BROWSER_SHARING_UI_BUNDLED_SHARING_POSITIONER_H_
