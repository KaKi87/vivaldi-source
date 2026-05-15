// Copyright 2024 Vivaldi Technologies. All rights reserved.

#import "ios/ui/ntp/vivaldi_start_page_coordinator.h"

#import "browser/features/vivaldi_features.h"
#import "components/bookmarks/browser/bookmark_model.h"
#import "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"

using bookmarks::BookmarkModel;

@interface VivaldiStartPageCoordinator () {
  // The browser where the settings are being displayed.
  Browser* _browser;
  // Bookmark model in which meditor and speed dial home controller depends
  BookmarkModel* _bookmarkModel;
  // Profile in which meditor depends
  ProfileIOS* _profile;
}

// View controller for the speed dial home/start page.
@property(nonatomic, strong) VivaldiSpeedDialBaseController* viewController;

// Navigation controller for the base view controller.
@property(nonatomic, strong) UINavigationController* navigationController;

// Mediator class for the speed dial home/start page.
@property(nonatomic, strong) VivaldiSpeedDialHomeMediator* mediator;

@end

@implementation VivaldiStartPageCoordinator

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser {
  self = [super initWithBaseViewController:viewController browser:browser];

  if (self) {
    _browser = browser;
    _profile = _browser->GetProfile()->GetOriginalProfile();
    _bookmarkModel = ios::BookmarkModelFactory::GetForProfile(_profile);
  }

  return self;
}

#pragma mark - ChromeCoordinator

- (void)start {
  if (vivaldi_features::IsVivaldiIOSShowRefactoredStartPageEnabled()) {
    // VIB-1121 - TODO: Implement @prio
  } else {
    VivaldiSpeedDialBaseController* controller =
        [[VivaldiSpeedDialBaseController alloc] initWithBrowser:_browser
                                                  bookmarkModel:_bookmarkModel];
    self.viewController = controller;

    UINavigationController* navigationController =
        [[UINavigationController alloc] initWithRootViewController:controller];
    self.navigationController = navigationController;
    [navigationController.view layoutIfNeeded];

    self.mediator =
        [[VivaldiSpeedDialHomeMediator alloc] initWithProfile:_profile
                                                bookmarkModel:_bookmarkModel];
    self.mediator.consumer = controller;
    [self.mediator startMediating];

    controller.delegate = self.mediator;
  }
}

- (void)stop {
  [super stop];
  [self.viewController invalidate];
  self.viewController.delegate = nil;
  self.viewController = nil;
  self.navigationController = nil;

  self.mediator.consumer = nil;
  [self.mediator disconnect];

  _browser = nullptr;
  _bookmarkModel = nullptr;
  _profile = nullptr;
}

@end
