// Copyright 2025 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SEARCH_ENGINE_ADD_SEARCH_ENGINE_VIVALDI_ADD_SEARCH_ENGINE_MEDIATOR_H_
#define IOS_UI_SEARCH_ENGINE_ADD_SEARCH_ENGINE_VIVALDI_ADD_SEARCH_ENGINE_MEDIATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/browser_container/model/edit_menu_builder.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"

@protocol EditMenuAlertDelegate;

// Mediator responsible for adding the "Add as search engine" context menu
// entry on text inputs.
@interface VivaldiAddSearchEngineMediator : NSObject <EditMenuBuilder>

// Initializer for a mediator.
- (instancetype)initWithBaseViewController:(UIViewController*)baseViewController
                                   browser:(Browser*)browser
                             alertDelegate:(id<EditMenuAlertDelegate>)alertDelegate
    NS_DESIGNATED_INITIALIZER;


- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_UI_SEARCH_ENGINE_ADD_SEARCH_ENGINE_VIVALDI_ADD_SEARCH_ENGINE_MEDIATOR_H_
