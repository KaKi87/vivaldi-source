// Copyright 2024 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SETTINGS_SEARCH_ENGINE_VIVALDI_SEARCH_ENGINE_SETTINGS_COORDINATOR_H_
#define IOS_UI_SETTINGS_SEARCH_ENGINE_VIVALDI_SEARCH_ENGINE_SETTINGS_COORDINATOR_H_

#import "ios/chrome/browser/shared/coordinator/chrome_coordinator/chrome_coordinator.h"

class Browser;
@class VivaldiSearchEngineSettingsCoordinator;

@protocol VivaldiSearchEngineSettingsCoordinatorDelegate <NSObject>

// Called when the coordinator's root view controller is removed.
- (void)vivaldiSearchEngineSettingsCoordinatorWasRemoved:
    (VivaldiSearchEngineSettingsCoordinator*)coordinator;

@end

// This class is the coordinator for the search engine base settings.
@interface VivaldiSearchEngineSettingsCoordinator : ChromeCoordinator

// Designated initializer.
- (instancetype)initWithBaseNavigationController:
                    (UINavigationController*)navigationController
                                         browser:(Browser*)browser
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser NS_UNAVAILABLE;

@property(nonatomic, weak) id<VivaldiSearchEngineSettingsCoordinatorDelegate>
    delegate;

@end

#endif  // IOS_UI_SETTINGS_SEARCH_ENGINE_VIVALDI_SEARCH_ENGINE_SETTINGS_COORDINATOR_H_
