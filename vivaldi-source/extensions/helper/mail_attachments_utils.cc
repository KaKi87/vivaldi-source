// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "mail_attachments_utils.h"

#include "extensions/api/downloads/mail_attachment_path.h"

#include "base/files/file_util.h"
#include "base/strings/escape.h"
#include "components/download/public/common/download_item.h"

namespace vivaldi {

namespace {

bool IsPathSafe(const base::FilePath& folder_path) {
  return !folder_path.empty() && !folder_path.ReferencesParent() &&
         folder_path.IsAbsolute();
}

}  // namespace

base::FilePath GetMailAttachmentSavePath(
    const std::optional<std::string>& path,
    const base::FilePath& creator_suggested_filename) {
  if (!path) {
    return base::FilePath();
  }
  std::string unescaped_path = base::UnescapeURLComponent(
      *path, base::UnescapeRule::SPACES | base::UnescapeRule::PATH_SEPARATORS |
                 base::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS);
  base::FilePath creator_suggested_path =
      base::FilePath::FromUTF8Unsafe(unescaped_path);

  if (!IsPathSafe(creator_suggested_path)) {
    return base::FilePath();
  }

  if (!base::DirectoryExists(creator_suggested_path)) {
    return base::FilePath();
  }

  base::FilePath full_path;
  full_path = creator_suggested_path.Append(creator_suggested_filename);

  if (!IsPathSafe(full_path)) {
    return base::FilePath();
  }

  return full_path;
}

base::FilePath GetMailAttachmentDownloadPath(download::DownloadItem* item) {
  if (!item)
    return base::FilePath();

  auto* state = static_cast<vivaldi::MailAttachmentPath*>(item->GetUserData(
      &vivaldi::MailAttachmentPath::kMailAttachmentPathUserDataKey));

  if (state && !state->save_path().empty()) {
    return state->save_path();
  }

  return base::FilePath();
}

void SetMailAttachmentDownloadPath(download::DownloadItem* item,
                                   base::FilePath download_path) {
  item->SetUserData(
      &vivaldi::MailAttachmentPath::kMailAttachmentPathUserDataKey,
      std::make_unique<vivaldi::MailAttachmentPath>(std::move(download_path)));
}

}  // namespace vivaldi
