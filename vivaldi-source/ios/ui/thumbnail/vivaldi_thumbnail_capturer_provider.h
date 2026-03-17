// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_THUMBNAIL_VIVALDI_THUMBNAIL_CAPTURER_PROVIDER_H_
#define IOS_UI_THUMBNAIL_VIVALDI_THUMBNAIL_CAPTURER_PROVIDER_H_

#import <Foundation/Foundation.h>

class ProfileIOS;
@class VivaldiThumbnailCapturer;

// Provides a shared thumbnail capturer tied to a ProfileIOS lifetime.
// This is required to keep the capturer alive when user navigates to any page
// after starting the capture. Otherwise capture fails silently since the
// caller objects get deallocated.
@interface VivaldiThumbnailCapturerProvider : NSObject

// Returns a shared capturer for |profile|. The capturer is owned by the
// profile and survives view controller teardown.
+ (VivaldiThumbnailCapturer*)sharedCapturerForProfile:(ProfileIOS*)profile;

@end

#endif  // IOS_UI_THUMBNAIL_VIVALDI_THUMBNAIL_CAPTURER_PROVIDER_H_
