// Copyright (c) 2013 Vivaldi Technologies AS. All rights reserved

#include "importer/viv_importer.h"

#include "build/build_config.h"
#include "ui/base/l10n/l10n_util.h"

#include "app/vivaldi_resources.h"
#include "importer/viv_importer_utils.h"

namespace viv_importer {

void DetectOperaMailProfiles(
    std::vector<user_data_importer::SourceProfile>* profiles) {
  user_data_importer::SourceProfile opera;
  opera.importer_name = l10n_util::GetStringUTF16(IDS_IMPORT_FROM_OPERA_MAIL);
  opera.importer_type = user_data_importer::TYPE_OPERA;
  opera.source_path = GetProfileDir(true);
  opera.mail_path = GetMailDirectory(true);
  opera.services_supported = user_data_importer::EMAIL;

  profiles->push_back(opera);
}

void DetectOperaProfiles(
    std::vector<user_data_importer::SourceProfile>* profiles) {
  user_data_importer::SourceProfile opera;
  opera.importer_name = l10n_util::GetStringUTF16(IDS_IMPORT_FROM_OPERA);
  opera.importer_type = user_data_importer::TYPE_OPERA;
  opera.source_path = GetProfileDir();
  opera.mail_path = GetMailDirectory();
#if BUILDFLAG(IS_WIN)
  opera.app_path = GetOperaInstallPathFromRegistry();
#endif
  opera.services_supported =
      user_data_importer::SPEED_DIAL | user_data_importer::FAVORITES |
      user_data_importer::NOTES | user_data_importer::PASSWORDS |
      user_data_importer::EMAIL | user_data_importer::MASTER_PASSWORD;

  profiles->push_back(opera);

  DetectOperaMailProfiles(profiles);
}
}  // namespace viv_importer
