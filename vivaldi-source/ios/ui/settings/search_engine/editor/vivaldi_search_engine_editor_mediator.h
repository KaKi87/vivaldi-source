// Copyright 2025 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_MEDIATOR_H_
#define IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_MEDIATOR_H_

#import <Foundation/Foundation.h>

#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_consumer.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_entry_point.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_entry_reason.h"

class ProfileIOS;

@interface VivaldiSearchEngineEditorMediator
    : NSObject <VivaldiSearchEngineEditorConsumer>

- (instancetype)initWithProfile:(ProfileIOS*)profile
                    entryReason:
                        (VivaldiSearchEngineEditorEntryReason)entryReason
                     entryPoint:(VivaldiSearchEngineEditorEntryPoint)entryPoint
                           item:(VivaldiSearchEngineEditorItem*)item
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// The consumer of the search engine settings mediator.
@property(nonatomic, weak) id<VivaldiSearchEngineEditorConsumer> consumer;

- (void)saveChanges;
- (void)disconnect;

@end

#endif  // IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_MEDIATOR_H_
