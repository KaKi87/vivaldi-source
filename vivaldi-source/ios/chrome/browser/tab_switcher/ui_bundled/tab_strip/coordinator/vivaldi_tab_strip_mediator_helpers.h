// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_CHROME_BROWSER_TAB_SWITCHER_UI_BUNDLED_TAB_STRIP_COORDINATOR_VIVALDI_TAB_STRIP_MEDIATOR_HELPERS_H_
#define IOS_CHROME_BROWSER_TAB_SWITCHER_UI_BUNDLED_TAB_STRIP_COORDINATOR_VIVALDI_TAB_STRIP_MEDIATOR_HELPERS_H_

#import <Foundation/Foundation.h>

#import <optional>

#import "components/prefs/ios/pref_observer_bridge.h"
#import "components/tab_groups/tab_group_id.h"
#import "ios/chrome/browser/shared/model/utils/observable_boolean.h"
#import "ios/chrome/browser/shared/model/web_state_list/tab_group_range.h"

@protocol TabStripConsumer;

@class TabStripItemData;
@class TabStripItemIdentifier;
@class TabStripTabItem;

class PrefService;
class ProfileIOS;
class TabGroup;
class WebStateList;
struct UrlLoadParams;

namespace web {
class WebState;
}  // namespace web

// Pref helper (observer-backed).
// Owns and manages Vivaldi tab strip related pref observers.
@interface VivaldiTabStripMediatorPrefsHelper
    : NSObject <BooleanObserver, PrefObserverDelegate>

- (instancetype)initWithConsumer:(id<TabStripConsumer>)consumer
           twoLevelActiveGroupId:
               (std::optional<tab_groups::TabGroupId>*)activeGroupId;

- (void)startObservingWithPrefService:(PrefService*)prefService;
- (void)stopObserving;
- (BOOL)isTwoLevelTabStacksEnabled;
- (NSString*)customAccentColor;

@end

// Helper functions (stateless).
// Returns Vivaldi's new tab URL load params using current preferences.
UrlLoadParams VivaldiNewTabURLLoadParams(ProfileIOS* profile);

// Returns the `TabStripItemIdentifier` for `web_state` using Vivaldi rules.
TabStripItemIdentifier* CreateTabItemIdentifier(web::WebState* web_state,
                                                WebStateList* web_state_list);

// Returns whether a tab item should be visible based on group collapse state.
bool VivaldiShouldIncludeTabItem(
    const TabGroup* group_of_web_state,
    bool including_hidden_tab_items,
    const std::optional<tab_groups::TabGroupId>& active_group_id);

// Returns whether `group` is the active group rendered in the secondary row of
// the two-level stacks UI.
bool VivaldiIsTwoLevelActiveGroup(
    const TabGroup* group,
    bool two_level_enabled,
    const std::optional<tab_groups::TabGroupId>& two_level_active_group_id);

// Returns the `TabStripItemIdentifier` elements for WebStates and TabGroups in
// `range` in `web_state_list`, including collapsed tabs from `active_group_id`.
NSMutableArray<TabStripItemIdentifier*>* VivaldiCreateItemIdentifiers(
    WebStateList* web_state_list,
    bool including_hidden_tab_items,
    bool including_group_items,
    TabGroupRange range,
    const std::optional<tab_groups::TabGroupId>& active_group_id);

// Returns the `TabStripItemIdentifier` elements for WebStates and TabGroups in
// `range` in `web_state_list`. If `including_groups` is set to false, then
// TabGroups are not included in the result.
NSMutableArray<TabStripItemIdentifier*>* CreateItemIdentifiers(
    WebStateList* web_state_list,
    bool including_hidden_tab_items = true,
    bool including_group_items = true,
    TabGroupRange range = TabGroupRange::InvalidRange());

// Returns whether `web_state` is pinned in `web_state_list`.
bool VivaldiIsPinnedWebState(WebStateList* web_state_list,
                             const web::WebState* web_state);

// Updates Vivaldi-specific fields on `data`.
void VivaldiUpdateTabStripItemData(TabStripItemData* data,
                                   WebStateList* web_state_list,
                                   const web::WebState* web_state,
                                   bool has_group);

// Updates Vivaldi-specific fields on `item`.
void VivaldiUpdateTabStripTabItemPinnedState(TabStripTabItem* item,
                                             WebStateList* web_state_list,
                                             const web::WebState* web_state);

// Returns visible item identifiers, including collapsed tabs from the active
// group when two-level stacks are enabled.
NSMutableArray<TabStripItemIdentifier*>* VivaldiVisibleItemIdentifiers(
    WebStateList* web_state_list,
    bool two_level_enabled,
    const std::optional<tab_groups::TabGroupId>& two_level_active_group_id);

// Returns the WebStateList index corresponding to dropping before
// `destination_item_index` in Vivaldi's visible-item order.
int VivaldiWebStateListInsertionIndexForItemIndex(
    NSArray<TabStripItemIdentifier*>* items,
    NSUInteger destination_item_index,
    bool two_level_enabled,
    const std::optional<tab_groups::TabGroupId>& two_level_active_group_id);

// Returns the active group that should keep ownership of a drop between
// `previous_item` and `next_item` in two-level mode. Returns nullptr when
// Vivaldi-specific two-level handling does not apply.
const TabGroup* VivaldiGroupForInsertionBetweenItems(
    WebStateList* web_state_list,
    TabStripItemIdentifier* previous_item,
    TabStripItemIdentifier* next_item,
    bool two_level_enabled,
    const std::optional<tab_groups::TabGroupId>& two_level_active_group_id);

#endif  // IOS_CHROME_BROWSER_TAB_SWITCHER_UI_BUNDLED_TAB_STRIP_COORDINATOR_VIVALDI_TAB_STRIP_MEDIATOR_HELPERS_H_
