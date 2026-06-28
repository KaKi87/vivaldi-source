// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#include "browser/tab_probe.h"

#include <optional>
#include <string>
#include <vector>
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"

namespace vivaldi {

namespace {
std::optional<std::string> GetExtDataString(
    const content::WebContents* contents,
    TabExtKey key) {
  if (!contents)
    return std::nullopt;
  auto* value = TabExtData::Get(contents)->Get(key);
  if (!value)
    return std::nullopt;
  const std::string* rv = value->GetIfString();
  if (rv) {
    return *rv;
  }
  return std::nullopt;
}
}  // namespace

namespace tab_probe {

std::optional<std::string> GetExtId(const TabProbe& probe) {
  return GetExtDataString(probe.contents, TabExtKey::kExtId);
}

std::optional<std::string> GetParentExtId(const TabProbe& probe) {
  return GetExtDataString(probe.contents, TabExtKey::kParentExtId);
}

std::optional<std::string> GetGroupId(const TabProbe& probe) {
  return GetExtDataString(probe.contents, TabExtKey::kGroupId_);
}

std::optional<std::string> GetGroupId(
    const std::vector<vivaldi::TabProbe>& probes) {
  if (probes.empty())
    return std::nullopt;
  std::optional<std::string> first = GetGroupId(probes[0]);
  if (!first)
    return std::nullopt;

  for (auto it = std::next(probes.begin()); it != probes.end(); ++it) {
    if (GetGroupId(*it) != first)
      return std::nullopt;
  }

  return first;
}

std::optional<std::string> GetPanelId(const TabProbe& probe) {
  return GetExtDataString(probe.contents, TabExtKey::kPanelId);
}

bool IsPinned(const TabProbe& probe) {
  CHECK(probe.tab_strip_model);
  return probe.tab_strip_model->IsTabPinned(probe.index);
}

std::optional<double> GetWorkspaceId(const TabProbe& probe) {
  if (!probe.contents)
    return std::nullopt;
  std::optional<double> rv = TabExtData::Get(probe.contents)->GetWorkspaceId();
  if (rv && *rv == 0)
    return std::nullopt;
  return rv;
}

BrowserWindowInterface* FindWorkspace(double workspace_id) {
  if (workspace_id == 0) {
    return nullptr;
  }
  for (BrowserWindowInterface* browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip = browser->GetTabStripModel();
    if (!tab_strip)
      continue;

    if (tab_strip->GetActiveWorkspace() == workspace_id) {
      return browser;
    }

    for (tabs::TabInterface* tab : *tab_strip) {
      content::WebContents* contents = tab->GetContents();
      TabExtData* ext = TabExtData::Get(contents);
      std::optional<double> wsid = ext->GetWorkspaceId();
      if (wsid && *wsid == workspace_id)
        return browser;
    }
  }

  return nullptr;
}

// --- ResolveTab Implementations ---
std::vector<TabProbe> ResolveTabs(
    const absl::flat_hash_set<int>& tab_ids,
    const absl::flat_hash_set<std::string>& ext_ids) {
  std::vector<TabProbe> result;
  for (BrowserWindowInterface* browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip = browser->GetTabStripModel();
    if (!tab_strip)
      continue;

    for (int i = 0; i < tab_strip->count(); ++i) {
      bool match = false;

      content::WebContents* contents = tab_strip->GetWebContentsAt(i);
      if (tab_ids.count(sessions::SessionTabHelper::IdForTab(contents).id())) {
        match = true;
      }

      if (!match) {
        TabExtData* ext = TabExtData::Get(contents);
        if (ext_ids.count(ext->GetExtId())) {
          match = true;
        } else {
          std::optional<std::string> group = ext->GetGroupId();
          if (group && ext_ids.count(*group)) {
            match = true;
          }
        }
      }

      if (!match)
        continue;

      TabProbe probe;
      probe.tab_strip_model = tab_strip;
      probe.contents = contents;
      probe.index = i;
      probe.tab_id = sessions::SessionTabHelper::IdForTab(contents).id();
      result.push_back(probe);
    }
  }
  return result;
}

std::vector<TabProbe> ResolveTabs(const absl::flat_hash_set<int>& tab_set) {
  absl::flat_hash_set<std::string> dummy;
  return ResolveTabs(tab_set, absl::flat_hash_set<std::string>());
}

std::vector<TabProbe> ResolveTabs(const std::vector<int>& tab_ids) {
  absl::flat_hash_set<int> tab_set(tab_ids.begin(), tab_ids.end());
  return ResolveTabs(tab_set);
}

std::optional<TabProbe> ResolveTab(int search_tab_id) {
  if (search_tab_id == -1) {
    return std::nullopt;
  }
  for (BrowserWindowInterface* temp_browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip_temp = temp_browser->GetTabStripModel();
    if (!tab_strip_temp)
      continue;

    for (int i = 0; i < tab_strip_temp->count(); ++i) {
      content::WebContents* current_contents =
          tab_strip_temp->GetWebContentsAt(i);
      if (!current_contents) {
        continue;
      }
      int current_tab_id =
          sessions::SessionTabHelper::IdForTab(current_contents).id();
      if (current_tab_id == search_tab_id) {
        TabProbe probe;
        probe.tab_strip_model = tab_strip_temp;
        probe.contents = current_contents;
        probe.index = i;
        probe.tab_id = current_tab_id;
        return probe;
      }
    }
  }
  return std::nullopt;
}

std::optional<TabProbe> ResolveTab(content::WebContents* search_contents) {
  if (!search_contents) {
    return std::nullopt;
  }

  for (BrowserWindowInterface* temp_browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip_temp = temp_browser->GetTabStripModel();
    if (!tab_strip_temp)
      continue;
    for (int i = 0; i < tab_strip_temp->count(); ++i) {
      content::WebContents* current_contents =
          tab_strip_temp->GetWebContentsAt(i);
      if (current_contents == search_contents) {
        TabProbe probe;
        probe.tab_strip_model = tab_strip_temp;
        probe.contents = current_contents;
        probe.index = i;
        probe.tab_id =
            sessions::SessionTabHelper::IdForTab(current_contents).id();
        return probe;
      }
    }
  }
  return std::nullopt;
}

std::optional<TabProbe> ResolveTabByExtId(const std::string& search_ext_id,
                                          bool* is_group) {
  if (is_group)
    *is_group = false;
  for (BrowserWindowInterface* temp_browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip_temp = temp_browser->GetTabStripModel();
    if (!tab_strip_temp)
      continue;

    for (int i = 0; i < tab_strip_temp->count(); ++i) {
      content::WebContents* current_contents =
          tab_strip_temp->GetWebContentsAt(i);
      if (!current_contents)
        continue;

      TabExtData* ext = TabExtData::Get(current_contents);

      bool found = false;
      if (search_ext_id == ext->GetExtId()) {
        found = true;
      } else {
        std::optional<std::string> group = ext->GetGroupId();
        if (group && search_ext_id == *group) {
          found = true;
          if (is_group) {
            *is_group = true;
          }
        }
      }

      if (found) {
        TabProbe probe;
        probe.tab_strip_model = tab_strip_temp;
        probe.contents = current_contents;
        probe.index = i;
        probe.tab_id =
            sessions::SessionTabHelper::IdForTab(current_contents).id();
        return probe;
      }
    }
  }
  return std::nullopt;
}

std::optional<TabProbe> TabLookup(int index, TabStripModel* tab_strip) {
  if (!tab_strip) {
    return std::nullopt;
  }

  content::WebContents* contents = tab_strip->GetWebContentsAt(index);
  if (!contents) {
    return std::nullopt;
  }

  TabProbe probe;
  probe.tab_strip_model = tab_strip;
  probe.contents = contents;
  probe.index = index;
  probe.tab_id = sessions::SessionTabHelper::IdForTab(contents).id();
  return probe;
}

std::optional<std::string> GetExtIdAt(TabStripModel* tab_strip, int index) {
  if (!tab_strip)
    return std::nullopt;

  return GetExtDataString(tab_strip->GetWebContentsAt(index),
                          TabExtKey::kExtId);
}

std::optional<std::string> GetParentExtIdAt(TabStripModel* tab_strip,
                                            int index) {
  if (!tab_strip)
    return std::nullopt;

  return GetExtDataString(tab_strip->GetWebContentsAt(index),
                          TabExtKey::kParentExtId);
}

TabProbe GetLastInGroup(TabProbe probe, bool reverse) {
  CHECK(probe.contents);
  std::optional<std::string> group = GetGroupId(probe);
  if (!group)
    return probe;
  const int increment = reverse ? -1 : 1;
  for (int i = probe.index + increment;; i += increment) {
    auto next_tab = tab_probe::TabLookup(i, probe.tab_strip_model);
    if (!next_tab)
      return probe;
    if (GetGroupId(*next_tab) == group) {
      probe = *next_tab;
      continue;
    }
    break;
  }
  return probe;
}

TabProbe GetLastInWorkspaceIgnorePin(TabProbe probe, bool reverse) {
  CHECK(probe.contents);
  const int increment = reverse ? -1 : 1;
  std::optional<double> workspace_id = GetWorkspaceId(probe);
  for (int i = probe.index + increment;; i += increment) {
    auto next_tab = tab_probe::TabLookup(i, probe.tab_strip_model);
    if (!next_tab || GetPanelId(*next_tab))
      return probe;

    if (GetWorkspaceId(*next_tab) == workspace_id) {
      probe = *next_tab;
    }
  }
}

TabProbe GetLastInWorkspace(TabProbe probe, bool reverse) {
  CHECK(probe.contents);
  const int increment = reverse ? -1 : 1;
  const bool pinned = IsPinned(probe);
  std::optional<double> workspace_id = GetWorkspaceId(probe);
  for (int i = probe.index + increment;; i += increment) {
    auto next_tab = tab_probe::TabLookup(i, probe.tab_strip_model);
    if (!next_tab || GetPanelId(*next_tab) || IsPinned(*next_tab) != pinned)
      return probe;

    if (GetWorkspaceId(*next_tab) == workspace_id) {
      probe = *next_tab;
    }
  }
}

std::optional<TabProbe> GetLastInWorkspace(TabStripModel* tab_strip,
                                           std::optional<double> workspace_id) {
  std::optional<TabProbe> probe;
  if (workspace_id && *workspace_id == 0)
    workspace_id = std::nullopt;

  for (int i = 0;; ++i) {
    auto next_tab = tab_probe::TabLookup(i, tab_strip);
    if (!next_tab || tab_probe::GetPanelId(*next_tab)) {
      return probe;
    }
    if (GetWorkspaceId(*next_tab) == workspace_id) {
      probe = *next_tab;
      continue;
    }
    // There can be gaps in the workspace, so we must iterate over the all tabs.
  }
}

std::optional<TabProbe> GetNext(const TabProbe& probe, bool reverse) {
  if (GetPanelId(probe)) {
    return std::nullopt;
  }
  int increment = reverse ? -1 : 1;
  std::optional<double> workspace = GetWorkspaceId(probe);
  for (int i = probe.index + increment;; i += increment) {
    std::optional<TabProbe> rv = TabLookup(i, probe.tab_strip_model);
    if (!rv)
      return std::nullopt;
    if (GetWorkspaceId(*rv) != workspace)
      continue;
    if (GetPanelId(*rv))
      return std::nullopt;
    return rv;
  }
}

std::pair<int, int> GetGroupRange(TabStripModel* tab_strip,
                                  std::string_view group_id) {
  int first = -1;
  int last = -1;
  for (int i = 0; i < tab_strip->count(); ++i) {
    content::WebContents* current_contents = tab_strip->GetWebContentsAt(i);
    if (!current_contents)
      continue;

    std::optional<std::string> grp =
        TabExtData::Get(current_contents)->GetGroupId();
    if (grp && *grp == group_id) {
      if (first == -1) {
        first = i;
      }
      last = i;
    }
  }
  return std::make_pair(first, last);
}

bool IsPinnedGroup(TabStripModel* tab_strip, const std::string& group_id) {
  for (int i = 0; i < tab_strip->IndexOfFirstNonPinnedTab(); ++i) {
    content::WebContents* contents = tab_strip->GetWebContentsAt(i);
    TabExtData* ext = TabExtData::Get(contents);
    if (ext->GetGroupId() == group_id)
      return true;
  }
  return false;
}

bool IsNextToGroup(const TabProbe& probe,
                   const std::string& group,
                   TabProbe* sample) {
  std::optional<TabProbe> next = GetNext(probe);
  if (next && GetGroupId(*next) == group) {
    if (sample)
      *sample = *next;
    return true;
  }

  next = GetNext(probe, true);
  if (next && GetGroupId(*next) == group) {
    if (sample)
      *sample = *next;
    return true;
  }

  return false;
}

std::optional<std::string> IdentifyGroup(
    const std::vector<::vivaldi::TabProbe>& probes) {
  std::optional<std::string> group = GetGroupId(probes);

  if (!group)
    return std::nullopt;

  CHECK(!probes.empty());

  // Range where the group is in the tab-strip.
  auto range = GetGroupRange(probes[0].tab_strip_model, *group);
  if (range.second == range.first)
    return std::nullopt;
  const int range_size = range.second - range.first + 1;

  // The group is bigger.
  if (static_cast<int>(probes.size()) < range_size)
    return std::nullopt;

  // Handle possible duplicates.
  absl::flat_hash_set<int> indices;
  for (auto& probe : probes) {
    indices.insert(probe.index);
  }

  if (static_cast<int>(indices.size()) != range_size)
    return std::nullopt;

  return group;
}

std::optional<TabProbe> GetLastInActiveWorkspace(TabProbe probe) {
  return GetLastInActiveWorkspace(probe.tab_strip_model);
}

std::optional<TabProbe> GetLastInActiveWorkspace(TabStripModel* tab_strip) {
  if (!tab_strip)
    return std::nullopt;
  double workspace_id = tab_strip->GetActiveWorkspace();
  std::optional<TabProbe> probe;
  for (int i = 0; i < tab_strip->count(); ++i) {
    std::optional<TabProbe> next_tab = tab_probe::TabLookup(i, tab_strip);
    if (!next_tab || tab_probe::GetPanelId(*next_tab)) {
      return probe;
    }
    if (GetWorkspaceId(*next_tab).value_or(0) == workspace_id) {
      probe = *next_tab;
    }
  }
  return std::nullopt;
}

std::optional<TabProbe> FindByPurpose(TabStripModel* tab_strip,
                                      TabPurpose purpose) {
  std::optional<TabProbe> rv;
  double active_workspace = tab_strip->GetActiveWorkspace();
  for (int i = 0; i < tab_strip->count(); ++i) {
    std::optional<TabProbe> probe = TabLookup(i, tab_strip);
    if (!probe)
      break;

    TabExtData* ext = TabExtData::Get(probe->contents);

    if (ext->GetPurpose() != purpose)
      continue;

    rv = probe;

    // Find the email tab that is preferably on the
    // active workspace.
    if (ext->GetWorkspaceId().value_or(0) == active_workspace)
      break;
  }
  return rv;
}
}  // namespace tab_probe
}  // namespace vivaldi
