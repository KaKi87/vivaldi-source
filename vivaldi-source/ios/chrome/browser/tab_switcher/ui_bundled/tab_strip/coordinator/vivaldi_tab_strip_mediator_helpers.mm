// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_strip/coordinator/vivaldi_tab_strip_mediator_helpers.h"

#import <memory>

#import "base/check.h"
#import "components/prefs/ios/pref_observer_bridge.h"
#import "components/prefs/pref_change_registrar.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/browser/shared/model/prefs/pref_backed_boolean.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#import "ios/chrome/browser/tab_switcher/tab_strip/coordinator/tab_strip_mediator_utils.h"
#import "ios/chrome/browser/tab_switcher/tab_strip/ui/swift.h"
#import "ios/chrome/browser/tab_switcher/tab_strip/ui/tab_strip_tab_item.h"
#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_utils.h"
#import "ios/chrome/browser/url_loading/model/url_loading_params.h"
#import "ios/ui/settings/appearance/vivaldi_appearance_settings_prefs.h"
#import "ios/ui/settings/appearance/vivaldi_appearance_settings_prefs_helper.h"
#import "ios/ui/settings/tabs/vivaldi_tab_settings_helper.h"
#import "ios/web/public/web_state.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"
#import "url/gurl.h"

namespace {

void StopAndClearPrefBackedBoolean(PrefBackedBoolean* __strong* pref) {
  if (!pref || !*pref) {
    return;
  }
  [*pref stop];
  [*pref setObserver:nil];
  *pref = nil;
}

// Finds any TabGroup in `web_state_list` whose range starts at `index`.
const TabGroup* FindTabGroupStartingAtIndex(int index,
                                            WebStateList* web_state_list) {
  CHECK(web_state_list);
  for (const TabGroup* group : web_state_list->GetGroups()) {
    if (group->range().range_begin() == index) {
      return group;
    }
  }
  return nullptr;
}

}  // namespace

@interface VivaldiTabStripMediatorPrefsHelper ()

@property(nonatomic, weak) id<TabStripConsumer> consumer;

@end

@implementation VivaldiTabStripMediatorPrefsHelper {
  std::unique_ptr<PrefObserverBridge> _prefObserverBridge;
  PrefChangeRegistrar _prefChangeRegistrar;
  PrefBackedBoolean* _dynamicAccentColorEnabled;
  PrefBackedBoolean* _tabBarEnabled;
  PrefBackedBoolean* _showXButtonBackgroundTabsEnabled;
  std::optional<tab_groups::TabGroupId>* _twoLevelActiveGroupId;
  PrefService* _prefService;
  VivaldiTabStackStyle _tabStackStyle;
}

- (instancetype)initWithConsumer:(id<TabStripConsumer>)consumer
           twoLevelActiveGroupId:
               (std::optional<tab_groups::TabGroupId>*)activeGroupId {
  if ((self = [super init])) {
    _consumer = consumer;
    _twoLevelActiveGroupId = activeGroupId;
  }
  return self;
}

- (void)startObservingWithPrefService:(PrefService*)prefService {
  if (!prefService) {
    return;
  }
  _prefService = prefService;

  [VivaldiAppearanceSettingPrefs setPrefService:prefService];

  _tabBarEnabled = [[PrefBackedBoolean alloc]
      initWithPrefService:prefService
                 prefName:vivaldiprefs::kVivaldiDesktopTabsEnabled];
  [_tabBarEnabled setObserver:self];

  _showXButtonBackgroundTabsEnabled = [[PrefBackedBoolean alloc]
      initWithPrefService:prefService
                 prefName:vivaldiprefs::
                              kVivaldiShowXButtonBackgroundTabsEnabled];
  [_showXButtonBackgroundTabsEnabled setObserver:self];
  [self booleanDidChange:_showXButtonBackgroundTabsEnabled];

  _dynamicAccentColorEnabled = [[PrefBackedBoolean alloc]
      initWithPrefService:prefService
                 prefName:vivaldiprefs::kVivaldiDynamicAccentColorEnabled];
  [_dynamicAccentColorEnabled setObserver:self];
  [self booleanDidChange:_dynamicAccentColorEnabled];

  _prefChangeRegistrar.Init(prefService);
  _prefObserverBridge.reset(new PrefObserverBridge(self));
  _prefObserverBridge->ObserveChangesForPreference(
      vivaldiprefs::kVivaldiCustomAccentColor, &_prefChangeRegistrar);
  _prefObserverBridge->ObserveChangesForPreference(
      vivaldiprefs::kVivaldiTabStackStyle, &_prefChangeRegistrar);
  [self onPreferenceChanged:vivaldiprefs::kVivaldiCustomAccentColor];
  [self onPreferenceChanged:vivaldiprefs::kVivaldiTabStackStyle];
}

- (void)stopObserving {
  StopAndClearPrefBackedBoolean(&_showXButtonBackgroundTabsEnabled);
  StopAndClearPrefBackedBoolean(&_tabBarEnabled);
  StopAndClearPrefBackedBoolean(&_dynamicAccentColorEnabled);
  _prefChangeRegistrar.RemoveAll();
  _prefObserverBridge.reset();
  _prefService = nullptr;
  _tabStackStyle = VivaldiTabStackStyleAccordion;
  if (_twoLevelActiveGroupId) {
    *_twoLevelActiveGroupId = std::nullopt;
  }
}

- (BOOL)isTwoLevelTabStacksEnabled {
  return _tabStackStyle == VivaldiTabStackStyleTwoLevel;
}

- (NSString*)customAccentColor {
  return [VivaldiAppearanceSettingsPrefsHelper getCustomAccentColor];
}

#pragma mark - BooleanObserver

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  if (observableBoolean == _showXButtonBackgroundTabsEnabled) {
    [self.consumer setCloseButtonVisible:[observableBoolean value]];
  }

  if (observableBoolean == _tabBarEnabled) {
    [self.consumer setTabBarEnabled:[observableBoolean value]];
  }

  if (observableBoolean == _dynamicAccentColorEnabled) {
    [self.consumer setDynamicAccentColorEnabled:[observableBoolean value]];
  }
}

#pragma mark - PrefObserverDelegate

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  if (preferenceName == vivaldiprefs::kVivaldiCustomAccentColor) {
    [self.consumer setCustomAccentColor:[self customAccentColor]];
  } else if (preferenceName == vivaldiprefs::kVivaldiTabStackStyle) {
    if (_prefService) {
      _tabStackStyle = static_cast<VivaldiTabStackStyle>(
          _prefService->GetInteger(vivaldiprefs::kVivaldiTabStackStyle));
      [self.consumer setTabStackStyle:_tabStackStyle];
      if (_tabStackStyle != VivaldiTabStackStyleTwoLevel &&
          _twoLevelActiveGroupId) {
        *_twoLevelActiveGroupId = std::nullopt;
      }
    }
  }
}

@end

UrlLoadParams VivaldiNewTabURLLoadParams(ProfileIOS* profile) {
  UrlLoadParams params = UrlLoadParams::InNewTab(GURL(kChromeUINewTabURL));
  if (!profile || profile->IsOffTheRecord()) {
    return params;
  }
  PrefService* prefService = profile->GetPrefs();
  if (!prefService) {
    return params;
  }
  NSString* urlString =
      [VivaldiTabSettingsHelper getNewTabURLWithPref:prefService];
  return UrlLoadParams::InNewTab(GURL([urlString UTF8String]));
}

TabStripItemIdentifier* CreateTabItemIdentifier(web::WebState* web_state,
                                                WebStateList* web_state_list) {
  TabStripTabItem* tab_item =
      [[TabStripTabItem alloc] initWithWebState:web_state];
  tab_item.isPinned = VivaldiIsPinnedWebState(web_state_list, web_state);
  return [TabStripItemIdentifier tabIdentifier:tab_item];
}

bool VivaldiShouldIncludeTabItem(
    const TabGroup* group_of_web_state,
    bool including_hidden_tab_items,
    const std::optional<tab_groups::TabGroupId>& active_group_id) {
  if (!group_of_web_state) {
    return true;
  }
  if (!group_of_web_state->visual_data().is_collapsed()) {
    return true;
  }
  if (including_hidden_tab_items) {
    return true;
  }
  return active_group_id &&
         group_of_web_state->tab_group_id() == *active_group_id;
}

bool VivaldiIsTwoLevelActiveGroup(
    const TabGroup* group,
    bool two_level_enabled,
    const std::optional<tab_groups::TabGroupId>& two_level_active_group_id) {
  return group && two_level_enabled && two_level_active_group_id &&
         group->tab_group_id() == *two_level_active_group_id;
}

NSMutableArray<TabStripItemIdentifier*>* VivaldiCreateItemIdentifiers(
    WebStateList* web_state_list,
    bool including_hidden_tab_items,
    bool including_group_items,
    TabGroupRange range,
    const std::optional<tab_groups::TabGroupId>& active_group_id) {
  CHECK(web_state_list);
  if (!range.valid()) {
    range = {0, web_state_list->count()};
  }
  CHECK_GE(range.range_begin(), 0);
  CHECK_LE(range.range_end(), web_state_list->count());
  NSMutableArray<TabStripItemIdentifier*>* item_identifiers =
      [[NSMutableArray alloc] init];
  for (int index : range) {
    const TabGroup* group_of_web_state = nullptr;
    CHECK(web_state_list->ContainsIndex(index));
    group_of_web_state = web_state_list->GetGroupOfWebStateAt(index);
    if (including_group_items) {
      const TabGroup* group_starting_at_index =
          FindTabGroupStartingAtIndex(index, web_state_list);
      if (group_starting_at_index) {
        [item_identifiers
            addObject:CreateGroupItemIdentifier(group_starting_at_index)];
      }
    }

    // The tab associated with WebState at `index` should be included in the
    // output if it has no group, or its group is not collapsed, or
    // `including_hidden_tab_items` is true, or it belongs to the active group.
    const bool should_include_tab_item = VivaldiShouldIncludeTabItem(
        group_of_web_state, including_hidden_tab_items, active_group_id);
    if (should_include_tab_item) {
      web::WebState* web_state = web_state_list->GetWebStateAt(index);
      [item_identifiers
          addObject:CreateTabItemIdentifier(web_state, web_state_list)];
    }
  }
  return item_identifiers;
}

NSMutableArray<TabStripItemIdentifier*>* CreateItemIdentifiers(
    WebStateList* web_state_list,
    bool including_hidden_tab_items,
    bool including_group_items,
    TabGroupRange range) {
  return VivaldiCreateItemIdentifiers(
      web_state_list, including_hidden_tab_items, including_group_items, range,
      std::nullopt);
}

bool VivaldiIsPinnedWebState(WebStateList* web_state_list,
                             const web::WebState* web_state) {
  if (!web_state_list || !web_state) {
    return false;
  }
  web::WebState* pinned_web_state = GetWebState(
      web_state_list,
      WebStateSearchCriteria{
          .identifier = web_state->GetUniqueIdentifier(),
          .pinned_state = WebStateSearchCriteria::PinnedState::kPinned,
      });
  return pinned_web_state != nullptr;
}

void VivaldiUpdateTabStripItemData(TabStripItemData* data,
                                   WebStateList* web_state_list,
                                   const web::WebState* web_state,
                                   bool has_group) {
  if (!data) {
    return;
  }
  if (has_group) {
    data.isTabInGroup = YES;
  }
  data.isPinned = VivaldiIsPinnedWebState(web_state_list, web_state);
}

void VivaldiUpdateTabStripTabItemPinnedState(TabStripTabItem* item,
                                             WebStateList* web_state_list,
                                             const web::WebState* web_state) {
  if (!item) {
    return;
  }
  item.isPinned = VivaldiIsPinnedWebState(web_state_list, web_state);
}

NSMutableArray<TabStripItemIdentifier*>* VivaldiVisibleItemIdentifiers(
    WebStateList* web_state_list,
    bool two_level_enabled,
    const std::optional<tab_groups::TabGroupId>& two_level_active_group_id) {
  if (!two_level_enabled) {
    return VivaldiCreateItemIdentifiers(web_state_list,
                                        /*including_hidden_tab_items=*/false,
                                        /*including_group_items=*/true,
                                        TabGroupRange::InvalidRange(),
                                        std::nullopt);
  }

  CHECK(web_state_list);
  NSMutableArray<TabStripItemIdentifier*>* item_identifiers =
      [[NSMutableArray alloc] init];
  for (int index = 0; index < web_state_list->count(); ++index) {
    const TabGroup* group_of_web_state =
        web_state_list->GetGroupOfWebStateAt(index);
    if (const TabGroup* group_starting_at_index =
            FindTabGroupStartingAtIndex(index, web_state_list)) {
      [item_identifiers
          addObject:CreateGroupItemIdentifier(group_starting_at_index)];
    }

    // Two-level stacks render every group header in the primary row, then
    // render only the active group's tabs in the secondary row. Interleave that
    // active group's tabs directly after its header so drag/drop indices use
    // the same flattened order in the UI and mediator.
    const bool include_tab_item =
        !group_of_web_state ||
        VivaldiIsTwoLevelActiveGroup(group_of_web_state, two_level_enabled,
                                     two_level_active_group_id);
    if (include_tab_item) {
      web::WebState* web_state = web_state_list->GetWebStateAt(index);
      [item_identifiers
          addObject:CreateTabItemIdentifier(web_state, web_state_list)];
    }
  }
  return item_identifiers;
}

int VivaldiWebStateListInsertionIndexForItemIndex(
    NSArray<TabStripItemIdentifier*>* items,
    NSUInteger destination_item_index,
    bool two_level_enabled,
    const std::optional<tab_groups::TabGroupId>& two_level_active_group_id) {
  int web_state_list_insertion_index = 0;
  for (NSUInteger item_index = 0; item_index < destination_item_index;
       ++item_index) {
    if (items[item_index].itemType == TabStripItemTypeTab) {
      ++web_state_list_insertion_index;
      continue;
    }

    const TabGroup* group = items[item_index].tabGroupItem.tabGroup;
    const bool count_group_tabs_from_header =
        two_level_enabled
            ? !VivaldiIsTwoLevelActiveGroup(group, two_level_enabled,
                                            two_level_active_group_id)
            : group->visual_data().is_collapsed();
    if (count_group_tabs_from_header) {
      web_state_list_insertion_index += group->range().count();
    }
  }
  return web_state_list_insertion_index;
}

const TabGroup* VivaldiGroupForInsertionBetweenItems(
    WebStateList* web_state_list,
    TabStripItemIdentifier* previous_item,
    TabStripItemIdentifier* next_item,
    bool two_level_enabled,
    const std::optional<tab_groups::TabGroupId>& two_level_active_group_id) {
  if (!previous_item.tabSwitcherItem) {
    return nullptr;
  }

  const int index_of_previous_web_state = GetWebStateIndex(
      web_state_list,
      WebStateSearchCriteria{
          .identifier = previous_item.tabSwitcherItem.identifier,
      });
  if (!web_state_list->ContainsIndex(index_of_previous_web_state)) {
    return nullptr;
  }

  const TabGroup* group_of_previous_web_state =
      web_state_list->GetGroupOfWebStateAt(index_of_previous_web_state);
  if (!VivaldiIsTwoLevelActiveGroup(group_of_previous_web_state,
                                    two_level_enabled,
                                    two_level_active_group_id)) {
    return nullptr;
  }

  // In two-level mode, the active group's tabs are displayed outside the
  // primary row. Dropping after the last secondary-row tab still means "stay in
  // the active group", even though the next visible item is either nil or from
  // another group.
  if (!next_item.tabSwitcherItem) {
    return group_of_previous_web_state;
  }

  const int index_of_next_web_state = GetWebStateIndex(
      web_state_list, WebStateSearchCriteria{
                          .identifier = next_item.tabSwitcherItem.identifier,
                      });
  const TabGroup* group_of_next_web_state =
      web_state_list->ContainsIndex(index_of_next_web_state)
          ? web_state_list->GetGroupOfWebStateAt(index_of_next_web_state)
          : nullptr;
  return group_of_next_web_state != group_of_previous_web_state
             ? group_of_previous_web_state
             : nullptr;
}
