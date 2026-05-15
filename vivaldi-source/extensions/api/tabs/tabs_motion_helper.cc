// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include "extensions/api/tabs/tabs_motion_helper.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "base/uuid.h"
#include "base/strings/string_number_conversions.h"
#include "browser/related_tab_strip_helper.h"
#include "browser/tab_probe.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/extensions/window_controller_list.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"
#include "components/sessions/content/session_tab_helper.h"

using namespace ::vivaldi::tab_probe;

namespace vivaldi {
namespace {
// No-reparent is also reparent. Setting it invalid means we are certain at the
// moment, the reparent_id should be none.
static constexpr const char* INVALID_EXT_ID = "-";

TabStripModel* GetTabStripForWindowId(int window_id) {
  if (window_id == -1)
    return nullptr;
  for (extensions::WindowController* window :
       *extensions::WindowControllerList::GetInstance()) {
    if (window->GetWindowId() == window_id) {
      Browser* browser = window->GetBrowser();
      return browser->tab_strip_model();
    }
  }
  return nullptr;
}

void CopyExtValue(TabExtKey key, TabExtData* target, TabExtData* source) {
  CHECK(target);
  const base::Value* value = source ? source->Get(key) : nullptr;
  if (value) {
    target->Set(key, *value);
  } else {
    target->Remove(key);
  }
}

bool IsDirectionTweak(TabsMotionHelper::TabMotionTweaks tweak) {
  switch(tweak) {
    case TabsMotionHelper::TabMotionTweaks::kBelow:
    case TabsMotionHelper::TabMotionTweaks::kAbove:
    case TabsMotionHelper::TabMotionTweaks::kOn:
      return true;
    default:
      return false;
  }
}

bool IsSortedByIndex(const std::vector<::vivaldi::TabProbe>& probes) {
  return std::is_sorted(
      probes.begin(), probes.end(),
      [](const auto& a, const auto& b) { return a.index < b.index; });
}

void LogLines(const std::vector<std::string> &lines, const char * prefix) {
  for (const std::string& line : lines) {
    LOG(INFO) << prefix << line;
  }
}
}  // namespace

TabsMotionHelper::TabsMotionHelper(Params params)
    : params_(std::move(params)) {
}

std::optional<std::string> TabsMotionHelper::CheckGroupChange() {
  if (is_step_
    && Has(TabMotionTweaks::kStripDown)
    && !GetTabProbes().empty())
  {
    std::optional<std::string> group =
      tab_probe::GetGroupId(GetTabProbes()[0]);

    if (group && SuggestGroup() != group) {
      return "can't change group";
    }
  }
  return std::nullopt;
}

void TabsMotionHelper::ForceInDirection(bool reverse) {
  if (reverse) {
    force_direction_ = TabsMotionHelper::TabMotionTweaks::kAbove;
  } else {
    force_direction_ = TabsMotionHelper::TabMotionTweaks::kBelow;
  }
}

void TabsMotionHelper::HandleMoveLeftRight(bool reverse) {
  if (reverse) {
    target_probe_ = tab_probe::GetNext(GetTabProbes().front(), reverse);
  } else {
    target_probe_ = tab_probe::GetNext(GetTabProbes().back(), reverse);
  }
  if (!target_probe_)
    return;

  if (Has(TabMotionTweaks::kStripUp)) {
    // Movig tab/group to the right/left, we need to jump over the group if we
    // are in the regular tab-strip. In the sub-strip, we're moveing withing the
    // group.
    target_probe_ = tab_probe::GetLastInGroup(*target_probe_, reverse);
  }

  ForceInDirection(reverse);
  is_step_ = true;
}

void TabsMotionHelper::HandleMoveFirstLast(bool reverse) {
  group_cache_valid_ = true;
  group_cache_ = tab_probe::GetGroupId(GetTabProbes().front());
  if (group_cache_) {
    target_probe_ = tab_probe::GetLastInGroup(GetTabProbes().front(), reverse);
  } else {
    target_probe_ = tab_probe::GetLastInWorkspace(GetTabProbes().front(), reverse);
  }
  ForceInDirection(reverse);
}

void TabsMotionHelper::RecognizeSpecialTarget(const std::string_view& target) {
  if (GetTabProbes().empty())
    return;

  if (target == "left") {
    HandleMoveLeftRight(true);
  } else if (target == "right") {
    HandleMoveLeftRight(false);
  } else if (target == "first") {
    HandleMoveFirstLast(true);
  } else if (target == "last") {
    HandleMoveFirstLast(false);
  }
}

std::optional<std::string> TabsMotionHelper::RecognizeTarget() {
  // Window id can be either included in the params, or deduced form the target
  // tab.
  int window_id = params_.move_properties.window_id.value_or(
      SessionID::InvalidValue().id());

  const bool target_is_tab = Has(TabMotionTweaks::kTargetIsTab);
  bool below = Has(TabMotionTweaks::kBelow);
  bool is_group = false;
  // The target could be:
  // 1 - enother tab defined by tab_id + above/below tweak
  // 2 - another tab defined by extId + above/below tweak
  // 3 - index
  //

  if (target_is_tab) {
    if (params_.move_properties.target.as_integer) {
      target_probe_ = ResolveTab(*params_.move_properties.target.as_integer);
    } else if (params_.move_properties.target.as_string) {
      RecognizeSpecialTarget(*params_.move_properties.target.as_string);

      if (!target_probe_) {
        target_probe_ = ResolveTabByExtId(
            *params_.move_properties.target.as_string, &is_group);
      } else {
        below = Has(TabMotionTweaks::kBelow);
      }
    }

    // Dragging a tab in the upper tab-strip (kStripUp), not dropping to another
    // tab or a group.
    // In this case, if the target is a member of a group, we must translate the
    // target to the group as we need the tab to be landed after or before this
    // group, so we don't care.
    if (target_probe_ && !Has(TabMotionTweaks::kOn) &&
        Has(TabMotionTweaks::kStripUp) && !is_group) {
      std::optional<std::string> group_redir =
          tab_probe::GetGroupId(*target_probe_);
      if (group_redir) {
        target_probe_ = ResolveTabByExtId(*group_redir, &is_group);
      }
    }

    if (!target_probe_) {
      return "can't resolve target";
    }

    if (is_group) {
      // The extId is group. So, it is above or below the group.
      if (!IsFollowingTarget()) {
        // Normal behavior is to fall into the group
        // TODO: this is confusing, investigate and simplify.
        target_group_ = *params_.move_properties.target.as_string;
      }
      // else ...
      if (below) {
        // Below the group means below its last tab.
        target_probe_ = tab_probe::GetLastInGroup(*target_probe_);
        CHECK(target_probe_);
      }
    } else {
      if (IsFollowingTarget()) {
        // Below or above a tab which is in the a group.
        target_group_ = tab_probe::GetGroupId(*target_probe_);
      }
    }

    window_id_ =
        extensions::ExtensionTabUtil::GetWindowIdOfTab(target_probe_->contents);

    if (window_id_ == SessionID::InvalidValue().id()) {
      return "invalid window";
    }

    // The target tab is determined. Now we need to shift the actual index
    // according to on/above/below tweaks. (above is by default)
    tab_strip_ = target_probe_->tab_strip_model;
    target_index_ = target_probe_->index;

    if (below && Has(TabMotionTweaks::kCollapsedAbove)) {
      // Dropping a tab below the collapsed node. We must skip the whole tree.
      target_index_ = related_tabs::GetLastInTree(*target_probe_);

      // By setting INVALID_EXT_ID we enforce root parent id.
      reparent_id_ =
          tab_probe::GetParentExtId(*target_probe_).value_or(INVALID_EXT_ID);
    }
  }

  if (!target_is_tab) {
    if (!params_.move_properties.target.as_integer) {
      return "invalid argument, missing index";
    }

    int idx = *params_.move_properties.target.as_integer;

    // Moving to the right from the first moving tab means moving below
    // unless we ask to move above explicitly.
    if (GetExpandedProbes()[0].index < idx && !Has(TabMotionTweaks::kAbove)) {
      below = true;
    }

    if (params_.move_properties.workspace_id.value_or(0) != 0) {
      double workspace_id = *params_.move_properties.workspace_id;
      BrowserWindowInterface* browser = FindWorkspace(workspace_id);
      if (browser) {
        window_id_ = browser->GetSessionID().id();
        tab_strip_ = browser->GetTabStripModel();
      }
    }

    if (window_id == SessionID::InvalidValue().id()) {
      CHECK(!tab_probes_.empty());
      tab_strip_ = tab_probes_[0].tab_strip_model;
      window_id_ = extensions::ExtensionTabUtil::GetWindowIdOfTab(
          tab_probes_[0].contents);
    } else {
      tab_strip_ = GetTabStripForWindowId(window_id);
      window_id_ = window_id;
    }

    if (!tab_strip_ || window_id_ == -1) {
      return "unknown target browser window";
    }

    if (idx == -1 || idx >= tab_strip_->count()) {
      // idx == -1 - the last tab in the workspace.
      std::optional<TabProbe> last_in_workspace =
          tab_probe::GetLastInWorkspace(tab_strip_, GetTargetWorkspaceId());
      if (last_in_workspace) {
        idx = last_in_workspace->index;
        below = true;
      } else if (idx == -1) {
        idx = 0;  // This can happen when moving to an empty workspace.
      } else {
        return "invalid target";
      }
    }

    target_index_ = idx;
  }

  if (below) {
    target_index_++;
  }

  CHECK(target_index_ >= 0);
  CHECK(tab_strip_);
  raw_target_index_ = target_index_;

  // Calculate how many tabs are moving from above and under the target index.
  std::pair<int, int> up_down = CountBeforeAndAfter();

  if (Has(TabMotionTweaks::kOn) && !target_group_) {
    std::optional<TabProbe> stick_on_info =
        TabLookup(target_index_, tab_strip_);
    if (!stick_on_info) {
      return "can't resolve on-target";
    }

    if (up_down.second > 0) {
      // There are tabs under the target index, we are moving up.
      // The tab would land on the target index, but we need
      // to land "on", so under.
      target_index_++;
    }

    reparent_id_ = tab_probe::GetExtId(*stick_on_info);
  } else {
    if (up_down.first > 0 && up_down.second == 0) {
      target_index_--;
    }
    CHECK(target_index_ >= 0);
  }

  if (!tab_probe::IsPinned(GetExpandedProbes()[0])) {
    // Can't move unpinned to pinned. Shift the target index to the first
    // unpined.
    std::optional<TabProbe> probe = TabLookup(target_index_, tab_strip_);
    while (probe && tab_probe::IsPinned(*probe)) {
      probe = TabLookup(++target_index_, tab_strip_);
    }
  }

  // Moving last tab in the tree under itself does not change the tab position,
  // but it changes its parent.
  // TODO: Consider an edge case when the tabs are not a sequence. In this case,
  // changing parent could break the tree.
  if (MovingOverSelf() && !Has(TabMotionTweaks::kOn)) {
    vivaldi::TabProbe last = GetLast();
    std::optional<TabProbe> next_probe = GetNext(last);
    if (!next_probe) {
      reparent_id_ = std::nullopt;
    } else {
      reparent_id_ = GetParentExtId(*next_probe);
    }
  }

  UpdateTargetIndexByTweaks();
  return std::nullopt;
}

void TabsMotionHelper::UpdateTargetIndexByTweaks() {
  if (Has(TabMotionTweaks::kCreateNewGroup)) {
    std::optional<TabProbe> probe =
        tab_probe::TabLookup(target_index_, tab_strip_);
    if (!probe)
      return;
    std::optional<std::string> group_id = tab_probe::GetGroupId(*probe);
    if (!group_id)
      return;
    std::pair<int, int> range =
        tab_probe::GetGroupRange(probe->tab_strip_model, *group_id);
    if (range.first == -1)
      return;
    // FIXME: select a tab from a stack + tab outside of the stack and create a
    // stack from those tabs => the index is wrong. It is difficult to figure
    // out why. This is an edge case, but would be nice to have it fixed.
    target_index_ = range.second;
  }
}

bool TabsMotionHelper::IsFollowingTarget() const {
  if (Has(TabMotionTweaks::kOn))
    return false;
  return Has(TabMotionTweaks::kGroupFollowsTarget);
}

TabProbe TabsMotionHelper::GetLast() const {
  size_t size = GetExpandedProbes().size();
  CHECK(size > 0);
  return GetExpandedProbes()[size - 1];
}

std::optional<std::string> TabsMotionHelper::UpdateGroupsCount() {
  auto& expanded_probes = GetExpandedProbes();
  for (BrowserWindowInterface* temp_browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip = temp_browser->GetTabStripModel();
    if (!tab_strip)
      continue;

    for (tabs::TabInterface* tab : *tab_strip) {
      content::WebContents* contents = tab->GetContents();

      auto group_id = TabExtData::Get(contents)->GetGroupId();

      if (!group_id)
        continue;

      int tab_id = sessions::SessionTabHelper::IdForTab(contents).id();

      auto it = std::find_if(
          expanded_probes.begin(), expanded_probes.end(),
          [tab_id](const TabProbe& probe) { return probe.tab_id == tab_id; });

      if (it != expanded_probes.end()) {
        groups_expanded_count_[*group_id]++;
      }

      groups_count_[*group_id]++;
    }
  }
  return std::nullopt;
}

int TabsMotionHelper::GetGroupsCount(const std::string& group_ext_id) const {
  auto it = groups_count_.find(group_ext_id);
  if (it == groups_count_.end())
    return 0;
  return it->second;
}

int TabsMotionHelper::GetGroupsExpandedCount(
    const std::string& group_ext_id) const {
  auto it = groups_expanded_count_.find(group_ext_id);
  if (it == groups_expanded_count_.end())
    return 0;
  return it->second;
}

const std::optional<std::string>& TabsMotionHelper::SuggestGroup() const {
  CHECK(group_cache_valid_);
  return group_cache_;
}

// Set suggested group to the target ext_data. It also sets kGroupColor anb
// kFixedGroupTitle according to the group state.
void TabsMotionHelper::ConfigureGroup(const vivaldi::TabProbe& probe) const {
  TabExtData* ext_data = TabExtData::Get(probe.contents);

  CHECK(ext_data);

  std::optional<std::string> group = SuggestGroup();
  if (!group) {
    return;
  }

  TabExtData* source = nullptr;
  if (group_sample_) {
    source = TabExtData::Get(group_sample_->contents);
  }

  ext_data->Set(::vivaldi::TabExtKey::kGroupId, *group);
  CopyExtValue(TabExtKey::kGroupColor, ext_data, source);
  CopyExtValue(TabExtKey::kFixedGroupTitle, ext_data, source);
}

std::optional<std::string> TabsMotionHelper::RecognizeTargetGroup() {
  if (group_cache_valid_) {
    return std::nullopt;
  }
  group_cache_valid_ = true;

  if (Has(TabMotionTweaks::kCreateNewGroup)) {
    group_cache_ = base::Uuid::GenerateRandomV4().AsLowercaseString();
    new_group_ = true;
    return std::nullopt;
  }

  group_cache_ = SuggestGroupInternal();
  if (!group_cache_ && Has(TabMotionTweaks::kCreateGroup)) {
    group_cache_ = base::Uuid::GenerateRandomV4().AsLowercaseString();
    new_group_ = true;
    return std::nullopt;
  }

  if (!group_cache_) {
    return std::nullopt;
  }

  // We need a sample tab from the group.
  bool is_group = false;
  group_sample_ = ResolveTabByExtId(*group_cache_, &is_group);
  if (!is_group) {
    group_sample_ = std::nullopt;
  }
  return std::nullopt;
}

// Pinned groups are headache. We must ensure the pinned tabs cannot be added to
// unpinned groups, and vice versa.
std::optional<std::string> TabsMotionHelper::HandlePinning() {
  if (group_cache_valid_ || new_group_ || !group_cache_)
    return std::nullopt;

  if (tab_probe::IsPinnedGroup(GetTargetTabStrip(), *group_cache_)) {
    if (!moving_pinned_) {
      group_cache_ = std::nullopt;
    }
  } else {
    if (moving_pinned_) {
      group_cache_ = std::nullopt;
    }
  }
  return std::nullopt;
}

// The tabs are ready to move. Now we need to decide, into what group.
std::optional<std::string> TabsMotionHelper::SuggestGroupInternal() const {
  if (!Has(TabMotionTweaks::kOn) && Has(TabMotionTweaks::kStripUp)) {
    // The user is dropping the tab in between another two tabs in the upper
    // tab-strip. This operation will never put the tab into a group.
    return std::nullopt;
  }

  if (is_step_ && Has(TabMotionTweaks::kStripUp)) {
    return std::nullopt;
  }

  TabExtData* ext_data_above = GetExtDataClassRelative(-1);
  std::string ext_id_above;
  std::optional<std::string> group_above;
  if (ext_data_above) {
    ext_id_above = ext_data_above->GetExtId();
    group_above = ext_data_above->GetGroupId();
  }
  TabExtData* ext_data_below = GetExtDataClassRelative(0);
  std::optional<std::string> group_below;
  if (ext_data_below) {
    group_below = ext_data_below->GetGroupId();
  }

  // Above and below the target is the same group, we set this group. No
  // discussion! Prevents group inconsistency.
  // THINK TWICE BEFORE CHANGING THIS!
  if (group_above && group_above == group_below) {
    return group_above;
  }

  if (IsFollowingTarget()) {
    return target_group_;
  }

  // Collapsed group node above => no group
  if (Has(TabMotionTweaks::kCollapsedAbove)) {
    return std::nullopt;
  }

  // There are 2 different group above and below. The hint help us decide which
  // one we should take.
  if (params_.move_properties.group_hint) {
    if ((group_above && *params_.move_properties.group_hint == *group_above) ||
        (group_below && *params_.move_properties.group_hint == *group_below)) {
      return *params_.move_properties.group_hint;
    }
  }

  // Target ext_id is a group (typically a group node in the WindowTree).
  if (target_group_) {
    // Moving tab ON the group node = obviously the group.
    if (Has(TabMotionTweaks::kOn))
      return *target_group_;

    return std::nullopt;
  }

  if (Has(TabMotionTweaks::kOn)) {
    return group_below;
  }

  TabExtData* first_moving_ext = TabExtData::Get(GetFirstMove().contents);

  if (!group_above && group_below) {
    // The target is just above a group.
    if (Has(TabMotionTweaks::kInto)) {
      // so we need this tweak. We need to choose between those cases:
      // a * [ b c ]
      // a [ * b c ]
      // By just the given position, we can't decide, whether to drop the tab
      // into the group. This is why we need the tweak.
      return *group_below;
    }
    // Above first in group.
    return std::nullopt;
  }

  // moving below the last tab in the group
  if (group_above && !group_below) {
    // This is for better user experience.
    // The user drags the tab just below itself. As this is the last tab in the
    // group, he probably wants to move the tab out of the group.
    if (first_moving_ext->GetExtId() == ext_id_above) {
      return std::nullopt;
    }

    if (common_group_ && GetMoves().size() > 1) {
      return common_group_;
    }

    return *group_above;
  }

  std::optional<std::string> moving_group = first_moving_ext->GetGroupId();

  if (Has(TabMotionTweaks::kBelow) && !group_below) {
    return std::nullopt;
  }

  // A [ B  C ] D - moving above B, tha tab will land in the group.
  if (group_below && Has(TabMotionTweaks::kAbove))
    return *group_below;

  if (moving_group &&
      (moving_group == group_above || moving_group == group_below)) {
    return moving_group;
  }

  return std::nullopt;
}

bool TabsMotionHelper::IsVoidMotion() {
  // Detect when the entire group is moving just next to itself. So, it does not
  // move. This prevents useless callback while dragging the group-tab over the
  // tab-bar.
  //
  // If all the moving tabs are within one group, common_group would be this
  // group.

  if (!common_group_)
    return false;

  auto* ext_data_above = GetExtDataClassRelative(-1);
  auto* ext_data_below = GetExtDataClassRelative(0);
  TabExtData* temp = nullptr;

  if (ext_data_above && ext_data_above->GetGroupId() == common_group_)
    temp = ext_data_above;
  if (ext_data_below && ext_data_below->GetGroupId() == common_group_)
    temp = ext_data_below;
  if (GetGroupsCount(*common_group_) !=
      static_cast<int>(GetExpandedProbes().size()))
    temp = nullptr;

  bool has_explicit = false;
  std::optional<double> explicit_workspace_id =
      params_.move_properties.workspace_id;
  if (explicit_workspace_id) {
    has_explicit = true;
    if (*explicit_workspace_id == 0) {
      explicit_workspace_id = std::nullopt;
    }
  }

  if (temp && has_explicit && temp->GetWorkspaceId() != explicit_workspace_id)
    temp = nullptr;

  if (temp) {
    return true;
  }

  return false;
}

const std::vector<vivaldi::TabProbe>& TabsMotionHelper::GetTabProbes() const {
  return tab_probes_;
}

const std::vector<vivaldi::TabProbe>& TabsMotionHelper::GetExpandedProbes()
    const {
  return expanded_tab_probes_;
}

bool TabsMotionHelper::Has(TabMotionTweaks tweak) const {
  if (force_direction_ && IsDirectionTweak(tweak)) {
    return tweak == *force_direction_;
  }

  const auto& move_properties = params_.move_properties;

  // If the target is a string, it is obvious, it is another tab.
  if (tweak == TabMotionTweaks::kTargetIsTab &&
      move_properties.target.as_string) {
    return true;
  }

  if (!move_properties.tweaks)
    return false;

  auto& tweaks = move_properties.tweaks;
  return std::find(tweaks->begin(), tweaks->end(), tweak) != tweaks->end();
}

std::optional<double> TabsMotionHelper::GetTargetWorkspaceId() const {
  if (params_.move_properties.workspace_id) {
    if (*params_.move_properties.workspace_id == 0)
      return std::nullopt;
    return params_.move_properties.workspace_id;
  }

  double workspace_id = GetTargetTabStrip()->GetActiveWorkspace();
  if (workspace_id == 0)
    return std::nullopt;
  return workspace_id;
}

int TabsMotionHelper::GetTargetIndex() const {
  return target_index_;
}

int TabsMotionHelper::GetWindowId() const {
  return window_id_;
}

TabStripModel* TabsMotionHelper::GetTargetTabStrip() const {
  CHECK(tab_strip_);
  return tab_strip_;
}

void TabsMotionHelper::UpdateReparenting(int fix) {
  if (reparent_id_)
    return;

  TabStripModel* current_tab_strip_model =
      GetTabStripForWindowId(GetWindowId());

  if (!current_tab_strip_model)
    return;

  const int target_index = GetTargetIndex();

  // The tabs are going to land in betweend those 2.
  std::optional<TabProbe> before =
      TabLookup(target_index + fix, current_tab_strip_model);
  std::optional<TabProbe> after =
      TabLookup(target_index + 1 + fix, current_tab_strip_model);

  if (!before) {                  // Cannot resolve 'before' tab
    reparent_id_ = std::nullopt;  // or handle appropriately
    return;
  }

  if (Has(TabMotionTweaks::kCollapsedAbove)) {
    // Dropping a tab after a collapsed node should never fall into the node.
    reparent_id_ = after ? tab_probe::GetParentExtId(*after) : std::nullopt;
  } else if (after && tab_probe::GetParentExtId(*after) ==
                          tab_probe::GetExtId(*before)) {
    reparent_id_ = tab_probe::GetExtId(*before);
  } else {
    reparent_id_ = tab_probe::GetParentExtId(*before);
  }
}

std::optional<std::string> TabsMotionHelper::SecureReparentId() {
  if (!reparent_id_)
    return std::nullopt;

  CHECK(!moves_.empty());
  content::WebContents* first_tab_contents = moves_[0].contents;
  ::vivaldi::TabExtData* first_tab_ext = TabExtData::Get(first_tab_contents);

  if (*reparent_id_ == first_tab_ext->GetExtId()) {
    // Handle glitch in topological sort, prevents an infinite loop in
    // recursion. Note, the algorithm works over the topologivally sorted
    // tree, but it must handle the cases, when the tree is not sorted!
    // Unsorted tree is actually a valid state, as there are many
    // possible tab manipulations and not all of them are related-tabs
    // friendly.
    reparent_id_ = first_tab_ext->GetParentExtId();
  }

  if (reparent_id_ && *reparent_id_ == first_tab_ext->GetExtId()) {
    // This should never happen, but we are not crashing, right?
    LOG(WARNING) << "potential paretn-child loop detected"
                 << reparent_id_.value_or("n/a");
    reparent_id_ = std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string> TabsMotionHelper::GetReparentId() const {
  if (reparent_id_ && *reparent_id_ == INVALID_EXT_ID)
    return std::nullopt;
  return reparent_id_;
}

bool TabsMotionHelper::ShouldReparent() const {
  if (Has(TabMotionTweaks::kDoNotReparent)) {
    // This tweak is typically used while dragging the tab in the tab-bar.
    return false;
  }
  return true;
}

TabExtData* TabsMotionHelper::GetExtDataClassRelative(int offset) const {
  if (!tab_strip_ || target_index_ == -1) {
    return nullptr;
  }

  std::optional<TabProbe> info =
      TabLookup(raw_target_index_ + offset, tab_strip_);
  if (!info) {
    return nullptr;
  }

  return TabExtData::Get(info->contents);
}

std::optional<std::string> TabsMotionHelper::TweaksConsistencyCheck() {
  int i = 0;
  i += Has(TabMotionTweaks::kBelow) ? 1 : 0;
  i += Has(TabMotionTweaks::kAbove) ? 1 : 0;
  i += Has(TabMotionTweaks::kOn) ? 1 : 0;

  if (i > 1) {
    return "Only one of below/above/on tweak is allowed.";
  }

  if (Has(TabMotionTweaks::kStripUp) || Has(TabMotionTweaks::kStripDown)) {
    if (!Has(TabMotionTweaks::kTargetIsTab)) {
      return "strip-* tweak can be used together with target-is-tab only.";
    }
  }
  return std::nullopt;
}

std::pair<int, int> TabsMotionHelper::CountBeforeAndAfter() {
  auto res = std::make_pair(0, 0);

  for (auto& tab_info : expanded_tab_probes_) {
    // Ignore the tabs from the different tabstrips.
    if (tab_info.tab_strip_model != GetTargetTabStrip())
      continue;

    if (tab_info.index < GetTargetIndex()) {
      res.first++;
    } else {
      res.second++;
    }
  }

  return res;
}

std::optional<std::string> TabsMotionHelper::TakeTargetTabsFromParam() {
  const auto& move_properties = params_.move_properties;
  absl::flat_hash_set<int> tab_set;
  absl::flat_hash_set<std::string> ext_set;

  if (move_properties.tab_ids) {
    if (move_properties.tab_ids->as_integers) {
      tab_set = absl::flat_hash_set<int>(
          move_properties.tab_ids->as_integers->begin(),
          move_properties.tab_ids->as_integers->end());
    } else if (move_properties.tab_ids->as_integer) {
      tab_set.insert(*move_properties.tab_ids->as_integer);
    }
  }

  if (move_properties.ext_ids) {
    if (move_properties.ext_ids->as_strings) {
      ext_set = absl::flat_hash_set<std::string>(
          move_properties.ext_ids->as_strings->begin(),
          move_properties.ext_ids->as_strings->end());
    } else if (move_properties.ext_ids->as_string) {
      ext_set.insert(*move_properties.ext_ids->as_string);
    }
  }

  tab_probes_ = ResolveTabs(tab_set, ext_set);

  if (tab_probes_.empty()) {
    return "nothing is moving";
  }

  if (Has(TabMotionTweaks::kExpandRelated)) {
    std::optional<std::string> error;
    expanded_tab_probes_ = related_tabs::Expand(tab_probes_, &error);
    if (error) {
      return error;
    }
  } else {
    expanded_tab_probes_ = tab_probes_;
  }

  if (expanded_tab_probes_.empty()) {
    return "tabs expand to nothing";
  }

  std::pair<int, int> pinned_unpinned =
      related_tabs::CountPinned(expanded_tab_probes_);

  if (pinned_unpinned.first && pinned_unpinned.second) {
    return "can't move pinned and unpinned tabs together";
  }

  if (pinned_unpinned.first)
    moving_pinned_ = true;

  return std::nullopt;
}

const std::vector<TabsMotionHelper::Move>& TabsMotionHelper::GetMoves() const {
  return moves_;
}

const TabsMotionHelper::Move& TabsMotionHelper::GetFirstMove() const {
  CHECK(!moves_.empty());
  return moves_[0];
}



std::optional<std::string> TabsMotionHelper::CreateMoves() {
  CreateMovesInternal();
  if (GetMoves().empty()) {
    return "nothing to move";
  }
  return std::nullopt;
}


// Moving tab_probes to the given index and the window. The function creates a
// list of moves to get the tabs in to the target state.
void TabsMotionHelper::CreateMovesInternal() {
  auto& items = GetExpandedProbes();

  if (IsVoidMotion() || items.empty()) {
    return;
  }

  moves_.reserve(items.size());

  // Now, we have a list of tabs (Item's) and we have all the information we
  // need to actually move the tabs.
  std::vector<::vivaldi::TabProbe> same_left;
  std::vector<::vivaldi::TabProbe> same_right;
  std::vector<::vivaldi::TabProbe> cross;

  same_left.reserve(items.size());
  same_right.reserve(items.size());
  cross.reserve(items.size());

  int insert_index = GetTargetIndex();

  // Split the tabs to 3 groups. Tha tabs moving before/after the target index
  // and the tabs moving to the different window.
  for (const auto& it : items) {
    int src_window_id =
        extensions::ExtensionTabUtil::GetWindowIdOfTab(it.contents);
    if (src_window_id != GetWindowId()) {
      cross.push_back(it);
    } else if (it.index < insert_index) {
      same_left.push_back(it);
    } else {
      same_right.push_back(it);
    }
  }

  int first_direction = 0;
  if (!same_right.empty()) {
    first_direction = -1;
  }

  CHECK(IsSortedByIndex(same_left));
  CHECK(IsSortedByIndex(same_right));

  const int insert_index_orig = insert_index;
  for (const auto& it : same_right) {
    moves_.push_back({it.tab_id, insert_index, it.contents});
    ++insert_index;
  }

  if (!same_right.empty())
    insert_index = insert_index_orig - 1;

  // The tabs in 'same_left' are moved from left to right. When these moves are
  // executed by TabsPrivateMoveFunction::MoveTab (it behaves the same way as
  // chrome.tabs.move() while moving a single tab. The related logic was
  // actually taken from there), specifying the same 'insert_index' for all of
  // them results in preserving their original relative order. This is due to
  // the internal mechanics of how MoveTab reorders tabs when multiple
  // operations target the same index, effectively "slotting" them into place
  // without reversing their sequence.
  for (const auto& it : same_left) {
    moves_.push_back({it.tab_id, insert_index, it.contents});
  }

  for (const auto& it : cross) {
    moves_.push_back({it.tab_id, insert_index, it.contents});
    ++insert_index;
  }

  if (!moves_.empty()) {
    UpdateReparenting(first_direction);
  }
}

// Gets the common group for a list of tabs.
// Returns the group ID if all the tabs share the same group, otherwise nullopt.
std::optional<std::string> TabsMotionHelper::UpdateCommonGroup() {
  const std::vector<::vivaldi::TabProbe>& probes = GetExpandedProbes();
  CHECK(!common_group_);
  std::optional<std::string> common_group;

  for (auto probe : probes) {
    std::optional<std::string> current_group = tab_probe::GetGroupId(probe);
    if (!current_group) {
      return std::nullopt;
    }

    if (!common_group) {
      common_group = current_group;
    } else if (common_group != current_group) {
      return std::nullopt;
    }
  }

  if (common_group) {
    auto it = groups_count_.find(*common_group);
    if (it == groups_count_.end())
      return std::nullopt;

    if (it->second != static_cast<int>(probes.size()))
      return std::nullopt;
  }

  common_group_ = common_group;
  return std::nullopt;
}

std::optional<TabProbe> TabsMotionHelper::GetTargetProbe() const {
  return target_probe_;
}

bool TabsMotionHelper::MovingOverSelf() const {
  CHECK(tab_strip_);
  if (GetExpandedProbes().empty())
    return false;
  std::optional<vivaldi::TabProbe> target =
      tab_probe::TabLookup(target_index_, tab_strip_);
  if (!target)
    return false;
  size_t size = GetExpandedProbes().size();
  CHECK(size > 0);
  return GetExpandedProbes()[size - 1].contents == target->contents;
}


TabsMotionHelper::Expected TabsMotionHelper::Create(Params params, bool verbose) {
  std::unique_ptr<TabsMotionHelper> helper(new TabsMotionHelper(std::move(params)));
  using InitFunction = std::optional<std::string> (TabsMotionHelper::*)();
  constexpr InitFunction init_functions[] = {
      &TabsMotionHelper::TweaksConsistencyCheck,
      &TabsMotionHelper::TakeTargetTabsFromParam,
      &TabsMotionHelper::RecognizeTarget,
      &TabsMotionHelper::UpdateGroupsCount,
      &TabsMotionHelper::UpdateCommonGroup,
      &TabsMotionHelper::CreateMoves,
      &TabsMotionHelper::SecureReparentId,
      &TabsMotionHelper::RecognizeTargetGroup,
      &TabsMotionHelper::HandlePinning,
      &TabsMotionHelper::CheckGroupChange,
  };

  std::optional<std::string> err;
  for (InitFunction fn : init_functions) {
    err = (helper.get()->*fn)();
    if (err) {
      break;
    }
  }

  if (err) {
    if (verbose) {
      return base::unexpected(Error{
          .error_message = *err,
          .diagnostics = helper->GetDiagnostics(),
      });
    }

    return base::unexpected(Error{
        .error_message = *err,
    });
  }

  return std::move(helper);
}

TabsMotionHelper::Diagnostics TabsMotionHelper::GetDiagnostics() const {
  std::vector<std::string> lines;

  if (params_.move_properties.debug) {
    lines.emplace_back("MotionDebug >>>> : " +
                       *params_.move_properties.debug);
  }

  if (params_.move_properties.target.as_integer) {
    lines.emplace_back("TARGET[i]: " +
        base::NumberToString(
          *params_.move_properties.target.as_integer));
  } else if (params_.move_properties.target.as_string) {
    lines.emplace_back("TARGET[s]: " +
        *params_.move_properties.target.as_string);
  } else {
    lines.emplace_back("TARGET: n/a");
  }

  lines.emplace_back("TARGET index=" +
                     base::NumberToString(target_index_));

  const auto& moves = GetMoves();
  int first_index = moves.empty() ? -1 : moves[0].insert_index;

  std::string target_ext_id = "-";
  if (GetTargetProbe()) {
    target_ext_id =
        tab_probe::GetExtId(*GetTargetProbe()).value_or("-");
  }

  lines.emplace_back("TARGET_TAB=" + target_ext_id);
  lines.emplace_back("TARGET_WINDOW_ID=" +
                     base::NumberToString(GetWindowId()));


  // We want Dump to work even in case of error.
  if (tab_strip_) {
    if (GetTargetWorkspaceId()) {
      lines.emplace_back("TARGET_WORKSPACE=" +
          base::NumberToString(
            static_cast<int64_t>(GetTargetWorkspaceId().value())));
    } else {
      lines.emplace_back("TARGET_WORKSPACE=n/a");
    }

    lines.emplace_back("MOVING_OVER_SELF=" +
        std::string(MovingOverSelf() ? "true" : "false"));

    lines.emplace_back("MOVES count=" +
        base::NumberToString(moves.size()) +
        " first_index=" +
        base::NumberToString(first_index));

    if (target_group_) {
      lines.emplace_back("TARGET_IS_GROUP: " + *target_group_);
    }

    if (common_group_) {
      lines.emplace_back("COMMON_GROUP: " + *common_group_);
    }

    if (group_cache_valid_) {
      lines.emplace_back("SUGGESTED_GROUP: " +
          SuggestGroup().value_or("-"));
    }
  } else {
      lines.emplace_back("TAB_STRIP=n/a");
  }

  for (int i = 0;
       i <= static_cast<int>(TabMotionTweaks::kMaxValue);
       ++i) {
    TabMotionTweaks tweak = static_cast<TabMotionTweaks>(i);
    const std::string tweak_str =
        extensions::vivaldi::tabs_private::ToString(tweak);

    if (Has(tweak)) {
      lines.emplace_back("TWEAK *: " + tweak_str);
    } else {
      lines.emplace_back("TWEAK -: " + tweak_str);
    }
  }
  return lines;
}

void TabsMotionHelper::Dump(const TabsMotionHelper::Error& error) {
  LOG(INFO) << "--- TabsMotionHelper::DumpParams BEGIN ---";
  LOG(INFO) << ">>> ERROR: " << error.error_message;
  LogLines(error.diagnostics, " ");
  LOG(INFO) << "--- TabsMotionHelper::DumpParams END ---";
}

// For debugging only.
void TabsMotionHelper::Dump(const TabsMotionHelper::Diagnostics& lines) {
  if (lines.empty())
    return;
  LOG(INFO) << "--- TabsMotionHelper::DumpParams BEGIN ---";
  LogLines(lines, " ");
  LOG(INFO) << "--- TabsMotionHelper::DumpParams END ---";
}

const TabsMotionHelper::Params& TabsMotionHelper::GetParameters() const {
  return params_;
}

}  // namespace vivaldi
