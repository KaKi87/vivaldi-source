// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "installer/mini_installer/util/google_update_settings.h"

#include <stdint.h>

#include <algorithm>
#include <optional>
#include <string_view>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/lazy_thread_pool_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/win/registry.h"
#include "build/branding_buildflags.h"
#include "chrome/install_static/install_util.h"
#include "installer/mini_installer/util/install_util.h"
#include "installer/mini_installer/util/installation_state.h"

#include "app/vivaldi_apptools.h"
#include "installer/util/vivaldi_install_util.h"

using base::win::RegKey;
using installer::InstallationState;

const wchar_t GoogleUpdateSettings::kPoliciesKey[] =
    L"SOFTWARE\\Policies\\Vivaldi\\Update";
const wchar_t GoogleUpdateSettings::kUpdatePolicyValue[] = L"UpdateDefault";
const wchar_t GoogleUpdateSettings::kDownloadPreferencePolicyValue[] =
    L"DownloadPreference";
const wchar_t GoogleUpdateSettings::kUpdateOverrideValuePrefix[] = L"Update";
const wchar_t GoogleUpdateSettings::kCheckPeriodOverrideMinutes[] =
    L"AutoUpdateCheckPeriodMinutes";

// Don't allow update periods longer than six weeks.
const int GoogleUpdateSettings::kCheckPeriodOverrideMinutesMax =
    60 * 24 * 7 * 6;

const GoogleUpdateSettings::UpdatePolicy
    GoogleUpdateSettings::kDefaultUpdatePolicy =
        GoogleUpdateSettings::UPDATES_DISABLED;

namespace {

base::LazyThreadPoolSequencedTaskRunner g_collect_stats_consent_task_runner =
    LAZY_THREAD_POOL_SEQUENCED_TASK_RUNNER_INITIALIZER(
        base::TaskTraits(base::TaskPriority::USER_VISIBLE,
                         base::TaskShutdownBehavior::BLOCK_SHUTDOWN));

// Reads the value |name| from the app's ClientState registry key in |root| into
// |value|.
bool ReadGoogleUpdateStrKeyFromRoot(HKEY root,
                                    const wchar_t* const name,
                                    std::wstring* value) {
  RegKey key;
  return key.Open(root, install_static::GetClientStateKeyPath().c_str(),
                  KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS &&
         key.ReadValue(name, value) == ERROR_SUCCESS;
}

// Returns the value |name| from the app's ClientState cohort registry key in
// |root|.
std::optional<std::wstring> ReadGoogleUpdateCohortStrKeyFromRoot(
    HKEY root,
    const wchar_t* const name) {
  std::wstring value;
  RegKey key;
  if (key.Open(
          root,
          install_static::GetClientStateKeyPath().append(L"\\cohort").c_str(),
          KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS &&
      key.ReadValue(name, &value) == ERROR_SUCCESS) {
    return value;
  }
  return std::nullopt;
}

// Reads the value |name| from the app's ClientState registry key in
// HKEY_CURRENT_USER into |value|. This function is only provided for legacy
// use. New code needing to load/store per-user data should use
// install_details::GetRegistryPath().
bool ReadUserGoogleUpdateStrKey(const wchar_t* const name,
                                std::wstring* value) {
  return ReadGoogleUpdateStrKeyFromRoot(HKEY_CURRENT_USER, name, value);
}

// Reads the value |name| from the app's ClientState registry key into |value|.
// This is primarily to be used for reading values written by Google Update
// during app install.
bool ReadGoogleUpdateStrKey(const wchar_t* const name, std::wstring* value) {
  return ReadGoogleUpdateStrKeyFromRoot(install_static::IsSystemInstall()
                                            ? HKEY_LOCAL_MACHINE
                                            : HKEY_CURRENT_USER,
                                        name, value);
}

// Reads the value |name| from the app's ClientState/cohort registry key.
std::optional<std::wstring> ReadGoogleUpdateCohortStrKey(
    const wchar_t* const name) {
  return ReadGoogleUpdateCohortStrKeyFromRoot(install_static::IsSystemInstall()
                                                  ? HKEY_LOCAL_MACHINE
                                                  : HKEY_CURRENT_USER,
                                              name);
}

// Writes |value| into |name| in the app's ClientState key in HKEY_CURRENT_USER.
// This function is only provided for legacy use. New code needing to load/store
// per-user data should use install_details::GetRegistryPath().
bool WriteUserGoogleUpdateStrKey(const wchar_t* const name,
                                 const std::wstring& value) {
  RegKey key;
  return key.Create(HKEY_CURRENT_USER,
                    install_static::GetClientStateKeyPath().c_str(),
                    KEY_SET_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS &&
         key.WriteValue(name, value.c_str()) == ERROR_SUCCESS;
}

// Deletes the value |name| from the app's ClientState key in HKEY_CURRENT_USER.
// Returns true if the value does not exist or is successfully deleted.
bool RemoveUserGoogleUpdateStrKey(const wchar_t* const name) {
  RegKey key;
  auto result = key.Open(HKEY_CURRENT_USER,
                         install_static::GetClientStateKeyPath().c_str(),
                         KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_32KEY);
  if (result == ERROR_PATH_NOT_FOUND || result == ERROR_FILE_NOT_FOUND)
    return true;  // The key doesn't exist; consider the value cleared.
  if (result != ERROR_SUCCESS)
    return false;  // Failed to open the key.

  std::wstring value;
  return key.ReadValue(name, &value) == ERROR_FILE_NOT_FOUND ||
         key.DeleteValue(name) == ERROR_SUCCESS;
}

}  // namespace

// TODO(grt): Remove this now that it has no added value.
bool GoogleUpdateSettings::IsSystemInstall() {
  return !InstallUtil::IsPerUserInstall();
}

base::SequencedTaskRunner*
GoogleUpdateSettings::CollectStatsConsentTaskRunner() {
  // TODO(fdoray): Use LazyThreadPoolSequencedTaskRunner::GetRaw() here instead
  // of .Get().get() when it's added to the API, http://crbug.com/730170.
  return g_collect_stats_consent_task_runner.Get().get();
}

bool GoogleUpdateSettings::GetCollectStatsConsent() {
  return false;
}

bool GoogleUpdateSettings::SetCollectStatsConsent(bool consented) {
  return false;
}

// static
bool GoogleUpdateSettings::GetCollectStatsConsentDefault(
    bool* stats_consent_default) {
  return false;
}

std::unique_ptr<metrics::ClientInfo>
GoogleUpdateSettings::LoadMetricsClientInfo() {
  std::wstring client_id_16;
  if (!ReadUserGoogleUpdateStrKey(google_update::kRegMetricsId,
                                  &client_id_16) ||
      client_id_16.empty()) {
    return nullptr;
  }

  std::unique_ptr<metrics::ClientInfo> client_info(new metrics::ClientInfo);
  client_info->client_id = base::WideToUTF8(client_id_16);

  std::wstring installation_date_str;
  if (ReadUserGoogleUpdateStrKey(google_update::kRegMetricsIdInstallDate,
                                 &installation_date_str)) {
    base::StringToInt64(installation_date_str, &client_info->installation_date);
  }

  std::wstring reporting_enbaled_date_date_str;
  if (ReadUserGoogleUpdateStrKey(google_update::kRegMetricsIdEnabledDate,
                                 &reporting_enbaled_date_date_str)) {
    base::StringToInt64(reporting_enbaled_date_date_str,
                        &client_info->reporting_enabled_date);
  }

  return client_info;
}

// static
std::optional<uint32_t> GoogleUpdateSettings::GetHashedCohortId() {
  std::optional<std::wstring> id =
      ReadGoogleUpdateCohortStrKey(google_update::kRegDefaultField);
  if (!id) {
    return std::nullopt;
  }
  std::string id_utf8 = base::WideToUTF8(*id);
  // Duplicate the logic of
  // ComponentMetricsProvider::ProvideSystemProfileMetrics: For component_id
  // strings in the "1:A:B" format, ignore the B segment; including it will
  // result in two clients assigned to the same cohort lineage (A) hashing to
  // different values. (The B segment tracks data about the size of fractional
  // cohorts that do not contain this client.)
  size_t last_colon = id_utf8.find_last_of(":");
  if (last_colon == std::string::npos) {
    // No colon separator indicates some unexpected id format; abandon trying
    // to interpret it.
    return std::nullopt;
  }
  return base::PersistentHash(std::string_view(id_utf8.c_str(), last_colon));
}

void GoogleUpdateSettings::StoreMetricsClientInfo(
    const metrics::ClientInfo& client_info) {
  // Attempt a best-effort at backing |client_info| in the registry (but don't
  // handle/report failures).
  WriteUserGoogleUpdateStrKey(google_update::kRegMetricsId,
                              base::UTF8ToWide(client_info.client_id));
  WriteUserGoogleUpdateStrKey(
      google_update::kRegMetricsIdInstallDate,
      base::NumberToWString(client_info.installation_date));
  WriteUserGoogleUpdateStrKey(
      google_update::kRegMetricsIdEnabledDate,
      base::NumberToWString(client_info.reporting_enabled_date));
}

// EULA consent is only relevant for system-level installs.
bool GoogleUpdateSettings::SetEulaConsent(
    const InstallationState& machine_state,
    bool consented) {
  const DWORD eula_accepted = consented ? 1 : 0;
  const REGSAM kAccess = KEY_SET_VALUE | KEY_WOW64_32KEY;
  RegKey key;

  // Write the consent value into the product's ClientStateMedium key.
  return key.Create(HKEY_LOCAL_MACHINE,
                    install_static::GetClientStateMediumKeyPath().c_str(),
                    kAccess) == ERROR_SUCCESS &&
         key.WriteValue(google_update::kRegEulaAceptedField, eula_accepted) ==
             ERROR_SUCCESS;
}

int GoogleUpdateSettings::GetLastRunTime() {
  std::wstring time_s;
  if (!ReadUserGoogleUpdateStrKey(google_update::kRegLastRunTimeField, &time_s))
    return -1;
  int64_t time_i;
  if (!base::StringToInt64(time_s, &time_i))
    return -1;
  base::TimeDelta td =
      base::Time::NowFromSystemTime() - base::Time::FromInternalValue(time_i);
  return td.InDays();
}

bool GoogleUpdateSettings::SetLastRunTime() {
  int64_t time = base::Time::NowFromSystemTime().ToInternalValue();
  return WriteUserGoogleUpdateStrKey(google_update::kRegLastRunTimeField,
                                     base::NumberToWString(time));
}

bool GoogleUpdateSettings::RemoveLastRunTime() {
  return RemoveUserGoogleUpdateStrKey(google_update::kRegLastRunTimeField);
}

bool GoogleUpdateSettings::GetBrowser(std::wstring* browser) {
  // Written by Google Update.
  return ReadGoogleUpdateStrKey(google_update::kRegBrowserField, browser);
}

bool GoogleUpdateSettings::GetLanguage(std::wstring* language) {
  // Written by Google Update.
  // Also written by the Vivaldi installer.
  if (vivaldi::IsVivaldiRunning()) {
    RegKey vivaldi_key;
    std::wstring lang;
    if (vivaldi_key.Open(HKEY_CURRENT_USER, vivaldi::constants::kVivaldiKey,
                         KEY_QUERY_VALUE) == ERROR_SUCCESS &&
        vivaldi_key.ReadValue(google_update::kRegLangField, &lang) ==
            ERROR_SUCCESS) {
      *language = lang;
      return true;
    }
  }

  return ReadGoogleUpdateStrKey(google_update::kRegLangField, language);
}

bool GoogleUpdateSettings::GetBrand(std::wstring* brand) {
  return false;  // Not used by Vivaldi
}

bool GoogleUpdateSettings::GetReactivationBrand(std::wstring* brand) {
  return false;  // Not used by Vivaldi
}

bool GoogleUpdateSettings::GetReferral(std::wstring* referral) {
  return false;  // Not used by Vivaldi
}

bool GoogleUpdateSettings::ClearReferral() {
  return false;  // Not used by Vivaldi
}

void GoogleUpdateSettings::SetProgress(bool system_install,
                                       const std::wstring& path,
                                       int progress) {
  DCHECK_GE(progress, 0);
  DCHECK_LE(progress, 100);
  const HKEY root = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  base::win::RegKey key(root, path.c_str(), KEY_SET_VALUE | KEY_WOW64_32KEY);
  if (key.Valid()) {
    key.WriteValue(google_update::kRegInstallerProgress,
                   static_cast<DWORD>(progress));
  }
}

// static
bool GoogleUpdateSettings::AreAutoupdatesEnabled() {
  // Chromium does not auto update.
  return false;
}

// static
bool GoogleUpdateSettings::ReenableAutoupdates() {
  // Non Google Chrome isn't going to autoupdate.
  return true;
}

// Reads and sanitizes the value of
// "HKLM\SOFTWARE\Policies\Google\Update\DownloadPreference". A valid
// group policy option must be a single alpha numeric word of up to 32
// characters.
std::wstring GoogleUpdateSettings::GetDownloadPreference() {
  RegKey policy_key;
  std::wstring value;
  if (policy_key.Open(HKEY_LOCAL_MACHINE, kPoliciesKey, KEY_QUERY_VALUE) ==
          ERROR_SUCCESS &&
      policy_key.ReadValue(kDownloadPreferencePolicyValue, &value) ==
          ERROR_SUCCESS) {
    // Validates that |value| matches `[a-zA-z]{0-32}`.
    const size_t kMaxValueLength = 32;
    if (value.size() > kMaxValueLength)
      return std::wstring();
    for (auto ch : value) {
      if (!base::IsAsciiAlpha(ch))
        return std::wstring();
    }
    return value;
  }
  return std::wstring();
}

std::wstring GoogleUpdateSettings::GetUninstallCommandLine(
    bool system_install) {
  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  std::wstring cmd_line;
  RegKey update_key;

  if (update_key.Open(root_key, google_update::kRegPathGoogleUpdate,
                      KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS) {
    update_key.ReadValue(google_update::kRegUninstallCmdLine, &cmd_line);
  }

  return cmd_line;
}

base::Version GoogleUpdateSettings::GetGoogleUpdateVersion(
    bool system_install) {
  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  std::wstring version;
  RegKey key;

  if (key.Open(root_key, google_update::kRegPathGoogleUpdate,
               KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS &&
      key.ReadValue(google_update::kRegGoogleUpdateVersion, &version) ==
          ERROR_SUCCESS) {
    return base::Version(base::WideToASCII(version));
  }

  return base::Version();
}

base::Time GoogleUpdateSettings::GetGoogleUpdateLastStartedAU(
    bool system_install) {
  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  RegKey update_key;

  if (update_key.Open(root_key, google_update::kRegPathGoogleUpdate,
                      KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS) {
    DWORD last_start;
    if (update_key.ReadValueDW(google_update::kRegLastStartedAUField,
                               &last_start) == ERROR_SUCCESS) {
      return base::Time::FromTimeT(last_start);
    }
  }

  return base::Time();
}

base::Time GoogleUpdateSettings::GetGoogleUpdateLastChecked(
    bool system_install) {
  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  RegKey update_key;

  if (update_key.Open(root_key, google_update::kRegPathGoogleUpdate,
                      KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS) {
    DWORD last_check;
    if (update_key.ReadValueDW(google_update::kRegLastCheckedField,
                               &last_check) == ERROR_SUCCESS) {
      return base::Time::FromTimeT(last_check);
    }
  }

  return base::Time();
}

bool GoogleUpdateSettings::GetUpdateDetailForApp(bool system_install,
                                                 const wchar_t* app_guid,
                                                 ProductData* data) {
  DCHECK(app_guid);
  DCHECK(data);

  bool product_found = false;

  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  std::wstring clientstate_reg_path(google_update::kRegPathClientState);
  clientstate_reg_path.append(L"\\");
  clientstate_reg_path.append(app_guid);

  RegKey clientstate;
  if (clientstate.Open(root_key, clientstate_reg_path.c_str(),
                       KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS) {
    std::wstring version;
    DWORD dword_value;
    if ((clientstate.ReadValueDW(google_update::kRegLastCheckSuccessField,
                                 &dword_value) == ERROR_SUCCESS) &&
        (clientstate.ReadValue(google_update::kRegVersionField, &version) ==
         ERROR_SUCCESS)) {
      product_found = true;
      data->version = base::WideToASCII(version);
      data->last_success = base::Time::FromTimeT(dword_value);
      data->last_result = 0;
      data->last_error_code = 0;
      data->last_extra_code = 0;

      if (clientstate.ReadValueDW(google_update::kRegLastInstallerResultField,
                                  &dword_value) == ERROR_SUCCESS) {
        // Google Update convention is that if an installer writes an result
        // code that is invalid, it is clamped to an exit code result.
        const DWORD kMaxValidInstallResult = 4;  // INSTALLER_RESULT_EXIT_CODE
        data->last_result = std::min(dword_value, kMaxValidInstallResult);
      }
      if (clientstate.ReadValueDW(google_update::kRegLastInstallerErrorField,
                                  &dword_value) == ERROR_SUCCESS) {
        data->last_error_code = dword_value;
      }
      if (clientstate.ReadValueDW(google_update::kRegLastInstallerExtraField,
                                  &dword_value) == ERROR_SUCCESS) {
        data->last_extra_code = dword_value;
      }
    }
  }

  return product_found;
}

bool GoogleUpdateSettings::GetUpdateDetailForGoogleUpdate(ProductData* data) {
  return GetUpdateDetailForApp(!InstallUtil::IsPerUserInstall(),
                               google_update::kGoogleUpdateUpgradeCode, data);
}

bool GoogleUpdateSettings::GetUpdateDetail(ProductData* data) {
  return GetUpdateDetailForApp(!InstallUtil::IsPerUserInstall(),
                               install_static::GetAppGuid(), data);
}
