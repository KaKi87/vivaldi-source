// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "installer/mini_installer/util/helper.h"

#include <array>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/version.h"
#include "base/win/windows_version.h"
#include "build/build_config.h"
#include "chrome/install_static/install_util.h"
#include "installer/mini_installer/util/initial_preferences.h"
#include "installer/mini_installer/util/initial_preferences_constants.h"
#include "installer/mini_installer/util/install_util.h"
#include "installer/mini_installer/util/installation_state.h"
#include "installer/mini_installer/util/util_constants.h"

#include "base/command_line.h"

#include "installer/util/vivaldi_install_util.h"

namespace {

// Returns the path denoted by `key`. If `base::PathService` fails to return the
// path to a directory that exists, the value of the environment variable
// corresponding to `key`, if any, is used if it names an absolute directory
// that exists. Returns an empty path if all attempts fail.
base::FilePath GetPathWithEnvironmentFallback(int key) {
  if (base::FilePath path; base::PathService::Get(key, &path) &&
                           !path.empty() && base::DirectoryExists(path)) {
    return path;
  }

  static constexpr auto kKeyToVariable =
      base::MakeFixedFlatMap<int, std::wstring_view>(
          {{base::DIR_PROGRAM_FILES, L"PROGRAMFILES"},
           {base::DIR_PROGRAM_FILESX86, L"PROGRAMFILES(X86)"},
           {base::DIR_PROGRAM_FILES6432, L"ProgramW6432"},
           {base::DIR_LOCAL_APP_DATA, L"LOCALAPPDATA"}});
  if (auto it = kKeyToVariable.find(key); it != kKeyToVariable.end()) {
    std::array<wchar_t, MAX_PATH> value;
    value[0] = L'\0';
    if (DWORD ret = ::GetEnvironmentVariableW(it->second.data(), value.data(),
                                              value.size());
        ret && ret < value.size()) {
      if (base::FilePath path(value.data()); path.IsAbsolute() &&
                                             !path.ReferencesParent() &&
                                             base::DirectoryExists(path)) {
        return path;
      }
    }
  }

  return {};
}

// Returns the default install path given an install level.
base::FilePath GetDefaultChromeInstallPathChecked(bool system_install) {
  base::FilePath install_path = GetPathWithEnvironmentFallback(
      system_install ? base::DIR_PROGRAM_FILES : base::DIR_LOCAL_APP_DATA);

  // Later steps assume a valid install path was found.
  CHECK(!install_path.empty());
  return install_path.Append(install_static::GetChromeInstallSubDirectory())
      .Append(installer::kInstallBinaryDir);
}

// Returns path keys for the standard installation locations for either
// per-user or per-machine installs. In cases where more than one location is
// possible, the default is always first.
base::span<const int> GetInstallationPathKeys(bool system_install) {
  if (!system_install) {
    // %LOCALAPPDATA% is the only location for per-user installs.
    static constexpr int kPerUserKeys[] = {base::DIR_LOCAL_APP_DATA};
    return base::span(kPerUserKeys);
  }
  if (base::win::OSInfo::GetArchitecture() ==
      base::win::OSInfo::X86_ARCHITECTURE) {
    // %PROGRAMFILES% is the only location for 32-bit Windows.
    static constexpr int kPerMachineKeys[] = {base::DIR_PROGRAM_FILES};
    return base::span(kPerMachineKeys);
  }
  // %PROGRAMFILES%, which matches the current binary's bitness, is the default
  // for 64-bit Windows (x64 and arm64). The "opposite" location is the
  // secondary.
  static constexpr int kx64PerMachineKeys[] = {
      base::DIR_PROGRAM_FILES,  // Native folder for this bitness.
#if defined(ARCH_CPU_64_BITS)
      base::DIR_PROGRAM_FILESX86,  // Folder for 32-bit apps.
#else
      base::DIR_PROGRAM_FILES6432,  // Folder for 64-bit apps.
#endif
  };
  return base::span(kx64PerMachineKeys);
}

}  // namespace

namespace installer {

base::FilePath GetInstalledDirectory(bool system_install) {
  return vivaldi::GetInstallBinaryDir();
}

base::FilePath GetDefaultChromeInstallPath(bool system_install) {
  return GetDefaultChromeInstallPathChecked(system_install);
}

base::FilePath GetChromeInstallPathWithPrefs(bool system_install,
                                             const InitialPreferences& prefs) {
  return vivaldi::GetInstallBinaryDir();
}

base::FilePath FindInstallPath(bool system_install,
                               const base::Version& version) {
  CHECK(version.IsValid());

  // Is there an installation in one of the standard locations with a matching
  // version directory?
  for (int path_key : GetInstallationPathKeys(system_install)) {
    if (auto path = GetPathWithEnvironmentFallback(path_key); !path.empty()) {
      path = path.Append(install_static::GetChromeInstallSubDirectory())
                 .Append(kInstallBinaryDir)
                 .AppendASCII(version.GetString());
      if (base::DirectoryExists(path)) {
        return path;
      }
    }
  }
  return {};
}

bool IsCurrentProcessInstalled() {
  // Get the directory in which the product is installed as per its registration
  // with the updater.
  const base::FilePath install_dir = GetInstalledDirectory(
      /*system_install=*/!InstallUtil::IsPerUserInstall());

  if (install_dir.empty()) {
    return false;  // No product installed at this level and mode.
  }

  // Return true if the current process resides within that directory.
  const base::FilePath this_exe = base::PathService::CheckedGet(base::FILE_EXE);
  return install_dir.IsParent(this_exe);
}

}  // namespace installer.
