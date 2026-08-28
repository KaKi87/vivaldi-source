// Copyright (c) 2015 Vivaldi Technologies AS. All rights reserved

#include "importer/chromium_importer.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/grit/branded_strings.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "importer/imported_raw_password_form.h"
#include "importer/imported_tab_entry.h"
#include "sql/statement.h"

#include "app/vivaldi_resources.h"
#include "components/sessions/vivaldi_session_service_commands.h"
#include "importer/chromium_extension_importer.h"
#include "importer/chromium_session_importer.h"
#include "importer/import_error_utils.h"

namespace {

// Returns the product name used by the source browser for keyring lookups.
std::string GetSourceProductName(user_data_importer::ImporterType type) {
  switch (type) {
    case user_data_importer::TYPE_CHROME:
      return "Google Chrome";
    case user_data_importer::TYPE_CHROMIUM:
      return "Chromium";
    case user_data_importer::TYPE_BRAVE:
      return "Brave-Browser";
    case user_data_importer::TYPE_EDGE_CHROMIUM:
      return "Microsoft Edge";
    case user_data_importer::TYPE_OPERA_OPIUM:
    case user_data_importer::TYPE_OPERA_OPIUM_BETA:
    case user_data_importer::TYPE_OPERA_OPIUM_DEV:
      return "Opera";
    case user_data_importer::TYPE_OPERA_GX:
      return "Opera GX";
    case user_data_importer::TYPE_VIVALDI:
      return "Vivaldi";
    case user_data_importer::TYPE_YANDEX:
      return "Yandex";
    case user_data_importer::TYPE_ARC:
      return "Arc";
    default:
      return l10n_util::GetStringUTF8(IDS_PRODUCT_NAME);
  }
}

}  // namespace

ChromiumImporter::ChromiumImporter() {}

ChromiumImporter::~ChromiumImporter() {}

void ChromiumImporter::StartImport(
    const user_data_importer::SourceProfile& source_profile,
    uint16_t items,
    ImporterBridge* bridge) {
  bridge_ = bridge;
  std::string name = source_profile.selected_profile_name;
  if (name.empty()) {
    name = "Default";  // Fallback to Default profile if no profile specified
  }
  profile_dir_ = source_profile.source_path.AppendASCII(name);

  bridge_->NotifyStarted();

  if ((items & user_data_importer::HISTORY) && !cancelled()) {
    bridge_->NotifyItemStarted(user_data_importer::HISTORY);
    const auto res = ImportHistory();
    import_result::NotifyBridge(res, bridge_.get(),
                                user_data_importer::HISTORY);
  }

  if ((items & user_data_importer::FAVORITES) && !cancelled()) {
    base::FilePath bookmarkFilePath = profile_dir_.AppendASCII("Bookmarks");

    bookmarkfilename_ = bookmarkFilePath.value();
    if (!base::PathExists(bookmarkFilePath)) {
      // Just notify about start and end if the file doesn't exist, otherwise
      // the end of import detection won't work.
      bridge_->NotifyItemStarted(user_data_importer::FAVORITES);
      bridge_->NotifyItemEnded(user_data_importer::FAVORITES);
    } else {
      bridge_->NotifyItemStarted(user_data_importer::FAVORITES);
      const auto res = ImportBookMarks();
      import_result::NotifyBridge(res, bridge_.get(),
                                  user_data_importer::FAVORITES);
    }
  }

  if ((items & user_data_importer::PASSWORDS) && !cancelled()) {
    bridge_->NotifyItemStarted(user_data_importer::PASSWORDS);

    const auto res = ImportPasswords(source_profile.importer_type);
    import_result::NotifyBridge(res, bridge_.get(),
                                user_data_importer::PASSWORDS);
  }

  if ((items & user_data_importer::TABS) && !cancelled()) {
    bridge_->NotifyItemStarted(user_data_importer::TABS);
    const auto res = ImportTabs(source_profile.importer_type);
    import_result::NotifyBridge(res, bridge_.get(), user_data_importer::TABS);
  }

  if ((items & user_data_importer::EXTENSIONS) && !cancelled()) {
    bridge_->NotifyItemStarted(user_data_importer::EXTENSIONS);
    const auto res = ImportExtensions();
    import_result::NotifyBridge(res, bridge_.get(),
                                user_data_importer::EXTENSIONS);
  }
  bridge_->NotifyEnded();
}

ImportResult ChromiumImporter::ImportPasswords(
    user_data_importer::ImporterType importer_type) {
  base::FilePath file = profile_dir_.AppendASCII("Login Data");
  auto file_check = ImportFileOperations::CheckFileExists(
      file, IDS_IMPORT_ERROR_LOGIN_DATA_NOT_FOUND);
  if (!file_check.has_value()) {
    return file_check;
  }

  // Read raw (encrypted) forms and send via bridge for browser-process
  // decryption. This avoids blocking on keyring/DPAPI/keychain calls in the
  // importer (utility) process which has no message loop.
  std::vector<ImportedRawPasswordForm> raw_forms;
  auto read_result = ReadRawSignons(file, &raw_forms, importer_type);
  if (!read_result.has_value()) {
    return read_result;
  }

  if (!cancelled() && !raw_forms.empty()) {
    bridge_->AddRawPasswords(raw_forms);
  }

  return import_result::Success();
}

// Reads raw (encrypted) sign-ons from Login Data and populates |forms| with
// un-decrypted forms.  Decryption is deferred to the browser process via the
// AddRawPasswords bridge.
ImportResult ChromiumImporter::ReadRawSignons(
    const base::FilePath& sqlite_file,
    std::vector<ImportedRawPasswordForm>* forms,
    user_data_importer::ImporterType importer_type) {
  sql::Database db("Importer");
  auto db_result = ImportDatabaseOperations::OpenDatabase(
      sqlite_file, &db, IDS_IMPORT_ERROR_LOGIN_DATABASE_OPEN_FAILED);
  if (!db_result.has_value()) {
    return db_result;
  }

  const std::string query =
      "SELECT origin_url, action_url, username_element, username_value, "
      "password_element, password_value, signon_realm "
      "FROM logins";

  sql::Statement s2{db.GetUniqueStatement(query)};
  if (!s2.is_valid()) {
    return import_result::Error(IDS_IMPORT_ERROR_LOGIN_DATABASE_READ_FAILED);
  }

  const std::string product_name = GetSourceProductName(importer_type);

  while (s2.Step()) {
    ImportedRawPasswordForm form;
    form.url = GURL(s2.ColumnString(0));
    form.action = GURL(s2.ColumnString(1));
    form.username_element = base::UTF8ToUTF16(s2.ColumnString(2));
    form.username_value = base::UTF8ToUTF16(s2.ColumnString(3));
    form.password_element = base::UTF8ToUTF16(s2.ColumnString(4));
    form.password_value_cipher = s2.ColumnBlobAsString(5);
    form.signon_realm = s2.ColumnString(6);
    form.source_product_name = product_name;

    forms->push_back(std::move(form));
  }

  return import_result::Success();
}

ImportResult ChromiumImporter::ImportHistory() {
  std::vector<user_data_importer::ImporterURLRow> historyRows;
  base::FilePath source_path = profile_dir_;

  base::FilePath file = source_path.AppendASCII("History");
  auto file_check = ImportFileOperations::CheckFileExists(
      file, IDS_IMPORT_ERROR_HISTORY_FILE_NOT_FOUND);
  if (!file_check.has_value()) {
    return file_check;
  }

  auto res = ReadAndParseHistory(file, &historyRows);
  if (!res.has_value()) {
    return res;
  }

  if (!historyRows.empty() && !cancelled()) {
    bridge_->SetHistoryItems(
        historyRows, user_data_importer::VISIT_SOURCE_CHROMIUM_IMPORTED);
  }

  return import_result::Success();
}

ImportResult ChromiumImporter::ReadAndParseHistory(
    const base::FilePath& sqlite_file,
    std::vector<user_data_importer::ImporterURLRow>* forms) {
  sql::Database db("Importer");
  auto db_result = ImportDatabaseOperations::OpenDatabase(
      sqlite_file, &db, IDS_IMPORT_ERROR_HISTORY_DATABASE_OPEN_FAILED);
  if (!db_result.has_value()) {
    return db_result;
  }

  const std::string query =
      "SELECT url, title, visit_count, hidden, typed_count, case when "
      "last_visit_time = 0 then 1 else last_visit_time end as last_visit_time "
      "FROM urls";

  sql::Statement s2{db.GetUniqueStatement(query)};
  if (!s2.is_valid()) {
    return import_result::Error(IDS_IMPORT_ERROR_HISTORY_DATABASE_QUERY_FAILED);
  }

  while (s2.Step()) {
    user_data_importer::ImporterURLRow row(GURL(s2.ColumnString(0)));
    row.title = s2.ColumnString16(1);
    row.visit_count = s2.ColumnInt(2);
    row.hidden = s2.ColumnInt(3) == 1;
    row.typed_count = s2.ColumnInt(4);

    base::Time t = base::Time::FromInternalValue(s2.ColumnInt64(5));
    row.last_visit = t;
    forms->push_back(std::move(row));
  }
  return import_result::Success();
}

ImportResult ChromiumImporter::ImportExtensions() {
  const auto extensions =
      extension_importer::ChromiumExtensionsImporter::GetImportableExtensions(
          profile_dir_);
  if (!extensions.empty() && !cancelled()) {
    bridge_->AddExtensions(extensions);
  }

  return import_result::Success();
}

ImportResult ChromiumImporter::ImportTabs(
    user_data_importer::ImporterType importer_type) {
  const sessions::IdToSessionTab tabs =
      session_importer::ChromiumSessionImporter::GetOpenTabs(profile_dir_,
                                                             importer_type);

  std::vector<ImportedTabEntry> imported_tabs;

  std::transform(tabs.begin(), tabs.end(), std::back_inserter(imported_tabs),
                 [](const auto& it) {
                   return ImportedTabEntry::FromSessionTab(*it.second);
                 });

  bridge_->AddOpenTabs(std::move(imported_tabs));

  return import_result::Success();
}
