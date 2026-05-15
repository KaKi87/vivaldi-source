// Copyright 2022 Vivaldi Technologies. All rights reserved.

#import "ios/ui/ntp/legacy/legacy_speed_dial_home_mediator.h"

#import "base/apple/foundation_util.h"
#import "base/check.h"
#import "base/strings/sys_string_conversions.h"
#import "chromium/base/containers/stack.h"
#import "components/bookmarks/browser/bookmark_model.h"
#import "components/bookmarks/browser/bookmark_model_observer.h"
#import "components/bookmarks/common/bookmark_pref_names.h"
#import "components/bookmarks/managed/managed_bookmark_service.h"
#import "components/bookmarks/vivaldi_bookmark_kit.h"
#import "components/omnibox/browser/omnibox_pref_names.h"
#import "components/prefs/ios/pref_observer_bridge.h"
#import "components/prefs/pref_change_registrar.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/browser/bookmarks/model/bookmark_model_bridge_observer.h"
#import "ios/chrome/browser/bookmarks/model/managed_bookmark_service_factory.h"
#import "ios/chrome/browser/bookmarks/ui_bundled/bookmark_utils_ios.h"
#import "ios/chrome/browser/content_suggestions/most_visited_tiles/ui/most_visited_item.h"
#import "ios/chrome/browser/content_suggestions/most_visited_tiles/ui/most_visited_tiles_config.h"
#import "ios/chrome/browser/first_run/public/first_run_util.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/prefs/pref_backed_boolean.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/model/utils/observable_boolean.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/most_visited_sites/vivaldi_most_visited_sites_manager.h"
#import "ios/ui/helpers/vivaldi_global_helpers.h"
#import "ios/ui/ntp/top_toolbar/top_toolbar_swift.h"
#import "ios/ui/ntp/vivaldi_speed_dial_constants.h"
#import "ios/ui/ntp/vivaldi_speed_dial_page_type.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_prefs.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_prefs_helper.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"
#import "ui/base/l10n/l10n_util_mac.h"
#import "url/gurl.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using bookmarks::ManagedBookmarkService;
using l10n_util::GetNSString;
using vivaldi_bookmark_kit::GetSpeeddial;
using vivaldi_bookmark_kit::IsDirectChildOfRoot;
using vivaldi_bookmark_kit::IsSeparator;

@interface VivaldiSpeedDialHomeMediator () <BookmarkModelBridgeObserver,
                                            VivaldiMostVisitedSitesConsumer,
                                            PrefObserverDelegate,
                                            BooleanObserver> {
  // Preference service from the application context.
  PrefService* _prefs;
  // Pref observer to track changes to prefs.
  std::unique_ptr<PrefObserverBridge> _prefObserverBridge;
  // Registrar for pref changes notifications.
  PrefChangeRegistrar _prefChangeRegistrar;
  // The profile for this mediator.
  raw_ptr<ProfileIOS> _profile;
  // Observer for tab bar enabled/disabled state
  PrefBackedBoolean* _tabBarEnabled;
  // Observer for omnibox position
  PrefBackedBoolean* _bottomOmniboxEnabled;
  // Observer for frequently visited pages visibility state
  PrefBackedBoolean* _showFrequentlyVisited;
  // Observer for speed dials visibility state
  PrefBackedBoolean* _showSpeedDials;
  // Observer for start page customize button visibility state
  PrefBackedBoolean* _showCustomizeStartPageButton;
  // Observer for start page Add button visibility state
  PrefBackedBoolean* _showAddButton;
}

// Manager that provides most visited sites
@property(nonatomic, strong)
    VivaldiMostVisitedSitesManager* mostVisitedSiteManager;
// Most visited items from the MostVisitedSites service currently displayed.
@property(nonatomic, strong) MostVisitedTilesConfig* mostVisitedConfig;
// Collection of toolbar items
@property(nonatomic, strong)
    NSMutableArray<VivaldiNTPTopToolbarItem*>* toolbarItems;
// Collection of cached toolbar items. This is used to compare whether toolbar
// items count is changed due to CRUD operation either initiated by user or
// sync.
@property(nonatomic, strong)
    NSMutableArray<VivaldiNTPTopToolbarItem*>* cachedToolbarItems;
// Cache to quickly map folder node ids to toolbar items.
@property(nonatomic, strong)
    NSMutableDictionary<NSNumber*, VivaldiNTPTopToolbarItem*>*
        toolbarItemsByPrimaryId;
// The currently selected toolbar item computed by the mediator.
@property(nonatomic, strong) VivaldiNTPTopToolbarItem* selectedToolbarItem;
// The selected toolbar index corresponding to `selectedToolbarItem`.
@property(nonatomic, assign) NSInteger selectedToolbarItemIndex;
@end

@implementation VivaldiSpeedDialHomeMediator {
  // The model holding bookmark data.
  base::WeakPtr<BookmarkModel> _bookmarkModel;
  // Bridge to register for bookmark changes in the bookmarkModel.
  std::unique_ptr<BookmarkModelBridge> _bookmarkModelBridge;
}

@synthesize consumer = _consumer;
@synthesize toolbarItems = _toolbarItems;
@synthesize cachedToolbarItems = _cachedToolbarItems;
@synthesize toolbarItemsByPrimaryId = _toolbarItemsByPrimaryId;
@synthesize selectedToolbarItem = _selectedToolbarItem;
@synthesize selectedToolbarItemIndex = _selectedToolbarItemIndex;

#pragma mark - INITIALIZERS
- (instancetype)initWithProfile:(ProfileIOS*)profile
                  bookmarkModel:(BookmarkModel*)bookmarkModel {
  if ((self = [super init])) {
    _profile = profile;
    _bookmarkModel = bookmarkModel->AsWeakPtr();
    _bookmarkModelBridge =
        std::make_unique<BookmarkModelBridge>(self, _bookmarkModel.get());

    VivaldiMostVisitedSitesManager* mostVisitedSiteManager =
        [[VivaldiMostVisitedSitesManager alloc] initWithProfile:profile];
    mostVisitedSiteManager.consumer = self;
    _mostVisitedSiteManager = mostVisitedSiteManager;

    _prefs = profile->GetPrefs();
    _prefChangeRegistrar.Init(_prefs);
    _prefObserverBridge.reset(new PrefObserverBridge(self));

    _prefObserverBridge->ObserveChangesForPreference(
        vivaldiprefs::kVivaldiStartPageLayoutStyle, &_prefChangeRegistrar);
    _prefObserverBridge->ObserveChangesForPreference(
        vivaldiprefs::kVivaldiStartPageSDMaximumColumns, &_prefChangeRegistrar);

    _tabBarEnabled = [[PrefBackedBoolean alloc]
        initWithPrefService:_prefs
                   prefName:vivaldiprefs::kVivaldiDesktopTabsEnabled];
    [_tabBarEnabled setObserver:self];
    [self booleanDidChange:_tabBarEnabled];

    _bottomOmniboxEnabled = [[PrefBackedBoolean alloc]
        initWithPrefService:GetApplicationContext()->GetLocalState()
                   prefName:omnibox::kIsOmniboxInBottomPosition];
    [_bottomOmniboxEnabled setObserver:self];
    [self booleanDidChange:_bottomOmniboxEnabled];

    _showAddButton = [[PrefBackedBoolean alloc]
        initWithPrefService:GetApplicationContext()->GetLocalState()
                   prefName:vivaldiprefs::kVivaldiStartPageShowAddButton];
    [_showAddButton setObserver:self];
    [self booleanDidChange:_showAddButton];

    _showFrequentlyVisited = [[PrefBackedBoolean alloc]
        initWithPrefService:_prefs
                   prefName:vivaldiprefs::
                                kVivaldiStartPageShowFrequentlyVisited];
    [_showFrequentlyVisited setObserver:self];

    _showSpeedDials = [[PrefBackedBoolean alloc]
        initWithPrefService:_prefs
                   prefName:vivaldiprefs::kVivaldiStartPageShowSpeedDials];
    [_showSpeedDials setObserver:self];
    [self booleanDidChange:_showSpeedDials];

    _showCustomizeStartPageButton = [[PrefBackedBoolean alloc]
        initWithPrefService:_prefs
                   prefName:vivaldiprefs::kVivaldiStartPageShowCustomizeButton];
    [_showCustomizeStartPageButton setObserver:self];
    [self booleanDidChange:_showCustomizeStartPageButton];

    [VivaldiStartPagePrefs setPrefService:profile->GetPrefs()];

    self.toolbarItems = [[NSMutableArray alloc] init];
    self.cachedToolbarItems = [[NSMutableArray alloc] init];
    self.toolbarItemsByPrimaryId = [[NSMutableDictionary alloc] init];
  }
  return self;
}

#pragma mark - PUBLIC METHODS

- (void)startMediating {
  DCHECK(self.consumer);
  [self computeSpeedDialFolders];
}

- (void)disconnect {
  _profile = nil;
  _bookmarkModel = nullptr;
  _bookmarkModelBridge.reset();
  self.consumer = nil;

  _prefChangeRegistrar.RemoveAll();
  _prefObserverBridge.reset();
  _prefs = nil;

  [_mostVisitedSiteManager stop];
  _mostVisitedSiteManager.consumer = nil;
  _mostVisitedSiteManager = nil;
  _mostVisitedConfig = nil;

  [_tabBarEnabled stop];
  [_tabBarEnabled setObserver:nil];
  _tabBarEnabled = nil;

  [_bottomOmniboxEnabled stop];
  [_bottomOmniboxEnabled setObserver:nil];
  _bottomOmniboxEnabled = nil;

  [_showFrequentlyVisited stop];
  [_showFrequentlyVisited setObserver:nil];
  _showFrequentlyVisited = nil;

  [_showSpeedDials stop];
  [_showSpeedDials setObserver:nil];
  _showSpeedDials = nil;

  [_showCustomizeStartPageButton stop];
  [_showCustomizeStartPageButton setObserver:nil];
  _showCustomizeStartPageButton = nil;

  [_showAddButton stop];
  [_showAddButton setObserver:nil];
  _showAddButton = nil;

  [self.toolbarItemsByPrimaryId removeAllObjects];
  self.toolbarItemsByPrimaryId = nil;
}

- (void)setConsumer:(id<SpeedDialHomeConsumer>)consumer {
  _consumer = consumer;
  if (!self.consumer) {
    return;
  }

  NSInteger selectedIndex = [self clampedIndex:self.selectedToolbarItemIndex
                               forToolbarItems:self.toolbarItems];
  [self.consumer refreshMenuItems:self.toolbarItems
                    selectedIndex:selectedIndex];
}

- (void)removeMostVisited:(VivaldiSpeedDialItem*)item {
  for (MostVisitedItem* tile in _mostVisitedConfig.mostVisitedItems) {
    if (tile.URL == item.url) {
      [_mostVisitedSiteManager removeMostVisited:tile];
      break;
    }
  }
}

- (void)computeSpeedDialFolders {
  [_mostVisitedSiteManager start];

  if (_bookmarkModel && _bookmarkModel->loaded())
    [self computeTopToolbarItems];
}

- (void)computeSpeedDialChildItems:(VivaldiSpeedDialItem*)item {
  // If an item is provided fetch the children of that item.
  // Otherwise fetch all children of all speed dial folder items and notify
  // consumers to update them.
  if (item && item.bookmarkNode) {
    [self reloadChildrenForBookmarkNode:item.bookmarkNode];
  } else {
    for (VivaldiNTPTopToolbarItem* group in self.toolbarItems) {
      if ([group.uuid length] <= 0)
        continue;

      std::string uuidString =
          base::SysNSStringToUTF8([group.uuid lowercaseString]);
      base::Uuid uuid = base::Uuid::ParseLowercase(uuidString);

      const bookmarks::BookmarkNode* node = _bookmarkModel.get()->GetNodeByUuid(
          uuid, bookmarks::BookmarkModel::NodeTypeForUuidLookup::
                    kLocalOrSyncableNodes);

      [self reloadChildrenForBookmarkNode:node];
    }
  }
}

- (void)moveSpeedDialItem:(VivaldiSpeedDialItem*)item
                 position:(NSInteger)position {
  _bookmarkModel.get()->Move(item.bookmarkNode, item.parent, position);
}

- (void)deleteSpeedDialItem:(VivaldiSpeedDialItem*)item {
  if (_bookmarkModel.get()->loaded() && item.bookmarkNode) {
    std::vector<const bookmarks::BookmarkNode*> nodes;
    nodes.push_back(item.bookmarkNode);
    const BookmarkNode* trashFolder = _bookmarkModel.get()->trash_node();
    bookmark_utils_ios::MoveBookmarks(nodes, _bookmarkModel.get(), trashFolder);
  }
}

- (void)didSelectToolbarItem:(VivaldiNTPTopToolbarItem*)item
                     atIndex:(NSInteger)index {
  [self applySelectedItem:item index:index saveLastVisited:YES];
}

#pragma mark - PRIVATE METHODS

/// Layout style change handler
- (void)handleLayoutChangeNotification {
  [self.consumer reloadLayout];
}

/// Returns current sorting mode
- (SpeedDialSortingMode)currentSortingMode {
  return [VivaldiStartPagePrefsHelper getSDSortingMode];
}

/// Returns current sorting order
- (SpeedDialSortingOrder)currentSortingOrder {
  return [VivaldiStartPagePrefsHelper getSDSortingOrder];
}

/// Fetches speed dial folders and their children, notifies consumers.
- (void)computeTopToolbarItems {
  [self rebuildToolbarItems];
  [self publishToolbarItemsIfNeeded];
}

// Rebuild the toolbar model from bookmark folders, optional Top Sites, and
// the trailing Add Group page.
- (void)rebuildToolbarItems {
  // Clear old data so we don’t retain stale groups.
  [self.toolbarItems removeAllObjects];
  [self.toolbarItemsByPrimaryId removeAllObjects];

  [self appendSpeedDialGroupsIfNeeded];
  [self appendTopSitesIfNeeded];
  [self appendAddGroupItem];
}

// Build toolbar items first, then compute selection once against the old
// toolbar state. That keeps the controller focused on rendering and viewport
// preservation instead of business rules.
- (void)publishToolbarItemsIfNeeded {
  // Decide selection here so the controller only applies UI state and keeps
  // visual position stable during reloads.
  NSInteger selectedIndex =
      [self selectedIndexForToolbarItems:self.toolbarItems
                    previousToolbarItems:self.cachedToolbarItems
                    previousSelectedItem:self.selectedToolbarItem];
  BOOL itemsChanged = ![VivaldiNTPTopToolbarItemHelper
      compareEqualityForFirst:self.toolbarItems
                       second:self.cachedToolbarItems];
  VivaldiNTPTopToolbarItem* selectedItem =
      (selectedIndex >= 0 && selectedIndex < (NSInteger)self.toolbarItems.count)
          ? self.toolbarItems[selectedIndex]
          : nil;
  BOOL selectedPageChanged =
      selectedIndex != self.selectedToolbarItemIndex ||
      ![VivaldiNTPTopToolbarItemHelper
          representsSamePageForFirst:self.selectedToolbarItem
                              second:selectedItem];

  [self applySelectedItem:selectedItem index:selectedIndex saveLastVisited:NO];

  if (self.consumer && (itemsChanged || selectedPageChanged)) {
    [self.consumer refreshMenuItems:self.toolbarItems
                      selectedIndex:selectedIndex];
  }

  if (self.toolbarItems.count > 0 && itemsChanged) {
    self.cachedToolbarItems = [self.toolbarItems copy];
  }
}

- (void)appendSpeedDialGroupsIfNeeded {
  if (![self showSpeedDials]) {
    return;
  }

  std::vector<const BookmarkNode*> rootNodes;

  if (!_bookmarkModel->bookmark_bar_node()->children().empty()) {
    rootNodes.push_back(_bookmarkModel.get()->bookmark_bar_node());
  }
  if (!_bookmarkModel->mobile_node()->children().empty()) {
    rootNodes.push_back(_bookmarkModel.get()->mobile_node());
  }
  if (!_bookmarkModel->other_node()->children().empty()) {
    rootNodes.push_back(_bookmarkModel.get()->other_node());
  }

  bookmarks::ManagedBookmarkService* managedBookmarkService =
      ManagedBookmarkServiceFactory::GetForProfile(_profile.get());
  base::stack<const BookmarkNode*> stack;

  for (auto it = rootNodes.rbegin(); it != rootNodes.rend(); ++it) {
    stack.push(*it);
  }

  while (!stack.empty()) {
    const BookmarkNode* node = stack.top();
    stack.pop();

    if (GetSpeeddial(node) && !IsSeparator(node)) {
      VivaldiNTPTopToolbarItem* toolbarItem = [self buildGroupForNode:node];
      [self.toolbarItems addObject:toolbarItem];
      [self cacheToolbarItemIfNeeded:toolbarItem];
    }

    rootNodes.clear();
    for (const auto& child : node->children()) {
      if (child->is_folder() &&
          !managedBookmarkService->IsNodeManaged(child.get())) {
        rootNodes.push_back(child.get());
      }
    }

    for (auto it = rootNodes.rbegin(); it != rootNodes.rend(); ++it) {
      stack.push(*it);
    }
  }
}

- (void)appendTopSitesIfNeeded {
  if (![self showFrequentlyVisited]) {
    return;
  }

  VivaldiNTPTopToolbarItem* toolbarItem = [[VivaldiNTPTopToolbarItem alloc]
      initWithPrimaryId:nil
                   uuid:nil
                  title:GetNSString(IDS_IOS_START_PAGE_FREQUENTLY_VISITED_TITLE)
               pageType:VivaldiSpeedDialPageTypeTopSites];
  toolbarItem.children = [self listTopSiteItems];
  [self.toolbarItems insertObject:toolbarItem atIndex:0];
}

- (void)appendAddGroupItem {
  [self.toolbarItems
      addObject:[[VivaldiNTPTopToolbarItem alloc]
                    initWithPrimaryId:nil
                                 uuid:nil
                                title:@""
                             pageType:VivaldiSpeedDialPageTypeAddGroup]];
}

- (NSInteger)
    selectedIndexForToolbarItems:(NSArray<VivaldiNTPTopToolbarItem*>*)items
            previousToolbarItems:
                (NSArray<VivaldiNTPTopToolbarItem*>*)previousToolbarItems
            previousSelectedItem:
                (VivaldiNTPTopToolbarItem*)previousSelectedItem {
  if (items.count <= 0) {
    return 0;
  }

  // Selection logic lives here:
  // 1. On the first build, use the user's "open with" preference.
  // 2. On normal rebuilds, keep showing the same logical page when it still
  //    exists, even if its row moved.
  // 3. If Top Sites or the group list appears/disappears, re-evaluate with the
  //    startup rules because the visible page set changed.
  NSInteger clampedCurrentIndex =
      [self clampedIndex:self.selectedToolbarItemIndex forToolbarItems:items];

  if (!previousSelectedItem) {
    if (previousToolbarItems.count <= 0) {
      return [self startupSelectedIndexForToolbarItems:items];
    }
    return clampedCurrentIndex;
  }

  BOOL topSitesVisibilityChanged =
      [self indexOfFirstPageType:VivaldiSpeedDialPageTypeTopSites
                  inToolbarItems:previousToolbarItems] !=
      [self indexOfFirstPageType:VivaldiSpeedDialPageTypeTopSites
                  inToolbarItems:items];
  BOOL speedDialVisibilityChanged =
      [self indexOfFirstPageType:VivaldiSpeedDialPageTypeSpeedDial
                  inToolbarItems:previousToolbarItems] !=
      [self indexOfFirstPageType:VivaldiSpeedDialPageTypeSpeedDial
                  inToolbarItems:items];

  if (topSitesVisibilityChanged || speedDialVisibilityChanged) {
    return [self
        selectedIndexAfterVisibilityChangeForToolbarItems:items
                                     previousSelectedItem:previousSelectedItem];
  }

  NSInteger matchingIndex =
      [self indexOfMatchingToolbarItem:previousSelectedItem
                        inToolbarItems:items];
  if (matchingIndex != NSNotFound) {
    return matchingIndex;
  }

  return clampedCurrentIndex;
}

- (NSInteger)selectedIndexAfterVisibilityChangeForToolbarItems:
                 (NSArray<VivaldiNTPTopToolbarItem*>*)items
                                          previousSelectedItem:
                                              (VivaldiNTPTopToolbarItem*)
                                                  previousSelectedItem {
  NSInteger topSitesIndex =
      [self indexOfFirstPageType:VivaldiSpeedDialPageTypeTopSites
                  inToolbarItems:items];
  NSInteger firstGroupIndex =
      [self indexOfFirstPageType:VivaldiSpeedDialPageTypeSpeedDial
                  inToolbarItems:items];
  BOOL hasTopSites = topSitesIndex != NSNotFound;
  BOOL hasSpeedDials = firstGroupIndex != NSNotFound;

  if (hasTopSites && !hasSpeedDials) {
    return topSitesIndex;
  }

  if (hasSpeedDials && !hasTopSites) {
    NSInteger matchedPreviousGroupIndex =
        [self indexOfMatchingToolbarItem:previousSelectedItem
                          inToolbarItems:items];
    if (matchedPreviousGroupIndex != NSNotFound &&
        items[matchedPreviousGroupIndex].pageType ==
            VivaldiSpeedDialPageTypeSpeedDial) {
      return matchedPreviousGroupIndex;
    }
    return firstGroupIndex;
  }

  if (!hasTopSites && !hasSpeedDials) {
    return 0;
  }

  // Once both Top Sites and groups are available, fall back to the same
  // reopen rules used on startup.
  switch ([VivaldiStartPagePrefsHelper getReopenStartPageWithItem]) {
    case VivaldiStartPageStartItemTypeTopSites:
      return topSitesIndex;

    case VivaldiStartPageStartItemTypeLastVisited: {
      NSInteger matchedPreviousGroupIndex =
          [self indexOfMatchingToolbarItem:previousSelectedItem
                            inToolbarItems:items];
      if (matchedPreviousGroupIndex != NSNotFound &&
          items[matchedPreviousGroupIndex].pageType ==
              VivaldiSpeedDialPageTypeSpeedDial) {
        return matchedPreviousGroupIndex;
      }

      NSInteger savedIndex = [self lastVisitedGroupIndexInToolbarItems:items];
      if (savedIndex != NSNotFound) {
        return savedIndex;
      }

      return firstGroupIndex;
    }

    case VivaldiStartPageStartItemTypeFirstGroup:
    default:
      return firstGroupIndex;
  }
}

- (NSInteger)startupSelectedIndexForToolbarItems:
    (NSArray<VivaldiNTPTopToolbarItem*>*)items {
  if (items.count <= 0) {
    return 0;
  }

  VivaldiStartPageStartItemType openWith =
      [VivaldiStartPagePrefsHelper getReopenStartPageWithItem];

  switch (openWith) {
    case VivaldiStartPageStartItemTypeTopSites:
      return 0;

    case VivaldiStartPageStartItemTypeLastVisited: {
      NSInteger lastVisitedIndex =
          [self lastVisitedGroupIndexInToolbarItems:items];
      if (lastVisitedIndex != NSNotFound) {
        return lastVisitedIndex;
      }

      NSInteger firstGroupIndex =
          [self indexOfFirstPageType:VivaldiSpeedDialPageTypeSpeedDial
                      inToolbarItems:items];
      return firstGroupIndex != NSNotFound ? firstGroupIndex : 0;
    }

    case VivaldiStartPageStartItemTypeFirstGroup:
    default: {
      NSInteger firstGroupIndex =
          [self indexOfFirstPageType:VivaldiSpeedDialPageTypeSpeedDial
                      inToolbarItems:items];
      return firstGroupIndex != NSNotFound ? firstGroupIndex : 0;
    }
  }
}

- (NSInteger)indexOfFirstPageType:(VivaldiSpeedDialPageType)pageType
                   inToolbarItems:(NSArray<VivaldiNTPTopToolbarItem*>*)items {
  for (NSUInteger index = 0; index < items.count; index++) {
    if (items[index].pageType == pageType) {
      return index;
    }
  }

  return NSNotFound;
}

- (NSInteger)indexOfMatchingToolbarItem:(VivaldiNTPTopToolbarItem*)item
                         inToolbarItems:
                             (NSArray<VivaldiNTPTopToolbarItem*>*)items {
  if (!item) {
    return NSNotFound;
  }

  for (NSUInteger index = 0; index < items.count; index++) {
    if ([VivaldiNTPTopToolbarItemHelper representsSamePageForFirst:items[index]
                                                            second:item]) {
      return index;
    }
  }

  return NSNotFound;
}

- (NSInteger)lastVisitedGroupIndexInToolbarItems:
    (NSArray<VivaldiNTPTopToolbarItem*>*)items {
  // "Last visited group" is stored as a stable identifier, not a row index, so
  // toggling Top Sites or reordering groups cannot silently point at the wrong
  // page.
  NSString* lastVisitedGroupIdentifier =
      [VivaldiStartPagePrefsHelper getStartPageLastVisitedGroupIdentifier];
  if (lastVisitedGroupIdentifier.length <= 0) {
    return NSNotFound;
  }

  for (NSUInteger index = 0; index < items.count; index++) {
    VivaldiNTPTopToolbarItem* item = items[index];
    if (item.pageType != VivaldiSpeedDialPageTypeSpeedDial) {
      continue;
    }

    if ([self toolbarItem:item
            matchesSavedIdentifier:lastVisitedGroupIdentifier]) {
      return index;
    }
  }

  // The saved group was removed or can no longer be resolved. Fall back to the
  // default selection rules instead of trusting an old row index.
  return NSNotFound;
}

- (BOOL)toolbarItem:(VivaldiNTPTopToolbarItem*)item
    matchesSavedIdentifier:(NSString*)identifier {
  if (!item || item.pageType != VivaldiSpeedDialPageTypeSpeedDial ||
      identifier.length <= 0) {
    return NO;
  }

  if ([identifier hasPrefix:@"uuid:"]) {
    NSString* uuid = [identifier substringFromIndex:5];
    return item.uuid.length > 0 && [item.uuid isEqualToString:uuid];
  }

  if ([identifier hasPrefix:@"id:"]) {
    NSString* primaryId = [identifier substringFromIndex:3];
    return item.primaryId &&
           [[item.primaryId stringValue] isEqualToString:primaryId];
  }

  return NO;
}

- (NSString*)identifierForToolbarItem:(VivaldiNTPTopToolbarItem*)item {
  if (!item || item.pageType != VivaldiSpeedDialPageTypeSpeedDial) {
    return @"";
  }

  // Prefer UUID when available so the saved page survives row shifts and
  // bookmark id changes. Fall back to the bookmark id only for older items
  // that do not expose a valid UUID yet.
  if (item.uuid.length > 0) {
    return [NSString stringWithFormat:@"uuid:%@", item.uuid];
  }

  if (item.primaryId) {
    return [NSString stringWithFormat:@"id:%@", item.primaryId];
  }

  return @"";
}

- (NSInteger)clampedIndex:(NSInteger)index
          forToolbarItems:(NSArray<VivaldiNTPTopToolbarItem*>*)items {
  if (items.count <= 0) {
    return 0;
  }

  return MAX(0, MIN(index, (NSInteger)items.count - 1));
}

- (void)applySelectedItem:(VivaldiNTPTopToolbarItem*)item
                    index:(NSInteger)index
          saveLastVisited:(BOOL)saveLastVisited {
  self.selectedToolbarItem = item;
  self.selectedToolbarItemIndex = [self clampedIndex:index
                                     forToolbarItems:self.toolbarItems];

  if (saveLastVisited && item &&
      item.pageType == VivaldiSpeedDialPageTypeSpeedDial) {
    NSString* identifier = [self identifierForToolbarItem:item];
    if (identifier.length > 0) {
      [VivaldiStartPagePrefsHelper
          setStartPageLastVisitedGroupIdentifier:identifier];
    }
  }
}

// Create and return a ToolbarItem from provided BookmarkNode computing the
// children of that node.
- (VivaldiNTPTopToolbarItem*)buildGroupForNode:(const BookmarkNode*)node {
  NSString* uuidString;
  if (node->uuid().is_valid()) {
    uuidString = base::SysUTF8ToNSString(node->uuid().AsLowercaseString());
  }

  VivaldiNTPTopToolbarItem* groupItem = [[VivaldiNTPTopToolbarItem alloc]
      initWithPrimaryId:@(node->id())
                   uuid:uuidString
                  title:bookmark_utils_ios::TitleForBookmarkNode(node)
               pageType:VivaldiSpeedDialPageTypeSpeedDial];

  NSMutableArray* childrens = [[NSMutableArray alloc] init];
  if (node->is_folder()) {
    for (const auto& child : node->children()) {
      const BookmarkNode* childNode = child.get();
      if (IsSeparator(childNode))
        continue;
      VivaldiSpeedDialItem* item =
          [[VivaldiSpeedDialItem alloc] initWithBookmark:childNode];
      // If the group/folder is a direct children of one of the root nodes
      // then do not show the 'Move out of folder' action since that moves
      // the item to the root folder and user do not see it on StartPage
      // anymore.
      item.isMoveOutAble = !IsDirectChildOfRoot(_bookmarkModel.get(), node);
      [childrens addObject:item];
    }

    groupItem.children = [self sortSpeedDials:childrens
                                       byMode:self.currentSortingMode];
  }
  return groupItem;
}

// Reload only the children of the provided BookmarkNode and notify the
// consumers.
- (void)reloadChildrenForBookmarkNode:(const BookmarkNode*)bookmarkNode {
  if (!bookmarkNode)
    return;
  VivaldiNTPTopToolbarItem* updatedGroup =
      [self buildGroupForNode:bookmarkNode];
  VivaldiNTPTopToolbarItem* cachedGroup =
      [self toolbarItemForBookmarkNode:bookmarkNode];

  if (cachedGroup) {
    cachedGroup.uuid = updatedGroup.uuid;
    cachedGroup.title = updatedGroup.title;
    cachedGroup.children = updatedGroup.children;
    [self cacheToolbarItemIfNeeded:cachedGroup];
    [self.consumer refreshChildItems:cachedGroup.children parent:cachedGroup];
    return;
  }

  [self.consumer refreshChildItems:updatedGroup.children parent:updatedGroup];
}

// Reloads only the top site items and notify the consumers.
- (void)reloadChildrenForTopSite {
  if (![self showFrequentlyVisited]) {
    return;
  }

  VivaldiNTPTopToolbarItem* toolbarItem;
  NSMutableArray<VivaldiSpeedDialItem*>* topSites = [self listTopSiteItems];

  // Find the toolbar item with matching page type
  for (NSUInteger i = 0; i < self.toolbarItems.count; i++) {
    VivaldiNTPTopToolbarItem* item = self.toolbarItems[i];
    if (item.pageType == VivaldiSpeedDialPageTypeTopSites) {
      toolbarItem = item;
      toolbarItem.children = topSites;
      break;
    }
  }

  [self.consumer refreshChildItems:topSites parent:toolbarItem];
}

// Returns the mapped item for top site items loaded into the model.
// If the model is not loaded, it returns an empty array.
- (NSMutableArray<VivaldiSpeedDialItem*>*)listTopSiteItems {
  NSMutableArray<VivaldiSpeedDialItem*>* topSites =
      [[NSMutableArray alloc] init];
  for (MostVisitedItem* tile in _mostVisitedConfig.mostVisitedItems) {
    VivaldiSpeedDialItem* item =
        [[VivaldiSpeedDialItem alloc] initWithTitle:tile.title url:tile.URL];
    item.imageDataSource = _mostVisitedConfig.imageDataSource;
    [topSites addObject:item];
  }
  return topSites;
}

/// Sort and return children of a speed dial folder
- (NSArray*)sortSpeedDials:(NSArray*)items byMode:(SpeedDialSortingMode)mode {
  NSArray* sortedArray =
      [items sortedArrayUsingComparator:^NSComparisonResult(
                 VivaldiSpeedDialItem* first, VivaldiSpeedDialItem* second) {
        switch (mode) {
          case SpeedDialSortingManual:
            // Return as it is coming from bookmark model by default
            return NSOrderedAscending;
          case SpeedDialSortingByTitle:
            // Sort by title
            return [first.title compare:second.title
                                options:NSCaseInsensitiveSearch];
          case SpeedDialSortingByAddress:
            // Sort by address
            return [self compare:first.urlString second:second.urlString];
          case SpeedDialSortingByNickname:
            // Sort by nickname
            return [self compare:first.nickname second:second.nickname];
          case SpeedDialSortingByDescription:
            // Sort by description
            return [self compare:first.description second:second.description];
          case SpeedDialSortingByDate:
            // Sort by date
            return [first.createdAt compare:second.createdAt];
          case SpeedDialSortingByKind:
            // Sort by kind
            return [self compare:first.isFolder
                          second:second.isFolder
                    foldersFirst:YES];
          default:
            // Return as it is coming from bookmark model by default
            return NSOrderedAscending;
        }
      }];

  // If the current sorting order is descending
  // Reverse the array & check it is not sort by SpeedDialSortingManual
  if (self.currentSortingOrder == SpeedDialSortingOrderDescending &&
      self.currentSortingMode != SpeedDialSortingManual) {
    sortedArray =
        [[[sortedArray reverseObjectEnumerator] allObjects] mutableCopy];
  }

  return sortedArray;
}

/// Returns sorted result from two provided NSString keys.
- (NSComparisonResult)compare:(NSString*)first second:(NSString*)second {
  return [VivaldiGlobalHelpers compare:first second:second];
}

/// Returns sorted result from two provided BOOL keys, and sorting order.
- (NSComparisonResult)compare:(BOOL)first
                       second:(BOOL)second
                 foldersFirst:(BOOL)foldersFirst {
  return [VivaldiGlobalHelpers compare:first
                                second:second
                          foldersFirst:foldersFirst];
}

#pragma mark - Toolbar Item Helpers

// Stores the toolbar item in the lookup cache when a primary id is available.
- (void)cacheToolbarItemIfNeeded:(VivaldiNTPTopToolbarItem*)toolbarItem {
  if (!toolbarItem || !toolbarItem.primaryId) {
    return;
  }
  self.toolbarItemsByPrimaryId[toolbarItem.primaryId] = toolbarItem;
}

// Returns the cached toolbar item associated with the provided folder node.
- (VivaldiNTPTopToolbarItem*)toolbarItemForBookmarkNode:
    (const BookmarkNode*)bookmarkNode {
  if (!bookmarkNode) {
    return nil;
  }

  NSNumber* primaryId = @(bookmarkNode->id());
  VivaldiNTPTopToolbarItem* toolbarItem =
      self.toolbarItemsByPrimaryId[primaryId];
  if (toolbarItem) {
    return toolbarItem;
  }

  for (VivaldiNTPTopToolbarItem* item in self.toolbarItems) {
    if ([item.primaryId isEqualToNumber:primaryId]) {
      [self cacheToolbarItemIfNeeded:item];
      return item;
    }
  }

  return nil;
}

// Mutates the cached speed dial item to reflect the updated bookmark metadata.
- (BOOL)updateCachedItemForBookmarkNode:(const BookmarkNode*)bookmarkNode
                                 parent:(const BookmarkNode*)parent {
  if (!bookmarkNode || !parent) {
    return NO;
  }

  VivaldiNTPTopToolbarItem* toolbarItem =
      [self toolbarItemForBookmarkNode:parent];
  if (!toolbarItem) {
    return NO;
  }

  NSArray<VivaldiSpeedDialItem*>* children = toolbarItem.children;
  if (children.count == 0) {
    return NO;
  }

  VivaldiSpeedDialItem* updatedItem = nil;
  for (VivaldiSpeedDialItem* item in children) {
    if (item.bookmarkNode == bookmarkNode || [item id] == bookmarkNode->id()) {
      updatedItem = item;
      break;
    }
  }

  if (!updatedItem) {
    return NO;
  }

  updatedItem.bookmarkNode = bookmarkNode;
  updatedItem.title = bookmark_utils_ios::TitleForBookmarkNode(bookmarkNode);
  updatedItem.url = bookmarkNode->url();
  updatedItem.isFolder = bookmarkNode->is_folder();
  updatedItem.isSpeedDial = GetSpeeddial(bookmarkNode);
  updatedItem.isMoveOutAble =
      !IsDirectChildOfRoot(_bookmarkModel.get(), parent);

  return YES;
}

// Returns YES when the current sorting configuration requires a re-sort.
- (BOOL)shouldResortChildrenForToolbarItem:
    (VivaldiNTPTopToolbarItem*)toolbarItem {
  if (!toolbarItem || toolbarItem.children.count <= 1) {
    return NO;
  }
  return self.currentSortingMode != SpeedDialSortingManual;
}

// Checks whether two child arrays share the same order.
- (BOOL)children:(NSArray<VivaldiSpeedDialItem*>*)lhs
    haveSameOrderAs:(NSArray<VivaldiSpeedDialItem*>*)rhs {
  if (lhs == rhs) {
    return YES;
  }
  if (!lhs || !rhs || lhs.count != rhs.count) {
    return NO;
  }
  for (NSUInteger index = 0; index < lhs.count; ++index) {
    if (lhs[index] != rhs[index]) {
      return NO;
    }
  }
  return YES;
}

- (void)refreshContents {
  if (_bookmarkModel.get() && _bookmarkModel->IsDoingExtensiveChanges())
    return;
  [self computeSpeedDialFolders];
}

- (BOOL)showFrequentlyVisited {
  if (!_showFrequentlyVisited)
    return NO;
  return [_showFrequentlyVisited value];
}

- (BOOL)showSpeedDials {
  if (!_showSpeedDials)
    return YES;
  return [_showSpeedDials value];
}

#pragma mark - BooleanObserver

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  if (observableBoolean == _showSpeedDials) {
    [self refreshContents];
    [self.consumer setSpeedDialsEnabled:[observableBoolean value]];
  } else if (observableBoolean == _showFrequentlyVisited) {
    [self refreshContents];
    [self.consumer setFrequentlyVisitedPagesEnabled:[observableBoolean value]];
  } else if (observableBoolean == _showCustomizeStartPageButton) {
    [self.consumer
        setShowCustomizeStartPageButtonEnabled:[observableBoolean value]];
  } else if (observableBoolean == _showAddButton) {
    [self.consumer setShowAddButtonEnabled:[observableBoolean value]];
  } else if (observableBoolean == _tabBarEnabled ||
             observableBoolean == _bottomOmniboxEnabled) {
    [self handleLayoutChangeNotification];
  }
}

#pragma mark - PrefObserverDelegate

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  if (preferenceName == vivaldiprefs::kVivaldiStartPageLayoutStyle ||
      preferenceName == vivaldiprefs::kVivaldiStartPageSDMaximumColumns) {
    [self handleLayoutChangeNotification];
  }
}

#pragma mark - BOOKMARK MODEL OBSERVER

- (void)bookmarkModelLoaded {
  [self.consumer bookmarkModelLoaded];
  [self startMediating];
}

- (void)didChangeNode:(const bookmarks::BookmarkNode*)bookmarkNode {
  if (_bookmarkModel.get() && _bookmarkModel->IsDoingExtensiveChanges()) {
    return;
  }
  // If the node is a group reload the toolbar because it can be
  // that a Group is renamed. Otherwise, refresh the node.
  if (bookmarkNode->is_folder() && GetSpeeddial(bookmarkNode)) {
    [self computeTopToolbarItems];
    return;
  }

  const bookmarks::BookmarkNode* parent = bookmarkNode->parent();
  if (!bookmarkNode->is_url() || !parent || !parent->is_folder()) {
    return;
  }

  if (![self updateCachedItemForBookmarkNode:bookmarkNode parent:parent]) {
    [self reloadChildrenForBookmarkNode:parent];
    return;
  }

  VivaldiNTPTopToolbarItem* toolbarItem =
      [self toolbarItemForBookmarkNode:parent];
  if (!toolbarItem) {
    [self reloadChildrenForBookmarkNode:parent];
    return;
  }

  [self cacheToolbarItemIfNeeded:toolbarItem];

  if ([self shouldResortChildrenForToolbarItem:toolbarItem]) {
    NSArray<VivaldiSpeedDialItem*>* currentChildren = toolbarItem.children;
    NSArray<VivaldiSpeedDialItem*>* resortedChildren =
        [self sortSpeedDials:currentChildren byMode:self.currentSortingMode];
    BOOL orderChanged = ![self children:currentChildren
                        haveSameOrderAs:resortedChildren];
    toolbarItem.children = resortedChildren;
    if (orderChanged) {
      [self.consumer refreshChildItems:toolbarItem.children parent:toolbarItem];
      return;
    }
  }

  [self.consumer refreshNode:bookmarkNode];
}

- (void)didChangeChildrenForNode:(const bookmarks::BookmarkNode*)bookmarkNode {
  if (_bookmarkModel.get() && _bookmarkModel->IsDoingExtensiveChanges()) {
    return;
  }

  // This method gets called when any item added/removed/or reordered.
  // TODO: @prio: When reordered by user we should skip observing this method.
  if (bookmarkNode->is_folder()) {
    [self reloadChildrenForBookmarkNode:bookmarkNode];
  }
}

- (void)didMoveNode:(const bookmarks::BookmarkNode*)bookmarkNode
         fromParent:(const bookmarks::BookmarkNode*)oldParent
           toParent:(const bookmarks::BookmarkNode*)newParent {
  if (_bookmarkModel.get() && _bookmarkModel->IsDoingExtensiveChanges()) {
    return;
  }
  // No need to do a full refresh when movement happened within same folder.
  if (oldParent == newParent) {
    return;
  }

  // If the node that is moved is a group reload the toolbar beceause it can be
  // that a Group is removed. Otherwise, refresh the old and new parent.
  if (bookmarkNode->is_folder() && GetSpeeddial(bookmarkNode)) {
    [self computeTopToolbarItems];
  } else {
    [self reloadChildrenForBookmarkNode:oldParent];
    [self reloadChildrenForBookmarkNode:newParent];
  }
}

- (void)didDeleteNode:(const bookmarks::BookmarkNode*)node
           fromFolder:(const bookmarks::BookmarkNode*)folder {
  // No op since this is only called for us when items removed from trash
  // which has no UX with StartPage.
}

- (void)didChangeFaviconForNode:(const bookmarks::BookmarkNode*)bookmarkNode {
  if (_bookmarkModel.get() && _bookmarkModel->IsDoingExtensiveChanges()) {
    return;
  }

  // Only urls have favicons.
  if (!bookmarkNode->is_url())
    return;

  if (ShouldPresentFirstRunExperience()) {
    [self refreshContents];
  } else {
    [self.consumer refreshNode:bookmarkNode];
  }
}

- (void)bookmarkModelRemovedAllNodes {
  // No-op.
}

- (void)bookmarkMetaInfoChanged:(const bookmarks::BookmarkNode*)bookmarkNode {
  if (_bookmarkModel.get() && _bookmarkModel->IsDoingExtensiveChanges()) {
    return;
  }

  if (bookmarkNode->is_folder()) {
    [self refreshContents];
  }
}

- (void)extensiveBookmarkChangesEnded {
  [self computeTopToolbarItems];
}

#pragma mark - VivaldiMostVisitedSitesConsumer
- (void)setMostVisitedTilesConfig:(MostVisitedTilesConfig*)config {
  _mostVisitedConfig = config;
  [self.consumer topSitesModelDidLoad];
  [self reloadChildrenForTopSite];
}

@end
