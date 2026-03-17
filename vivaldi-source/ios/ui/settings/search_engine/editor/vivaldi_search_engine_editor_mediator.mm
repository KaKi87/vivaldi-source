// Copyright 2025 Vivaldi Technologies. All rights reserved.

#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_mediator.h"

#import "base/strings/sys_string_conversions.h"
#import "components/prefs/pref_service.h"
#import "components/search_engines/search_engines_helper.h"
#import "components/search_engines/template_url.h"
#import "components/search_engines/template_url_service.h"
#import "components/search_engines/template_url_service_observer.h"
#import "components/search_engines/util.h"
#import "ios/chrome/browser/search_engines/model/search_engine_observer_bridge.h"
#import "ios/chrome/browser/search_engines/model/template_url_service_factory.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_swift.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"

@interface VivaldiSearchEngineEditorMediator () <SearchEngineObserving> {
  // Profile and service dependencies
  ProfileIOS* _profile;
  TemplateURLService* _templateURLService;

  // Internal model representation for editing
  VivaldiSearchEngineEditorItem* _editingItemInternal;

  // Observer bridge for template URL service changes
  std::unique_ptr<SearchEngineObserverBridge> _observer;

  // Array of existing search engine nicknames for validation
  NSMutableArray<VivaldiSearchEngineNicknameModel*>* _nicknames;

  VivaldiSearchEngineEditorEntryReason _entryReason;

  VivaldiSearchEngineEditorEntryPoint _entryPoint;
  BOOL _shouldSetAsDefault;
  VivaldiSearchEngineEditorItem* _prefilledItem;
}

@end

@implementation VivaldiSearchEngineEditorMediator

#pragma mark - Initialization

- (instancetype)initWithProfile:(ProfileIOS*)profile
                    entryReason:
                        (VivaldiSearchEngineEditorEntryReason)entryReason
                     entryPoint:(VivaldiSearchEngineEditorEntryPoint)entryPoint
                           item:(VivaldiSearchEngineEditorItem*)item {
  self = [super init];
  if (self) {
    _profile = profile;
    _entryReason = entryReason;
    _entryPoint = entryPoint;
    _prefilledItem = item;
    _editingItemInternal = item ?: [[VivaldiSearchEngineEditorItem alloc] init];
    _shouldSetAsDefault = NO;

    [self setupTemplateURLService];
  }
  return self;
}

#pragma mark - Public Methods

- (void)saveChanges {
  if (!_editingItemInternal.isFormValid) {
    return;
  }

  [self.consumer searchEngineBackendWillChange];

  TemplateURL* newly_added_url = nullptr;
  if (_entryReason == VivaldiSearchEngineEditorEntryReasonEdit) {
    [self updateExistingSearchEngine];
  } else {
    newly_added_url = [self createNewSearchEngine];

    if (_entryPoint == VivaldiSearchEngineEditorEntryPointContextMenu &&
        _shouldSetAsDefault && newly_added_url &&
        _templateURLService->CanMakeDefault(newly_added_url)) {
      _templateURLService->SetUserSelectedDefaultSearchProvider(
          newly_added_url);
    }
  }

  [self.consumer searchEngineBackendDidChange];
}

- (void)disconnect {
  [self cleanup];
}

#pragma mark - Private Setup Methods

- (void)setupTemplateURLService {
  _templateURLService = ios::TemplateURLServiceFactory::GetForProfile(_profile);
  _observer =
      std::make_unique<SearchEngineObserverBridge>(self, _templateURLService);
  _templateURLService->Load();

  if (_templateURLService->loaded()) {
    [self loadSearchEngines];
  }
}

#pragma mark - Private Save Methods

- (void)updateExistingSearchEngine {
  if (!_templateURLService)
    return;

  TemplateURL* itemToUpdate = [self templateURLForCurrentEditorItem];

  if (!itemToUpdate) {
    return;
  }

  _templateURLService->ResetTemplateURL(
      itemToUpdate, base::SysNSStringToUTF16(_editingItemInternal.name),
      base::SysNSStringToUTF16(_editingItemInternal.nickname),
      GetUrlFromDisplay(base::SysNSStringToUTF8(_editingItemInternal.url)),
      GetUrlFromDisplay(
          base::SysNSStringToUTF8(_editingItemInternal.postParameters)),
      GetUrlFromDisplay(
          base::SysNSStringToUTF8(_editingItemInternal.suggestURL)),
      itemToUpdate->suggestions_url_post_params(),
      GetUrlFromDisplay(
          base::SysNSStringToUTF8(_editingItemInternal.imageSearchURL)),
      GetUrlFromDisplay(base::SysNSStringToUTF8(
          _editingItemInternal.imageSearchPostParameters)),
      TemplateURL::GenerateFaviconURL(
          GURL(base::SysNSStringToUTF8(_editingItemInternal.url))));
}

- (TemplateURL*)createNewSearchEngine {
  if (!_templateURLService)
    return nullptr;

  TemplateURLData data;
  data.SetShortName(base::SysNSStringToUTF16(_editingItemInternal.name));
  data.SetKeyword(base::SysNSStringToUTF16(_editingItemInternal.nickname));
  data.SetURL(
      GetUrlFromDisplay(base::SysNSStringToUTF8(_editingItemInternal.url)));
  data.suggestions_url = GetUrlFromDisplay(
      base::SysNSStringToUTF8(_editingItemInternal.suggestURL));
  data.image_url = GetUrlFromDisplay(
      base::SysNSStringToUTF8(_editingItemInternal.imageSearchURL));
  data.search_url_post_params = GetUrlFromDisplay(
      base::SysNSStringToUTF8(_editingItemInternal.postParameters));
  data.image_url_post_params = GetUrlFromDisplay(
      base::SysNSStringToUTF8(_editingItemInternal.imageSearchPostParameters));
  data.favicon_url = TemplateURL::GenerateFaviconURL(
      GURL(base::SysNSStringToUTF8(_editingItemInternal.url)));
  data.safe_for_autoreplace = false;

  TemplateURL* added =
      _templateURLService->Add(std::make_unique<TemplateURL>(data));
  return added;
}

#pragma mark - Private Helper Methods

- (void)loadSearchEngines {
  std::vector<raw_ptr<TemplateURL, VectorExperimental>> urls =
      _templateURLService->GetTemplateURLs();

  _nicknames = [[NSMutableArray alloc] init];

  for (TemplateURL* url : urls) {
    if (url->is_active() != TemplateURLData::ActiveStatus::kFalse) {
      VivaldiSearchEngineNicknameModel* nicknameObject =
          [self createNicknameModelFromTemplateURL:url];
      [_nicknames addObject:nicknameObject];
    }
  }

  [self.consumer availableNicknamesDidUpdate:[_nicknames copy]];
}

- (VivaldiSearchEngineNicknameModel*)createNicknameModelFromTemplateURL:
    (TemplateURL*)url {
  NSString* guid = base::SysUTF8ToNSString(url->sync_guid());
  NSString* name = base::SysUTF16ToNSString(url->short_name());
  NSString* nickname = base::SysUTF16ToNSString(url->keyword());

  return [[VivaldiSearchEngineNicknameModel alloc] initWithGuid:guid
                                                           name:name
                                                       nickname:nickname];
}

- (void)cleanup {
  _observer.reset();
  _profile = nullptr;
  _templateURLService = nullptr;
  _editingItemInternal = nil;
  _consumer = nil;
  _nicknames = nil;
}

- (TemplateURL*)templateURLForCurrentEditorItem {
  if (!_templateURLService)
    return nullptr;
  NSString* guid = _prefilledItem.guid ?: _editingItemInternal.guid;
  if (!guid.length) {
    return nullptr;
  }
  std::string guid_utf8 = base::SysNSStringToUTF8(guid);
  if (guid_utf8.empty()) {
    return nullptr;
  }
  return _templateURLService->GetTemplateURLForGUID(guid_utf8);
}

#pragma mark - Property Accessors

- (void)setConsumer:(id<VivaldiSearchEngineEditorConsumer>)consumer {
  _consumer = consumer;

  // Provide initial data to consumer
  [consumer availableNicknamesDidUpdate:[_nicknames copy]];

  if (_prefilledItem) {
    _editingItemInternal = _prefilledItem;
    [consumer searchEngineEditorItemDidChange:_prefilledItem];
  } else if (_editingItemInternal) {
    [consumer searchEngineEditorItemDidChange:_editingItemInternal];
  }
}

#pragma mark - VivaldiSearchEngineEditorConsumer

- (void)searchEngineEditorItemDidChange:(VivaldiSearchEngineEditorItem*)item {
  _editingItemInternal = item;
}

- (void)searchEngineEditorDefaultToggleDidChange:(BOOL)isOn {
  _shouldSetAsDefault = isOn;
}

#pragma mark - SearchEngineObserving

- (void)searchEngineChanged {
  [self loadSearchEngines];
}

@end
