// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/first_run/coordinator/first_run_coordinator.h"

#import <UIKit/UIKit.h>

#import "base/apple/foundation_util.h"
#import "base/feature_list.h"
#import "base/metrics/histogram_functions.h"
#import "base/notreached.h"
#import "base/time/time.h"
#import "components/feature_engagement/public/event_constants.h"
#import "components/feature_engagement/public/tracker.h"
#import "components/metrics/metrics_service.h"
#import "components/signin/public/base/signin_metrics.h"
#import "ios/chrome/browser/authentication/fullscreen_signin_screen/coordinator/fullscreen_signin_screen_coordinator.h"
#import "ios/chrome/browser/authentication/history_sync/coordinator/history_sync_coordinator.h"
#import "ios/chrome/browser/authentication/ui_bundled/continuation.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/signin_constants.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/signin_context_style.h"
#import "ios/chrome/browser/docking_promo/coordinator/docking_promo_coordinator.h"
#import "ios/chrome/browser/feature_engagement/model/tracker_factory.h"
#import "ios/chrome/browser/first_run/animated_lens/coordinator/animated_lens_promo_coordinator.h"
#import "ios/chrome/browser/first_run/best_features/coordinator/best_features_screen_coordinator.h"
#import "ios/chrome/browser/first_run/default_browser/coordinator/default_browser_screen_coordinator.h"
#import "ios/chrome/browser/first_run/interactive_lens/coordinator/interactive_lens_promo_coordinator.h"
#import "ios/chrome/browser/first_run/model/first_run_metrics.h"
#import "ios/chrome/browser/first_run/public/features.h"
#import "ios/chrome/browser/first_run/public/first_run_screen_delegate.h"
#import "ios/chrome/browser/first_run/public/first_run_util.h"
#import "ios/chrome/browser/intelligence/features/features.h"
#import "ios/chrome/browser/screen/ui_bundled/screen_provider.h"
#import "ios/chrome/browser/screen/ui_bundled/screen_type.h"
#import "ios/chrome/browser/search_engine_choice/coordinator/search_engine_choice_coordinator.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#import "ios/chrome/browser/shared/public/commands/new_tab_page_commands.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/public/provider/chrome/browser/signin/choice_api.h"

// Vivaldi
#import <AVFoundation/AVFoundation.h>

#import "app/vivaldi_apptools.h"
#import "base/functional/bind.h"
#import "base/functional/concurrent_closures.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/browser/main/ui/browser_layout_view_controller.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#import "ios/ui/ad_tracker_blocker/manager/vivaldi_atb_manager.h"
#import "ios/ui/helpers/vivaldi_global_helpers.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"
#import "ios/ui/modal_page/modal_page_commands.h"
#import "ios/ui/modal_page/modal_page_coordinator.h"
#import "ios/ui/onboarding/vivaldi_onboarding_swift.h"
#import "ios/ui/settings/tabs/vivaldi_tab_setting_prefs.h"
#import "ios/ui/settings/vivaldi_settings_constants.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"
// End Vivaldi

namespace first_run {

// Helper class used to access the passkey needed to call
// MetricsService::StartOutOfBandUploadIfPossible().
class FirstRunCoordinatorMetricsHelper final {
 public:
  FirstRunCoordinatorMetricsHelper() {
    metrics_service_ = GetApplicationContext()->GetMetricsService();
  }
  ~FirstRunCoordinatorMetricsHelper() {}

  // Triggers an UMA metrics log upload.
  void StartOutOfBandUploadIfPossible() {
    metrics_service_->StartOutOfBandUploadIfPossible(
        metrics::MetricsService::OutOfBandUploadPasskey());
  }

 private:
  raw_ptr<metrics::MetricsService> metrics_service_;
};

}  // namespace first_run

@interface FirstRunCoordinator () <FirstRunScreenDelegate,

#if defined(VIVALDI_BUILD)
                                   HistorySyncCoordinatorDelegate,
                                   ModalPageCommands>
#else
                                   HistorySyncCoordinatorDelegate>
#endif // End Vivaldi

@property(nonatomic, strong) ScreenProvider* screenProvider;
@property(nonatomic, strong) ChromeCoordinator* childCoordinator;

// Vivaldi
@property(strong,nonatomic)
    VivaldiOnboardingActionsBridge *onboardingActionsBridge;
@property(nonatomic, weak) id<ModalPageCommands> modalPageHandler;
@property(nonatomic, strong) ModalPageCoordinator* modalPageCoordinator;
@property(nonatomic, strong) UIViewController* onboardingVC;
@property(nonatomic, weak) UIViewController* onboardingCoveredViewController;
@property(nonatomic, assign)
    BOOL onboardingCoveredViewControllerAccessibilityElementsHidden;
@property(nonatomic, assign) BOOL onboardingFinishInProgress;
// End Vivaldi

@end

@implementation FirstRunCoordinator {
  // First Run navigation controller.
  UINavigationController* _navigationController;
}

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser
                            screenProvider:(ScreenProvider*)screenProvider {
  self = [super initWithBaseViewController:viewController browser:browser];
  if (self) {
    CHECK_EQ(browser->type(), Browser::Type::kRegular,
             base::NotFatalUntil::M145);
    _screenProvider = screenProvider;
    _navigationController =
        [[UINavigationController alloc] initWithNavigationBarClass:nil
                                                      toolbarClass:nil];
    _navigationController.modalPresentationStyle = UIModalPresentationFormSheet;

    // Vivaldi
    [browser->GetCommandDispatcher()
      stopDispatchingForProtocol:@protocol(ModalPageCommands)];
    [browser->GetCommandDispatcher()
      startDispatchingToTarget:self
                   forProtocol:@protocol(ModalPageCommands)];
    id<ModalPageCommands> modalPageHandler = HandlerForProtocol(
        browser->GetCommandDispatcher(), ModalPageCommands);
    _modalPageHandler = modalPageHandler;
    // End Vivaldi

  }
  return self;
}

- (void)dealloc {
  CHECK(!_navigationController, base::NotFatalUntil::M155);
  CHECK(!_childCoordinator, base::NotFatalUntil::M155);
}

- (void)start {
  [self presentScreen:[self.screenProvider nextScreenType]];
  void (^completion)(void) = ^{
    base::UmaHistogramEnumeration(first_run::kFirstRunStageHistogram,
                                  first_run::kStart);
  };

  if (vivaldi::IsVivaldiRunning()) {
    [self presentOnboarding];
  } else {
  [_navigationController setNavigationBarHidden:YES animated:NO];
  [self.baseViewController presentViewController:_navigationController
                                        animated:NO
                                      completion:completion];
  } // End Vivaldi

}

- (void)stopWithCompletion:(ProceduralBlock)completionHandler {
  if (self.childCoordinator) {
    // If the child coordinator is not nil, then the FRE is stopped because
    // Chrome is being shutdown.
    base::UmaHistogramEnumeration(first_run::kFirstRunStageHistogram,
                                  first_run::kFirstRunInterrupted);
    [self stopChildCoordinator];
  }

  if (vivaldi::IsVivaldiRunning()) {
    UIViewController* presentingViewController =
        _navigationController.presentingViewController;
    // Vivaldi hosts onboarding as a child view controller, so there is no
    // presenting view controller to invoke the dismissal completion.
    BOOL shouldRunCompletionManually =
        completionHandler && _navigationController && !presentingViewController;
    [presentingViewController dismissViewControllerAnimated:YES
                                                 completion:completionHandler];

    [self.modalPageCoordinator stop];
    self.modalPageCoordinator = nil;
    [self.browser->GetCommandDispatcher() stopDispatchingToTarget:self];
    self.modalPageHandler = nil;

    if (shouldRunCompletionManually) {
      completionHandler();
    }
  } else {
  [_navigationController.presentingViewController
      dismissViewControllerAnimated:YES
                         completion:completionHandler];
  }  // End Vivaldi

  _navigationController = nil;
  [super stop];
}

- (void)stop {
  [self stopWithCompletion:nil];
}

#pragma mark - FirstRunScreenDelegate

- (void)firstRunScreenCoordinatorWantsToBeStopped:
    (ChromeCoordinator*)coordinator {
  CHECK_EQ(coordinator, self.childCoordinator, base::NotFatalUntil::M155);
  [self stopChildCoordinator];

  // Vivaldi
  _onboardingVC = nil;
  _onboardingActionsBridge = nil;
  // End Vivaldi

  [self presentScreen:[self.screenProvider nextScreenType]];

  if (base::FeatureList::IsEnabled(first_run::kManualLogUploadsInTheFRE)) {
    // Trigger a metrics log upload with the MetricsService.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(^{
          std::unique_ptr<first_run::FirstRunCoordinatorMetricsHelper>
              metricsHelper = std::make_unique<
                  first_run::FirstRunCoordinatorMetricsHelper>();
          metricsHelper->StartOutOfBandUploadIfPossible();
        }));
  }
}

#pragma mark - Helper

// Presents the screen of certain `type`.
- (void)presentScreen:(ScreenType)type {
  // If no more screen need to be present, call delegate to stop presenting
  // screens.
  if (type == kStepsCompleted) {
    // The user went through all screens of the FRE.
    base::UmaHistogramEnumeration(first_run::kFirstRunStageHistogram,
                                  first_run::kComplete);

    feature_engagement::Tracker* tracker =
        feature_engagement::TrackerFactory::GetForProfile(self.profile);

    if (tracker) {
      tracker->NotifyEvent(feature_engagement::events::kIOSFirstRunComplete);
    }

    WriteFirstRunSentinel();
    [self.delegate didFinishFirstRun];

    if (self.browser == nullptr) {
      // Speculative fix for some of the crashes in crbug.com/474279386. There
      // is most likely an underlying issue somewhere in the cleanup logic, but
      // null checking here should at least prevent some crashes.
      return;
    }

    if (IsAppStoreInAppEventsEnabled() && self.profile &&
        self.profile->GetPrefs() &&
        self.profile->GetPrefs()->GetBoolean(
            prefs::kAppStoreGeminiPromoTriggered)) {
      // If first run started due to app store external action, do not show
      // any follow up IPH.
      return;
    }

    if (IsBestOfAppLensAnimatedPromoEnabled()) {
      // Present the Lens entrypoint IPH.
      [HandlerForProtocol(self.browser->GetCommandDispatcher(),
                          NewTabPageCommands) presentLensIconBubble];
    } else {
      // Present feed swipe IPH.
      [HandlerForProtocol(self.browser->GetCommandDispatcher(),
                          NewTabPageCommands) presentFeedSwipeFirstRunBubble];
    }

    return;
  }
  self.childCoordinator = [self createChildCoordinatorWithScreenType:type];
  [self.childCoordinator start];
}

// Creates a screen coordinator according to `type`.
- (ChromeCoordinator*)createChildCoordinatorWithScreenType:(ScreenType)type {
  switch (type) {
    case kSignIn:
      return [[FullscreenSigninScreenCoordinator alloc]
           initWithBaseNavigationController:_navigationController
                                    browser:self.browser
                                   delegate:self
                               contextStyle:SigninContextStyle::kDefault
                                accessPoint:signin_metrics::AccessPoint::
                                                kStartPage
                                promoAction:signin_metrics::PromoAction::
                                                PROMO_ACTION_NO_SIGNIN_PROMO
          changeProfileContinuationProvider:DoNothingContinuationProvider()];
    case kHistorySync:
      return [[HistorySyncCoordinator alloc]
          initWithBaseNavigationController:_navigationController
                                   browser:self.browser
                                  delegate:self
                                  firstRun:YES
                             showUserEmail:NO
                                isOptional:YES
                              contextStyle:SigninContextStyle::kDefault
                               accessPoint:signin_metrics::AccessPoint::
                                               kStartPage];
    case kDefaultBrowserPromo:
      return [[DefaultBrowserScreenCoordinator alloc]
          initWithBaseNavigationController:_navigationController
                                   browser:self.browser
                                  delegate:self];
    case kChoice:
      return [[SearchEngineChoiceCoordinator alloc]
          initForFirstRunWithBaseNavigationController:_navigationController
                                              browser:self.browser
                                     firstRunDelegate:self];
    case kBestFeatures:
      return [[BestFeaturesScreenCoordinator alloc]
          initWithBaseNavigationController:_navigationController
                                   browser:self.browser
                                  delegate:self];
    case kLensInteractivePromo: {
      InteractiveLensPromoCoordinator* lensInteractivePromoCoordinator =
          [[InteractiveLensPromoCoordinator alloc]
              initWithBaseNavigationController:_navigationController
                                       browser:self.browser];
      lensInteractivePromoCoordinator.firstRunDelegate = self;
      return lensInteractivePromoCoordinator;
    }
    case kLensAnimatedPromo: {
      AnimatedLensPromoCoordinator* lensAnimatedPromoCoordinator =
          [[AnimatedLensPromoCoordinator alloc]
              initWithBaseNavigationController:_navigationController
                                       browser:self.browser];
      lensAnimatedPromoCoordinator.firstRunDelegate = self;
      return lensAnimatedPromoCoordinator;
    }
    case kSyncedSetUp:
    case kGuidedTour:
    case kSafariImport:
    case kStepsCompleted:
      NOTREACHED() << "Reaches kStepsCompleted unexpectedly.";
  }
  return nil;
}

#pragma mark - HistorySyncCoordinatorDelegate

- (void)historySyncCoordinator:(HistorySyncCoordinator*)historySyncCoordinator
                    withResult:(HistorySyncResult)result {
  CHECK_EQ(self.childCoordinator, historySyncCoordinator);
  [self firstRunScreenCoordinatorWantsToBeStopped:historySyncCoordinator];
}

#pragma mark - Private

- (void)stopChildCoordinator {
  [self.childCoordinator stop];
  self.childCoordinator = nil;
}

#pragma mark: - VIVALDI
- (void)presentOnboarding {
  // Alter audio session
  [self modifyAudioSession];

  // Initiate the onboarding pages
  self.onboardingActionsBridge = [[VivaldiOnboardingActionsBridge alloc] init];
  UIViewController *onboardingVC =
      [self.onboardingActionsBridge makeViewController];
  _onboardingVC = onboardingVC;

  // Note: (prio@vivaldi.com) On iPads, modal presentation styles are treated
  // as adaptive interfaces, which means they can be presented as floating
  // cards that users can easily dismiss. And this dismisses onboarding views if
  // user goes to Settings page for default browser settings or moves the app to
  // background. This behavior is present in Chrome too.

  // To prevent our onboarding view from being dismissed in such scenarios,
  // we've chosen to add it as a child view controller. This ensures the
  // onboarding view is treated
  // as part of the main view hierarchy rather than a dismissible modal. This
  // also fixes the issues with start page being visible momentariliy after
  // splash screen and before onboarding pages.

  // `baseViewController` is the BrowserViewController. From Chr148 the tab
  // strip is owned by its parent BrowserLayoutViewController, so hosting
  // onboarding on the BVC leaves the tab strip visible. Host onboarding on the
  // layout controller's parent (the TabGridViewController) instead so it covers
  // both the BVC and layout controller UI components.
  BrowserLayoutViewController* browserLayoutViewController =
      base::apple::ObjCCast<BrowserLayoutViewController>(
          self.baseViewController.parentViewController);
  UIViewController* hostViewController =
      browserLayoutViewController.parentViewController ?:
          self.baseViewController;
  if (browserLayoutViewController.parentViewController) {
    self.onboardingCoveredViewController = browserLayoutViewController;
    self.onboardingCoveredViewControllerAccessibilityElementsHidden =
        browserLayoutViewController.view.accessibilityElementsHidden;
    // Disable the covered browser layout's accessibility tree while onboarding
    // is modal, otherwise VoiceOver can still reach controls behind it.
    browserLayoutViewController.view.accessibilityElementsHidden = YES;
  }
  onboardingVC.view.accessibilityViewIsModal = YES;
  [onboardingVC willMoveToParentViewController:hostViewController];
  [hostViewController addChildViewController:onboardingVC];
  [hostViewController.view addSubview:onboardingVC.view];
  [hostViewController.view bringSubviewToFront:onboardingVC.view];
  [onboardingVC didMoveToParentViewController:hostViewController];
  [onboardingVC.view fillSuperview];
  UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification,
                                  onboardingVC.view);

  [self.onboardingActionsBridge observeTOSURLTapEvent:^(NSURL *url,
                                                        NSString *title) {
    [self.modalPageHandler showModalPage:url
                                   title:title];
  }];

  [self.onboardingActionsBridge observePrivacyURLTapEvent:^(NSURL *url,
                                                            NSString *title) {
    [self.modalPageHandler showModalPage:url
                                   title:title];
  }];

  [self.onboardingActionsBridge
    observeAdblockerSettingChange:^(ATBSettingType setting) {
    // Create a weak reference and store the settings to pref.
    VivaldiATBManager* adblockManager =
        [[VivaldiATBManager alloc] initWithBrowser:self.browser];
    if (!adblockManager)
      return;
    [adblockManager setExceptionFromBlockingType:setting];
  }];

  [self.onboardingActionsBridge
    observeTabStyleChange:^(BOOL isTabsOn) {
    [VivaldiTabSettingPrefs
      setDesktopTabsMode:isTabsOn
          inPrefServices:self.browser->GetProfile()->GetPrefs()];
  }];

  [self.onboardingActionsBridge
    observeOmniboxPositionChange:^(BOOL isBottomOmniboxEnabled) {
    [VivaldiTabSettingPrefs
        setBottomOmniboxEnabled:isBottomOmniboxEnabled
            inPrefServices:GetApplicationContext()->GetLocalState()];
    [VivaldiTabSettingPrefs
        setReverseSearchSuggestionsEnabled:isBottomOmniboxEnabled
            inPrefServices:self.browser->GetProfile()->GetPrefs()];
  }];

  [self.onboardingActionsBridge observeOnboardingFinishedState:^{
    [self finishOnboardingAfterPersistingPrefs];
  }];
}

- (void)finishOnboardingAfterPersistingPrefs {
  if (self.onboardingFinishInProgress) {
    return;
  }
  self.onboardingFinishInProgress = YES;

  // The last onboarding page writes profile prefs and Local State, then
  // immediately finishes. Commit both stores before writing the first-run
  // sentinel so a quick kill/restart cannot skip onboarding with default
  // settings still on disk.
  base::ConcurrentClosures prefCommits;
  ProfileIOS* profile = self.browser ? self.browser->GetProfile() : nullptr;
  if (profile && profile->GetPrefs()) {
    profile->GetPrefs()->CommitPendingWrite(prefCommits.CreateClosure());
  }
  PrefService* localState = GetApplicationContext()->GetLocalState();
  if (localState) {
    localState->CommitPendingWrite(prefCommits.CreateClosure());
  }

  __weak __typeof(self) weakSelf = self;
  std::move(prefCommits).Done(base::BindOnce(^{
    __typeof(self) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    [strongSelf dismissOnboarding];
    [strongSelf.delegate didFinishFirstRun];
  }));
}

- (void)dismissOnboarding {
  if (_onboardingVC) {
    // Enable browser view accessibility after onboarding.
    if (self.onboardingCoveredViewController) {
      self.onboardingCoveredViewController.view.accessibilityElementsHidden =
          self.onboardingCoveredViewControllerAccessibilityElementsHidden;
      self.onboardingCoveredViewController = nil;
    }
    [_onboardingVC willMoveToParentViewController:nil];
    [_onboardingVC.view removeFromSuperview];
    [_onboardingVC removeFromParentViewController];
    [_onboardingVC didMoveToParentViewController:nil];
    _onboardingVC = nil;

    WriteFirstRunSentinel();

    [self restoreAudioSession];
  }
}

// Makes sure audio from other apps are not interrupted when onboarding page is
// presented
- (void)modifyAudioSession {
  AVAudioSession *audioSession = [AVAudioSession sharedInstance];
  // Set the audio session category for onboarding
  [audioSession setCategory:AVAudioSessionCategoryPlayback
                withOptions:AVAudioSessionCategoryOptionMixWithOthers
                      error:nil];
  [audioSession setActive:YES error:nil];
}

- (void)restoreAudioSession {
  AVAudioSession *audioSession = [AVAudioSession sharedInstance];

  [audioSession setActive:NO
            withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                    error:nil];
  [audioSession setCategory:AVAudioSessionCategoryAmbient error:nil];
  [audioSession setMode:AVAudioSessionModeDefault error:nil];
  [audioSession setActive:YES error:nil];
}

#pragma mark - ModalPageCommands
- (void)showModalPage:(NSURL*)url
                title:(NSString*)title {
  if (!self.onboardingVC || !self.browser)
    return;

  self.modalPageCoordinator = [[ModalPageCoordinator alloc]
      initWithBaseViewController:self.onboardingVC
                         browser:self.browser
                         pageURL:url
                           title:title];
  [self.modalPageCoordinator start];
}

- (void)closeModalPage {
  [self.modalPageCoordinator stop];
  self.modalPageCoordinator = nil;
}
// End Vivaldi

@end
