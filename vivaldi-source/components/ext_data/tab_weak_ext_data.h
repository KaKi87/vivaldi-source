// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_CONTENT_TAB_WEAK_EXT_DATA_H_
#define COMPONENTS_CONTENT_TAB_WEAK_EXT_DATA_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"

namespace content {
class WebContents;
}

namespace vivaldi {

// Copy of vivaldi.tabs.new_placement prefs. Keep the number equal to the prefs
// numbers!
enum struct TabPlacingStrategy {
  kRightOfCurrent = 0,             // rightofcurrent
  kDirectRightOfCurrent = 1,       // directrightofcurrent
  kAlwaysLast = 2,                 // alwayslast
  kOpenInTabstackWithRelated = 3,  // openintabstackwithrelated
};

// * WeakExtData is just a structure stored in TabExtData anybody can use for
//   anything in C++ code.
// * It does not persist.
// * It is used for communication between TabStripModel and the outer world.
// * Typically used for passing chosen prefs, particularly to determine a new
// tab position.
struct WeakExtData {
  // The target workspace for the new tab.
  std::optional<double> workspace_id;
  // When the tab is created, its parent would be enforced.
  // Used by chrome.tabs.duplicate().
  std::optional<std::string> forced_parent_id;

  std::optional<std::string> create_in_group_request;

  // True if the tab was created by calling the chrome.tab.create() API with
  // extData { create: true }. This is because we need to identify tabs opened
  // by Ctrl+T or "New Tab" from the menu.
  bool creating = false;

  // The new tab would like to be in stack with the parent.
  bool stack_with_related = false;
};

}  // namespace vivaldi

#endif  // COMPONENTS_CONTENT_TAB_WEAK_EXT_DATA_H_
