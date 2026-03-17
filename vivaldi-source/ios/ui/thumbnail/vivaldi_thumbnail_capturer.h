// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_THUMBNAIL_VIVALDI_THUMBNAIL_CAPTURER_H_
#define IOS_UI_THUMBNAIL_VIVALDI_THUMBNAIL_CAPTURER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class WKWebViewConfiguration;

typedef void (^VivaldiThumbnailCapturerCompletion)(UIImage* _Nullable image,
                                                   NSError* _Nullable error);

NS_ASSUME_NONNULL_BEGIN

@interface VivaldiThumbnailCapturer : NSObject

- (instancetype)init;
- (instancetype)initWithConfigurationProvider:
    (WKWebViewConfiguration* _Nullable (^_Nullable)(void))configurationProvider
    NS_DESIGNATED_INITIALIZER;

- (void)captureSnapshotWithURL:(NSURL*)url
                    completion:(VivaldiThumbnailCapturerCompletion)completion;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_UI_THUMBNAIL_VIVALDI_THUMBNAIL_CAPTURER_H_
