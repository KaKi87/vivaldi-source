// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// This file is based on media_backgrounding_javascript_feature.mm from
// brave-core (https://github.com/brave/brave-core), licensed under the MPL 2.0.
// Modifications Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/background_audio/background_audio_javascript_feature.h"

#import "base/functional/bind.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/web/public/browser_state.h"
#import "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"

namespace {
constexpr char kBackgroundAudioJavaScriptFeatureKeyName[] =
    "background_audio_java_script_feature";
constexpr char kScriptName[] = "background_audio";
}  // namespace

BackgroundAudioJavaScriptFeature::BackgroundAudioJavaScriptFeature(
    ProfileIOS* profile)
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        /*feature_scripts=*/{}),
      profile_(profile) {
  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      vivaldiprefs::kVivaldiBackgroundAudioEnabled,
      base::BindRepeating(&BackgroundAudioJavaScriptFeature::OnPrefUpdated,
                          base::Unretained(this)));
}

BackgroundAudioJavaScriptFeature::~BackgroundAudioJavaScriptFeature() = default;

// static
BackgroundAudioJavaScriptFeature*
BackgroundAudioJavaScriptFeature::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);
  BackgroundAudioJavaScriptFeature* feature =
      static_cast<BackgroundAudioJavaScriptFeature*>(
          browser_state->GetUserData(kBackgroundAudioJavaScriptFeatureKeyName));
  if (!feature) {
    feature = new BackgroundAudioJavaScriptFeature(
        ProfileIOS::FromBrowserState(browser_state));
    browser_state->SetUserData(kBackgroundAudioJavaScriptFeatureKeyName,
                               base::WrapUnique(feature));
  }
  return feature;
}

void BackgroundAudioJavaScriptFeature::OnPrefUpdated() {
  // Feature scripts must be explicitly updated after they change.
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(profile_);
  config_provider.UpdateScripts();
}

std::vector<web::JavaScriptFeature::FeatureScript>
BackgroundAudioJavaScriptFeature::GetScripts() const {
  bool is_background_audio_enabled = profile_->GetPrefs()->GetBoolean(
      vivaldiprefs::kVivaldiBackgroundAudioEnabled);
  return {FeatureScript::CreateWithFilename(
      kScriptName, FeatureScript::InjectionTime::kDocumentStart,
      FeatureScript::TargetFrames::kAllFrames,
      FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
      base::BindRepeating(
          [](bool is_background_audio_enabled)
              -> FeatureScript::PlaceholderReplacements {
            return @{
              @"window.gCrWebPlaceholderBackgroundAudioEnabled" :
                      is_background_audio_enabled ? @"true" : @"false"
            };
          },
          is_background_audio_enabled))};
}
