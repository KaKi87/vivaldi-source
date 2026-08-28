// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// This file is based on media_backgrounding_javascript_feature.h from
// brave-core (https://github.com/brave/brave-core), licensed under the MPL 2.0.
// Modifications Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_BACKGROUND_AUDIO_BACKGROUND_AUDIO_JAVASCRIPT_FEATURE_H_
#define IOS_BACKGROUND_AUDIO_BACKGROUND_AUDIO_JAVASCRIPT_FEATURE_H_

#import "base/memory/raw_ptr.h"
#import "base/supports_user_data.h"
#import "components/prefs/pref_change_registrar.h"
#import "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class BrowserState;
}  // namespace web

class ProfileIOS;

class BackgroundAudioJavaScriptFeature : public web::JavaScriptFeature,
                                         public base::SupportsUserData::Data {
 public:
  ~BackgroundAudioJavaScriptFeature() override;

  static BackgroundAudioJavaScriptFeature* FromBrowserState(
      web::BrowserState* browser_state);

  // web::JavaScriptFeature
  std::vector<web::JavaScriptFeature::FeatureScript> GetScripts()
      const override;

 private:
  PrefChangeRegistrar pref_change_registrar_;
  raw_ptr<ProfileIOS> profile_;

  void OnPrefUpdated();

  explicit BackgroundAudioJavaScriptFeature(ProfileIOS* profile);
};

#endif  // IOS_BACKGROUND_AUDIO_BACKGROUND_AUDIO_JAVASCRIPT_FEATURE_H_
