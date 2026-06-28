// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.
#include "browser/related_tab_strip_helper.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/uuid.h"
#include "browser/tab_probe.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"
#include "components/ext_data/tab_positioning_params.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

#include "browser/tab_strip_sanitizer.h"

namespace vivaldi {
namespace {
// Normalizes a map of deleted nodes to their former parents.
//
// Each entry in `reparents` represents a node that was deleted:
//   { deleted_node -> former_parent }
//
// Some parents may also be deleted, forming chains like A->B, B->C.
// This function resolves such chains so that every deleted node
// directly points to its nearest *surviving* ancestor, or std::nullopt
// if the entire chain leads to the root.
//
// The function modifies `reparents` in place.
//
// Even if, the loops should not happen, Normalize function detects and handles
// them.
void Normalize(std::map<std::string, std::optional<std::string>>& reparents) {
  auto resolve = [&](const std::string& n,
                     absl::flat_hash_set<std::string>& visited,
                     auto&& self) -> std::optional<std::string> {
    if (visited.count(n)) {
      LOG(INFO) << "Cycle detected in tab reparenting for " << n;
      return std::nullopt;
    }
    visited.insert(n);

    auto it = reparents.find(n);
    if (it == reparents.end()) {
      // node not deleted -> return itself (a surviving node)
      return n;
    }
    auto& p = it->second;
    if (!p) {
      // deleted root
      return std::nullopt;
    }

    if (it->first == *p) {
      // This should never happen!
      LOG(ERROR) << "parent ext_id points to itself " << *p;
      return std::nullopt;
    }

    // parent might also be deleted, recurse
    std::optional<std::string> new_parent = self(*p, visited, self);

    // store the final surviving ancestor
    it->second = new_parent;
    return new_parent;
  };

  for (auto& [parent_id, new_parent_id] : reparents) {
    // By moving and deleting nodes, their order and relationships may
    // become quite messy. It appears that a cycle is possible here. From a
    // user-experience perspective, this causes no issues; however, we must
    // prevent a crash if it does happen.
    // TODO: Investigate how such a cycle may occur. This could be difficult.
    // The only case where I observed it was during shutdown, when all tabs
    // were being deleted.
    absl::flat_hash_set<std::string> visited;
    new_parent_id = resolve(parent_id, visited, resolve);
  }
}

std::optional<int> GetLastInTreeInternal(int index,
                                         TabStripModel* tab_strip,
                                         std::optional<double> workspace) {
  std::optional<TabProbe> info = tab_probe::TabLookup(index, tab_strip);
  if (!info) {
    return std::nullopt;
  }
  std::optional<std::string> root_id = tab_probe::GetExtId(*info);
  if (!root_id)
    return std::nullopt;

  std::optional<std::string> group = tab_probe::GetGroupId(*info);

  int last = index;
  int i = index + 1;
  while (i < tab_strip->count()) {
    info = tab_probe::TabLookup(i, tab_strip);
    if (tab_probe::GetWorkspaceId(*info) != workspace) {
      // Skip tabs from another workspaces. This is unfortunate as workspace
      // tabs may be mix together.
      i++;
      continue;
    }

    // Expand only within the group.
    if (info && tab_probe::GetGroupId(*info) != group)
      break;

    std::optional<std::string> parent_id =
        info ? tab_probe::GetParentExtId(*info) : std::nullopt;

    if (parent_id && *parent_id == *root_id) {
      // The node is one of the children of its parent.
      std::optional<int> sub_end =
          GetLastInTreeInternal(i, tab_strip, workspace);
      if (!sub_end) {
        // This should never happen. CHECK instead?
        break;
      }
      last = *sub_end;
      i = *sub_end + 1;
    } else {
      break;
    }
  }
  return last;
}

}  // namespace

namespace related_tabs {
std::optional<int> GetLastInTree(int index, TabStripModel* tab_strip) {
  std::optional<TabProbe> probe = tab_probe::TabLookup(index, tab_strip);
  if (!probe) {
    return std::nullopt;
  }
  std::optional<double> workspace = tab_probe::GetWorkspaceId(*probe);
  return GetLastInTreeInternal(index, tab_strip, workspace);
}

int GetLastInTree(const TabProbe& probe) {
  std::optional<int> last = GetLastInTree(probe.index, probe.tab_strip_model);
  return last.value_or(probe.index);
}

std::pair<int, int> CountPinned(const std::vector<TabProbe>& probes) {
  std::pair<int, int> rv(0, 0);
  for (const TabProbe& probe : probes) {
    if (tab_probe::IsPinned(probe)) {
      rv.first++;
    } else {
      rv.second++;
    }
  }
  return rv;
}

std::vector<TabProbe> Expand(const std::vector<TabProbe>& tab_probes,
                             std::optional<std::string>* error) {
  std::pair<int, int> pinned_unpinned = CountPinned(tab_probes);
  if (pinned_unpinned.first && pinned_unpinned.second) {
    if (error) {
      *error = "can't move pinned and unpinned together";
    }
    return std::vector<TabProbe>();
  }
  absl::flat_hash_set<int> expanded_tab_ids;

  for (const TabProbe& tab_info : tab_probes) {
    // [start, end] - Is the interval of the subtree within the tabstrip.
    int start = tab_info.index;
    std::optional<double> workspace = tab_probe::GetWorkspaceId(tab_info);
    int end = GetLastInTree(tab_info);

    // Check some invariants.
    CHECK(start <= end);
    CHECK(start >= 0);
    CHECK(end < tab_info.tab_strip_model->count());

    for (int i = start; i <= end; ++i) {
      std::optional<TabProbe> info =
          tab_probe::TabLookup(i, tab_info.tab_strip_model);
      CHECK(info);

      if (tab_probe::GetWorkspaceId(*info) != workspace)
        continue;

      // The tree of the pinned tabs can continue by the unpinned tabs.
      // We must cut them off.
      if (tab_probe::IsPinned(*info)) {
        if (pinned_unpinned.second)
          continue;
      } else {
        if (pinned_unpinned.first)
          continue;
      }

      expanded_tab_ids.insert(info->tab_id);
    }
  }

  return tab_probe::ResolveTabs(expanded_tab_ids);
}

void HandleOrphans(TabStripModel* tab_strip_model,
                   const TabStripModelChange& change) {
  if (change.type() != TabStripModelChange::kRemoved)
    return;

  // When a tab is deleted, we need to reparent its children.
  auto* removed = change.GetRemove();

  // children -> new parent relation. std::nullopt = no parent (root)
  std::map<std::string, std::optional<std::string>> reparents;

  // Iterate all the deleted tabs.
  for (auto& removed_tab : removed->contents) {
    // We don't clean parents when moving the tab to another window.
    // If this wasn't there, the children would lose their parent and it would
    // flatten the tree.
    if (removed_tab.remove_reason != TabRemovedReason::kDeleted) {
      continue;
    }

    // The node will be deleted. The child is to be passed to the parent of the
    // deleted parent.
    TabExtData* ext = TabExtData::Get(removed_tab.contents);
    std::optional<std::string> parent_ext_id = ext->GetParentExtId();
    std::string ext_id = ext->GetExtId();
    reparents[ext_id] = parent_ext_id;
  }

  if (reparents.empty())
    return;

  // A -> B -> C -> D and B, C were deleted, we need to reduce the relation to
  // A -> D
  Normalize(reparents);

  // Finally, rewrite the parents.
  for (tabs::TabInterface* tab : *tab_strip_model) {
    content::WebContents* contents = tab->GetContents();
    TabExtData* ext = TabExtData::Get(contents);
    std::optional<std::string> parent_ext_id = ext->GetParentExtId();
    if (!parent_ext_id)
      continue;
    auto it = reparents.find(*parent_ext_id);
    if (it == reparents.end())
      continue;

    if (it->second) {
      ext->Set(::vivaldi::TabExtKey::kParentExtId, *it->second);
    } else {
      ext->Remove(::vivaldi::TabExtKey::kParentExtId);
    }
  }
}

void HandleGroups(TabStripModel* tab_strip_model,
                  const TabStripModelChange& change) {
  std::vector<std::pair<content::WebContents*, int>> tab_contents;

  if (change.type() == TabStripModelChange::kInserted) {
    auto* tmp = change.GetInsert();
    for (auto& insert_tab : tmp->contents) {
      tab_contents.push_back(
          std::make_pair(insert_tab.contents, insert_tab.index));
    }
  }

  if (change.type() == TabStripModelChange::kMoved) {
    auto* tmp = change.GetMove();
    if (tmp->contents) {
      tab_contents.push_back(std::make_pair(tmp->contents, tmp->to_index));
    }
  }

  // Detect the case, when the tab is dropped into the group. In this case, it
  // should become the group member.
  for (auto affected : tab_contents) {
    TabExtData* ext = TabExtData::Get(affected.first);
    const int index = affected.second;

    std::optional<std::string> parent_id = ext->GetParentExtId();

    std::optional<std::string> group_id;

    std::optional<TabProbe> probe =
        tab_probe::TabLookup(index, tab_strip_model);

    if (!probe)
      continue;

    TabExtData* group_origin = nullptr;

    // The tab before.
    std::optional<TabProbe> up = tab_probe::GetNext(*probe, true);
    auto group_up = up ? tab_probe::GetGroupId(*up) : std::nullopt;

    // The tab after
    std::optional<TabProbe> down = tab_probe::GetNext(*probe);
    auto group_down = down ? tab_probe::GetGroupId(*down) : std::nullopt;

    if (group_up) {
      if (group_down && group_up == group_down) {
        group_origin = TabExtData::Get(down->contents);
      } else if (parent_id && up && tab_probe::GetExtId(*up) == parent_id) {
        group_origin = TabExtData::Get(up->contents);
      }
    }

    if (group_origin) {
      ext->JoinGroup(*group_origin);
    }
  }

  // It must be possible to disable the sanitizer.
  // For instance, this would delete all the groups during restore since the
  // groupId is restored one-by-one and as the size-1 groups are not allowed,
  // they would be all sanitized out.
  if (tab_strip_model->IsVivaldiSanitizerEnabled()) {
    vivaldi::SanitizeGroups(tab_strip_model);
  }
}

std::vector<int> GetTabIdsFromExtIds(const std::vector<std::string>& ext_ids) {
  std::vector<int> tab_ids;
  if (ext_ids.empty()) {
    return tab_ids;
  }
  absl::flat_hash_set<std::string> ext_ids_set(ext_ids.begin(), ext_ids.end());
  for (BrowserWindowInterface* temp_browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip = temp_browser->GetTabStripModel();
    if (!tab_strip)
      continue;
    for (tabs::TabInterface* tab : *tab_strip) {
      content::WebContents* c = tab->GetContents();
      int tab_id = sessions::SessionTabHelper::IdForTab(c).id();
      auto* ext = TabExtData::Get(c);
      auto group_id = ext->GetGroupId();
      if (group_id && ext_ids_set.count(*group_id)) {
        tab_ids.push_back(tab_id);
        continue;
      }
      if (ext_ids_set.count(ext->GetExtId())) {
        tab_ids.push_back(tab_id);
        continue;
      }
    }
  }
  return tab_ids;
}

VivaldiSanitizerGuard::VivaldiSanitizerGuard(TabStripModel* tab_strip) {
  Insert(tab_strip);
}

void VivaldiSanitizerGuard::Insert(TabStripModel* tab_strip) {
  if (!tab_strip || tab_strips_.count(tab_strip))
    return;

  tab_strips_.insert(tab_strip);
  tab_strip->AddVivaldiSanitizerGuardRef(1);
}

VivaldiSanitizerGuard::~VivaldiSanitizerGuard() {
  for (TabStripModel* tab_strip : tab_strips_) {
    tab_strip->AddVivaldiSanitizerGuardRef(-1);
    if (postponed_sanitize && tab_strip->IsVivaldiSanitizerEnabled()) {
      ::vivaldi::SanitizeGroups(tab_strip);
    }
  }
}

}  // namespace related_tabs
}  // namespace vivaldi
