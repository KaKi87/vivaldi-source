// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_EXT_DATA_TAB_EXT_DATA_H_
#define COMPONENTS_EXT_DATA_TAB_EXT_DATA_H_

#include <optional>
#include <set>
#include <string>

#include "base/observer_list_types.h"
#include "base/values.h"
#include "content/common/content_export.h"

namespace content {
class WebContents;
}

namespace vivaldi {

struct TabPositioningParams;
class TabExtDataImpl;
class TabsMotionHelper;

CONTENT_EXPORT const void* GetTabExtDataKey();

enum class TabPurpose {
  kMail,
  kUndefined,
};

enum class TabExtKey {
  kCollapsedTab,
  kExpandStatus,
  kExtId,
  kFixedTitle,

  kInterval,
  kPanelId,
  kParentExtId,
  kRestoreStatus,
  kThumbnail,
  kTiling,
  kUrlForThumbnail,
  kVivaldiTabMuted,
  kWorkspaceId,
  kFollowerTabExtId,
  kParentFollowerTabExtId,
  kRestrictPinnedTabs,
  kTabZoom,
  kRestrictPinnedTab,

  // Private keys, settable by SetUnsafe().
  kGroupId_,  // If you want to set the group directly, you must ensure the
              // color and title are the same for all tab group members;
              // otherwise, it would make the group data inconsistent.
              //
              // This '_' suffix is preparation for replacing
              // TabExtKey::kGroupId_ (and TabExtKey in general) with
              // TabExtKey member functions.
  kFixedGroupTitle,
  kGroupColor,
  kPurpose,
};

enum class StackingMode {
  // Force stacking with the tab to the left. If the stack does not exist,
  // create it.
  kForced,

  // If the tab to the left is in a stack, put the tab into that stack.
  kSticky,

  // No stacking. The tab may still end up in a stack if it is created in the
  // middle of an existing stack.
  kAvoid
};

namespace ext_data_helper {
void SetGroupExtData(const std::string& group_id,
                     ::vivaldi::TabExtKey key,
                     const std::optional<std::string>& value);
}

class TabExtData {
 public:
  friend void ext_data_helper::SetGroupExtData(
      const std::string& group_id,
      TabExtKey key,
      const std::optional<std::string>& value);

  friend class TabExtDataImpl;
  friend class TabsMotionHelper;

  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override;

    virtual void OnKeysChanged(TabExtData* tab_ext_data,
                               const std::set<std::string>& changed_keys) = 0;
  };

  struct RestoreArgs {
    enum Type { kRestore, kMerge };
    Type type = kRestore;
    // TODO: replace with ext_id_salt
    // true - restore on startup
    // false - restore from chrome.session.* call
    bool foreign = false;
    // Mix salt with the chosen values, typically groupId and workspaceId to
    // prevent duplicates.
    std::optional<std::string> ext_id_salt;
    bool workspace_as_tabs = false;
  };

  static constexpr uint32_t kChangeWorkspaceId = (1 << 0);
  enum Result { kUpdated, kUnchanged };

  static TabExtData* Get(content::WebContents*);
  static const TabExtData* Get(const content::WebContents*);
  static bool Has(const content::WebContents*);

  virtual ~TabExtData();
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
  virtual void Restore(const std::string& json, const RestoreArgs& args) = 0;
  virtual void CopyFrom(TabExtData*) = 0;
  virtual void Merge(const std::string& json) = 0;
  virtual void Merge(const base::DictValue& new_dict) = 0;
  virtual std::string ToString() const = 0;

  virtual Result Set(TabExtKey key, std::string value) = 0;
  virtual Result Set(TabExtKey key, double value) = 0;
  virtual Result Set(TabExtKey key, bool value) = 0;
  virtual Result Set(TabExtKey key, int value) = 0;
  virtual Result Set(TabExtKey key, const base::Value& value) = 0;
  virtual Result Remove(TabExtKey key) = 0;

  virtual std::string GetExtId() const = 0;
  virtual std::optional<std::string> GetParentExtId() const = 0;
  virtual std::optional<std::string> GetGroupId() const = 0;
  virtual std::optional<std::string> GetPanelId() const = 0;
  virtual std::optional<std::string> GetFollowerExtId() const = 0;
  virtual std::optional<double> GetTabZoom() const = 0;
  virtual std::optional<bool> IsPinnedTabRestricted() const = 0;
  virtual bool IsCollapsedTab() const = 0;
  virtual bool IsTabMuted() const = 0;

  virtual const base::Value* Get(TabExtKey key) const = 0;
  virtual const base::Value* Get(std::string_view key) const = 0;
  virtual std::optional<double> GetWorkspaceId() const = 0;

  // Called by TabStripModelChange observer
  virtual void OnTabAdded() = 0;
  virtual void OnTabRemoved() = 0;

  virtual void Ungroup() = 0;
  virtual void JoinGroup(TabExtData& source,
                         bool create_if_not_exist = false) = 0;

  // This method is only reliable around the time the WebContents is created as
  // current callers only rely on it at that time. See the implementation for
  // details
  virtual bool HasWorkspaceIdSet() const = 0;

  virtual content::WebContents* GetWebContents() = 0;

  virtual Result SetForTesting(TabExtKey key, const base::Value& value) = 0;

  virtual StackingMode GetStackingMode() const = 0;
  virtual void SetStackingMode(StackingMode mode) = 0;
  virtual void SetPositioningParams(const TabPositioningParams &) = 0;
  virtual const TabPositioningParams & GetPositioningParams() = 0;
  virtual TabPurpose GetPurpose() const = 0;

 protected:
  // Can be used to set unsafe keys (kGroupColor, and kFixedGroupTitle so far).
  virtual Result SetUnsafe(TabExtKey key, const base::Value& value) = 0;
};

}  // namespace vivaldi

#endif  // COMPONENTS_EXT_DATA_TAB_EXT_DATA_H_
