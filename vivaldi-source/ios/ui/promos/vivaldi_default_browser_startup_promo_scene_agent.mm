// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/ui/promos/vivaldi_default_browser_startup_promo_scene_agent.h"

#import "base/time/time.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/app/profile/profile_init_stage.h"
#import "ios/chrome/app/profile/profile_state.h"
#import "ios/chrome/app/profile/profile_state_observer.h"
#import "ios/chrome/browser/default_browser/model/utils.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/browser/browser_provider.h"
#import "ios/chrome/browser/shared/model/browser/browser_provider_interface.h"
#import "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#import "ios/chrome/browser/shared/public/commands/promos_manager_commands.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"

@interface VivaldiDefaultBrowserStartupPromoSceneAgent () <ProfileStateObserver>
@end

namespace {

constexpr base::TimeDelta kDefaultBrowserStartupPromoInactivityThreshold =
    base::Days(7);

}  // namespace

@implementation VivaldiDefaultBrowserStartupPromoSceneAgent {
  BOOL _foregroundActivationHandled;
  BOOL _observingProfileState;
}

#pragma mark - ObservingSceneAgent

- (void)setSceneState:(SceneState*)sceneState {
  [super setSceneState:sceneState];
  [self startObservingProfileStateIfNeeded];
  [self handleForegroundActivationIfNeeded];
}

#pragma mark - SceneStateObserver

- (void)sceneState:(SceneState*)sceneState
    profileStateConnected:(ProfileState*)profileState {
  [self startObservingProfileStateIfNeeded];
  [self handleForegroundActivationIfNeeded];
}

- (void)sceneState:(SceneState*)sceneState
    transitionedToActivationLevel:(SceneActivationLevel)level {
  [self handleForegroundActivationIfNeeded];
}

- (void)sceneStateDidEnableUI:(SceneState*)sceneState {
  [self handleForegroundActivationIfNeeded];
}

- (void)sceneStateDidHideModalOverlay:(SceneState*)sceneState {
  [self handleForegroundActivationIfNeeded];
}

- (void)signinDidEnd:(SceneState*)sceneState {
  [self handleForegroundActivationIfNeeded];
}

- (void)sceneStateDidDisableUI:(SceneState*)sceneState {
  [self stopObservingProfileStateIfNeeded];
  [self.sceneState removeObserver:self];
}

#pragma mark - ProfileStateObserver

- (void)profileState:(ProfileState*)profileState
    didTransitionToInitStage:(ProfileInitStage)nextInitStage
               fromInitStage:(ProfileInitStage)fromInitStage {
  [self handleForegroundActivationIfNeeded];
}

#pragma mark - Private

- (void)startObservingProfileStateIfNeeded {
  if (_observingProfileState || !self.sceneState.profileState) {
    return;
  }

  [self.sceneState.profileState addObserver:self];
  _observingProfileState = YES;
}

- (void)stopObservingProfileStateIfNeeded {
  if (!_observingProfileState || !self.sceneState.profileState) {
    return;
  }

  [self.sceneState.profileState removeObserver:self];
  _observingProfileState = NO;
}

- (void)handleForegroundActivationIfNeeded {
  if (_foregroundActivationHandled || !self.sceneState.profileState ||
      self.sceneState.profileState.initStage < ProfileInitStage::kFinal ||
      !self.sceneState.UIEnabled ||
      self.sceneState.activationLevel < SceneActivationLevelForegroundActive ||
      self.sceneState.signinInProgress ||
      self.sceneState.presentingModalOverlay) {
    return;
  }

  _foregroundActivationHandled = YES;

  PrefService* localState = GetApplicationContext()->GetLocalState();
  if (!localState) {
    return;
  }

  // Use the previous foreground activation as the user's last browser use.
  base::Time now = base::Time::Now();
  base::Time lastActiveTime = localState->GetTime(
      vivaldiprefs::kVivaldiDefaultBrowserStartupPromoLastActiveTime);
  localState->SetTime(
      vivaldiprefs::kVivaldiDefaultBrowserStartupPromoLastActiveTime, now);

  if (lastActiveTime.is_null() ||
      now - lastActiveTime < kDefaultBrowserStartupPromoInactivityThreshold ||
      self.sceneState.startupHadExternalIntent ||
      IsChromeLikelyDefaultBrowser()) {
    return;
  }

  Browser* browser =
      self.sceneState.browserProviderInterface.mainBrowserProvider.browser;
  if (!browser) {
    return;
  }

  id<PromosManagerCommands> handler = HandlerForProtocol(
      browser->GetCommandDispatcher(), PromosManagerCommands);
  [handler showDefaultBrowserPromo];
}

@end
