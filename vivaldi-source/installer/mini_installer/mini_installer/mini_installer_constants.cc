// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "installer/mini_installer/mini_installer/mini_installer_constants.h"

#include "build/branding_buildflags.h"

namespace mini_installer {

// Various filenames and prefixes.
// The target name of the installer extracted from resources.
const wchar_t kSetupExe[] = L"setup.exe";
// The prefix of the chrome archive resource.
const wchar_t kChromeArchivePrefix[] = L"vivaldi";
// The prefix of the installer resource.
const wchar_t kSetupPrefix[] = L"setup";

// Command line switch names for setup.exe.
const wchar_t kCmdInstallArchive[] = L"install-archive";
const wchar_t kCmdUncompressedArchive[] = L"uncompressed-archive";
const wchar_t kCmdUpdateSetupExe[] = L"update-setup-exe";
const wchar_t kCmdNewSetupExe[] = L"new-setup-exe";
const wchar_t kCmdPreviousVersion[] = L"previous-version";

// Temp directory prefix that this process creates.
const wchar_t kTempPrefix[] = L"CR_";
// ap value suffix to force subsequent updates to use the full rather than
// differential updater.
const wchar_t kFullInstallerSuffix[] = L"-full";

// The resource types that would be unpacked from the mini installer.
// Uncompressed binary.
const wchar_t kBinResourceType[] = L"BN";
#if defined(COMPONENT_BUILD)
// Uncompressed dependency for component builds.
const wchar_t kDepResourceType[] = L"BD";
#endif
// LZ compressed binary.
const wchar_t kLZCResourceType[] = L"BL";
// 7zip archive.
const wchar_t kLZMAResourceType[] = L"B7";

// Registry value names.
// The name of the value in kCleanupRegistryKey that tells the installer not to
// delete extracted files.
const wchar_t kCleanupRegistryValue[] = L"ChromeInstallerCleanup";
// The name of an app's Client State registry value that holds the path to its
// uninstaller.
const wchar_t kUninstallRegistryValue[] = L"UninstallString";

// Registry key paths.
// The path to the key containing each app's Clients registry key.
// No trailing slash on this one because the app's GUID is not appended.
const wchar_t kClientsKeyBase[] = L"Software\\Vivaldi";
// The path to the key containing each app's Client State registry key.
const wchar_t kClientStateKeyBase[] =
    L"Software\\Vivaldi\\Update\\ClientState\\";
// The path to the key in which kCleanupRegistryValue is found.
const wchar_t kCleanupRegistryKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Vivaldi";

}  // namespace mini_installer
