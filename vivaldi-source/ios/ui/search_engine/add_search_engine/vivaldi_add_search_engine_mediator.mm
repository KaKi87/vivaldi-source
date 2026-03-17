// Copyright 2025 Vivaldi Technologies. All rights reserved.

#import "ios/ui/search_engine/add_search_engine/vivaldi_add_search_engine_mediator.h"

#import <optional>

#include <memory>

#import "base/strings/string_util.h"
#import "base/strings/sys_string_conversions.h"
#import "base/strings/utf_string_conversions.h"
#import "components/search_engines/template_url.h"
#import "components/search_engines/template_url_data.h"
#import "components/search_engines/template_url_service.h"
#import "ios/chrome/browser/browser_content/ui_bundled/browser_edit_menu_utils.h"
#import "ios/chrome/browser/browser_content/ui_bundled/edit_menu_alert_delegate.h"
#import "ios/chrome/browser/search_engines/model/template_url_service_factory.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/search/vivaldi_add_search_engine_java_script_feature.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_coordinator.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_swift.h"
#import "ios/web/public/web_state.h"
#import "ui/base/l10n/l10n_util.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

namespace {
using MenuItemsBlock = void (^)(NSArray<UIMenuElement*>* items);
constexpr NSTimeInterval kSearchableFormRequestTimeoutSeconds = 1.5;

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

  void OnFocusedSearchableForm(web::WebState* web_state,
                               std::optional<vivaldi::SearchableFormData> data,
                               std::optional<int64_t> request_id) override;

 private:
  VivaldiAddSearchEngineMediator* owner_;
};

NSString* vMenuIdentifier = @"vivaldi.menu.addsearchengine";
}  // namespace

@interface VivaldiPendingFocusedSearchableFormRequest : NSObject
@property(nonatomic, copy) MenuItemsBlock completion;
@property(nonatomic, assign) int64_t requestID;
@end

@implementation VivaldiPendingFocusedSearchableFormRequest
@end

// VivaldiSearchEngineEditorCoordinatorDelegate
@interface VivaldiAddSearchEngineMediator ()
@property(nonatomic, assign) Browser* browser;
@property(nonatomic, weak) id<EditMenuAlertDelegate> alertDelegate;
@property(nonatomic, strong)
    NSMapTable<NSValue*,
               NSMutableArray<VivaldiPendingFocusedSearchableFormRequest*>*>*
        pendingRequests;
@property(nonatomic, strong)
    VivaldiSearchEngineEditorCoordinator* vivaldiSearchEditorCoordinator;

// The base view controller to present UI.
@property(nonatomic, weak) UIViewController* baseViewController;

- (void)handleSearchableForm:(std::optional<vivaldi::SearchableFormData>)data
                    webState:(web::WebState*)webState
                   requestID:(std::optional<int64_t>)requestID;
- (void)buildDeferredMenuWithBuilder:(id<UIMenuBuilder>)builder
                          inWebState:(web::WebState*)webState;
- (int64_t)enqueueCompletion:(MenuItemsBlock)completion
                 forWebState:(web::WebState*)webState;
- (VivaldiPendingFocusedSearchableFormRequest*)dequeueCompletionForWebState:
    (web::WebState*)webState;
- (VivaldiPendingFocusedSearchableFormRequest*)
    dequeueCompletionForWebState:(web::WebState*)webState
                       requestID:(int64_t)requestID;
- (void)resolveCompletionForWebState:(web::WebState*)webState
                           requestID:(int64_t)requestID
                           withItems:(NSArray<UIMenuElement*>*)items;
- (void)scheduleTimeoutForRequestID:(int64_t)requestID
                           webState:(web::WebState*)webState;
@end

@implementation VivaldiAddSearchEngineMediator {
  std::unique_ptr<SearchableFormObserver> observer_;
  int64_t nextRequestID_;
}

- (instancetype)initWithBaseViewController:(UIViewController*)baseViewController
                                   browser:(Browser*)browser
                             alertDelegate:
                                 (id<EditMenuAlertDelegate>)alertDelegate {
  if ((self = [super init])) {
    _baseViewController = baseViewController;
    _browser = browser;
    _alertDelegate = alertDelegate;
    _pendingRequests = [NSMapTable strongToStrongObjectsMapTable];
    nextRequestID_ = 1;
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
  [self buildDeferredMenuWithBuilder:builder inWebState:webState];
}

- (void)buildSelectionlessEditMenuWithBuilder:(id<UIMenuBuilder>)builder
                                   inWebState:(web::WebState*)webState {
  [self buildDeferredMenuWithBuilder:builder inWebState:webState];
}

- (void)buildDeferredMenuWithBuilder:(id<UIMenuBuilder>)builder
                          inWebState:(web::WebState*)webState {
  if (!webState || !self.browser) {
    return;
  }

  __weak __typeof(self) weakSelf = self;
  UIDeferredMenuElement* deferred = [UIDeferredMenuElement
      elementWithProvider:^(void (^completion)(NSArray<UIMenuElement*>*)) {
        int64_t requestID = [weakSelf enqueueCompletion:completion
                                            forWebState:webState];
        bool dispatched =
            vivaldi::VivaldiAddSearchEngineJavaScriptFeature::GetInstance()
                ->RequestFocusedSearchFormData(webState, requestID);
        if (!dispatched) {
          [weakSelf resolveCompletionForWebState:webState
                                       requestID:requestID
                                       withItems:@[]];
          return;
        }
        [weakSelf scheduleTimeoutForRequestID:requestID webState:webState];
      }];

  UIMenu* addSearchMenu = [UIMenu menuWithTitle:@""
                                          image:nil
                                     identifier:vMenuIdentifier
                                        options:UIMenuOptionsDisplayInline
                                       children:@[ deferred ]];
  [builder insertChildMenu:addSearchMenu atEndOfMenuForIdentifier:UIMenuRoot];
}

#pragma mark - Observer callback

- (void)handleSearchableForm:(std::optional<vivaldi::SearchableFormData>)data
                    webState:(web::WebState*)webState
                   requestID:(std::optional<int64_t>)requestID {
  VivaldiPendingFocusedSearchableFormRequest* pendingRequest = nil;
  if (requestID) {
    pendingRequest = [self dequeueCompletionForWebState:webState
                                              requestID:*requestID];
  } else {
    pendingRequest = [self dequeueCompletionForWebState:webState];
  }

  if (!pendingRequest) {
    return;
  }

  MenuItemsBlock block = pendingRequest.completion;
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

- (int64_t)enqueueCompletion:(MenuItemsBlock)completion
                 forWebState:(web::WebState*)webState {
  const int64_t requestID = nextRequestID_++;
  NSValue* key = KeyForWebState(webState);
  NSMutableArray<VivaldiPendingFocusedSearchableFormRequest*>* queue =
      [self.pendingRequests objectForKey:key];
  if (!queue) {
    queue = [[NSMutableArray alloc] init];
    [self.pendingRequests setObject:queue forKey:key];
  }
  VivaldiPendingFocusedSearchableFormRequest* request =
      [[VivaldiPendingFocusedSearchableFormRequest alloc] init];
  request.requestID = requestID;
  request.completion = [completion copy];
  [queue addObject:request];
  return requestID;
}

- (VivaldiPendingFocusedSearchableFormRequest*)dequeueCompletionForWebState:
    (web::WebState*)webState {
  NSValue* key = KeyForWebState(webState);
  NSMutableArray<VivaldiPendingFocusedSearchableFormRequest*>* queue =
      [self.pendingRequests objectForKey:key];
  if (!queue || !queue.count) {
    return nil;
  }
  VivaldiPendingFocusedSearchableFormRequest* request = [queue firstObject];
  [queue removeObjectAtIndex:0];
  if (!queue.count) {
    [self.pendingRequests removeObjectForKey:key];
  }
  return request;
}

- (VivaldiPendingFocusedSearchableFormRequest*)
    dequeueCompletionForWebState:(web::WebState*)webState
                       requestID:(int64_t)requestID {
  NSValue* key = KeyForWebState(webState);
  NSMutableArray<VivaldiPendingFocusedSearchableFormRequest*>* queue =
      [self.pendingRequests objectForKey:key];
  if (!queue || !queue.count) {
    return nil;
  }

  NSUInteger index = [queue indexOfObjectPassingTest:^BOOL(
                                VivaldiPendingFocusedSearchableFormRequest* obj,
                                NSUInteger idx, BOOL* stop) {
    return obj.requestID == requestID;
  }];
  if (index == NSNotFound) {
    return nil;
  }

  VivaldiPendingFocusedSearchableFormRequest* request =
      [queue objectAtIndex:index];
  [queue removeObjectAtIndex:index];
  if (!queue.count) {
    [self.pendingRequests removeObjectForKey:key];
  }
  return request;
}

- (void)resolveCompletionForWebState:(web::WebState*)webState
                           requestID:(int64_t)requestID
                           withItems:(NSArray<UIMenuElement*>*)items {
  VivaldiPendingFocusedSearchableFormRequest* request =
      [self dequeueCompletionForWebState:webState requestID:requestID];
  if (request && request.completion) {
    request.completion(items);
  }
}

- (void)scheduleTimeoutForRequestID:(int64_t)requestID
                           webState:(web::WebState*)webState {
  __weak __typeof(self) weakSelf = self;
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                               (int64_t)(kSearchableFormRequestTimeoutSeconds *
                                         NSEC_PER_SEC)),
                 dispatch_get_main_queue(), ^{
                   VivaldiPendingFocusedSearchableFormRequest* request =
                       [weakSelf dequeueCompletionForWebState:webState
                                                    requestID:requestID];
                   if (!request || !request.completion) {
                     return;
                   }
                   request.completion(@[]);
                 });
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

  const std::string template_spec = base::SysNSStringToUTF8(templateURL);
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
    display_name = "Custom search";  // todo tommi, translatable string
  }

  VivaldiSearchEngineEditorItem* item =
      [[VivaldiSearchEngineEditorItem alloc] init];
  item.name = base::SysUTF8ToNSString(display_name);
  item.nickname = @"";
  item.url = templateURL;
  [self presentAddSearchEngineEditorWithItem:item];
}

- (void)presentAddSearchEngineEditorWithItem:
    (VivaldiSearchEngineEditorItem*)item {
  self.vivaldiSearchEditorCoordinator = [[VivaldiSearchEngineEditorCoordinator
      alloc]
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
    std::optional<vivaldi::SearchableFormData> data,
    std::optional<int64_t> request_id) {
  [owner_ handleSearchableForm:data webState:web_state requestID:request_id];
}

}  // namespace
