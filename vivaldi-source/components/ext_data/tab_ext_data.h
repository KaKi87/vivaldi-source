// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_EXT_DATA_TAB_EXT_DATA_H_
#define COMPONENTS_EXT_DATA_TAB_EXT_DATA_H_

#include <optional>
#include <set>
#include <string>

#include "base/observer_list_types.h"
#include "base/values.h"
#include "components/ext_data/tab_weak_ext_data.h"
#include "content/common/content_export.h"

namespace content {
class WebContents;
}

namespace vivaldi {

CONTENT_EXPORT const void* GetTabExtDataKey();

enum class TabExtKey {
  kCollapsedTab,
  kExpandStatus,
  kExtId,
  kFixedGroupTitle,
  kFixedTitle,
  kGroupColor,
  kGroupId,
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
};

class TabExtData {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override;

    virtual void OnKeysChanged(TabExtData* tab_ext_data,
                               const std::set<std::string>& changed_keys) = 0;
  };

  struct RestoreArgs {
    // true - restore on startup
    // false - restore from chrome.session.* call
    bool foreign = false;
  };

  static constexpr uint32_t kChangeWorkspaceId = (1 << 0);
  enum Result { kUpdated, kUnchanged };

  static TabExtData* Get(content::WebContents*);
  static const TabExtData* Get(const content::WebContents*);
  static bool Has(const content::WebContents*);

  virtual ~TabExtData();
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
  virtual void Restore(const std::string& json, const RestoreArgs &args) = 0;
  virtual void CopyFrom(TabExtData *) = 0;
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

  // This method is only reliable around the time the WebContents is created as
  // current callers only rely on it at that time. See the implementation for
  // details
  virtual bool HasWorkspaceIdSet() const = 0;

  virtual WeakExtData* GetWeakExtData() = 0;

  virtual content::WebContents* GetWebContents() = 0;
};

}  // namespace vivaldi

#endif  // COMPONENTS_EXT_DATA_TAB_EXT_DATA_H_
