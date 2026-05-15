// Copyright 2024 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_NTP_VIVALDI_START_PAGE_ADD_GROUP_VIEW_H_
#define IOS_UI_NTP_VIVALDI_START_PAGE_ADD_GROUP_VIEW_H_

#import <UIKit/UIKit.h>

@protocol VivaldiStartPageAddGroupViewDelegate
- (void)didTapAddNewGroup;
@end

// A view to hold the add new group view on start page.
@interface VivaldiStartPageAddGroupView : UIView

@property(nonatomic, weak) id<VivaldiStartPageAddGroupViewDelegate> delegate;

// Public
- (void)refreshLayoutWithVerticalSizeClass:
    (UIUserInterfaceSizeClass)verticalSizeClass;

@end

#endif  // IOS_UI_NTP_VIVALDI_START_PAGE_ADD_GROUP_VIEW_H_
