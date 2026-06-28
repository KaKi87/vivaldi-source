// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#include "browser/tab_strip_sanitizer.h"

#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace {
struct GroupRun {
  int start = 0;
  int length = 0;
};
}  // namespace

namespace vivaldi {

// If the group splits due to moving tabs and you get the group of the same id
// in 2 splits like [ g g x g g ], keep just 1 split, so [ g g x x x ] to keep
// the group consistent.
//
// It keeps the largest part. So, ggxggg => xxxggg.
void SanitizeGroupSplit(TabStripModel* tab_strip) {
  CHECK(tab_strip);

  absl::flat_hash_map<std::string, std::vector<GroupRun>> runs_by_group;

  int index = 0;
  while (index < tab_strip->count()) {
    tabs::TabInterface* tab = tab_strip->GetTabAtIndex(index);
    TabExtData* ext = TabExtData::Get(tab->GetContents());

    std::optional<std::string> group_id = ext->GetGroupId();
    if (!group_id) {
      ++index;
      continue;
    }

    int start = index;
    int length = 1;
    ++index;

    while (index < tab_strip->count()) {
      tabs::TabInterface* next_tab = tab_strip->GetTabAtIndex(index);
      TabExtData* next_ext = TabExtData::Get(next_tab->GetContents());
      std::optional<std::string> next_group_id = next_ext->GetGroupId();
      if (next_group_id != group_id)
        break;

      ++length;
      ++index;
    }

    runs_by_group[*group_id].push_back(GroupRun{
        .start = start,
        .length = length,
    });
  }

  for (const auto& [group_id, group_runs] : runs_by_group) {
    if (group_runs.size() <= 1) {
      continue;
    }

    int longest_run = 0;
    for (auto& run : group_runs) {
      if (run.length > longest_run) {
        longest_run = run.length;
      }
    }

    bool kept_longest_run = false;
    for (auto& run : group_runs) {
      if (run.length == longest_run && !kept_longest_run) {
        kept_longest_run = true;
        continue;
      }

      for (int i = 0; i < run.length; ++i) {
        tabs::TabInterface* tab = tab_strip->GetTabAtIndex(run.start + i);
        TabExtData::Get(tab->GetContents())->Ungroup();
      }
    }
  }
}

void SanitizeGroups(TabStripModel* tab_strip) {
  CHECK(tab_strip);
  absl::flat_hash_map<std::string, int> counts;

  for (tabs::TabInterface* tab : *tab_strip) {
    TabExtData* ext = TabExtData::Get(tab->GetContents());
    std::optional<std::string> group_id = ext->GetGroupId();
    if (group_id) {
      counts[*group_id]++;
    }
  }

  for (tabs::TabInterface* tab : *tab_strip) {
    TabExtData* ext = TabExtData::Get(tab->GetContents());
    std::optional<std::string> group_id = ext->GetGroupId();
    if (!group_id || counts[*group_id] > 1)
      continue;
    ext->Ungroup();
  }
}

void SanitizeAllTabs() {
  for (BrowserWindowInterface* browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip = browser->GetTabStripModel();
    if (!tab_strip) {
      continue;
    }
    SanitizeGroupSplit(tab_strip);
    SanitizeGroups(tab_strip);
  }
}

}  // namespace vivaldi
