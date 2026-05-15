// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_EXT_DATA_TAB_EXT_DATA_IMPL_H_
#define COMPONENTS_EXT_DATA_TAB_EXT_DATA_IMPL_H_

#include <optional>
#include <set>
#include <string>

#include "base/containers/enum_set.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "components/ext_data/tab_ext_data.h"
#include "components/ext_data/tab_weak_ext_data.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// Forward declarations
namespace content {
class WebContents;
}

namespace vivaldi {

class TabExtDataImpl : public TabExtData,
                       public content::WebContentsUserData<TabExtDataImpl>,
                       public content::WebContentsObserver
{
 public:
  ~TabExtDataImpl() override;

  void Merge(const base::DictValue& new_dict) override;
  void PauseNotifications();
  void UnpauseNotifications();

  // Implementing TabExtData
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  void Restore(const std::string& json, const TabExtData::RestoreArgs& args) override;
  void Merge(const std::string& json) override;
  std::string ToString() const override;
  Result Set(TabExtKey key, std::string value) override;
  Result Set(TabExtKey key, double value) override;
  Result Set(TabExtKey key, bool value) override;
  Result Set(TabExtKey key, int value) override;
  Result Set(TabExtKey key, const base::Value& value) override;
  Result Remove(TabExtKey key) override;
  std::string GetExtId() const override;
  std::optional<std::string> GetParentExtId() const override;
  std::optional<std::string> GetGroupId() const override;
  std::optional<std::string> GetPanelId() const override;
  std::optional<std::string> GetFollowerExtId() const override;
  std::optional<double> GetTabZoom() const override;
  std::optional<bool> IsPinnedTabRestricted() const override;
  bool IsCollapsedTab() const override;
  bool IsTabMuted() const override;
  const base::Value* Get(std::string_view key) const override;
  const base::Value* Get(TabExtKey key) const override;
  std::optional<double> GetWorkspaceId() const override;
  bool HasWorkspaceIdSet() const override;
  WeakExtData* GetWeakExtData() override;

  void OnTabAdded() override;
  void OnTabRemoved() override;

  void CopyFrom(TabExtData *) override;
  content::WebContents* GetWebContents() override;

  static TabExtData * Create(content::WebContents *);

 protected:
  // content::WebContentsObserver
  void DidOpenRequestedURL(content::WebContents* new_contents,
                           content::RenderFrameHost* source_render_frame_host,
                           const GURL& url,
                           const content::Referrer& referrer,
                           WindowOpenDisposition disposition,
                           ui::PageTransition transition,
                           bool started_from_context_menu,
                           bool renderer_initiated) override;

 private:
  friend class content::WebContentsUserData<TabExtDataImpl>;
  explicit TabExtDataImpl(content::WebContents*);

  enum class MergeType { kNormal, kRestore, kRestoreForeign };

  void MergeInternal(const base::DictValue& new_dict, MergeType merge_type);
  void MergeInternal(const std::string& json, MergeType merge_type);

  enum class Action {
    kContinue,
    kRemove,
    kAbort,
  };

  enum class UpdateFlag {
    // The value is being restored (session restore)
    kRestoreFlag,
    // Deprecated chrome.tabs.update(extData...) call, avoid some CHECKs to keep
    // it functional.
    kMergeFlag,

    // extData passed over the chrome.tabs.create() call
    kCreateFlag,

    // Restore over chrome.session.restore() call
    kForeign,

    // Disable all the consistency checks.
    kMigration,
  };

  using UpdateFlags = base::
      EnumSet<UpdateFlag, UpdateFlag::kRestoreFlag, UpdateFlag::kMigration>;

  Action SanitizeBeforeUpdate(std::string_view key,
                              base::Value& new_val,
                              const base::Value* old_val,
                              UpdateFlags flags);

  void SanitizeExtId();
  void SanitizeExtId() const;

  void SanitizeAfterRemove(std::string_view key);

  void OnKeyChange(std::string_view key, const base::Value* new_val);
  void NotifyChange();
  bool Check(std::string_view key,
             const base::Value& new_val,
             const base::Value* old_val);
  Result UpdateValue(std::string_view key,
                     const base::Value& value,
                     UpdateFlags flags);
  Result RemoveValue(std::string_view key);

  base::ObserverList<Observer> observers_;
  mutable std::optional<std::string> cache_;
  base::DictValue value_;
  bool used_ = false;
  int notifications_paused_count_ = 0;
  bool has_ext_id_ = false;
  bool workspace_id_chosen_ = false;
  std::set<std::string> changed_keys_;
  WeakExtData weak_ext_data_;

  bool in_tab_strip_ = false;
};

}  // namespace vivaldi

namespace content {
template <>
inline const void* WebContentsUserData<vivaldi::TabExtDataImpl>::UserDataKey() {
  return vivaldi::GetTabExtDataKey();
}
}  // namespace content

#endif  // COMPONENTS_EXT_DATA_TAB_EXT_DATA_IMPL_H_
