// Copyright (c) 2013 Vivaldi Technologies AS. All rights reserved

#ifndef IMPORTER_CHROMIUM_IMPORTER_H_
#define IMPORTER_CHROMIUM_IMPORTER_H_

#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/utility/importer/importer.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "components/user_data_importer/common/importer_type.h"
#include "components/user_data_importer/common/importer_url_row.h"

#include "importer/viv_import_result.h"

class ImportedNoteEntry;
struct ImportedRawPasswordForm;

class ChromiumImporter : public Importer {
 public:
  ChromiumImporter();
  ImportResult ImportPasswords(user_data_importer::ImporterType importer_type);

  // Importer
  void StartImport(const user_data_importer::SourceProfile& source_profile,
                   uint16_t items,
                   ImporterBridge* bridge) override;

 protected:
  ~ChromiumImporter() override;
  ChromiumImporter(const ChromiumImporter&) = delete;
  ChromiumImporter& operator=(const ChromiumImporter&) = delete;

 private:
  base::FilePath profile_dir_;
  base::FilePath::StringType bookmarkfilename_;
  ImportResult ImportBookMarks();
  ImportResult ImportHistory();
  ImportResult ImportExtensions();
  ImportResult ImportTabs(user_data_importer::ImporterType importer_type);

  /// Reads raw (encrypted) sign-ons from Login Data and populates |forms| with
  /// un-decrypted forms.  Decryption is deferred to the browser process via the
  /// AddRawPasswords bridge.
  ImportResult ReadRawSignons(const base::FilePath& sqlite_file,
                              std::vector<ImportedRawPasswordForm>* forms,
                              user_data_importer::ImporterType importer_type);

  ImportResult ReadAndParseHistory(
      const base::FilePath& sqlite_file,
      std::vector<user_data_importer::ImporterURLRow>* forms);
};

#endif  // IMPORTER_CHROMIUM_IMPORTER_H_
