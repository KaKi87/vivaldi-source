// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef BROWSER_TAB_POSITIONING_PREFS_H_
#define BROWSER_TAB_POSITIONING_PREFS_H_

#include <optional>

class PrefService;

namespace vivaldi::tab_positioning {

// Keep sync with mode in vivapp/src/prefs_definitions.json
enum struct TabPlacingStrategy {
  kRightOfCurrent = 0,             // rightofcurrent
  kDirectRightOfCurrent = 1,       // directrightofcurrent
  kAlwaysLast = 2,                 // alwayslast
  kOpenInTabstackWithRelated = 3,  // openintabstackwithrelated
};

enum class TabstackMode {
  kOff = 0,
  kDotted = 1,
  kSubstrip = 2,
  kAccordion = 3,
  kUnknown = 100,
};

enum TabSource {
  kGeneral,
  kExternalApp,
};

enum class ClonedTabPosition {
  kAlwaysLast,
  kRightToCurrent,
};

struct TabBarState {
  bool is_substrip_locked = false;
  // Open Tabs in Current Tab Stack
  bool open_in_current_tab_stack = true;
  TabSource source = TabSource::kGeneral;
  std::optional<TabstackMode> tab_stack_mode;
  std::optional<TabPlacingStrategy> placement_strategy;
};

ClonedTabPosition GetClonedTabPosition(PrefService* prefs);
TabBarState GetTabBarState(PrefService* prefs, TabSource source);

}  // namespace vivaldi::tab_positioning

#endif  // BROWSER_TAB_POSITIONING_PREFS_H_
