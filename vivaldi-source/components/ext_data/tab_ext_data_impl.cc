// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#include "components/ext_data/tab_ext_data_impl.h"

#include "app/vivaldi_apptools.h"
#include "base/callback_list.h"
#include "base/containers/enum_set.h"
#include "base/containers/fixed_flat_map.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/uuid.h"

namespace vivaldi {
namespace {

enum class ExtDataFlag {

  kSetOnce,  // Can be set once, never again.
  kCtl,  // Can't be set. Used for control while calling chrome.tas.create() or
         // chrome.tabs.update().

  kString,
  kDouble,
  kBool,
  kInt,
  kDict,
  kNullAllowed,
};

using ExtDataFlags = base::
    EnumSet<ExtDataFlag, ExtDataFlag::kSetOnce, ExtDataFlag::kNullAllowed>;

constexpr char kExtId[] = "ext_id";
constexpr char kParentExtId[] = "parent_ext_id";
constexpr char kPanelId[] = "panelId";
constexpr char kWorkspaceId[] = "workspaceId";
constexpr char kRestoreStatus[] = "restoreStatus";
constexpr char kExpandStatus[] = "expandStatus";
constexpr char kCollapsedTab[] = "collapsed";
constexpr char kGroupId[] = "group";
constexpr char kFixedGroupTitle[] = "fixedGroupTitle";
constexpr char kGroupColor[] = "groupColor";
constexpr char kInterval[] = "interval";
constexpr char kThumbnail[] = "thumbnail";
constexpr char kUrlForThumbnail[] = "urlForThumbnail";
constexpr char kVivaldiTabMuted[] = "vivaldi_tab_muted";
constexpr char kTiling[] = "tiling";
constexpr char kFixedTitle[] = "fixedTitle";
constexpr char kFollowerTabExtId[] = "followerTabExtId";
constexpr char kParentFollowerTabExtId[] = "parentFollowerTabExtId";
constexpr char kRestrictPinnedTabs[] = "restrictPinnedTab";
constexpr char kTabZoom[] = "vivaldi_tab_zoom";

// Use only in chrome.tabs.update(). It disables ext_data write checks.
// Used by the migration scripts with caution.
constexpr char kMigration[] = "migration";
constexpr char kCreate[] = "create";

struct KeyParams {
  ExtDataFlags flags;
  std::optional<TabExtKey> key_enum;
};

// We should replace all the ext_data structure keys by the constants
// in KeyParams.
constexpr auto kKeyParams = base::MakeFixedFlatMap<std::string_view, KeyParams>(
    {{kExtId,
      {ExtDataFlags{ExtDataFlag::kSetOnce, ExtDataFlag::kString},
       TabExtKey::kExtId}},
     {kParentExtId,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kParentExtId}},
     {kExpandStatus,
      {ExtDataFlags{ExtDataFlag::kBool, ExtDataFlag::kNullAllowed},
       TabExtKey::kExpandStatus}},
     {kRestoreStatus,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kRestoreStatus}},
     {kCollapsedTab,
      {ExtDataFlags{ExtDataFlag::kBool, ExtDataFlag::kNullAllowed},
       TabExtKey::kCollapsedTab}},
     {kFixedTitle,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kFixedTitle}},
     {kFollowerTabExtId,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kFollowerTabExtId}},
     {kGroupId,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kGroupId}},
     {kGroupColor,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kGroupColor}},
     {kFixedGroupTitle,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kFixedGroupTitle}},
     {kPanelId,
      {ExtDataFlags{ExtDataFlag::kSetOnce, ExtDataFlag::kString},
       TabExtKey::kPanelId}},
     {kWorkspaceId,
      {ExtDataFlags{ExtDataFlag::kDouble, ExtDataFlag::kNullAllowed},
       TabExtKey::kWorkspaceId}},
     {kInterval,
      {ExtDataFlags{ExtDataFlag::kDouble, ExtDataFlag::kNullAllowed},
       TabExtKey::kInterval}},
     {kThumbnail,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kThumbnail}},
     {kUrlForThumbnail,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kUrlForThumbnail}},
     {kVivaldiTabMuted,
      {ExtDataFlags{ExtDataFlag::kBool, ExtDataFlag::kNullAllowed},
       TabExtKey::kVivaldiTabMuted}},
     {kParentFollowerTabExtId,
      {ExtDataFlags{ExtDataFlag::kString, ExtDataFlag::kNullAllowed},
       TabExtKey::kParentFollowerTabExtId}},
     {kTiling,
      {ExtDataFlags{ExtDataFlag::kDict, ExtDataFlag::kNullAllowed},
       TabExtKey::kTiling}},
     {kRestrictPinnedTabs,
      {ExtDataFlags{ExtDataFlag::kBool, ExtDataFlag::kNullAllowed},
       TabExtKey::kRestrictPinnedTabs}},
     {kTabZoom,
      {ExtDataFlags{ExtDataFlag::kDouble, ExtDataFlag::kNullAllowed},
       TabExtKey::kTabZoom}}});

constexpr auto kKeyEnumToString =
    base::MakeFixedFlatMap<TabExtKey, std::string_view>(
        {{TabExtKey::kExtId, kExtId},
         {TabExtKey::kParentExtId, kParentExtId},
         {TabExtKey::kPanelId, kPanelId},
         {TabExtKey::kWorkspaceId, kWorkspaceId},
         {TabExtKey::kRestoreStatus, kRestoreStatus},
         {TabExtKey::kExpandStatus, kExpandStatus},
         {TabExtKey::kCollapsedTab, kCollapsedTab},
         {TabExtKey::kGroupId, kGroupId},
         {TabExtKey::kFixedGroupTitle, kFixedGroupTitle},
         {TabExtKey::kGroupColor, kGroupColor},
         {TabExtKey::kInterval, kInterval},
         {TabExtKey::kThumbnail, kThumbnail},
         {TabExtKey::kUrlForThumbnail, kUrlForThumbnail},
         {TabExtKey::kVivaldiTabMuted, kVivaldiTabMuted},
         {TabExtKey::kTiling, kTiling},
         {TabExtKey::kFixedTitle, kFixedTitle},
         {TabExtKey::kFollowerTabExtId, kFollowerTabExtId},
         {TabExtKey::kParentFollowerTabExtId, kParentFollowerTabExtId},
         {TabExtKey::kRestrictPinnedTabs, kRestrictPinnedTabs},
         {TabExtKey::kTabZoom, kTabZoom}});

ExtDataFlags GetKeyFlags(std::string_view s) {
  auto it = kKeyParams.find(s);
  if (it == kKeyParams.end())
    return ExtDataFlags();
  return it->second.flags;
}

std::optional<TabExtKey> GetExtKeyEnum(std::string_view s) {
  auto it = kKeyParams.find(s);
  if (it == kKeyParams.end())
    return std::nullopt;
  return it->second.key_enum;
}

class ExtDataNotifyGuard {
 public:
  ExtDataNotifyGuard(TabExtDataImpl* ext_data) : ext_data_(ext_data) {
    ext_data_->PauseNotifications();
  }
  ~ExtDataNotifyGuard() { ext_data_->UnpauseNotifications(); }

 private:
  TabExtDataImpl* ext_data_;
};

}  // namespace

TabExtDataImpl::TabExtDataImpl(content::WebContents* contents)
    : content::WebContentsUserData<TabExtDataImpl>(*contents),
      content::WebContentsObserver(contents)
{}

TabExtDataImpl::~TabExtDataImpl() {}

void TabExtDataImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void TabExtDataImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void TabExtDataImpl::PauseNotifications() {
  notifications_paused_count_++;
}

void TabExtDataImpl::UnpauseNotifications() {
  notifications_paused_count_--;
  CHECK(notifications_paused_count_ >= 0);
  if (notifications_paused_count_ == 0) {
    NotifyChange();
  }
}

void TabExtDataImpl::MergeInternal(const std::string& json,
                                   MergeType merge_type) {
  std::optional<base::Value> new_json_data =
      base::JSONReader::Read(json, base::JSON_PARSE_RFC);
  if (new_json_data && new_json_data->is_dict()) {
    MergeInternal(new_json_data->GetDict(), merge_type);
  } else {
    LOG(ERROR) << "merging invalid extData JSON: [" << json << "]";
  }
}

void TabExtDataImpl::MergeInternal(const base::DictValue& new_dict,
                                   MergeType merge_type) {
  used_ = true;
  ExtDataNotifyGuard pause_notifications(this);
  UpdateFlags flags{UpdateFlag::kMergeFlag};

  if (merge_type == MergeType::kRestore ||
      merge_type == MergeType::kRestoreForeign) {
    flags.Put(UpdateFlag::kRestoreFlag);
  }

  if (merge_type == MergeType::kRestoreForeign) {
    flags.Put(UpdateFlag::kForeign);
  }

  if (new_dict.FindBool(kMigration).value_or(false)) {
    flags.Put(UpdateFlag::kMigration);
  }

  if (new_dict.FindBool(kCreate).value_or(false)) {
    weak_ext_data_.creating = true;
    flags.Put(UpdateFlag::kCreateFlag);
  }

  for (const auto [key, new_val] : new_dict) {
    if (key == kMigration || key == kCreate) {
      continue;
    }

    UpdateValue(key, new_val, flags);
  }
}

void TabExtDataImpl::Merge(const std::string& json) {
  MergeInternal(json, MergeType::kNormal);
}

void TabExtDataImpl::Merge(const base::DictValue& new_dict) {
  MergeInternal(new_dict, MergeType::kNormal);
}

void TabExtDataImpl::Restore(const std::string& json,
                             const TabExtData::RestoreArgs& args) {
  CHECK(!used_);
  ExtDataNotifyGuard pause_notifications(this);

  if (args.foreign) {
    MergeInternal(json, MergeType::kRestoreForeign);
  } else {
    MergeInternal(json, MergeType::kRestore);
  }

  Set(TabExtKey::kRestoreStatus, std::string("restored"));

  // Restore does not send notifications.
  changed_keys_.clear();
}

// Later, instead of logging an error, we can panic.
bool TabExtDataImpl::Check(std::string_view key,
                           const base::Value& new_val,
                           const base::Value* old_val) {
  ExtDataFlags key_flags = GetKeyFlags(key);

  if (key_flags.empty()) {
    LOG(ERROR) << "unknown viv_ext_data: " << key;
    // Setting anyway...
    return true;
  }

  if (key_flags.Has(ExtDataFlag::kNullAllowed)) {
    if (new_val.is_none()) {
      return true;
    }
  }
  if (key_flags.Has(ExtDataFlag::kSetOnce) && old_val) {
    LOG(ERROR) << "viv_ext_data: " << key
             << " is supposed to be set once; old="
             << old_val->DebugString()
             << "; new=" << new_val.DebugString();
    return false;
  }

  static const std::array<std::pair<ExtDataFlag, base::Value::Type>, 5>
      type_flags{{{ExtDataFlag::kString, base::Value::Type::STRING},
                  {ExtDataFlag::kDict, base::Value::Type::DICT},
                  {ExtDataFlag::kDouble, base::Value::Type::DOUBLE},
                  {ExtDataFlag::kInt, base::Value::Type::INTEGER},
                  {ExtDataFlag::kBool, base::Value::Type::BOOLEAN}}};

  bool has_type = false;
  bool has_valid_type = false;

  for (const auto& [flag, type] : type_flags) {
    if (!key_flags.Has(flag))
      continue;
    has_type = true;
    if (new_val.type() == type) {
      has_valid_type = true;
      break;
    }
  }

  if (has_type && !has_valid_type) {
    int old = -1;
    if (old_val) {
      old = static_cast<int>(old_val->type());
    }
    LOG(INFO) << "ext_data: " << key << " has invalid type "
              << static_cast<int>(new_val.type()) << " old=" << old;
    return false;
  }

  return true;
}

void TabExtDataImpl::OnKeyChange(std::string_view key,
                                 const base::Value* new_val) {
  // If there are too many updated keys, something is terribly wrong.
  CHECK(changed_keys_.size() < 200);
  changed_keys_.insert(std::string(key));
  cache_ = std::nullopt;

  NotifyChange();
}

TabExtDataImpl::Action TabExtDataImpl::SanitizeBeforeUpdate(
    std::string_view key,
    base::Value& new_val,
    const base::Value* old_val,
    UpdateFlags flags) {
  ExtDataFlags key_flags = GetKeyFlags(key);

  if (key_flags.Has(ExtDataFlag::kCtl)) {
    // Never allow to set a control key.
    return Action::kAbort;
  }

  if (flags.Has(UpdateFlag::kMigration)) {
    // Allow averything if chrome.tabs.update() has migration=true key.
    return Action::kContinue;
  }
  std::optional<TabExtKey> key_enum = GetExtKeyEnum(key);

  if (key_enum == TabExtKey::kWorkspaceId) {
    // Remember, someone set the workspace.
    workspace_id_chosen_ = true;
  }

  if (old_val && *old_val == new_val) {
    return Action::kAbort;
  }

  if (key_enum == TabExtKey::kExtId) {
    // Empty string is like undefined.
    // This is due to flow, where ext_id is mandatory, but we still call
    // chorme.tabs.update() in order to set extData. This is deprecated approach
    // but it need the ExtData structure to be created and it must contain
    // ext_id. Once we stop using update(), we can remove this wrokaround.
    std::string* s = new_val.GetIfString();
    if (s && *s == "") {
      return Action::kAbort;
    }
  }

  // Enforce double as WorkspaceId in JSON could be interpreted as integer.
  // TODO: Make workspaceId UUID; VB-122760
  if (key_enum == TabExtKey::kWorkspaceId) {
    std::optional<int> new_id = new_val.GetIfInt();
    if (new_id) {
      new_val = base::Value(static_cast<double>(*new_id));
    }
  }

  if (!Check(key, new_val, old_val)) {
    return Action::kAbort;
  }

  if (key_enum == TabExtKey::kFixedGroupTitle) {
    const std::string* s = new_val.GetIfString();
    if (s && s->empty()) {
      // For better user experience, consider empty group title as no title.
      new_val = base::Value(base::Value::Type::NONE);
    }
  }

  const bool is_restore = flags.Has(UpdateFlag::kRestoreFlag);
  const bool is_merge = flags.Has(UpdateFlag::kMergeFlag);
  const bool is_create = flags.Has(UpdateFlag::kCreateFlag);
  const bool is_foreign = flags.Has(UpdateFlag::kForeign);

  if (!key_enum)
    return Action::kContinue;

  switch (*key_enum) {
    case TabExtKey::kWorkspaceId: {
      if (GetPanelId()) {
        // Can't put panel into the workspace.
        return Action::kAbort;
      }
      std::optional<double> new_id = new_val.GetIfDouble();
      if (new_id && *new_id == 0) {
        // workspaceId == 0 explictly means delete the workspace.
        return Action::kRemove;
      }
      break;
    }
    case TabExtKey::kExtId:
    case TabExtKey::kPanelId:
      if (is_foreign) {
        // chrome.session.restore() must not restore extId and/or panelId.
        return Action::kAbort;
      }
      if (GetWorkspaceId()) {
        // Can't create panel in workspace.
        return Action::kAbort;
      }
      break;
    case TabExtKey::kParentExtId: {
        // Prevent parent->parent loop.
        const std::string* s = new_val.GetIfString();
        if (has_ext_id_ && s && *s == GetExtId()) {
          return Action::kAbort;
        }
      }
      break;
    case TabExtKey::kFixedGroupTitle:
    case TabExtKey::kGroupColor:
      if (is_merge && !is_restore) {
        LOG(ERROR) << "can't update group color/title directly: " << key;
        return Action::kAbort;
      }
      break;
    case TabExtKey::kGroupId:
      if (is_create) {
        const std::string* s = new_val.GetIfString();
        if (s) {
          weak_ext_data_.create_in_group_request = *s;
        }
        return Action::kAbort;
      }
      if (is_merge && !is_restore) {
        // Prevents from using chrome.tabs.update to set groupId as it is
        // deprecated.
        LOG(ERROR) << "can't update groupId directly: " << key;
        return Action::kAbort;
      }
      break;
    default:
      break;
  }

  return Action::kContinue;
}

void TabExtDataImpl::SanitizeAfterRemove(std::string_view key) {
  std::optional<TabExtKey> key_enum = GetExtKeyEnum(key);
  if (!key_enum) {
    return;
  }
  switch (*key_enum) {
    case TabExtKey::kGroupId:
      Remove(TabExtKey::kFixedGroupTitle);
      Remove(TabExtKey::kGroupColor);
      break;
    default:
      break;
  }
}

TabExtData::Result TabExtDataImpl::UpdateValue(std::string_view key,
                                               const base::Value& new_val_arg,
                                               UpdateFlags flags) {
  // This is here due to CHECK in TabExtDataImpl::Restore() to
  // ensure nobody calls UpdateValue before Restore.
  used_ = true;

  const base::Value* old_val = value_.Find(key);
  base::Value new_val = new_val_arg.Clone();

  switch (SanitizeBeforeUpdate(key, new_val, old_val, flags)) {
    case Action::kRemove:
      return RemoveValue(key);
    case Action::kAbort:
      return kUnchanged;
    case Action::kContinue:
      break;
  }

  // If the new value is none, delete the key.
  if (new_val.is_none()) {
    return RemoveValue(key);
  }

  if (!old_val || *old_val != new_val) {
    base::Value* value_set = value_.Set(key, std::move(new_val));
    OnKeyChange(key, value_set);
    return kUpdated;
  }
  return kUnchanged;
}

TabExtData::Result TabExtDataImpl::RemoveValue(std::string_view key) {
  CHECK(key != kExtId);
  if (!value_.Remove(key)) {
    return Result::kUnchanged;
  }

  SanitizeAfterRemove(key);
  OnKeyChange(key, nullptr);
  return Result::kUpdated;
}

void TabExtDataImpl::NotifyChange() {
  if (!in_tab_strip_)
    return;

  // Notifications are not paused.
  if (notifications_paused_count_ > 0)
    return;

  // ...and there is a change.
  if (changed_keys_.empty())
    return;

  // Prevent recursion call.
  ExtDataNotifyGuard pause_notifications(this);

  std::set<std::string> changed_keys;
  changed_keys_.swap(changed_keys);

  for (auto& observer : observers_) {
    observer.OnKeysChanged(this, changed_keys);
  }
  vivaldi::GetExtDataUpdatedCallbackList().Notify(GetWebContents());
}

std::string TabExtDataImpl::GetExtId() const {
  const base::Value* value = Get(kExtId);
  CHECK(value);
  const std::string* s = value->GetIfString();
  CHECK(s);
  return *s;
}

std::optional<std::string> TabExtDataImpl::GetParentExtId() const {
  const std::string* s = value_.FindString(kParentExtId);
  if (s) {
    return *s;
  }
  return std::nullopt;
}

std::optional<std::string> TabExtDataImpl::GetGroupId() const {
  const std::string* s = value_.FindString(kGroupId);
  if (s) {
    return *s;
  }
  return std::nullopt;
}

bool TabExtDataImpl::IsCollapsedTab() const {
  return value_.FindBool(kCollapsedTab).value_or(false);
}

bool TabExtDataImpl::IsTabMuted() const {
  return value_.FindBool(kVivaldiTabMuted).value_or(false);
}

std::optional<std::string> TabExtDataImpl::GetPanelId() const {
  const std::string* s = value_.FindString(kPanelId);
  if (s) {
    return *s;
  }
  return std::nullopt;
}

std::optional<std::string> TabExtDataImpl::GetFollowerExtId() const {
  const std::string* s = value_.FindString(kFollowerTabExtId);
  if (s) {
    return *s;
  }
  return std::nullopt;
}

std::optional<double> TabExtDataImpl::GetTabZoom() const {
  return value_.FindDouble(kTabZoom);
}

std::optional<bool> TabExtDataImpl::IsPinnedTabRestricted() const {
  return value_.FindBool(kRestrictPinnedTabs);
}

WeakExtData* TabExtDataImpl::GetWeakExtData() {
  return &weak_ext_data_;
}

TabExtData::Result TabExtDataImpl::Set(TabExtKey key,
                                       const base::Value& value) {
  return UpdateValue(kKeyEnumToString.at(key), value, {});
}

TabExtData::Result TabExtDataImpl::Set(TabExtKey key, std::string value) {
  return Set(key, base::Value(std::move(value)));
}

TabExtData::Result TabExtDataImpl::Set(TabExtKey key, double value) {
  return Set(key, base::Value(std::move(value)));
}

TabExtData::Result TabExtDataImpl::Set(TabExtKey key, bool value) {
  return Set(key, base::Value(std::move(value)));
}

TabExtData::Result TabExtDataImpl::Set(TabExtKey key, int value) {
  return Set(key, base::Value(std::move(value)));
}

TabExtData::Result TabExtDataImpl::Remove(TabExtKey key) {
  return RemoveValue(kKeyEnumToString.at(key));
}

void TabExtDataImpl::SanitizeExtId() {
  if (has_ext_id_)
    return;

  has_ext_id_ = true;
  const std::string* ext_id = value_.FindString(kExtId);

  if (ext_id) {
    // This can happen after restore.
    std::optional<std::string> parent = GetParentExtId();
    // Prevent parant<->child loop.
    // Notice, if it happens, the extId set in value_ has not been used yet, so
    // we can still change it to the random one as nobody knows the first one.
    if (!parent || *ext_id != *parent) {
      return;
    }
  }

  // Do this without any notification going out. We don't need validation either
  // since we know that thevalue we use here is correct.
  value_.Set(kExtId, base::Uuid::GenerateRandomV4().AsLowercaseString());
  OnKeyChange(kExtId, nullptr);
}


void TabExtDataImpl::SanitizeExtId() const {
  const_cast<TabExtDataImpl*>(this)->SanitizeExtId();
}

const base::Value* TabExtDataImpl::Get(std::string_view key) const {
  if (key == kExtId) {
    SanitizeExtId();
  }
  return value_.Find(key);
}

const base::Value* TabExtDataImpl::Get(TabExtKey key) const {
  if (key == TabExtKey::kExtId) {
    SanitizeExtId();
  }
  return value_.Find(kKeyEnumToString.at(key));
}

std::string TabExtDataImpl::ToString() const {
  SanitizeExtId();
  if (cache_) {
    return *cache_;
  }
  std::string json;
  CHECK(base::JSONWriter::WriteWithOptions(value_, 0, &json));
  cache_ = std::move(json);
  return *cache_;
}

std::optional<double> TabExtDataImpl::GetWorkspaceId() const {
  const base::Value* val = Get(TabExtKey::kWorkspaceId);
  if (!val)
    return std::nullopt;

  return val->GetIfDouble();
}

void TabExtDataImpl::OnTabAdded() {
  if (in_tab_strip_) {
    return;
  }
  in_tab_strip_ = true;
  NotifyChange();
}

void TabExtDataImpl::OnTabRemoved() {
  in_tab_strip_ = false;
}

void TabExtDataImpl::CopyFrom(TabExtData *orig) {
  CHECK(orig);
  CHECK(!used_);

  TabExtDataImpl * ext = static_cast<TabExtDataImpl *>(orig);

  value_ = ext->value_.Clone();
  cache_ = std::nullopt;

  changed_keys_.clear();
  changed_keys_.swap(ext->changed_keys_);

  // Do NOT set in_tab_strip_ here. It is set in OnTabStripModelChanged on
  // Type::kReplaced

  workspace_id_chosen_ = ext->workspace_id_chosen_;
  has_ext_id_ = ext->has_ext_id_;
  used_ = true;
}

// This is used when a tab is created and about to be inserted into the
// tab strip. If the target workspace has not been decided yet, the current
// active workspace is used.
//
// This only matters in an edge case: when the tab strip has an active
// workspace selected, but the UI explicitly requests the tab to be
// created in workspace-0 (no workspace).
//
// When the tab is inserted into the tab strip, its workspace is set
// explicitly and this information/variable won't be used anymore.
bool TabExtDataImpl::HasWorkspaceIdSet() const {
  return workspace_id_chosen_;
}

content::WebContents* TabExtDataImpl::GetWebContents() {
  return &content::WebContentsUserData<TabExtDataImpl>::GetWebContents();
}

void TabExtDataImpl::DidOpenRequestedURL(
    content::WebContents* new_contents,
    content::RenderFrameHost* source_render_frame_host,
    const GURL& url,
    const content::Referrer& referrer,
    WindowOpenDisposition disposition,
    ui::PageTransition transition,
    bool started_from_context_menu,
    bool renderer_initiated) {
  Create(new_contents)->Set(::vivaldi::TabExtKey::kParentExtId, GetExtId());
}

// static
TabExtData* TabExtDataImpl::Create(content::WebContents* contents) {
  return GetOrCreateForWebContents(contents);
}

}  // namespace vivaldi
