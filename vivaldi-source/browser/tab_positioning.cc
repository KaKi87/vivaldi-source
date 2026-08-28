// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#include "browser/tab_positioning.h"

#include <optional>
#include <string>
#include <utility>

#include "app/vivaldi_apptools.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "browser/tab_positioning_prefs.h"
#include "browser/tab_probe.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"
#include "components/ext_data/tab_positioning_params.h"
#include "content/public/browser/web_contents.h"

namespace vivaldi::tab_positioning {
namespace {

// Choose the tab position for the "After Related Tabs" option.
TabProbe GetLastRelatedInternal(TabProbe probe) {
  std::optional<std::string> ext_id = tab_probe::GetExtId(probe);
  const std::optional<std::string> group = tab_probe::GetGroupId(probe);
  CHECK(ext_id);

  for (;;) {
    std::optional<TabProbe> next_probe = tab_probe::GetNext(probe);

    if (!next_probe)
      break;

    if (tab_probe::GetGroupId(*next_probe) != group)
      break;

    std::optional<TabProbe> after_next = tab_probe::GetNext(*next_probe);

    // The tab after next is a child.
    if (after_next && tab_probe::GetExtId(*next_probe) ==
                          tab_probe::GetParentExtId(*after_next)) {
      break;
    }

    if (group) {
      if (tab_probe::GetGroupId(*next_probe) != group)
        break;
    }

    auto parent = tab_probe::GetParentExtId(*next_probe);

    if (!parent || *parent != *ext_id)
      break;

    CHECK(probe.index < next_probe->index);
    probe = *next_probe;
  }
  return probe;
}

struct ResultPosition {
  // pin the tab
  bool pin = false;

  // the target index
  int index = 0;

  // kAvoid: Do not put the tab into any group unless it is surrounded by tabs
  // from the same group on both sides.
  //
  // kSticky: If the tab to the left/up belongs to a group, put the tab into
  // that group as well.
  //
  // kForced: Like kSticky, but if there is no group to the left/up, create a
  // new group together with the tab to the left/up.
  StackingMode stacking = StackingMode::kAvoid;
};

bool IsLinkClick(TabInvokedBy invoked_by) {
  return invoked_by == TabInvokedBy::kHtml ||
         invoked_by == TabInvokedBy::kBackground ||
         invoked_by == TabInvokedBy::kEmailLink ||
         invoked_by == TabInvokedBy::kEmailLinkBackground;
}

enum struct LocalStrategy {
  kRightOfCurrent = 0,
  kDirectRightOfCurrent = 1,
  kAlwaysLast = 2,
  kOpenInTabstackWithRelated = 3,
  kExternalApp = 4,
  kBookmarks = 5,
};

LocalStrategy ToLocalStrategy(std::optional<TabPlacingStrategy> strategy) {
  if (!strategy) {
    return LocalStrategy::kAlwaysLast;
  }

  switch (*strategy) {
    case TabPlacingStrategy::kRightOfCurrent:
      return LocalStrategy::kRightOfCurrent;
    case TabPlacingStrategy::kDirectRightOfCurrent:
      return LocalStrategy::kDirectRightOfCurrent;
    case TabPlacingStrategy::kAlwaysLast:
      return LocalStrategy::kAlwaysLast;
    case TabPlacingStrategy::kOpenInTabstackWithRelated:
      return LocalStrategy::kOpenInTabstackWithRelated;
  }
}

struct LocalState {
  static LocalState Create(const TabPositioningParams& params,
                           const TabBarState& state,
                           const TabProbe& active);

  // Open Tab in Current Tab Stack
  bool ShouldAvoidOpenInStack() const;
  // Avoid stack and force to open the tab as the last.
  bool ShouldForceLast() const;

  ResultPosition GetResultAfter(TabProbe probe,
                                StackingMode stacking_mode,
                                bool pin_state) const;

  LocalStrategy strategy;
  std::optional<std::string> active_group;
  bool is_substrip_locked;
  TabInvokedBy invoked_by;
  TabstackMode tab_stack_mode;
  TabProbe active_probe;
  bool open_in_current_tab_stack;
  bool force_avoid_stack = false;
  bool force_last = false;

  // forces branch of the related tabs tree
  bool direct_child = false;

 private:
  LocalState() = default;
};

TabProbe GetLastInGroup(const LocalState& local) {
  return tab_probe::GetLastInGroup(local.active_probe);
}

TabProbe GetLastInWorkspace(const LocalState& local) {
  if (auto probe = tab_probe::GetLastInActiveWorkspace(local.active_probe)) {
    return *probe;
  }
  return tab_probe::GetLastInWorkspaceIgnorePin(local.active_probe);
}

TabProbe GetLastRelated(const LocalState& local) {
  return GetLastRelatedInternal(local.active_probe);
}

struct InvokeByExtraArg {
  std::string type;
};

std::optional<InvokeByExtraArg> ParseExtraArg(const std::string& json) {
  std::optional<base::Value> value =
      base::JSONReader::Read(json, base::JSON_PARSE_RFC);
  if (!value || !value->is_dict())
    return std::nullopt;
  const base::DictValue& dict = value->GetDict();
  const std::string* type = dict.FindString("type");
  if (!type) {
    return std::nullopt;
  }
  InvokeByExtraArg res;
  res.type = *type;
  return res;
}

// Last, unless kDirectRightOfCurrent.
// Rule for "About Vivaldi" and other Menu->Help info pages, and also mouse
// gestures described in VB-126235.
void SetSemiLast(LocalState& local) {
  if (local.strategy != LocalStrategy::kDirectRightOfCurrent) {
    local.strategy = LocalStrategy::kAlwaysLast;
    local.force_last = true;
  }
  local.force_avoid_stack = true;
  local.is_substrip_locked = false;
}

void AdjustByInvokeByExtraArg(LocalState& local, const std::string& json) {
  std::optional<InvokeByExtraArg> args = ParseExtraArg(json);
  if (!args) {
    LOG(WARNING) << "Can't parse invoke_by extra arg: " << json;
    return;
  }
  if (local.invoked_by == TabInvokedBy::kCommand) {
    if (args->type == "COMMAND_NEW_TAB_OUTSIDE_GROUP"  // "New Top Level Tab"
        || args->type == "INFO_PAGE_TAG") {            // "Menu->Help->..."
      SetSemiLast(local);
      return;
    } else if (args->type == "COMMAND_NEW_TAB_LINK") {
      // VB-126235 - open by mouse gesture is equal to open by clicking a link
      local.invoked_by = TabInvokedBy::kHtml;
      // Enforce the active tab to be a parent.
      local.direct_child = true;
      return;
    }
  }
  LOG(WARNING) << "Unknwon invoke_by extra arg: " << json;
}

LocalState LocalState::Create(const TabPositioningParams& params,
                              const TabBarState& state,
                              const TabProbe& active) {
  LocalState local;
  local.open_in_current_tab_stack = state.open_in_current_tab_stack;
  local.active_group = tab_probe::GetGroupId(active);
  local.is_substrip_locked = state.is_substrip_locked;
  local.tab_stack_mode = state.tab_stack_mode.value_or(TabstackMode::kUnknown);
  local.active_probe = active;
  local.invoked_by = params.invoked_by;

  local.strategy = ToLocalStrategy(state.placement_strategy);

  if (state.tab_stack_mode == TabstackMode::kOff) {
    local.is_substrip_locked = false;
    local.active_group = std::nullopt;
    if (local.strategy == LocalStrategy::kOpenInTabstackWithRelated)
      local.strategy = LocalStrategy::kRightOfCurrent;
  }

  if (state.source == TabSource::kExternalApp) {
    local.force_avoid_stack = true;
    if (local.strategy != LocalStrategy::kDirectRightOfCurrent) {
      local.strategy = LocalStrategy::kExternalApp;
    }
  }

  switch (local.invoked_by) {
    case TabInvokedBy::kEmailUi:
      local.invoked_by = TabInvokedBy::kTabBarButton;
      break;
    case TabInvokedBy::kSpeedDial:
      SetSemiLast(local);
      break;
    case TabInvokedBy::kBookmarks:
      local.force_avoid_stack = true;
      if (local.strategy != LocalStrategy::kDirectRightOfCurrent) {
        local.strategy = LocalStrategy::kBookmarks;
      }
      break;
    case TabInvokedBy::kHtml:
      local.direct_child = true;
      break;
    case TabInvokedBy::kBackground:
      local.direct_child = true;
      break;
    case TabInvokedBy::kEmailLink:
      SetSemiLast(local);
      break;
    case TabInvokedBy::kEmailLinkBackground:
      SetSemiLast(local);
      break;
    case TabInvokedBy::kPanelLinkBackground:
      local.force_avoid_stack = !local.active_group;
      break;
    case TabInvokedBy::kPanelLink:
      local.force_avoid_stack = !local.active_group;
      break;
    case TabInvokedBy::kVivaldiUi:
      SetSemiLast(local);
      break;
    case TabInvokedBy::kDownload:
      SetSemiLast(local);
      break;
    default:
      break;
  }

  if (params.invoked_by_extra_arg) {
    AdjustByInvokeByExtraArg(local, *params.invoked_by_extra_arg);
  }
  return local;
}

// Open Tab in Current Tab Stack
bool LocalState::ShouldAvoidOpenInStack() const {
  if (force_avoid_stack)
    return true;
  if (open_in_current_tab_stack ||
      strategy == LocalStrategy::kOpenInTabstackWithRelated ||
      tab_stack_mode == TabstackMode::kOff || !IsLinkClick(invoked_by))
    return false;
  return true;
}

bool LocalState::ShouldForceLast() const {
  if (force_last)
    return true;

  if (strategy == LocalStrategy::kAlwaysLast && !open_in_current_tab_stack)
    return true;  // VB-129049

  return false;
}

ResultPosition LocalState::GetResultAfter(TabProbe probe,
                                          StackingMode stacking_mode,
                                          bool pin_state) const {
  ResultPosition result;

  // Exception: adjust for disabled "Open Tab in Current Tab Stack"
  if (active_group && ShouldAvoidOpenInStack() &&
      tab_probe::GetGroupId(probe) == active_group) {
    probe = tab_probe::GetLastInGroup(probe);
    stacking_mode = StackingMode::kAvoid;
    pin_state = false;
  }

  if (force_avoid_stack && !active_group) {
    stacking_mode = StackingMode::kAvoid;
    pin_state = false;
  }

  if (ShouldForceLast()) {
    stacking_mode = StackingMode::kAvoid;
    probe = GetLastInWorkspace(*this);
  }

  result.index = probe.index + 1;
  result.stacking = stacking_mode;
  result.pin = pin_state;
  return result;
}

namespace decide {

std::optional<ResultPosition> AlwaysLast(const LocalState& local) {
  if (local.invoked_by == TabInvokedBy::kTabBarButton ||
      local.tab_stack_mode == TabstackMode::kOff) {
    return local.GetResultAfter(GetLastInWorkspace(local), StackingMode::kAvoid,
                                false);
  }
  if (local.active_group) {
    auto target = GetLastInGroup(local);
    return local.GetResultAfter(target, StackingMode::kSticky,
                                tab_probe::IsPinned(target));
  } else if (local.is_substrip_locked) {
    // NOTE: 7.9 behavior. Consider removing this condition for better user
    // experience.
    if (local.invoked_by == TabInvokedBy::kSubStrip) {
      bool pin = tab_probe::IsPinned(local.active_probe);
      return local.GetResultAfter(local.active_probe, StackingMode::kForced,
                                  pin);
    }
  }

  return local.GetResultAfter(GetLastInWorkspace(local), StackingMode::kAvoid,
                              false);
}

std::optional<ResultPosition> ExternalApp(const LocalState& local) {
  return local.GetResultAfter(GetLastInWorkspace(local), StackingMode::kAvoid,
                              false);
}

// Bokmarks behaves  the same as the tabs open from an extern app.
std::optional<ResultPosition> Bookmarks(const LocalState& local) {
  return local.GetResultAfter(GetLastInWorkspace(local), StackingMode::kAvoid,
                              false);
}

std::optional<ResultPosition> DirectRightOfCurrent(const LocalState& local) {
  if (local.invoked_by == TabInvokedBy::kTabBarButton) {  // [+] in main strip
    return local.GetResultAfter(GetLastInGroup(local), StackingMode::kAvoid,
                                false);
  }

  StackingMode new_stacking = StackingMode::kAvoid;
  bool pin = false;
  if (local.active_group) {
    pin = tab_probe::IsPinned(local.active_probe);
    new_stacking = StackingMode::kSticky;
  } else if (local.is_substrip_locked) {
    // NOTE: 7.9 behavior. Consider removing this condition for better user
    // experience.
    if (local.invoked_by == TabInvokedBy::kSubStrip) {
      pin = tab_probe::IsPinned(local.active_probe);
      new_stacking = StackingMode::kForced;
    }
  }

  return local.GetResultAfter(local.active_probe, new_stacking, pin);
}

std::optional<ResultPosition> NewRelatedCommon(const LocalState& local) {
  // upper [+] button
  if (local.invoked_by == TabInvokedBy::kTabBarButton) {
    return local.GetResultAfter(GetLastInWorkspace(local), StackingMode::kAvoid,
                                false);
  }

  // upper [+] button, ctrl-t, lower [+] button
  if (local.invoked_by == TabInvokedBy::kKeyboard ||
      local.invoked_by == TabInvokedBy::kSubStrip) {
    if (local.active_group) {
      TabProbe probe = GetLastInGroup(local);
      return local.GetResultAfter(probe, StackingMode::kSticky,
                                  tab_probe::IsPinned(probe));
    }
    if (local.is_substrip_locked &&
        (local.invoked_by == TabInvokedBy::kSubStrip ||
         local.invoked_by == TabInvokedBy::kKeyboard)) {
      return local.GetResultAfter(local.active_probe, StackingMode::kForced,
                                  tab_probe::IsPinned(local.active_probe));
    }
    return local.GetResultAfter(GetLastInWorkspace(local), StackingMode::kAvoid,
                                false);
  }

  if (local.active_group) {
    auto target = GetLastRelated(local);
    return local.GetResultAfter(target, StackingMode::kSticky,
                                tab_probe::IsPinned(target));
  }
  return std::nullopt;
}

std::optional<ResultPosition> RightOfCurrent(const LocalState& local) {
  if (std::optional<ResultPosition> result = NewRelatedCommon(local)) {
    return result;
  }
  auto target = GetLastRelated(local);
  return local.GetResultAfter(target, StackingMode::kAvoid, false);
}

std::optional<ResultPosition> OpenInTabstackWithRelated(
    const LocalState& local) {
  if (std::optional<ResultPosition> result = NewRelatedCommon(local)) {
    return result;
  }
  // Open in background, not grouped tab + As Tab Stack with Related Tab
  StackingMode new_stacking = StackingMode::kAvoid;
  if (!tab_probe::IsPinned(local.active_probe)) {
    new_stacking = StackingMode::kForced;
  }
  return local.GetResultAfter(local.active_probe, new_stacking, false);
}
}  // namespace decide

std::optional<ResultPosition> DetermineInternal(const LocalState& local) {
  switch (local.strategy) {
    // As Last Tab
    case LocalStrategy::kAlwaysLast:
      return decide::AlwaysLast(local);

    // After Active Tab
    case LocalStrategy::kDirectRightOfCurrent:
      return decide::DirectRightOfCurrent(local);

    // After Related Tab | As Tab Stack with Related Tab
    case LocalStrategy::kRightOfCurrent:
      return decide::RightOfCurrent(local);

    // As Tab Stack with Related Tab
    case LocalStrategy::kOpenInTabstackWithRelated:
      return decide::OpenInTabstackWithRelated(local);

    // From external app
    case LocalStrategy::kExternalApp:
      return decide::ExternalApp(local);

    // From Bookmark bar
    case LocalStrategy::kBookmarks:
      return decide::Bookmarks(local);
  }
}

std::optional<std::string> GetParentForNewTab(TabStripModel* tab_strip,
                                              content::WebContents* contents,
                                              const LocalState& local) {
  namespace tab_probe = ::vivaldi::tab_probe;
  std::optional<::vivaldi::TabProbe> prev =
      tab_probe::TabLookup(tab_strip->active_index(), tab_strip);

  if (!prev) {
    return std::nullopt;
  }

  std::optional<::vivaldi::TabProbe> next = tab_probe::GetNext(*prev);
  if (local.direct_child || (next && tab_probe::GetExtId(*prev) ==
                                         tab_probe::GetParentExtId(*next))) {
    return tab_probe::GetExtId(*prev);
  }
  return ::vivaldi::TabExtData::Get(prev->contents)->GetParentExtId();
}
}  // namespace

void HandleStacking(TabStripModel* tab_strip_model,
                    const TabStripModelChange& change) {
  if (change.type() != TabStripModelChange::kInserted)
    return;
  auto* insert = change.GetInsert();

  for (auto& insert_tab : insert->contents) {
    content::WebContents* contents = insert_tab.contents;
    CHECK(contents);

    std::optional<TabProbe> probe =
        tab_probe::TabLookup(insert_tab.index, tab_strip_model);

    CHECK(probe);

    TabExtData* ext = TabExtData::Get(contents);
    TabExtData* group_origin = nullptr;
    bool create_new_group = false;

    std::optional<TabProbe> prev = tab_probe::GetNext(*probe, true);

    if (prev) {
      switch (ext->GetStackingMode()) {
        case StackingMode::kForced:
          group_origin = TabExtData::Get(prev->contents);
          create_new_group = true;
          break;
        case StackingMode::kSticky:
          group_origin = TabExtData::Get(prev->contents);
          break;
        case StackingMode::kAvoid:
          break;
      }
    }

    if (group_origin) {
      ext->JoinGroup(*group_origin, create_new_group);
    }
  }
}

std::optional<TabPosition> DetermineInsertionIndex(
    TabStripModel* tab_strip,
    content::WebContents* contents,
    // Unused, but could be useful in the future.
    int /*add_types*/,
    ui::PageTransition /*transition*/,
    TabSource source) {
  if (!::vivaldi::IsVivaldiRunning()) {
    return std::nullopt;
  }
  const TabBarState state =
      GetTabBarState(tab_strip->profile()->GetPrefs(), source);
  return DetermineInsertionIndexFromState(tab_strip, contents, source, state);
}

std::optional<TabPosition> DetermineInsertionIndexFromState(
    TabStripModel* tab_strip,
    content::WebContents* contents,
    TabSource source,
    const TabBarState& state) {
  // Here we have all the information to decide where to
  // place the tab, so we don't need to move the tab back and
  // forth in JS.
  CHECK(tab_strip);
  CHECK(contents);

  TabExtData* ext = TabExtData::Get(contents);

  std::optional<TabProbe> active_probe =
      tab_probe::TabLookup(tab_strip->active_index(), tab_strip);

  if (!active_probe) {
    return std::nullopt;
  }

  if (!ext->HasWorkspaceIdSet()) {
    // Nobody has decided workspaceId for this tab yet.
    ext->Set(TabExtKey::kWorkspaceId, tab_strip->GetActiveWorkspace());
  }

  if (!state.placement_strategy)
    return std::nullopt;

  const LocalState local =
      LocalState::Create(ext->GetPositioningParams(), state, *active_probe);
  std::optional<ResultPosition> result = DetermineInternal(local);
  auto parent = GetParentForNewTab(tab_strip, contents, local);
  if (parent) {
    ext->Set(TabExtKey::kParentExtId, *parent);
  }

  if (!result)
    return std::nullopt;

  if (state.tab_stack_mode == TabstackMode::kOff) {
    ext->SetStackingMode(StackingMode::kAvoid);
  } else {
    ext->SetStackingMode(result->stacking);
  }

  TabPosition p;
  p.index = result->index;
  p.pinned = result->pin;
  return p;
}

std::optional<int> DetermineDuplicateIndex(TabStripModel* tab_strip,
                                           content::WebContents* origin,
                                           content::WebContents* contents,
                                           int add_type) {
  if (!::vivaldi::IsVivaldiRunning())
    return std::nullopt;

  TabExtData* origin_ext = TabExtData::Get(origin);
  TabExtData* ext = TabExtData::Get(contents);

  ext->Set(TabExtKey::kWorkspaceId, origin_ext->GetWorkspaceId().value_or(0));
  ext->Set(TabExtKey::kParentExtId, origin_ext->GetExtId());

  ClonedTabPosition position =
      GetClonedTabPosition(tab_strip->profile()->GetPrefs());

  std::optional<::vivaldi::TabProbe> probe = tab_probe::ResolveTab(origin);
  if (!probe)
    return std::nullopt;

  switch (position) {
    case ClonedTabPosition::kRightToCurrent:
      return probe->index + 1;

    case ClonedTabPosition::kAlwaysLast:
      return tab_probe::GetLastInWorkspaceIgnorePin(*probe).index + 1;
  }

  return std::nullopt;
}

bool IsEmailWebContents(content::WebContents* source_content) {
  // email displays in <webview> which is not a tab, so it does not have
  // TabExtData attached!
  if (source_content && !::vivaldi::TabExtData::Has(source_content) &&
      source_content->GetLastCommittedURL().ref() == "vivaldi-email") {
    return true;
  }
  return false;
}

void SetOpener(tabs::TabModel* tab,
               content::WebContents* contents,
               TabStripModel* tab_strip) {
  if (::vivaldi::IsVivaldiRunning() && IsEmailWebContents(contents)) {
    std::optional<TabProbe> email =
        tab_probe::FindByPurpose(tab_strip, TabPurpose::kMail);
    if (email) {
      tab->set_opener(
          tabs::TabInterface::MaybeGetFromContents(email->contents));
      return;
    }
  }
  tab->set_opener(tabs::TabInterface::MaybeGetFromContents(contents));
}

}  // namespace vivaldi::tab_positioning
