// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#ifndef EXTENSIONS_API_TABS_TABS_MOTION_HELPER_H_
#define EXTENSIONS_API_TABS_TABS_MOTION_HELPER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/types/expected.h"
#include "browser/related_tab_strip_helper.h"
#include "browser/tab_probe.h"
#include "extensions/schema/tabs_private.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

class TabStripModel;

namespace vivaldi {

class TabExtData;

class TabsMotionHelper {
 public:
  struct Move {
    int id;
    int insert_index;
    ::content::WebContents* contents;
  };

  using Diagnostics = std::vector<std::string>;

  struct Error {
    std::string error_message;
    Diagnostics diagnostics;
  };

  using TabMotionTweaks = extensions::vivaldi::tabs_private::TabMotionTweaks;
  using Params = extensions::vivaldi::tabs_private::Move::Params;
  using Expected = base::expected<std::unique_ptr<TabsMotionHelper>, Error>;

  static Expected Create(Params params, bool verbose = false);

  TabsMotionHelper(const TabsMotionHelper&) = delete;
  TabsMotionHelper& operator=(const TabsMotionHelper&) = delete;

  const std::vector<TabProbe>& GetTabProbes() const;
  const std::vector<TabProbe>& GetExpandedProbes() const;
  bool Has(TabMotionTweaks tweak) const;
  int GetTargetIndex() const;
  int GetWindowId() const;
  std::optional<double> GetTargetWorkspaceId() const;
  TabStripModel* GetTargetTabStrip() const;
  std::optional<std::string> GetReparentId() const;
  bool ShouldReparent() const;
  const std::optional<std::string>& SuggestGroup() const;
  int GetGroupsCount(const std::string& group_ext_id) const;
  int GetGroupsExpandedCount(const std::string& group_ext_id) const;
  const std::vector<Move>& GetMoves() const;
  const Move& GetFirstMove() const;
  std::optional<TabProbe> GetTargetProbe() const;
  void ConfigureGroup(const vivaldi::TabProbe& probe) const;
  TabProbe GetLast() const;
  bool ShouldPreserveGroups() const;
  bool IsEntireGroupMoving(const std::string& groupId) const;
  std::optional<std::string> GetNewGroupId() const;

  const Params& GetParameters() const;

  // For debugging.
  static void Dump(const Diagnostics&);
  static void Dump(const Error&);
  Diagnostics GetDiagnostics() const;

 private:
  explicit TabsMotionHelper(Params params);
  TabExtData* GetExtDataClassRelative(int offset = 0) const;
  // ReparentId can be determined by the scheduled moves.
  void UpdateReparenting(int fix);
  bool MovingOverSelf() const;
  void UpdateTargetIndexByTweaks();
  std::optional<std::string> SuggestGroupInternal() const;
  void ForceInDirection(bool reverse);
  void HandleMoveLeftRight(bool reverse);
  void HandleMoveFirstLast(bool reverse);
  void RecognizeSpecialTarget(const std::string_view& target);
  bool IsLinearStrip() const;  // accordion or disabled stacks

  // Init functions
  std::optional<std::string> TweaksConsistencyCheck();
  std::optional<std::string> TakeTargetTabsFromParam();
  std::optional<std::string> RecognizeTarget();
  std::optional<std::string> UpdateGroupsCount();
  std::optional<std::string> UpdateCommonGroup();
  std::optional<std::string> CreateMoves();
  std::optional<std::string> SecureReparentId();
  std::optional<std::string> RecognizeTargetGroup();
  std::optional<std::string> HandlePinning();
  std::optional<std::string> CheckGroupChange();

  std::optional<std::string> ChooseTargetWindowAndTabStrip();

  // Consider following tabs and a group [ B C ] withing them.
  // 0   1 2   3 4
  // A [ B C ] D E
  //
  // Moving grou [ B C ] to index 3 creates a set of moves, but nothing would
  // actually move. We need to avoid such moves as executing them woudl cause a
  // glitch in the UI.
  bool IsVoidMotion();

  void CreateMovesInternal();

  // Note, no group is also valid group. So, validity needed.
  bool group_cache_valid_ = false;  // only for CHECK
  std::optional<std::string> group_cache_;
  // If we're adding a tab into the group, we need a sample tab of this group
  // in order to set its color and title extData accordingly.
  std::optional<TabProbe> group_sample_;

  // Calculate how many tabs are moving from above and under the target index.
  std::pair<int /* before */, int /* after */> CountBeforeAndAfter();

  // Selected tabs to be moved.
  std::vector<vivaldi::TabProbe> tab_probes_;

  // Selected tabs to be moved, but expanded. If you drag a related-tabs tree
  // this would include all the nodes in the tree.
  std::vector<vivaldi::TabProbe> expanded_tab_probes_;

  int target_index_ = -1;
  int raw_target_index_ = -1;
  int window_id_ = -1;

  std::vector<TabsMotionHelper::Move> moves_;

  // Target tab_strip
  base::raw_ptr<TabStripModel> tab_strip_ = nullptr;

  // The future parent of the moved tabs.
  std::optional<std::string> reparent_id_;

  // groups_count_[group_id] == groups_expanded_count_[group_id] means, the
  // entire group is moving. Map group_id => number of tabs in the group.
  absl::flat_hash_map<std::string, int> groups_count_;
  // Map group_id => number of moving tabs in the group.
  absl::flat_hash_map<std::string, int> groups_expanded_count_;

  // If the target is a group, this is the group ext_id.
  // If the target is a tab, and the tab is in a group, this is the group.
  std::optional<std::string> target_group_;

  // If the moving tabs are entire group, common_group would be this
  // group.
  std::optional<std::string> common_group_;

  std::optional<::vivaldi::TabProbe> target_probe_;

  Params params_;

  bool moving_pinned_ = false;
  bool new_group_ = false;
  std::optional<TabMotionTweaks> force_direction_;
  bool is_step_ = false;
  bool avoid_groups_ = false;
};

}  // namespace vivaldi

#endif
