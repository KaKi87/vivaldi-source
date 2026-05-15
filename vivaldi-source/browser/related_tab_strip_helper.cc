// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.
#include "browser/related_tab_strip_helper.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "app/vivaldi_apptools.h"
#include "base/uuid.h"
#include "browser/tab_probe.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"
#include "ui/base/page_transition_types.h"

#include "browser/tab_strip_sanitizer.h"

namespace vivaldi {
namespace {
static const char* kTabsNewPlacement = "vivaldi.tabs.new_placement";

// This code could be useful, keeping it here commented out just in case we
// need it in the future.
#if 0
static const char* kWindowSorting = "vivaldi.panels.window.sorting";
static const char* kSortField = "sortField";
static const char* kRelatedSorting = "related";

std::optional<bool> IsRelatedSorting(PrefService* prefs) {
  const PrefService::Preference* sorting_pref =
      prefs->FindPreference(kWindowSorting);
  if (!sorting_pref)
    return std::nullopt;

  auto* value = sorting_pref->GetValue();
  if (!value)
    return std::nullopt;

  auto* dict = value->GetIfDict();
  if (!dict)
    return std::nullopt;

  auto* s = dict->FindString(kSortField);
  if (!s)
    return std::nullopt;

  return *s == kRelatedSorting;
}
#endif

// Copy value from ext_data to another ext_data.
void CopyExtData(content::WebContents* target_contents,
                 ::vivaldi::TabExtKey key,
                 const TabExtData* ext_data) {
  if (!target_contents)
    return;

  CHECK(ext_data);

  const base::Value* source_value = ext_data->Get(key);
  if (source_value) {
    TabExtData::Get(target_contents)->Set(key, *source_value);
  } else {
    TabExtData::Get(target_contents)->Remove(key);
  }
}
std::optional<vivaldi::TabPlacingStrategy> GetPlacementStrategy(
    PrefService* prefs) {
  const PrefService::Preference* placement_pref =
      prefs->FindPreference(kTabsNewPlacement);
  if (!placement_pref)
    return std::nullopt;

  auto* value = placement_pref->GetValue();
  if (!value)
    return std::nullopt;

  std::optional<int> placement = value->GetIfInt();
  if (!placement)
    return std::nullopt;

  return ::vivaldi::TabPlacingStrategy(*placement);
}

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

// Choose the tab position for the "After Related Tabs" option.
std::optional<int> GetLastRelated(int index,
                                  TabStripModel* tab_strip,
                                  bool within_group = false) {
  std::optional<TabProbe> probe = tab_probe::TabLookup(index, tab_strip);
  if (!probe)
    return std::nullopt;

  std::optional<std::string> ext_id = tab_probe::GetExtId(*probe);
  std::optional<std::string> group = tab_probe::GetGroupId(*probe);

  if (!ext_id)
    return std::nullopt;

  for (;;) {
    std::optional<TabProbe> next_probe = tab_probe::GetNext(*probe);

    if (!next_probe)
      break;

    std::optional<TabProbe> after_next = tab_probe::GetNext(*next_probe);

    // The tab after next is a child.
    if (after_next && tab_probe::GetExtId(*next_probe) ==
                          tab_probe::GetParentExtId(*after_next)) {
      break;
    }

    if (within_group && tab_probe::GetGroupId(*next_probe) != group)
      break;

    auto parent = tab_probe::GetParentExtId(*next_probe);

    if (!parent || *parent != *ext_id)
      break;

    CHECK(probe->index < next_probe->index);
    probe = next_probe;
  }

  return probe->index;
}

std::optional<int> GetLastRelated(const TabProbe& probe,
                                  bool within_group = false) {
  std::optional<int> last =
      GetLastRelated(probe.index, probe.tab_strip_model, within_group);
  return last.value_or(probe.index);
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
  if (change.type() == TabStripModelChange::kInserted) {
    auto* insert = change.GetInsert();

    for (auto& insert_tab : insert->contents) {
      content::WebContents* contents = insert_tab.contents;
      CHECK(contents);

      TabExtData* ext = TabExtData::Get(contents);
      WeakExtData* weak_ext = ext->GetWeakExtData();

      CHECK(ext);
      CHECK(weak_ext);

      if (weak_ext->forced_parent_id) {
        ext->Set(TabExtKey::kParentExtId, *weak_ext->forced_parent_id);
      }

      if (weak_ext->workspace_id) {
        ext->Set(TabExtKey::kWorkspaceId, *weak_ext->workspace_id);
      }

      std::optional<std::string> group;

      if (weak_ext->stack_with_related) {
        std::optional<TabProbe> probe = tab_probe::TabLookup(insert_tab.index, tab_strip_model);
        if (probe) {
          std::optional<TabProbe> prev = tab_probe::GetNext(*probe, true);
          if (prev) {
            TabExtData* prev_ext = TabExtData::Get(prev->contents);
            group = prev_ext->GetGroupId();
            if (!group) {
              group = base::Uuid::GenerateRandomV4().AsLowercaseString();
              prev_ext->Set(TabExtKey::kGroupId, *group);
            }
          }
        }
      } else if (weak_ext->create_in_group_request) {
        group = weak_ext->create_in_group_request;
      }

      if (group) {
        ext->Set(TabExtKey::kGroupId, *group);
      }
    }
  }

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

    // The tab before.
    std::optional<TabProbe> up =
        tab_probe::TabLookup(index - 1, tab_strip_model);
    auto group_up = up ? tab_probe::GetGroupId(*up) : std::nullopt;

    // The tab after
    std::optional<TabProbe> down =
        tab_probe::TabLookup(index + 1, tab_strip_model);
    auto group_down = down ? tab_probe::GetGroupId(*down) : std::nullopt;

    TabProbe* used_probe = nullptr;

    if (group_up) {
      if (group_down && group_up == group_down) {
        group_id = group_down;
        if (down)
          used_probe = &(*down);
      } else if (parent_id && up && tab_probe::GetExtId(*up) == parent_id) {
        group_id = group_up;
        used_probe = &(*up);
      }
    }

    if (group_id) {
      ext->Set(::vivaldi::TabExtKey::kGroupId, *group_id);
      if (used_probe) {
        TabExtData* ext_data = TabExtData::Get(used_probe->contents);
        // kFixedGroupTitle and kGroupColor set together with the group.
        CopyExtData(affected.first, TabExtKey::kFixedGroupTitle, ext_data);
        CopyExtData(affected.first, TabExtKey::kGroupColor, ext_data);
      }
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

namespace {
bool IsVivaldiDecidingIndex(int add_types) {
  if ((add_types & ADD_INHERIT_OPENER) != 0)
    return true;

  if ((add_types & ADD_FORCE_INDEX) != 0)
    return false;

  return true;
}

std::optional<std::string> GetParentForNewTab(TabStripModel* tab_strip,
                                              content::WebContents* contents) {
  namespace tab_probe = ::vivaldi::tab_probe;

  std::optional<::vivaldi::TabProbe> prev =
      tab_probe::TabLookup(tab_strip->active_index(), tab_strip);

  if (!prev) {
    return std::nullopt;
  }

  std::optional<::vivaldi::TabProbe> next = tab_probe::GetNext(*prev);

  if (next && tab_probe::GetExtId(*prev) == tab_probe::GetParentExtId(*next)) {
    return tab_probe::GetExtId(*prev);
  }

  return ::vivaldi::TabExtData::Get(prev->contents)->GetParentExtId();
}
}  // namespace

std::optional<int> DetermineInsertionIndex(TabStripModel* tab_strip,
                                           content::WebContents* contents,
                                           int add_types,
                                           ui::PageTransition transition,
                                           TabSource source) {
  if (!::vivaldi::IsVivaldiRunning()) {
    return std::nullopt;
  }

  // Here we have all the information to decide where to
  // place the tab, so we don't need to move the tab back and
  // forth in JS.
  CHECK(tab_strip);
  CHECK(contents);

  std::optional<TabProbe> active_probe =
      tab_probe::TabLookup(tab_strip->active_index(), tab_strip);

  if (!active_probe) {
    return std::nullopt;
  }

  content::WebContents* active_contents = active_probe->contents;
  const TabExtData* active_ext = TabExtData::Get(active_contents);
  TabExtData* ext = TabExtData::Get(contents);
  ::vivaldi::WeakExtData* new_contents_weak_ext = ext->GetWeakExtData();

  if (!ext->HasWorkspaceIdSet()) {
    // Nobody has decided workspaceId for this tab. Take it from the active
    // tab.
    ext->Set(TabExtKey::kWorkspaceId, active_ext->GetWorkspaceId().value_or(0));
  }

  Profile* profile = tab_strip->profile();
  PrefService* prefs = profile->GetPrefs();

  CHECK(new_contents_weak_ext);
  const bool creating = new_contents_weak_ext->creating;

  if (creating) {
    new_contents_weak_ext->creating = false;
    auto parent = GetParentForNewTab(tab_strip, contents);
    if (parent) {
     ext->Set(TabExtKey::kParentExtId, *parent);
    }
  }

  if (new_contents_weak_ext->create_in_group_request) {
    // TODO: this probably can be done over transition.
    std::pair<int, int> range = tab_probe::GetGroupRange(
        tab_strip, *new_contents_weak_ext->create_in_group_request);
    return range.second + 1;
  }

  if (!creating) {
    if (!IsVivaldiDecidingIndex(add_types)) {
      return std::nullopt;
    }
  }

  // In vivaldi, the tab placement position is determined based on the
  // preferences/settings.
  std::optional<vivaldi::TabPlacingStrategy> placing_strategy;

  if (source == TabSource::kExternalApp) {
    // Tab from the external app will always open the last in the tabstrip.
    placing_strategy = TabPlacingStrategy::kAlwaysLast;
  } else {
    placing_strategy = GetPlacementStrategy(prefs);
  }

  if (!placing_strategy)
    return std::nullopt;

  if (creating) {  // Ctrl+T case
    switch (*placing_strategy) {
      // The "After Active Tab" rule applies for a new tab created by Ctrl+T,
      // but "After Related Tabs" does not. In this case we let Chromium
      // decide the new tab position itself. (It will basically create the tab
      // at the end of the tab strip.) This weird exception is here because we
      // want the same behavior as the original Google Chrome, and we call the
      // option "After Related Tabs".
      case TabPlacingStrategy::kDirectRightOfCurrent:
        break;
      default:
        return std::nullopt;
    }
  }

  switch (*placing_strategy) {
    case TabPlacingStrategy::kRightOfCurrent: {  // After Related Tabs
      std::optional<int> last_in_tree = GetLastRelated(*active_probe);
      if (last_in_tree)
        return *last_in_tree + 1;
      return std::nullopt;
    }
    case TabPlacingStrategy::kDirectRightOfCurrent:  // After Active Tab
      return tab_strip->active_index() + 1;
    case TabPlacingStrategy::kAlwaysLast:  // As Last Tab
      return tab_strip->count();
    case TabPlacingStrategy::kOpenInTabstackWithRelated:  // As Tab Stack with
                                                          // Related Tab
      // Do not group with pinned tab
      if (!tab_probe::IsPinned(*active_probe)) {
        new_contents_weak_ext->stack_with_related = true;
        std::optional<int> last = GetLastRelated(*active_probe, true);
        if (last)
          return *last + 1;
        return std::nullopt;
      }

      break;
  }
  return tab_strip->active_index() + 1;
}

std::optional<int> DetermineDuplicateIndex(TabStripModel* tab_strip,
                                           content::WebContents* origin,
                                           content::WebContents* contents,
                                           int add_type) {
  if (!::vivaldi::IsVivaldiRunning()) {
    return std::nullopt;
  }

  // TODO: this is a little simplistic. We may want to introduce settings for
  // where the duplicate should appear and what parent it should get. However,
  // this is the most intuitive way of doing this.
  TabExtData* origin_ext = TabExtData::Get(origin);
  ::vivaldi::WeakExtData* weak_ext =
      TabExtData::Get(contents)->GetWeakExtData();
  weak_ext->workspace_id = origin_ext->GetWorkspaceId();
  weak_ext->forced_parent_id = origin_ext->GetExtId();
  return std::nullopt;
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
