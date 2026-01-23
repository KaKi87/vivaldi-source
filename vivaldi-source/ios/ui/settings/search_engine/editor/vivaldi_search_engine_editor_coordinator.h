// Copyright 2025 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_COORDINATOR_H_
#define IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_COORDINATOR_H_

#import "ios/chrome/browser/shared/coordinator/chrome_coordinator/chrome_coordinator.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_entry_point.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_entry_reason.h"

class Browser;
@class VivaldiSearchEngineEditorCoordinator;
@class VivaldiSearchEngineEditorItem;

@protocol VivaldiSearchEngineEditorCoordinatorDelegate
- (void)searchEngineEditorShouldDismiss:
    (VivaldiSearchEngineEditorCoordinator*)coordinator;
@end

@interface VivaldiSearchEngineEditorCoordinator : ChromeCoordinator

// Designated initializer.
- (instancetype)
    initWithBaseViewController:(UIViewController*)viewController
                       browser:(Browser*)browser
                    entryPoint:(VivaldiSearchEngineEditorEntryPoint)entryPoint
                   entryReason:(VivaldiSearchEngineEditorEntryReason)entryReason
                          item:(VivaldiSearchEngineEditorItem*)item
                  allowsCancel:(BOOL)allowsCancel NS_DESIGNATED_INITIALIZER;

- (instancetype)
    initWithBaseNavigationController:
        (UINavigationController*)navigationController
                             browser:(Browser*)browser
                          entryPoint:
                              (VivaldiSearchEngineEditorEntryPoint)entryPoint
                         entryReason:
                             (VivaldiSearchEngineEditorEntryReason)entryReason
                                item:(VivaldiSearchEngineEditorItem*)item
                        allowsCancel:(BOOL)allowsCancel;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser NS_UNAVAILABLE;

@property(nonatomic, weak) id<VivaldiSearchEngineEditorCoordinatorDelegate>
    delegate;

@end

#endif  // IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_COORDINATOR_H_
