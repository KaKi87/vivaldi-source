// Copyright 2024 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_NTP_VIVALDI_START_PAGE_COORDINATOR_H_
#define IOS_UI_NTP_VIVALDI_START_PAGE_COORDINATOR_H_

#import "ios/chrome/browser/shared/coordinator/chrome_coordinator/chrome_coordinator.h"
#import "ios/ui/ntp/legacy/legacy_speed_dial_base_controller.h"
#import "ios/ui/ntp/legacy/legacy_speed_dial_home_mediator.h"

class Browser;

// Coordinates the Vivaldi Start Page content hosted by Chromium's NTP.
@interface VivaldiStartPageCoordinator : ChromeCoordinator

// Designated initializer.
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser
    NS_DESIGNATED_INITIALIZER;

// Root controller for the current Start Page implementation.
@property(nonatomic, strong, readonly)
    VivaldiSpeedDialBaseController* viewController;

// Navigation controller for the base view controller.
@property(nonatomic, strong, readonly)
    UINavigationController* navigationController;

// Mediator used by the legacy Speed Dial implementation.
@property(nonatomic, strong, readonly) VivaldiSpeedDialHomeMediator* mediator;

@end

#endif  // IOS_UI_NTP_VIVALDI_START_PAGE_COORDINATOR_H_
