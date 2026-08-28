// Copyright (c) 2021 Vivaldi Technologies AS. All rights reserved

#include <string>

#include "extensions/vivaldi_standard_management_policy_provider.h"

#include "app/vivaldi_apptools.h"
#include "base/functional/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/extension.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

namespace extensions {

namespace {
constexpr char kProtonVpnId[] = "jplgfhpmjnbigmhklmmbgecoobifkmpa";
}  // namespace

VivaldiStandardManagementPolicyProvider::
    VivaldiStandardManagementPolicyProvider(ExtensionManagement* settings,
                                            Profile* profile)
    : StandardManagementPolicyProvider(settings, profile), profile_(profile) {
  pref_change_registrar_.Init(profile->GetPrefs());
  pref_change_registrar_.Add(
      vivaldiprefs::kPolicyVpnEnabled,
      base::BindRepeating(
          &VivaldiStandardManagementPolicyProvider::OnVpnPolicyChanged,
          base::Unretained(this)));
}

VivaldiStandardManagementPolicyProvider::
    ~VivaldiStandardManagementPolicyProvider() {}

bool VivaldiStandardManagementPolicyProvider::UserMayLoad(
    const Extension* extension,
    std::u16string* error) const {
  return StandardManagementPolicyProvider::UserMayLoad(extension, error);
}

void VivaldiStandardManagementPolicyProvider::UserMayInstall(
    scoped_refptr<const Extension> extension,
    base::OnceCallback<void(ManagementPolicy::Decision)> callback) const {
  return StandardManagementPolicyProvider::UserMayInstall(std::move(extension),
                                                          std::move(callback));
}

bool VivaldiStandardManagementPolicyProvider::UserMayModifySettings(
    const Extension* extension,
    std::u16string* error) const {
  if (vivaldi::IsVivaldiApp(extension->id())) {
    return false;
  }
  if (IsVpnBlockedByPolicy(extension)) {
    return false;
  }
  return StandardManagementPolicyProvider::UserMayModifySettings(extension,
                                                                 error);
}

bool VivaldiStandardManagementPolicyProvider::ExtensionMayModifySettings(
    const Extension* source_extension,
    const Extension* extension,
    std::u16string* error) const {
  if (vivaldi::IsVivaldiApp(extension->id())) {
    return false;
  }
  return StandardManagementPolicyProvider::ExtensionMayModifySettings(
      source_extension, extension, error);
}

bool VivaldiStandardManagementPolicyProvider::MustRemainEnabled(
    const Extension* extension,
    std::u16string* error) const {
  if (vivaldi::IsVivaldiApp(extension->id())) {
    return true;
  }
  return StandardManagementPolicyProvider::MustRemainEnabled(extension, error);
}

bool VivaldiStandardManagementPolicyProvider::MustRemainDisabled(
    const Extension* extension,
    disable_reason::DisableReason* reason) const {
  if (IsVpnBlockedByPolicy(extension)) {
    if (reason) {
      *reason = disable_reason::DISABLE_BLOCKED_BY_POLICY;
    }
    return true;
  }
  return StandardManagementPolicyProvider::MustRemainDisabled(extension,
                                                              reason);
}

bool VivaldiStandardManagementPolicyProvider::MustRemainInstalled(
    const Extension* extension,
    std::u16string* error) const {
  if (vivaldi::IsVivaldiApp(extension->id())) {
    return true;
  }
  return StandardManagementPolicyProvider::MustRemainInstalled(extension,
                                                               error);
}

bool VivaldiStandardManagementPolicyProvider::ShouldForceUninstall(
    const Extension* extension,
    std::u16string* error) const {
  return StandardManagementPolicyProvider::ShouldForceUninstall(extension,
                                                                error);
}

bool VivaldiStandardManagementPolicyProvider::IsVpnBlockedByPolicy(
    const Extension* extension) const {
  if (!extension || extension->id() != kProtonVpnId) {
    return false;
  }
  return !profile_->GetPrefs()->GetBoolean(vivaldiprefs::kPolicyVpnEnabled);
}

void VivaldiStandardManagementPolicyProvider::OnVpnPolicyChanged() {
  ExtensionsBrowserClient::Get()->CheckManagementPolicy(profile_);
}

}  // namespace extensions
