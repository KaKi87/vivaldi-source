// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_CHROME_BROWSER_UI_LOCATION_BAR_VIVALDI_LOCATION_BAR_STEADY_VIEW_CONTAINER_H_
#define IOS_CHROME_BROWSER_UI_LOCATION_BAR_VIVALDI_LOCATION_BAR_STEADY_VIEW_CONTAINER_H_

#import <UIKit/UIKit.h>

// Places `leadingButton` in a transparent, touch target inside
// `locationButton` without changing the leading button's visual constraints.
void AttachVivaldiLocationBarLeadingButtonContainer(UIView* locationButton,
                                                    UIButton* leadingButton);

#endif  // IOS_CHROME_BROWSER_UI_LOCATION_BAR_VIVALDI_LOCATION_BAR_STEADY_VIEW_CONTAINER_H_
