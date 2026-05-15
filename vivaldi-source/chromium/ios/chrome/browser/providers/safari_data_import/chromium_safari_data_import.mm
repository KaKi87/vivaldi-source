// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>

#import "ios/public/provider/chrome/browser/safari_data_import/safari_data_import_api.h"

namespace ios {
namespace provider {

void OpenSettingsToExportDataFromSafari() {

#if defined(VIVALDI_BUILD)
  // Note: App-Prefs: URLs do not work in the iOS Simulator,
  // the fallback opens the current app's Settings entry so the
  // button is not a no-op during Simulator-based development.
  NSURL* url = [NSURL URLWithString:@"App-Prefs:com.apple.mobilesafari"];
  [[UIApplication sharedApplication] openURL:url
      options:@{}
      completionHandler:^(BOOL success) {
        if (!success) {
          NSURL* fallback =
              [NSURL URLWithString:UIApplicationOpenSettingsURLString];
          [[UIApplication sharedApplication] openURL:fallback
                                             options:@{}
                                   completionHandler:nil];
        }
      }];
#else
  NSURL* url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
  if ([[UIApplication sharedApplication] canOpenURL:url]) {
    [[UIApplication sharedApplication] openURL:url
                                       options:@{}
                             completionHandler:nil];
  }
#endif // End Vivaldi

}

}  // namespace provider
}  // namespace ios
