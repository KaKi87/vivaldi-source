// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef EXTENSIONS_API_DOWNLOADS_MAIL_ATTACHMENT_PATH_H_
#define EXTENSIONS_API_DOWNLOADS_MAIL_ATTACHMENT_PATH_H_

#include "base/supports_user_data.h"
namespace vivaldi {

class MailAttachmentPath : public base::SupportsUserData::Data {
 public:
  static inline constexpr int kMailAttachmentPathUserDataKey = 0;

  explicit MailAttachmentPath(base::FilePath mail_save_path)
      : mail_save_path_(std::move(mail_save_path)) {}

  const base::FilePath& save_path() const { return mail_save_path_; }

 private:
  base::FilePath mail_save_path_;
};

}  // namespace vivaldi

#endif  // EXTENSIONS_API_DOWNLOADS_MAIL_ATTACHMENT_PATH_H_
