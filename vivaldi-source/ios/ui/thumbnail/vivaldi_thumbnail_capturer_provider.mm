// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/ui/thumbnail/vivaldi_thumbnail_capturer_provider.h"

#import <WebKit/WebKit.h>

#include <memory>

#import "base/supports_user_data.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/ui/thumbnail/vivaldi_thumbnail_capturer.h"
#import "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

namespace {
const char kThumbnailCapturerKey = 0;

class ThumbnailCapturerUserData : public base::SupportsUserData::Data {
 public:
  explicit ThumbnailCapturerUserData(ProfileIOS* profile) {
    web::WKWebViewConfigurationProvider* config_provider =
        &web::WKWebViewConfigurationProvider::FromBrowserState(profile);
    capturer_ = [[VivaldiThumbnailCapturer alloc]
        initWithConfigurationProvider:^WKWebViewConfiguration* {
          return config_provider->GetWebViewConfiguration();
        }];
  }

  VivaldiThumbnailCapturer* capturer() { return capturer_; }

 private:
  __strong VivaldiThumbnailCapturer* capturer_ = nil;
};
}  // namespace

@implementation VivaldiThumbnailCapturerProvider

+ (VivaldiThumbnailCapturer*)sharedCapturerForProfile:(ProfileIOS*)profile {
  if (!profile) {
    return [[VivaldiThumbnailCapturer alloc] init];
  }
  if (!profile->GetUserData(&kThumbnailCapturerKey)) {
    profile->SetUserData(&kThumbnailCapturerKey,
                         std::make_unique<ThumbnailCapturerUserData>(profile));
  }
  auto* data = static_cast<ThumbnailCapturerUserData*>(
      profile->GetUserData(&kThumbnailCapturerKey));
  return data->capturer();
}

@end
