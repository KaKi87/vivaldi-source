// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef BROWSER_TAB_POSITIONING_H_
#define BROWSER_TAB_POSITIONING_H_

#include <optional>

#include "browser/tab_positioning_prefs.h"
#include "ui/base/page_transition_types.h"

class TabStripModel;
class TabStripModelChange;

namespace content {
class WebContents;
}

namespace tabs {
class TabModel;
}

namespace vivaldi::tab_positioning {
struct TabBarState;
struct TabPosition {
  int index = -1;
  bool pinned = false;
};

std::optional<TabPosition> DetermineInsertionIndexFromState(
    TabStripModel* tab_strip,
    content::WebContents* contents,
    TabSource source,
    const TabBarState& state);

std::optional<TabPosition> DetermineInsertionIndex(
    TabStripModel* tab_strip,
    content::WebContents* contents,
    int add_types,
    ui::PageTransition transition,
    TabSource source);

std::optional<int> DetermineDuplicateIndex(TabStripModel* tab_strip,
                                           content::WebContents* origin,
                                           content::WebContents* contents,
                                           int add_types);

void HandleStacking(TabStripModel* tab_strip_model,
                    const TabStripModelChange& change);

bool IsEmailWebContents(content::WebContents* source_content);
void SetOpener(tabs::TabModel* tab,
               content::WebContents* contents,
               TabStripModel* tab_strip);
}  // namespace vivaldi::tab_positioning

#endif  // BROWSER_TAB_POSITIONING_H_
