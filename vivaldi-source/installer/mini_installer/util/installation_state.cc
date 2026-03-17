// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "installer/mini_installer/util/installation_state.h"

#include <memory>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/strings/cstring_view.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/version.h"
#include "base/win/registry.h"
#include "build/build_config.h"
#include "chrome/install_static/install_util.h"
#include "installer/mini_installer/util/app_commands.h"
#include "installer/mini_installer/util/google_update_constants.h"
#include "installer/mini_installer/util/helper.h"
#include "installer/mini_installer/util/install_util.h"
#include "installer/mini_installer/util/util_constants.h"

#include "installer/util/vivaldi_install_util.h"

namespace installer {

ProductState::ProductState()
    : uninstall_command_(base::CommandLine::NO_PROGRAM),
      eula_accepted_(0),
      usagestats_(0),
      msi_(false),
      has_eula_accepted_(false),
      has_oem_install_(false),
      has_usagestats_(false) {}

ProductState::~ProductState() = default;

bool ProductState::Initialize(bool system_install) {
  // For Vivaldi, as we support multiple installations, we read the version from
  // the executable and ignore any registry settings.
  Clear();

  base::Version version = vivaldi::GetInstallVersion();
  if (!version.IsValid())
    return false;

  version_ = std::make_unique<base::Version>(version);

  return true;
}

base::FilePath ProductState::GetSetupPath() const {
  return uninstall_command_.GetProgram();
}

const base::Version& ProductState::version() const {
  DCHECK(version_);
  return *version_;
}

ProductState& ProductState::CopyFrom(const ProductState& other) {
  version_.reset(other.version_.get() ? new base::Version(*other.version_)
                                      : nullptr);
  old_version_.reset(other.old_version_.get()
                         ? new base::Version(*other.old_version_)
                         : nullptr);
  channel_ = other.channel_;
  brand_ = other.brand_;
  uninstall_command_ = other.uninstall_command_;
  product_guid_ = other.product_guid_;
  oem_install_ = other.oem_install_;
  commands_.CopyFrom(other.commands_);
  eula_accepted_ = other.eula_accepted_;
  usagestats_ = other.usagestats_;
  msi_ = other.msi_;
  has_eula_accepted_ = other.has_eula_accepted_;
  has_oem_install_ = other.has_oem_install_;
  has_usagestats_ = other.has_usagestats_;

  return *this;
}

void ProductState::Clear() {
  version_.reset();
  old_version_.reset();
  channel_.clear();
  brand_.clear();
  oem_install_.clear();
  uninstall_command_ = base::CommandLine(base::CommandLine::NO_PROGRAM);
  product_guid_.clear();
  commands_.Clear();
  eula_accepted_ = 0;
  usagestats_ = 0;
  msi_ = false;
  has_eula_accepted_ = false;
  has_oem_install_ = false;
  has_usagestats_ = false;
}

bool ProductState::GetEulaAccepted(DWORD* eula_accepted) const {
  DCHECK(eula_accepted);
  if (!has_eula_accepted_)
    return false;
  *eula_accepted = eula_accepted_;
  return true;
}

bool ProductState::GetOemInstall(std::wstring* oem_install) const {
  DCHECK(oem_install);
  if (!has_oem_install_)
    return false;
  *oem_install = oem_install_;
  return true;
}

bool ProductState::GetUsageStats(DWORD* usagestats) const {
  DCHECK(usagestats);
  if (!has_usagestats_)
    return false;
  *usagestats = usagestats_;
  return true;
}

// static
std::wstring ProductState::FindProductGuid(std::wstring_view display_name,
                                           std::wstring_view hint) {
  constexpr base::wcstring_view kUninstallRootKey(
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\");
  constexpr size_t kGuidLength = 36;  // Does not include braces.
  constexpr REGSAM kViews[] = {
      0,  // The default view for this bitness.
#if defined(ARCH_CPU_64_BITS)
      KEY_WOW64_32KEY,  // 32-bit view.
#else
      KEY_WOW64_64KEY,  // 64-bit view.
#endif
  };
  // Returns true if the DisplayName value for the uninstall entry for `name` in
  // `view` of HKLM equals `display_name`.
  auto display_name_is = [&kUninstallRootKey, storage = std::wstring()](
                             REGSAM view, std::wstring_view name,
                             std::wstring_view display_name) mutable {
    return base::win::RegKey(HKEY_LOCAL_MACHINE,
                             base::StrCat({kUninstallRootKey, name}).c_str(),
                             KEY_QUERY_VALUE | view)
                   .ReadValue(kUninstallDisplayNameField, &storage) ==
               ERROR_SUCCESS &&
           storage == display_name;
  };

  // If the caller provided a hint, look first for an entry for it.
  if (!hint.empty()) {
    const std::wstring name = base::StrCat({L"{", hint, L"}"});
    for (const REGSAM view : kViews) {
      if (display_name_is(view, name, display_name)) {
        return std::wstring(hint);
      }
    }
  }

  // Otherwise, search through all subkeys named with GUIDs looking for a hit.
  for (const REGSAM view : kViews) {
    for (base::win::RegistryKeyIterator iter(HKEY_LOCAL_MACHINE,
                                             kUninstallRootKey.c_str(), view);
         iter.Valid(); ++iter) {
      const std::wstring_view key_name(iter.Name());
      // Skip this key if it doesn't plausibly look like a product guid.
      if (key_name.size() != kGuidLength + 2 || key_name.front() != L'{' ||
          key_name.back() != L'}') {
        continue;
      }
      if (display_name_is(view, key_name, display_name)) {
        return std::wstring(key_name.substr(1, kGuidLength));
      }
    }
  }

  return {};
}

InstallationState::InstallationState() = default;

void InstallationState::Initialize() {
  user_chrome_.Initialize(false);
  system_chrome_.Initialize(true);
}

const ProductState* InstallationState::GetProductState(
    bool system_install) const {
  const ProductState* product_state =
      GetNonVersionedProductState(system_install);
  return product_state->version_.get() ? product_state : nullptr;
}

const ProductState* InstallationState::GetNonVersionedProductState(
    bool system_install) const {
  return system_install ? &system_chrome_ : &user_chrome_;
}

}  // namespace installer
