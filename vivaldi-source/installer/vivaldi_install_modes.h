// Copyright (c) 2017 Vivaldi Technologies AS. All rights reserved.

#ifndef INSTALLER_VIVALDI_INSTALL_MODES_H_
#define INSTALLER_VIVALDI_INSTALL_MODES_H_

#include <array>

#include "base/files/file_path.h"
#include "chrome/install_static/install_constants.h"

namespace install_static {

// The brand-specific company name to be included as a component of the install
// and user data directory paths. May be empty if no such dir is to be used.
inline constexpr wchar_t kCompanyPathName[] = L"";

// The brand-specific product name to be included as a component of the install
// and user data directory paths.
inline constexpr wchar_t kProductPathName[] = L"Vivaldi";

inline constexpr char kSafeBrowsingName[] = "vivaldi";

// Hack to avoid enum type/namespace compile issues
#define VIVALDI_INDEX CHROMIUM_INDEX

extern const std::array<InstallConstants, 1> kInstallModes;

}  // namespace install_static

namespace vivaldi {
CLSID GetOrGenerateToastActivatorCLSID(const base::FilePath* = nullptr);
}  // namespace vivaldi
#endif  // INSTALLER_VIVALDI_INSTALL_MODES_H_
