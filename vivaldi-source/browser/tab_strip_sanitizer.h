// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#ifndef BROWSER_TAB_STRIP_SANITIZER_H_
#define BROWSER_TAB_STRIP_SANITIZER_H_

class TabStripModel;

namespace vivaldi {
// Ensure there is only 1 group with the unique ID.
void SanitizeGroupSplit(TabStripModel* tab_strip);

// So far deletes stacks with 1 member.
void SanitizeGroups(TabStripModel* tab_strip);

// Call all the sanitizers on the all tab-strips.
void SanitizeAllTabs();
}  // namespace vivaldi

#endif
