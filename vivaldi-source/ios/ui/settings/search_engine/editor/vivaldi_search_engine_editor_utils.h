// Copyright 2025 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_UTILS_H_
#define IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_UTILS_H_

#import <Foundation/Foundation.h>

@class VivaldiSearchEngineEditorItem;
class TemplateURL;

// Creates an editor item from the provided TemplateURL. Returns nil if the
// TemplateURL is invalid.
VivaldiSearchEngineEditorItem* VivaldiCreateSearchEngineEditorItemFromTemplateURL(
    const TemplateURL* templateURL);

#endif  // IOS_UI_SETTINGS_SEARCH_ENGINE_EDITOR_VIVALDI_SEARCH_ENGINE_EDITOR_UTILS_H_
