// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "installer/mini_installer/setup/installer_state.h"

#include <stddef.h>

#include <string>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/win/registry.h"
#include "chrome/install_static/install_util.h"
#include "installer/mini_installer/setup/setup_util.h"
#include "installer/mini_installer/util/google_update_settings.h"
#include "installer/mini_installer/util/helper.h"
#include "installer/mini_installer/util/initial_preferences.h"
#include "installer/mini_installer/util/initial_preferences_constants.h"
#include "installer/mini_installer/util/install_util.h"
#include "installer/mini_installer/util/installation_state.h"
#include "installer/mini_installer/util/work_item.h"
#include "installer/mini_installer/util/work_item_list.h"

#include "installer/mini_installer/util/registry_util.h"
#include "installer/util/vivaldi_setup_util.h"

namespace installer {

namespace {

// Returns the boolean value of the distribution preference in |prefs| named
// |pref_name|, or |default_value| if not set.
bool GetMasterPreference(const InitialPreferences& prefs,
                         const char* pref_name,
                         bool default_value) {
  bool value;
  return prefs.GetBool(pref_name, &value) ? value : default_value;
}

}  // namespace

InstallerState::InstallerState()
    : operation_(UNINITIALIZED),
      level_(UNKNOWN_LEVEL),
      root_key_(nullptr),
      msi_(false),
      verbose_logging_(false) {}

InstallerState::InstallerState(Level level)
    : operation_(UNINITIALIZED),
      level_(UNKNOWN_LEVEL),
      root_key_(nullptr),
      msi_(false),
      verbose_logging_(false) {
  // Use set_level() so that root_key_ is updated properly.
  set_level(level);
}

InstallerState::~InstallerState() = default;

void InstallerState::Initialize(const base::CommandLine& command_line,
                                const InitialPreferences& prefs,
                                const InstallationState& machine_state) {
  Clear();

  set_level(GetMasterPreference(prefs, initial_preferences::kSystemLevel, false)
                ? SYSTEM_LEVEL
                : USER_LEVEL);

  verbose_logging_ =
      GetMasterPreference(prefs, initial_preferences::kVerboseLogging, false);

  msi_ = GetMasterPreference(prefs, initial_preferences::kMsi, false);
  if (!msi_) {
    const ProductState* product_state =
        machine_state.GetProductState(system_install());
    if (product_state != nullptr)
      msi_ = product_state->is_msi();
  }

  const bool is_uninstall = command_line.HasSwitch(switches::kUninstall);

  target_path_ = GetChromeInstallPathWithPrefs(system_install(), prefs);

  state_key_ = install_static::GetClientStateKeyPath();

  VLOG(1) << (is_uninstall ? "Uninstall Chrome" : "Install Chrome");

  operation_ = is_uninstall ? UNINSTALL : SINGLE_INSTALL_OR_UPDATE;
}

void InstallerState::set_level(Level level) {
  level_ = level;
  switch (level) {
    case UNKNOWN_LEVEL:
      root_key_ = nullptr;
      return;
    case USER_LEVEL:
      root_key_ = HKEY_CURRENT_USER;
      return;
    case SYSTEM_LEVEL:
      root_key_ = HKEY_LOCAL_MACHINE;
      return;
  }
  NOTREACHED() << level;
}

bool InstallerState::system_install() const {
  DCHECK(level_ == USER_LEVEL || level_ == SYSTEM_LEVEL);
  return level_ == SYSTEM_LEVEL;
}

base::Version InstallerState::GetCurrentVersion(
    const InstallationState& machine_state) const {
  base::Version current_version;
  const ProductState* product_state =
      machine_state.GetProductState(level_ == SYSTEM_LEVEL);

  if (product_state) {
    // Be aware that there might be a pending "new_chrome.exe" already in the
    // installation path.  If so, we use old_version, which holds the version of
    // "chrome.exe" itself.
    if (base::PathExists(target_path().Append(kChromeNewExe)) &&
        product_state->old_version()) {
      current_version = *(product_state->old_version());
    } else {
      current_version = product_state->version();
    }
  }

  return current_version;
}

base::FilePath InstallerState::GetInstallerDirectory(
    const base::Version& version) const {
  return target_path().AppendASCII(version.GetString()).Append(kInstallerDir);
}

void InstallerState::Clear() {
  operation_ = UNINITIALIZED;
  target_path_.clear();
  state_key_.clear();
  level_ = UNKNOWN_LEVEL;
  root_key_ = nullptr;
  msi_ = false;
  verbose_logging_ = false;
}

void InstallerState::WriteInstallerResult(
    InstallStatus status,
    int string_resource_id,
    const std::wstring* const launch_cmd) const {
  if (operation_ != UNINSTALL && string_resource_id != 0) {
    vivaldi::ShowInstallerResultMessage(string_resource_id);
  }
}

bool InstallerState::RequiresActiveSetup() const {
  return system_install();
}

// See vivaldi::GetOrGenerateToastActivatorCLSID
// Delete Software\Vivaldi\ToastActivatorCLSID\<target_path().vivaldi.exe>
void InstallerState::ClearToastActivatorTargetExe() const {
  base::FilePath target_exe = target_path().Append(installer::kChromeExe);
  VLOG(1) << "Deleting kVivaldiToastActivatorCLSID: " << target_exe;
  installer::DeleteRegistryValue(
      HKEY_CURRENT_USER, vivaldi::constants::kVivaldiToastActivatorCLSID,
      WorkItem::kWow64Default, target_exe.value());
}

}  // namespace installer
