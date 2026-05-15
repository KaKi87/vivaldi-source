// Copyright 2024 Vivaldi Technologies. All rights reserved.

#import "ios/ui/settings/start_page/layout_settings/vivaldi_start_page_layout_settings_coordinator.h"

#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/ui/helpers/helpers_swift.h"
#import "ios/ui/settings/start_page/layout_settings/layout_settings_swift.h"
#import "ios/ui/settings/start_page/layout_settings/vivaldi_start_page_layout_settings_mediator.h"
#import "ios/ui/settings/vivaldi_settings_navigation_helper.h"
#import "ui/base/l10n/l10n_util_mac.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

@interface VivaldiStartPageLayoutSettingsCoordinator () <
    VivaldiHostingControllerPresentationDelegate> {
  // The browser where the settings are being displayed.
  Browser* _browser;
}
// View provider for the start page quick setting.
@property(nonatomic, strong)
    VivaldiStartPageLayoutSettingsViewProvider* viewProvider;
@property(nonatomic, strong) UIViewController* viewController;
// Start page quick settings preference mediator.
@property(nonatomic, strong) VivaldiStartPageLayoutSettingsMediator* mediator;

@end

@implementation VivaldiStartPageLayoutSettingsCoordinator

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
  VivaldiStartPageLayoutSettingsViewProvider* viewProvider =
      [VivaldiStartPageLayoutSettingsViewProvider new];
  self.viewProvider = viewProvider;
  self.viewController =
      [self.viewProvider makeViewControllerWithPresentationDelegate:self];
  self.viewController.title =
      l10n_util::GetNSString(IDS_IOS_VIVALDI_START_PAGE_LAYOUT_TITLE);
  self.viewController.navigationItem.largeTitleDisplayMode =
      UINavigationItemLargeTitleDisplayModeNever;

  // Add Done button
  UIBarButtonItem* doneItem = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                           target:self
                           action:@selector(handleDoneButtonTap)];
  self.viewController.navigationItem.rightBarButtonItem = doneItem;

  self.mediator = [[VivaldiStartPageLayoutSettingsMediator alloc]
      initWithOriginalPrefService:self.browser->GetProfile()
                                      ->GetOriginalProfile()
                                      ->GetPrefs()];
  self.mediator.consumer = self.viewProvider;
  self.viewProvider.settingsStateConsumer = self.mediator;

  [self.baseNavigationController pushViewController:self.viewController
                                           animated:YES];
}

- (void)stop {
  [super stop];
  self.viewController = nil;
  self.viewProvider.settingsStateConsumer = nil;
  self.viewProvider = nil;
  self.mediator.consumer = nil;
  [self.mediator disconnect];
  self.mediator = nil;
}

#pragma mark - Action

- (void)handleDoneButtonTap {
  if (VivaldiCloseSettingsIfPossible(self.baseNavigationController)) {
    return;
  }
  [self stop];
  [self.baseNavigationController dismissViewControllerAnimated:YES
                                                    completion:nil];
}

#pragma mark - VivaldiHostingControllerPresentationDelegate

- (void)hostingController:(UIViewController*)hostingController
                didMoveTo:(UIViewController* _Nullable)parent {
  DCHECK_EQ(self.viewController, hostingController);
  [self stop];
}

@end
