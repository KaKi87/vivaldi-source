// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#ifndef BROWSER_TAB_PROBE_H_
#define BROWSER_TAB_PROBE_H_

#include <optional>
#include <string>
#include <vector>
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

class Browser;
class TabStripModel;
class BrowserWindowInterface;

namespace content {
class WebContents;
}

namespace vivaldi {

class TabExtData;
enum class TabExtKey;

// A simple struct holding information about a tab.
// This is an output of Resolve* methods. As it is a structure, it does not
// guard its values, so you should not expect its consistency when the tab
// position in the tabstrip changes or is removed.
struct TabProbe {
  TabStripModel* tab_strip_model = nullptr;
  content::WebContents* contents = nullptr;
  int index = -1;
  int tab_id = -1;
};

namespace tab_probe {
// The complexity of all the Resolve* functions is O(total number of tabs in
// the browsers).

// Resolves tab information from different identifiers.
// Returns a populated TabProbe struct if the tab is found.
std::optional<TabProbe> ResolveTab(int tab_id);
std::optional<TabProbe> ResolveTab(content::WebContents* contents);

// If is_group is not nullptr, it finds also the first tab which group is
// ext_id. *is_group is set true ext_id was a group.
std::optional<TabProbe> ResolveTabByExtId(const std::string& ext_id,
                                          bool* is_group = nullptr);

// Find the last tab in the group where the probe is one of its tabs.
TabProbe GetLastInGroup(TabProbe probe, bool reverse = false);

TabProbe GetLastInWorkspace(TabProbe probe, bool reverse = false);

// Find the last tab in the workspace where the probe is one of tabs.
std::optional<TabProbe> GetLastInWorkspace(TabStripModel* tab_strip,
                                           std::optional<double> workspace_id);

// Resolves multiple tab_ids - VERY IMPORTANT, It ensures result tabs to be
// sorted by the tuple [tabstrip, tab_index] and unique.
std::vector<TabProbe> ResolveTabs(const std::vector<int>& tab_ids);
std::vector<TabProbe> ResolveTabs(const absl::flat_hash_set<int>& tab_ids);
std::vector<TabProbe> ResolveTabs(
    const absl::flat_hash_set<int>& tab_ids,
    const absl::flat_hash_set<std::string>& ext_ids);

// Like ResolveTab, but O(1).
std::optional<TabProbe> TabLookup(int index, TabStripModel* tab_strip);

std::optional<std::string> GetExtIdAt(TabStripModel* tab_strip, int index);
std::optional<std::string> GetParentExtIdAt(TabStripModel* tab_strip,
                                            int index);

// Convenience getters for extended data.
std::optional<std::string> GetExtId(const TabProbe& probe);
std::optional<std::string> GetParentExtId(const TabProbe& probe);
std::optional<std::string> GetGroupId(const TabProbe& probe);
std::optional<std::string> GetPanelId(const TabProbe& probe);
std::optional<double> GetWorkspaceId(const TabProbe& probe);
bool IsPinned(const TabProbe& probe);

std::pair<int, int> GetGroupRange(TabStripModel* tab_strip,
                                  std::string_view group_id);

// Get next tab in the tabstrip. Respects workspaces.
std::optional<TabProbe> GetNext(const TabProbe& probe,
                                bool reverse = false);

BrowserWindowInterface * FindWorkspace(double workspace_id);

bool IsPinnedGroup(TabStripModel* tab_strip, const std::string& group_id);

}  // namespace tab_probe

}  // namespace vivaldi

#endif  // BROWSER_TAB_PROBE_H_
