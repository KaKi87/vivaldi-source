// Copyright 2025 Vivaldi Technologies. All rights reserved.

#import "ios/ui/search_engine/add_search_engine/vivaldi_add_search_engine_mediator.h"

#import <optional>

#include <memory>

#import "base/strings/string_util.h"
#import "base/strings/sys_string_conversions.h"
#import "components/search_engines/template_url.h"
#import "components/search_engines/template_url_data.h"
#import "components/search_engines/template_url_service.h"
#import "ios/chrome/browser/browser_container/ui_bundled/browser_edit_menu_utils.h"
#import "ios/chrome/browser/browser_container/ui_bundled/edit_menu_alert_delegate.h"
#import "ios/chrome/browser/search_engines/model/template_url_service_factory.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_swift.h"
#import "ios/search/vivaldi_add_search_engine_java_script_feature.h"
#import "ios/web/public/web_state.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"
#import "ui/base/l10n/l10n_util.h"
#import "base/strings/utf_string_conversions.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_coordinator.h"

namespace {
using MenuItemsBlock = void (^)(NSArray<UIMenuElement*>* items);

NSValue* KeyForWebState(web::WebState* web_state) {
  return [NSValue valueWithPointer:web_state];
}

class SearchableFormObserver
    : public vivaldi::VivaldiAddSearchEngineJavaScriptFeature::Observer {
 public:
  explicit SearchableFormObserver(VivaldiAddSearchEngineMediator* owner)
      : owner_(owner) {}
  SearchableFormObserver(const SearchableFormObserver&) = delete;
  SearchableFormObserver& operator=(const SearchableFormObserver&) = delete;

  void OnFocusedSearchableForm(
      web::WebState* web_state,
      std::optional<vivaldi::SearchableFormData> data) override;

 private:
  VivaldiAddSearchEngineMediator* owner_;
};

NSString* vMenuIdentifier = @"vivaldi.menu.addsearchengine";
}  // namespace

//VivaldiSearchEngineEditorCoordinatorDelegate
@interface VivaldiAddSearchEngineMediator ()
@property(nonatomic, assign) Browser* browser;
@property(nonatomic, weak) id<EditMenuAlertDelegate> alertDelegate;
@property(nonatomic, strong)
    NSMapTable<NSValue*, NSMutableArray*>* pendingRequests;
@property(nonatomic, strong)
    VivaldiSearchEngineEditorCoordinator* vivaldiSearchEditorCoordinator;

// The base view controller to present UI.
@property(nonatomic, weak) UIViewController* baseViewController;

- (void)handleSearchableForm:(std::optional<vivaldi::SearchableFormData>)data
                   webState:(web::WebState*)webState;
@end

@implementation VivaldiAddSearchEngineMediator {
  std::unique_ptr<SearchableFormObserver> observer_;
}

- (instancetype)initWithBaseViewController:(UIViewController*)baseViewController
                                   browser:(Browser*)browser
                             alertDelegate:(id<EditMenuAlertDelegate>)alertDelegate {
  if ((self = [super init])) {
    _baseViewController = baseViewController;
    _browser = browser;
    _alertDelegate = alertDelegate;
    _pendingRequests =
        [NSMapTable strongToStrongObjectsMapTable];
    observer_ = std::make_unique<SearchableFormObserver>(self);
    vivaldi::VivaldiAddSearchEngineJavaScriptFeature::GetInstance()
        ->AddObserver(observer_.get());
  }
  return self;

}

- (void)dealloc {
  vivaldi::VivaldiAddSearchEngineJavaScriptFeature::GetInstance()
      ->RemoveObserver(observer_.get());
  observer_.reset();
}

#pragma mark - EditMenuBuilder

- (void)buildEditMenuWithBuilder:(id<UIMenuBuilder>)builder
                      inWebState:(web::WebState*)webState {
  if (!webState || !self.browser) {
    return;
  }

  __weak __typeof(self) weakSelf = self;
  UIDeferredMenuElement* deferred =
      [UIDeferredMenuElement
          elementWithProvider:^(void (^completion)(NSArray<UIMenuElement*>*)) {
            [weakSelf enqueueCompletion:completion forWebState:webState];
            bool dispatched =
                vivaldi::VivaldiAddSearchEngineJavaScriptFeature::GetInstance()
                    ->RequestFocusedSearchFormData(webState);
            if (!dispatched) {
              [weakSelf resolveNextCompletionForWebState:webState withItems:@[]];
            }
          }];

  UIMenu* addSearchMenu =
      [UIMenu menuWithTitle:@""
                      image:nil
                 identifier:vMenuIdentifier
                    options:UIMenuOptionsDisplayInline
                   children:@[ deferred ]];
  [builder insertChildMenu:addSearchMenu
      atEndOfMenuForIdentifier:UIMenuRoot];
}

- (void)buildSelectionlessEditMenuWithBuilder:(id<UIMenuBuilder>)builder
                               inWebState:(web::WebState*)webState {
  if (!webState || !self.browser) {
    return;
  }

  __weak __typeof(self) weakSelf = self;
  UIDeferredMenuElement* deferred =
      [UIDeferredMenuElement
          elementWithProvider:^(void (^completion)(NSArray<UIMenuElement*>*)) {
            [weakSelf enqueueCompletion:completion forWebState:webState];
            bool dispatched =
                vivaldi::VivaldiAddSearchEngineJavaScriptFeature::GetInstance()
                    ->RequestFocusedSearchFormData(webState);
            if (!dispatched) {
              [weakSelf resolveNextCompletionForWebState:webState withItems:@[]];
            }
          }];

  UIMenu* addSearchMenu =
      [UIMenu menuWithTitle:@""
                      image:nil
                 identifier:vMenuIdentifier
                    options:UIMenuOptionsDisplayInline
                   children:@[ deferred ]];
  [builder insertChildMenu:addSearchMenu
      atEndOfMenuForIdentifier:UIMenuRoot];
}

#pragma mark - Observer callback

- (void)handleSearchableForm:(std::optional<vivaldi::SearchableFormData>)data
                   webState:(web::WebState*)webState {
  MenuItemsBlock block = [self dequeueCompletionForWebState:webState];
  if (!block) {
    return;
  }
  if (!data) {
    block(@[]);
    return;
  }

  NSString* templateURL = base::SysUTF8ToNSString(data->query_template);
  NSString* actionURL = base::SysUTF8ToNSString(data->action_url);

  __weak __typeof(self) weakSelf = self;
  UIAction* action = [UIAction
      actionWithTitle:l10n_util::GetNSString(
                          IDS_VIVALDI_CONTEXT_MENU_ADD_AS_ENGINE_TITLE)
                image:nil
           identifier:vMenuIdentifier
              handler:^(__unused UIAction* a) {
                if (webState) {
                  [webState->GetView() endEditing:NO];
                }

                [weakSelf presentEditorWithTemplateURL:templateURL
                                             actionURL:actionURL];
              }];
  block(@[ action ]);
}

#pragma mark - Pending completions

- (void)enqueueCompletion:(MenuItemsBlock)completion
               forWebState:(web::WebState*)webState {
  NSValue* key = KeyForWebState(webState);
  NSMutableArray* queue = [self.pendingRequests objectForKey:key];
  if (!queue) {
    queue = [[NSMutableArray alloc] init];
    [self.pendingRequests setObject:queue forKey:key];
  }
  [queue addObject:[completion copy]];
}

- (MenuItemsBlock)dequeueCompletionForWebState:(web::WebState*)webState {
  NSValue* key = KeyForWebState(webState);
  NSMutableArray* queue = [self.pendingRequests objectForKey:key];
  if (!queue || !queue.count) {
    return nil;
  }
  MenuItemsBlock block = [queue firstObject];
  [queue removeObjectAtIndex:0];
  if (!queue.count) {
    [self.pendingRequests removeObjectForKey:key];
  }
  return block;
}

- (void)resolveNextCompletionForWebState:(web::WebState*)webState
                                withItems:(NSArray<UIMenuElement*>*)items {
  MenuItemsBlock block = [self dequeueCompletionForWebState:webState];
  if (block) {
    block(items);
  }
}

#pragma mark - Context menu flow helpers

- (void)presentEditorWithTemplateURL:(NSString*)templateURL
                           actionURL:(NSString*)actionURL {
  if (!self.browser) {
    return;
  }
  ProfileIOS* profile = self.browser->GetProfile();
  if (!profile || profile->IsOffTheRecord()) {
    return;
  }
  TemplateURLService* service =
      ios::TemplateURLServiceFactory::GetForProfile(profile);
  if (!service) {
    return;
  }
  service->Load();

  const std::string template_spec =
      base::SysNSStringToUTF8(templateURL);
  const GURL template_gurl(template_spec);
  if (!template_gurl.is_valid()) {
    return;
  }

  for (TemplateURL* existing : service->GetTemplateURLs()) {
    if (existing->url() == template_spec) {
      // TODO(tommi/prio): Show indicator in UI that it already exists
      // or just open up the full fledged settings editor

      // Template already exists.
      return;
    }
  }

  std::string display_name = std::string(template_gurl.host());
  if (base::StartsWith(display_name, "www.",
                       base::CompareCase::INSENSITIVE_ASCII)) {
    display_name = display_name.substr(4);
  }
  if (display_name.empty()) {
    display_name = template_gurl.spec();
  }
  base::TrimWhitespaceASCII(display_name, base::TRIM_ALL, &display_name);
  if (display_name.empty()) {
    display_name = "Custom search"; // todo tommi, translatable string
  }

  VivaldiSearchEngineEditorItem* item =
      [[VivaldiSearchEngineEditorItem alloc] init];
  item.name = base::SysUTF8ToNSString(display_name);
  item.nickname = @"";
  item.url = templateURL;
  [self presentAddSearchEngineEditorWithItem:item];
}

- (void)presentAddSearchEngineEditorWithItem:(VivaldiSearchEngineEditorItem*)item {
  self.vivaldiSearchEditorCoordinator =
      [[VivaldiSearchEngineEditorCoordinator alloc]
          initWithBaseViewController:self.baseViewController
                             browser:self.browser
                          entryPoint:VivaldiSearchEngineEditorEntryPointContextMenu
                         entryReason:VivaldiSearchEngineEditorEntryReasonAdd
                                item:item
                         allowsCancel:YES];
  [self.vivaldiSearchEditorCoordinator start];
}

@end

namespace {

void SearchableFormObserver::OnFocusedSearchableForm(
    web::WebState* web_state,
    std::optional<vivaldi::SearchableFormData> data) {
  [owner_ handleSearchableForm:data webState:web_state];
}

}  // namespace
