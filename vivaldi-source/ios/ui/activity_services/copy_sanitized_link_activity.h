// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef IOS_VIVALDI_UI_ACTIVITY_SERVICES_COPY_SANITIZED_LINK_ACTIVITY_H_
#define IOS_VIVALDI_UI_ACTIVITY_SERVICES_COPY_SANITIZED_LINK_ACTIVITY_H_

#import <UIKit/UIKit.h>

@class ShareToData;

// UIActivity that copies share URLs without tracking parameters
@interface CopySanitizedLinkActivity : UIActivity

- (instancetype)initWithDataItems:(NSArray<ShareToData*>*)dataItems;

@end

#endif  // IOS_VIVALDI_UI_ACTIVITY_SERVICES_COPY_SANITIZED_LINK_ACTIVITY_H_
