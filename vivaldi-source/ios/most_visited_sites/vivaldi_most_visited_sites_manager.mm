// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#import "ios/most_visited_sites/vivaldi_most_visited_sites_manager.h"

#import "base/strings/sys_string_conversions.h"
#import "components/ntp_tiles/most_visited_sites.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/browser/content_suggestions/most_visited_tiles/ui/most_visited_tiles_config.h"
#import "ios/chrome/browser/content_suggestions/ui/cells/content_suggestions_tile_saver.h"
#import "ios/chrome/browser/content_suggestions/ui/content_suggestions_image_data_source.h"
#import "ios/chrome/browser/favicon/model/ios_chrome_large_icon_cache_factory.h"
#import "ios/chrome/browser/favicon/model/ios_chrome_large_icon_service_factory.h"
#import "ios/chrome/browser/favicon/ui_bundled/favicon_attributes_provider.h"
#import "ios/chrome/browser/ntp_tiles/model/ios_most_visited_sites_factory.h"
#import "ios/chrome/browser/ntp_tiles/model/most_visited_sites_observer_bridge.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/signin/model/chrome_account_manager_service.h"
#import "ios/chrome/browser/signin/model/chrome_account_manager_service_factory.h"
#import "ios/chrome/common/app_group/app_group_constants.h"

namespace {
// Maximum number of most visited tiles fetched.
// Match the number chromium backend used.
const NSInteger kMaxNumMostVisitedTiles = 10;
// Size of the favicon returned by the provider for the most visited items.
const CGFloat kMostVisitedFaviconSize = 48;
// Size below which the provider returns a colored tile instead of an image.
const CGFloat kMostVisitedFaviconMinimalSize = 32;

}  // namespace

@interface VivaldiMostVisitedSitesManager () <
    MostVisitedSitesObserving,
    ContentSuggestionsImageDataSource> {
  std::unique_ptr<ntp_tiles::MostVisitedSites> _mostVisitedSites;
  std::unique_ptr<ntp_tiles::MostVisitedSitesObserverBridge> _mostVisitedBridge;
}
// The user's browser state model used.
@property(nonatomic, assign) ProfileIOS* profile;
// Most visited items from the MostVisitedSites service (copied upon receiving
// the callback). Those items are up to date with the model.
@property(nonatomic, strong)
    NSMutableArray<MostVisitedItem*>* freshMostVisitedItems;
@end

@implementation VivaldiMostVisitedSitesManager {
  // Local State prefs.
  PrefService* _localState;
  FaviconAttributesProvider* _mostVisitedAttributesProvider;
  std::map<GURL, FaviconCompletionHandler> _mostVisitedFetchFaviconCallbacks;
  // Most visited items from the MostVisitedSites service currently displayed.
  MostVisitedTilesConfig* _mostVisitedConfig;
  raw_ptr<ChromeAccountManagerService> _accountManagerService;
}

@synthesize consumer = _consumer;
@synthesize profile = _profile;

#pragma mark - INITIALIZERS
- (instancetype)initWithProfile:(ProfileIOS*)profile {
  if ((self = [super init])) {
    _profile = profile;
    _localState = _profile->GetPrefs();

    favicon::LargeIconService* largeIconService =
        IOSChromeLargeIconServiceFactory::GetForProfile(_profile);
    LargeIconCache* largeIconCache =
        IOSChromeLargeIconCacheFactory::GetForProfile(_profile);
    std::unique_ptr<ntp_tiles::MostVisitedSites> mostVisitedFactory =
        IOSMostVisitedSitesFactory::NewForBrowserState(_profile);

    _mostVisitedAttributesProvider = [[FaviconAttributesProvider alloc]
        initWithFaviconSize:kMostVisitedFaviconSize
             minFaviconSize:kMostVisitedFaviconMinimalSize
           largeIconService:largeIconService];
    // Set a cache only for the Most Visited provider, as the cache is
    // overwritten for every new results and the size of the favicon fetched for
    // the suggestions is much smaller.
    _mostVisitedAttributesProvider.cache = largeIconCache;
    _mostVisitedSites = std::move(mostVisitedFactory);
    _mostVisitedBridge =
        std::make_unique<ntp_tiles::MostVisitedSitesObserverBridge>(
            self, _mostVisitedSites.get());
    // Keep Speed Dial Top Sites backed by real Top Sites only. Upstream
    // defaults changed and now depend on explicit tile-type configuration.
    _mostVisitedSites->EnableTileTypes(
        ntp_tiles::MostVisitedSites::EnableTileTypesOptions()
            .with_top_sites(true)
            .with_custom_links(false));
    _mostVisitedSites->AddMostVisitedURLsObserver(_mostVisitedBridge.get(),
                                                  kMaxNumMostVisitedTiles);
  }
  return self;
}

#pragma mark - PUBLIC METHODS

- (void)start {
  [self refreshMostVisitedTiles];
}

- (void)stop {
  _mostVisitedBridge.reset();
  _mostVisitedSites.reset();
  _localState = nullptr;
}

- (void)removeMostVisited:(MostVisitedItem*)item {
  [self blockMostVisitedURL:item.URL];
}

- (void)refreshMostVisitedTiles {
  // Force TopSites to sync with history before refreshing UI.
  _mostVisitedSites->Refresh();
}

- (void)setConsumer:(id<VivaldiMostVisitedSitesConsumer>)consumer {
  _consumer = consumer;
  [self notifyConsumer];
}

#pragma mark - Private

- (void)notifyConsumer {
  [self.consumer setMostVisitedTilesConfig:_mostVisitedConfig];
}

- (void)blockMostVisitedURL:(GURL)URL {
  _mostVisitedSites->AddOrRemoveBlockedUrl(URL, true);
  [self useFreshMostVisited];
}

// Replaces the Most Visited items currently displayed by the most recent ones.
- (void)useFreshMostVisited {
  const base::ListValue& oldMostVisitedSites =
      _localState->GetList(prefs::kIosLatestMostVisitedSites);
  base::ListValue freshMostVisitedSites;
  for (MostVisitedItem* item in _freshMostVisitedItems) {
    freshMostVisitedSites.Append(item.URL.spec());
  }
  // Don't check for a change in the Most Visited Sites if the device doesn't
  // have any saved sites to begin with. This will not log for users with no
  // top sites that have a new top site, but the benefit of not logging for
  // new installs outweighs it.
  if (!oldMostVisitedSites.empty()) {
    [self lookForNewMostVisitedSite:freshMostVisitedSites
                oldMostVisitedSites:oldMostVisitedSites];
  }
  _localState->SetList(prefs::kIosLatestMostVisitedSites,
                       std::move(freshMostVisitedSites));

  _mostVisitedConfig =
      [[MostVisitedTilesConfig alloc] initWithLayoutGuideCenter:nil];
  _mostVisitedConfig.mostVisitedItems = _freshMostVisitedItems;
  _mostVisitedConfig.imageDataSource = self;
  [self.consumer setMostVisitedTilesConfig:_mostVisitedConfig];
}

// Logs a User Action if `freshMostVisitedSites` has at least one site that
// isn't in `oldMostVisitedSites`.
- (void)lookForNewMostVisitedSite:(const base::ListValue&)freshMostVisitedSites
              oldMostVisitedSites:(const base::ListValue&)oldMostVisitedSites {
  for (auto const& freshSiteURLValue : freshMostVisitedSites) {
    BOOL freshSiteInOldList = NO;
    for (auto const& oldSiteURLValue : oldMostVisitedSites) {
      if (freshSiteURLValue.GetString() == oldSiteURLValue.GetString()) {
        freshSiteInOldList = YES;
        break;
      }
    }
    if (!freshSiteInOldList) {
      // Reset impressions since freshness.
      _localState->SetInteger(
          prefs::kIosMagicStackSegmentationMVTImpressionsSinceFreshness, 0);
      return;
    }
  }
}

#pragma mark -  ContentSuggestionsImageDataSource
- (void)fetchFaviconForURL:(const GURL&)URL
                completion:(FaviconCompletionHandler)completion {
  _mostVisitedFetchFaviconCallbacks[URL] = completion;
  [_mostVisitedAttributesProvider fetchFaviconAttributesForURL:URL
                                                    completion:completion];
}

#pragma mark - MostVisitedSitesObserving

- (void)mostVisitedSites:(ntp_tiles::MostVisitedSites*)mostVisitedSites
          didUpdateTiles:(const ntp_tiles::NTPTilesVector&)tiles {
  (void)mostVisitedSites;
  [self onMostVisitedURLsAvailable:tiles];
}

- (void)mostVisitedSites:(ntp_tiles::MostVisitedSites*)mostVisitedSites
    didUpdateFaviconForURL:(const GURL&)siteURL {
  (void)mostVisitedSites;
  [self onIconMadeAvailable:siteURL];
}

- (void)onMostVisitedURLsAvailable:
    (const ntp_tiles::NTPTilesVector&)mostVisited {
  // This is used by the content widget.
  content_suggestions_tile_saver::SaveMostVisitedToDisk(
      mostVisited, _mostVisitedAttributesProvider,
      app_group::ShortcutsWidgetFaviconsFolder(),
      ChromeAccountManagerServiceFactory::GetForProfile(self.profile));

  _freshMostVisitedItems = [NSMutableArray array];
  for (const ntp_tiles::NTPTile& tile : mostVisited) {
    MostVisitedItem* item = [[MostVisitedItem alloc] init];
    item.title = base::SysUTF16ToNSString(tile.title);
    item.URL = tile.url;
    item.source = tile.source;
    item.titleSource = tile.title_source;
    [_freshMostVisitedItems addObject:item];
  }

  [self useFreshMostVisited];
}

- (void)onIconMadeAvailable:(const GURL&)siteURL {
  // This is used by the content widget.
  content_suggestions_tile_saver::UpdateSingleFavicon(
      siteURL, _mostVisitedAttributesProvider,
      app_group::ShortcutsWidgetFaviconsFolder(),
      ChromeAccountManagerServiceFactory::GetForProfile(self.profile));
  for (MostVisitedItem* item in _mostVisitedConfig.mostVisitedItems) {
    if (item.URL == siteURL) {
      FaviconCompletionHandler completion =
          _mostVisitedFetchFaviconCallbacks[siteURL];
      if (completion) {
        [_mostVisitedAttributesProvider
            fetchFaviconAttributesForURL:siteURL
                              completion:completion];
      }
      return;
    }
  }
}

@end
