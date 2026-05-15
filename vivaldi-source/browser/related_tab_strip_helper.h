// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#ifndef BROWSER_RELATED_TAB_STRIP_HELPER_H_
#define BROWSER_RELATED_TAB_STRIP_HELPER_H_

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "browser/tab_probe.h"
#include "ui/base/page_transition_types.h"

class Browser;
class TabStripModel;
class TabStripModelChange;

namespace content {
class WebContents;
}

namespace vivaldi {

namespace related_tabs {

enum TabSource {
  kGeneral,
  kExternalApp,
};

// Translate ext_ids to tab_ids. The ext_id may be either a tab ext_id or the
// group ext_id. In case of group ext_id all the tabs within the group would
// be added to the result.
std::vector<int> GetTabIdsFromExtIds(const std::vector<std::string>& ext_ids);

// Given a list of tabs. The result is an expanded list of tabs that are the
// smallest related tabs subtree. In other words, when a user grabs a node
// they want to move the whole subtree where the grabbed node is its root.
// This function calculates all the subtree nodes. The nodes are returned in
// the order in which they should be placed back to the tabstrip.
std::vector<TabProbe> Expand(const std::vector<TabProbe>& tab_probes,
                             std::optional<std::string>* error);

// When a tab is deleted, its children need a new parent. This function
// assigns new parents to the orphans.
void HandleOrphans(TabStripModel* tab_strip_model,
                   const TabStripModelChange& change);

// Handles cases:
// - Handle case when a tab is dropped into the group.
// - When a tab is created and there is a wish to put it in group with parent.
// - Deletes 1-member groups (calls SanitizeGroups)
void HandleGroups(TabStripModel* tab_strip_model,
                  const TabStripModelChange& change);

// Takes a tab as a root of the related-tabs tree and returns the last node in
// the tree.
//
// The related tabs tree is always topologically sorted. So, instead of
// returning the list of the expanded nodes, only the index of the last is
// needed. The actual tree is in between them.
// WARNING: There could be tabs from the different workspaces in between.
std::optional<int> GetLastInTree(int index, TabStripModel* tab_strip);
int GetLastInTree(const TabProbe& probe);

std::optional<int> DetermineInsertionIndex(TabStripModel* tab_strip,
                                           content::WebContents* contents,
                                           int add_types,
                                           ui::PageTransition transition,
                                           TabSource source);

std::optional<int> DetermineDuplicateIndex(TabStripModel* tab_strip,
                                           content::WebContents* origin,
                                           content::WebContents* contents,
                                           int add_types);

std::pair<int, int> CountPinned(const std::vector<TabProbe>& probes);

// Disable vivaldi sanitizer while the tabs are moving and we are responsible
// for the tab-strip consistency.
class VivaldiSanitizerGuard {
 public:
  VivaldiSanitizerGuard() = default;
  explicit VivaldiSanitizerGuard(TabStripModel* tab_strip);
  ~VivaldiSanitizerGuard();
  void Insert(TabStripModel* tab_strip);

  // Do not explicitly sanitize in destructor.
  bool postponed_sanitize = true;

 private:
  base::flat_set<TabStripModel*> tab_strips_;
};
}  // namespace related_tabs
}  // namespace vivaldi

#endif  // BROWSER_RELATED_TAB_STRIP_HELPER_H_
