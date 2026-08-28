// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#include "components/policy/vivaldi_vpn_policy_handler.h"

#include <string_view>

#include "base/values.h"
#include "components/policy/core/browser/policy_error_map.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/strings/grit/components_strings.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

namespace vivaldi {

namespace {

constexpr char kProtonVpnId[] = "jplgfhpmjnbigmhklmmbgecoobifkmpa";

bool ListContainsString(const base::Value* value, std::string_view item) {
  if (!value || !value->is_list()) {
    return false;
  }
  for (const base::Value& entry : value->GetList()) {
    if (entry.is_string() && entry.GetString() == item) {
      return true;
    }
  }
  return false;
}

// Returns whether the ExtensionInstallForcelist policy force-installs
// `extension_id`. Entries are formatted as "<extension_id>;<update_url>".
bool ForcelistInstalls(const policy::PolicyMap& policies,
                       std::string_view extension_id) {
  const base::Value* forcelist = policies.GetValue(
      policy::key::kExtensionInstallForcelist, base::Value::Type::LIST);
  if (!forcelist) {
    return false;
  }
  for (const base::Value& entry : forcelist->GetList()) {
    if (entry.is_string() && entry.GetString().starts_with(extension_id)) {
      return true;
    }
  }
  return false;
}

// Return the ExtensionSettings installation mode for key
// (an extension id or "*"), or nullptr not specified.
const std::string* ExtensionSettingsMode(const policy::PolicyMap& policies,
                                         std::string_view key) {
  const base::Value* settings = policies.GetValue(
      policy::key::kExtensionSettings, base::Value::Type::DICT);
  if (!settings) {
    return nullptr;
  }
  if (const base::DictValue* entry = settings->GetDict().FindDict(key)) {
    return entry->FindString("installation_mode");
  }
  return nullptr;
}

bool IsAllowingMode(const std::string* mode) {
  return mode && (*mode == "allowed" || *mode == "force_installed" ||
                  *mode == "normal_installed");
}

bool IsBlockingMode(const std::string* mode) {
  return mode && (*mode == "blocked" || *mode == "removed");
}

// Return whenther the expension is blocked by the policy.
// Mirrors Chromium ExtensionManagement from lowest to highest priority
// (wildcard < per-id entry):
// Blockist/ExtensionSettings wildcard < allowlist < blocklist < force-install <
// ExtensionSettings per-id entry.
bool ExtensionInstallBlockedByPolicy(const policy::PolicyMap& policies,
                                     std::string_view extension_id) {
  const base::Value* blocklist = policies.GetValue(
      policy::key::kExtensionInstallBlocklist, base::Value::Type::LIST);

  // Blocklist wildcard
  bool blocked = ListContainsString(blocklist, "*");

  // ExtensionSettings wildcard
  if (const std::string* wildcard_mode = ExtensionSettingsMode(policies, "*")) {
    blocked = IsBlockingMode(wildcard_mode);
  }
  // Per-id allowlist
  if (ListContainsString(
          policies.GetValue(policy::key::kExtensionInstallAllowlist,
                            base::Value::Type::LIST),
          extension_id)) {
    blocked = false;
  }
  // Per-id blocklist
  if (ListContainsString(blocklist, extension_id)) {
    blocked = true;
  }
  // Force-install overrides the allowlist/blocklist
  if (ForcelistInstalls(policies, extension_id)) {
    blocked = false;
  }
  // Per-id ExtensionSettings
  const std::string* per_id_mode =
      ExtensionSettingsMode(policies, extension_id);
  if (IsAllowingMode(per_id_mode)) {
    blocked = false;
  } else if (IsBlockingMode(per_id_mode)) {
    blocked = true;
  }

  return blocked;
}

bool ManagedProxyConfigured(const policy::PolicyMap& policies) {
  const base::Value* settings =
      policies.GetValue(policy::key::kProxySettings, base::Value::Type::DICT);
  // Any ProxySettings should disable the VPN button.
  return settings;
}

bool VpnDisabledByPolicy(const policy::PolicyMap& policies) {
  const base::Value* enabled = policies.GetValue(
      policy::key::kVivaldiVPNEnabled, base::Value::Type::BOOLEAN);
  return enabled && !enabled->GetBool();
}

}  // namespace

VivaldiVpnPolicyHandler::VivaldiVpnPolicyHandler() = default;

VivaldiVpnPolicyHandler::~VivaldiVpnPolicyHandler() = default;

bool VivaldiVpnPolicyHandler::CheckPolicySettings(
    const policy::PolicyMap& policies,
    policy::PolicyErrorMap* errors) {
  // When the VivaldiVPNEnabled is explicitly enabled, but the higher-priority
  // policy makes it unusable, show the warning about conflicting policies.
  if (errors && !VpnDisabledByPolicy(policies) &&
      (ManagedProxyConfigured(policies) ||
       ExtensionInstallBlockedByPolicy(policies, kProtonVpnId))) {
    errors->AddError(policy::key::kVivaldiVPNEnabled, IDS_POLICY_BLOCKED, {},
                     policy::PolicyMap::MessageType::kWarning);
  }
  // return true, because VivaldiVpnEnabled policy is always applicable
  return true;
}

void VivaldiVpnPolicyHandler::ApplyPolicySettings(
    const policy::PolicyMap& policies,
    PrefValueMap* prefs) {
  // Disable the VPN feature when:
  // * VivaldiVPNEnabled is set to false
  // * proxy is configured
  // * extension install policy blocks the Proton VPN extension.
  if (VpnDisabledByPolicy(policies) || ManagedProxyConfigured(policies) ||
      ExtensionInstallBlockedByPolicy(policies, kProtonVpnId)) {
    prefs->SetBoolean(vivaldiprefs::kPolicyVpnEnabled, false);
  }
}

}  // namespace vivaldi
