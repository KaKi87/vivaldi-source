// Copyright (c) 2016 Vivaldi. All rights reserved.
// included in chromium/chrome/installer/util/shell_util.cc

#include "base/vivaldi_md5.h"
#include "components/base32/base32.h"

namespace vivaldi {

namespace {
const wchar_t kRegShellOpenNewWindow[] = L"\\shell\\opennewwindow\\command";
}

// Cleans up DelegateExecute registry values from old Vivaldi installs.
void RemoveDelegateExecuteForVivaldi(const base::FilePath& chrome_exe,
                                     const std::wstring& suffix) {
  // <root>\Software\Classes\Vivaldi<.suffix>\.exe\shell\open\command
  std::wstring delegate_exe_key(ShellUtil::kRegClasses);
  delegate_exe_key.push_back(base::FilePath::kSeparators[0]);
  delegate_exe_key.append(install_static::GetBaseAppId() + suffix);
  delegate_exe_key.append(ShellUtil::kRegShellOpen);

  // <root>\Software\Classes\Vivaldi<.suffix>\.exe\shell\opennewwindow\command
  delegate_exe_key = ShellUtil::kRegClasses;
  delegate_exe_key.push_back(base::FilePath::kSeparators[0]);
  delegate_exe_key.append(install_static::GetBaseAppId() + suffix);
  delegate_exe_key.append(kRegShellOpenNewWindow);

  // <root>\Software\Classes\VivaldiHTM<.suffix>\shell\open\command
  delegate_exe_key = ShellUtil::kRegClasses;
  delegate_exe_key.push_back(base::FilePath::kSeparators[0]);
  delegate_exe_key.append(GetBrowserProgId(suffix));
  delegate_exe_key.append(ShellUtil::kRegShellOpen);
}

// Constructs a suffix based on path.
void GetPathSpecificSuffix(const base::FilePath& path, std::wstring* suffix) {
  DCHECK(!path.empty());
  DCHECK(suffix);
  std::string path_utf8(base::WideToUTF8(path.value()));
  auto md5_digest = vivaldi::obsolete::ObsoleteMd5Hash(base::as_byte_span(path_utf8));
  const std::string base32_md5(base32::Base32Encode(
      md5_digest,
      base32::Base32EncodePolicy::OMIT_PADDING));
  DCHECK_EQ(base32_md5.length(), 26U);
  suffix->reserve(base32_md5.length() + 1);
  suffix->assign(1, L'.');
  suffix->append(base::ASCIIToWide(base32_md5));
}

// Returns the application name, "Vivaldi" or "Vivaldi S [<folder>]".
std::wstring GetAppName(const std::wstring& install_path) {
  std::wstring app_name(L"Vivaldi");
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(vivaldi::constants::kVivaldiStandalone)) {
    // extract the install folder name for the standalone install
    std::wstring::size_type pos = install_path.rfind(L"\\Application");
    if (pos != std::wstring::npos) {
      app_name.append(L" S [");
      app_name.append(install_path.substr(0, pos));
      app_name.append(1, L']');
    }
  }
  return app_name;
}

namespace install_static {

// Vivaldi customization. For standalone installs,
// add the --user-data-dir argument.
std::wstring GetArguments()
{
  std::wstring arguments;
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(vivaldi::constants::kVivaldiStandalone) &&
      command_line.HasSwitch(vivaldi::constants::kVivaldiInstallDir)) {
    base::FilePath install_path = command_line.GetSwitchValuePath(
        vivaldi::constants::kVivaldiInstallDir).Append(L"User Data");
    arguments.assign(L"--user-data-dir=").append(install_path.value());
  }
  return arguments;
}

}  // namespace install_static

}  // namespace vivaldi