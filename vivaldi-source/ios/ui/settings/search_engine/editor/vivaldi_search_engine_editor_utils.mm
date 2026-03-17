// Copyright 2025 Vivaldi Technologies. All rights reserved.

#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_utils.h"

#import "base/strings/sys_string_conversions.h"
#import "components/search_engines/search_engines_helper.h"
#import "components/search_engines/template_url.h"
#import "components/search_engines/util.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_swift.h"

VivaldiSearchEngineEditorItem*
VivaldiCreateSearchEngineEditorItemFromTemplateURL(
    const TemplateURL* templateURL) {
  if (!templateURL) {
    return nil;
  }

  return [[VivaldiSearchEngineEditorItem alloc]
                   initWithGuid:base::SysUTF8ToNSString(
                                    templateURL->sync_guid())
                           name:base::SysUTF16ToNSString(
                                    templateURL->short_name())
                       nickname:base::SysUTF16ToNSString(templateURL->keyword())
                            url:base::SysUTF8ToNSString(
                                    GetUrlToDisplay(templateURL->url()))
                     suggestURL:base::SysUTF8ToNSString(GetUrlToDisplay(
                                    templateURL->suggestions_url()))
                 postParameters:base::SysUTF8ToNSString(GetUrlToDisplay(
                                    templateURL->suggestions_url_post_params()))
                 imageSearchURL:base::SysUTF8ToNSString(
                                    GetUrlToDisplay(templateURL->image_url()))
      imageSearchPostParameters:base::SysUTF8ToNSString(GetUrlToDisplay(
                                    templateURL->image_url_post_params()))];
}
