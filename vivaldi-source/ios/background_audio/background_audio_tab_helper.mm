// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#import "ios/background_audio/background_audio_tab_helper.h"

#import "components/prefs/pref_service.h"
#import "ios/chrome/browser/shared/model/prefs/pref_backed_boolean.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/model/utils/observable_boolean.h"
#import "ios/web/public/navigation/navigation_context.h"
#import "ios/web/public/navigation/navigation_manager.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"

@interface BackgroundAudioPrefObserver () <BooleanObserver>

// PrefBackedBoolean for "Allow Background Audio" setting state.
@property(nonatomic, strong, readonly) PrefBackedBoolean* allowBackgroundAudio;
@property(nonatomic, assign, readonly) BackgroundAudioTabHelper* owner;

@end

@implementation BackgroundAudioPrefObserver
- (instancetype)initWithOwner:(BackgroundAudioTabHelper*)owner
                  prefService:(PrefService*)prefs {
  self = [super init];
  if (self) {
    _owner = owner;

    _allowBackgroundAudio = [[PrefBackedBoolean alloc]
        initWithPrefService:prefs
                   prefName:vivaldiprefs::kVivaldiBackgroundAudioEnabled];
    [_allowBackgroundAudio setObserver:self];
  }
  return self;
}

- (void)stopObserving {
  [_allowBackgroundAudio stop];
  [_allowBackgroundAudio setObserver:nil];
  _allowBackgroundAudio = nil;
}

- (void)dealloc {
  _owner = nil;
}

#pragma mark - BooleanObserver

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  if (!_owner) {
    return;
  }
  if (observableBoolean == _allowBackgroundAudio) {
    _owner->BackgroundAudioPrefChanged();
  }
}

@end

BackgroundAudioTabHelper::~BackgroundAudioTabHelper() = default;

BackgroundAudioTabHelper::BackgroundAudioTabHelper(web::WebState* web_state)
    : prefs_(ProfileIOS::FromBrowserState(web_state->GetBrowserState())
                 ->GetPrefs()),
      web_state_(web_state) {
  web_state->AddObserver(this);
  allowBackgroundAudioObserver_ =
      [[BackgroundAudioPrefObserver alloc] initWithOwner:this
                                             prefService:prefs_];
}

#pragma mark - WebStateObserver methods.

void BackgroundAudioTabHelper::WebStateDestroyed(web::WebState* web_state) {
  CHECK_EQ(web_state, web_state_);
  web_state->RemoveObserver(this);
  [allowBackgroundAudioObserver_ stopObserving];
}

void BackgroundAudioTabHelper::BackgroundAudioPrefChanged() {
  if (web_state_->IsVisible()) {
    web_state_->GetNavigationManager()->Reload(web::ReloadType::NORMAL,
                                               /*check_for_repost=*/true);
  }
}
