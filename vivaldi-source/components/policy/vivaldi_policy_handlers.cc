// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#include "components/policy/vivaldi_policy_handlers.h"

#include <memory>
#include <optional>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/values.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/core/browser/configuration_policy_handler_list.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

namespace {

class VivaldiCallbackPolicyHandler : public policy::TypeCheckingPolicyHandler {
 public:
  using Callback =
      base::RepeatingCallback<void(const base::Value&, PrefValueMap*)>;

  VivaldiCallbackPolicyHandler(const char* policy_name,
                               base::Value::Type value_type,
                               Callback callback)
      : TypeCheckingPolicyHandler(policy_name, value_type),
        value_type_(value_type),
        callback_(std::move(callback)) {}

  VivaldiCallbackPolicyHandler(const VivaldiCallbackPolicyHandler&) = delete;
  VivaldiCallbackPolicyHandler& operator=(const VivaldiCallbackPolicyHandler&) =
      delete;

  void ApplyPolicySettings(const policy::PolicyMap& policies,
                           PrefValueMap* prefs) override {
    const base::Value* value = policies.GetValue(policy_name(), value_type_);
    if (value) {
      callback_.Run(*value, prefs);
    }
  }

 private:
  const base::Value::Type value_type_;
  Callback callback_;
};

}  // namespace

namespace vivaldi {

void AddVivaldiPolicyHandlers(
    policy::ConfigurationPolicyHandlerList* handlers) {
  // Helper function to set Vivaldi pref to forced valued, based on the trigger
  // value of the bool policy, use empty optional to set the pref just based on
  // the presence of the policy.
  const auto set_bool_prefs = [](std::optional<bool> trigger_value,
                                 bool forced_value,
                                 std::vector<std::string_view> pref_paths) {
    return base::BindRepeating(
        [](std::optional<bool> trigger, bool forced,
           std::vector<std::string_view> paths, const base::Value& value,
           PrefValueMap* prefs) {
          if (trigger.has_value() && value.GetBool() != trigger.value()) {
            return;
          }
          for (std::string_view path : paths) {
            prefs->SetBoolean(path, forced);
          }
        },
        trigger_value, forced_value, std::move(pref_paths));
  };

  // SyncDisabled (true) -> disable vivaldi sync related prefs.
  handlers->AddHandler(std::make_unique<VivaldiCallbackPolicyHandler>(
      policy::key::kSyncDisabled, base::Value::Type::BOOLEAN,
      set_bool_prefs(true, false,
                     {vivaldiprefs::kAddressBarOmniboxShowSyncTabs,
                      vivaldiprefs::kPanelsWindowListShowSyncedTabs,
                      vivaldiprefs::kQuickCommandsShowSyncedTabs,
                      vivaldiprefs::kTabsShowSyncedTabsButton})));

  // SearchSuggestEnabled -> mirror to vivaldi search suggest prefs.
  for (const char* pref : {
           vivaldiprefs::kAddressBarSearchSuggestEnabled,
           vivaldiprefs::kAddressBarInlineSearchSuggestEnabled,
           vivaldiprefs::kAddressBarInlineSearchSuggestOnNickname,
       }) {
    handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
        policy::key::kSearchSuggestEnabled, pref, base::Value::Type::BOOLEAN));
  }

  // SearchSuggestEnabled (any value) -> disable kSearchSuggestEnabled.
  handlers->AddHandler(std::make_unique<VivaldiCallbackPolicyHandler>(
      policy::key::kSearchSuggestEnabled, base::Value::Type::BOOLEAN,
      set_bool_prefs(std::nullopt, false,
                     {vivaldiprefs::kAddressBarShowSearchSuggestionsWarning})));

  // TranslateEnabled -> mirror to vivaldi kTranslateEnabled.
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kTranslateEnabled, vivaldiprefs::kTranslateEnabled,
      base::Value::Type::BOOLEAN));

  // TranslateEnabled (false) -> disable vivaldi translate panel prefs.
  handlers->AddHandler(std::make_unique<VivaldiCallbackPolicyHandler>(
      policy::key::kTranslateEnabled, base::Value::Type::BOOLEAN,
      set_bool_prefs(false, false,
                     {vivaldiprefs::kPanelsTranslateAutomatic,
                      vivaldiprefs::kPanelsTranslatePaste,
                      vivaldiprefs::kPanelsTranslateRedirect})));

  // QRCodeGeneratorEnabled -> mirror to vivaldi kAddressBarShowQrGenerator.
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kQRCodeGeneratorEnabled,
      vivaldiprefs::kAddressBarShowQrGenerator, base::Value::Type::BOOLEAN));

  // BookmarkBarEnabled -> mirror to vivaldi kBookmarksBarVisible.
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kBookmarkBarEnabled, vivaldiprefs::kBookmarksBarVisible,
      base::Value::Type::BOOLEAN));

  // SavingBrowserHistoryDisabled (true) -> disable vivaldi history prefs.
  handlers->AddHandler(std::make_unique<VivaldiCallbackPolicyHandler>(
      policy::key::kSavingBrowserHistoryDisabled, base::Value::Type::BOOLEAN,
      set_bool_prefs(true, false,
                     {vivaldiprefs::kAddressBarOmniboxShowBrowserHistory,
                      vivaldiprefs::kAddressBarOmniboxShowTypedHistory,
                      vivaldiprefs::kAddressBarOmniboxShowTypedHistoryButton,
                      vivaldiprefs::kAddressBarOmniboxSearchHistoryEnable,
                      vivaldiprefs::kAddressBarSearchTypedHistory,
                      vivaldiprefs::kQuickCommandsShowHistory})));
}

}  // namespace vivaldi
