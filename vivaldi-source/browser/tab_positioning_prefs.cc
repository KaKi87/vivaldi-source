// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#include "browser/tab_positioning_prefs.h"

#include "base/logging.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "vivaldi/prefs/vivaldi_gen_pref_enums.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

namespace vivaldi::tab_positioning {
namespace {

const base::Value* GetValue(PrefService* prefs, const char* name) {
  const PrefService::Preference* placement_pref = prefs->FindPreference(name);
  if (!placement_pref)
    return nullptr;
  return placement_pref->GetValue();
}

bool IsSubstripLocked(PrefService* prefs) {
  constexpr bool default_value = false;
  auto* value = GetValue(prefs, vivaldiprefs::kTabsStackingSubstripLocked);
  if (!value)
    return default_value;
  return value->GetIfBool().value_or(default_value);
}

bool GetOpenInCurrentTabStack(PrefService* prefs) {
  constexpr bool default_value = true;
  auto* value = GetValue(prefs, vivaldiprefs::kTabsStackingOpenInCurrent);
  if (!value)
    return default_value;
  return value->GetIfBool().value_or(default_value);
}

std::optional<TabstackMode> GetTabStackMode(PrefService* prefs) {
  auto* value = GetValue(prefs, vivaldiprefs::kTabsStackingMode);
  if (!value)
    return std::nullopt;
  std::optional<int> i = value->GetIfInt();
  if (!i)
    return std::nullopt;
  return TabstackMode(*i);
}

std::optional<TabPlacingStrategy> GetPlacementStrategy(PrefService* prefs) {
  auto* value = GetValue(prefs, vivaldiprefs::kTabsNewPlacement);
  if (!value)
    return std::nullopt;
  std::optional<int> placement = value->GetIfInt();
  if (!placement)
    return std::nullopt;
  return TabPlacingStrategy(*placement);
}
}  // namespace

ClonedTabPosition GetClonedTabPosition(PrefService* prefs) {
  constexpr ClonedTabPosition default_value =
      ClonedTabPosition::kRightToCurrent;
  auto* value = GetValue(prefs, vivaldiprefs::kTabsActivationOnClone);
  if (!value)
    return default_value;

  std::optional<int> i = value->GetIfInt();
  if (!i) {
    return default_value;
  }

  using vivaldiprefs::TabsActivationOnCloneValues;

  switch (TabsActivationOnCloneValues(*i)) {
    case TabsActivationOnCloneValues::kRightofcurrent:
      return ClonedTabPosition::kRightToCurrent;
    case TabsActivationOnCloneValues::kAlwayslast:
      return ClonedTabPosition::kAlwaysLast;
    default:
      LOG(ERROR) << "invalid TabsActivationOnCloneValues: "
                 << vivaldiprefs::kTabsActivationOnClone << " = " << *i;
      return default_value;
  }
}

TabBarState GetTabBarState(PrefService* prefs, TabSource source) {
  TabBarState state;
  state.source = source;
  state.tab_stack_mode = GetTabStackMode(prefs);
  if (state.tab_stack_mode == TabstackMode::kSubstrip) {
    state.is_substrip_locked = IsSubstripLocked(prefs);
  }
  state.placement_strategy = GetPlacementStrategy(prefs);
  state.open_in_current_tab_stack = GetOpenInCurrentTabStack(prefs);
  return state;
}

}  // namespace vivaldi::tab_positioning
