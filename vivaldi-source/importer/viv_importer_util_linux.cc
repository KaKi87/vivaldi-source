// Copyright (c) 2013 Vivaldi Technologies AS. All rights reserved

#include <string>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "chrome/browser/shell_integration.h"

#include "importer/viv_importer_utils.h"

base::FilePath GetProfileDir(bool mail) {
  // Standalone Client does not exists for Linux
  if (mail)
    return base::FilePath();

  base::FilePath profile_dir;
  // The default location of the profile folder containing user data is
  // under user HOME directory in .opera folder on Linux.
  base::FilePath home = base::GetHomeDir();
  if (!home.empty()) {
    profile_dir = home.Append(".opera/");
  }
  if (base::PathExists(profile_dir))
    return profile_dir;

  return base::FilePath();
}

base::FilePath GetMailDirectory(bool mail) {
  // Standalone Client does not exists for Linux
  if (mail)
    return base::FilePath();

  base::FilePath mail_directory;
  // The default location of the opera mail folder containing is
  // under user HOME directory in .opera/mail folder on Linux.
  base::FilePath home = base::GetHomeDir();
  if (!home.empty()) {
    mail_directory = home.Append(".opera/mail");
  }
  if (base::PathExists(mail_directory))
    return mail_directory;

  return base::FilePath();
}

base::FilePath GetThunderbirdMailDirectory() {
  base::FilePath mail_directory;
  base::FilePath home = base::GetHomeDir();
  if (!home.empty()) {
    mail_directory = home.Append(".thunderbird");
  }

  if (base::PathExists(mail_directory)) {
    return mail_directory;
  }

  // For thunderbird installed with snap
  mail_directory = home.Append("snap/thunderbird/common/.thunderbird");
  if (base::PathExists(mail_directory)) {
     return mail_directory;
  }

  return base::FilePath();
}
