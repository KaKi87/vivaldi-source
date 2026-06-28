// Copyright 2024 Vivaldi Technologies. All rights reserved.

#import "ios/ui/settings/start_page/quick_settings/vivaldi_start_page_quick_settings_coordinator.h"

#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/url_loading/model/url_loading_browser_agent.h"
#import "ios/chrome/browser/url_loading/model/url_loading_params.h"
#import "ios/ui/settings/start_page/quick_settings/quick_settings_swift.h"
#import "ios/ui/settings/start_page/quick_settings/vivaldi_start_page_quick_settings_mediator.h"
#import "ui/base/l10n/l10n_util_mac.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

@interface VivaldiStartPageQuickSettingsCoordinator () <
    UIAdaptivePresentationControllerDelegate> {
  // The browser where the settings are being displayed.
  Browser* _browser;
}
// View provider for the start page quick setting.
@property(nonatomic, strong)
    VivaldiStartPageQuickSettingsViewProvider* viewProvider;
// Start page quick settings preference mediator.
@property(nonatomic, strong) VivaldiStartPageQuickSettingsMediator* mediator;

@end

@implementation VivaldiStartPageQuickSettingsCoordinator

@synthesize baseNavigationController = _baseNavigationController;

- (instancetype)initWithBaseNavigationController:
                    (UINavigationController*)navigationController
                                         browser:(Browser*)browser {
  self = [super initWithBaseViewController:navigationController
                                   browser:browser];

  if (self) {
    _browser = browser;
    _baseNavigationController = navigationController;
  }

  return self;
}

#pragma mark - ChromeCoordinator

- (void)start {
  VivaldiStartPageQuickSettingsViewProvider* viewProvider =
      [VivaldiStartPageQuickSettingsViewProvider new];
  self.viewProvider = viewProvider;
  UIViewController* controller = [self.viewProvider makeViewController];
  controller.title = l10n_util::GetNSString(IDS_IOS_START_PAGE_CUSTOMIZE_TITLE);
  controller.navigationItem.largeTitleDisplayMode =
      UINavigationItemLargeTitleDisplayModeNever;
  controller.modalPresentationStyle = UIModalPresentationPageSheet;

  UINavigationController* navController =
      [[UINavigationController alloc] initWithRootViewController:controller];
  navController.presentationController.delegate = self;

  // Add Done button
  UIBarButtonItem* doneItem = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                           target:self
                           action:@selector(handleDoneButtonTap)];
  controller.navigationItem.rightBarButtonItem = doneItem;

  UISheetPresentationController* sheetPc =
      navController.sheetPresentationController;

  // When iPad full screen or 2/3 SplitView support only large detent because
  // medium detent cuts the contents makes the dialog small and off centered.
  if (IsSplitToolbarMode(self.baseViewController)) {
    sheetPc.detents = @[
      UISheetPresentationControllerDetent.mediumDetent,
      UISheetPresentationControllerDetent.largeDetent
    ];
  } else {
    sheetPc.detents = @[ UISheetPresentationControllerDetent.largeDetent ];
  }

  sheetPc.prefersScrollingExpandsWhenScrolledToEdge = NO;
  sheetPc.widthFollowsPreferredContentSizeWhenEdgeAttached = YES;
  [self.baseViewController presentViewController:navController
                                        animated:YES
                                      completion:nil];

  self.mediator = [[VivaldiStartPageQuickSettingsMediator alloc]
      initWithOriginalPrefService:self.browser->GetProfile()
                                      ->GetOriginalProfile()
                                      ->GetPrefs()];
  self.mediator.consumer = self.viewProvider;

  self.viewProvider.settingsStateConsumer = self.mediator;

  __weak __typeof(self) weakSelf = self;
  [self.viewProvider observePhotoCreditLinkTap:^(NSURL* url) {
    [weakSelf openURLInNewTab:url];
  }];
}

- (void)stop {
  [super stop];
  self.viewProvider.settingsStateConsumer = nil;
  self.viewProvider = nil;
  self.mediator.consumer = nil;
  [self.mediator disconnect];
  self.mediator = nil;
}

- (void)handleDoneButtonTap {
  [self stop];
  [self.baseViewController dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - Private
- (void)openURLInNewTab:(NSURL*)url {
  GURL gurl(url.absoluteString.UTF8String);
  if (!gurl.is_valid())
    return;
  UIViewController* presentedController =
      self.baseViewController.presentedViewController;
  if (!presentedController) {
    [self loadGURLInNewTab:gurl];
    [self stop];
    return;
  }
  // Dismiss the active sheet first, then load URL. Loading first can alter the
  // presentation state and make the sheet dismissal unreliable.
  __weak __typeof(self) weakSelf = self;
  [presentedController
      dismissViewControllerAnimated:YES
                         completion:^{
                           __strong __typeof(weakSelf) strongSelf = weakSelf;
                           if (!strongSelf)
                             return;
                           [strongSelf loadGURLInNewTab:gurl];
                           [strongSelf stop];
                         }];
}

- (void)loadGURLInNewTab:(const GURL&)gurl {
  UrlLoadParams params = UrlLoadParams::InNewTab(gurl);
  params.in_incognito = _browser->GetProfile()->IsOffTheRecord();
  UrlLoadingBrowserAgent::FromBrowser(_browser)->Load(params);
}

#pragma mark - UIAdaptivePresentationControllerDelegate
- (void)presentationControllerDidDismiss:
    (UIPresentationController*)presentationController {
  [self stop];
}

@end
