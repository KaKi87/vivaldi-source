// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef EXTENSIONS_HELPER_MAIL_ATTACHMENTS_UTILS_H_
#define EXTENSIONS_HELPER_MAIL_ATTACHMENTS_UTILS_H_

#include <optional>
#include <string>
#include "base/files/file_path.h"

namespace download {
class DownloadItem;
}  // namespace download

namespace vivaldi {

base::FilePath GetMailAttachmentSavePath(
    const std::optional<std::string>& path,
    const base::FilePath& creator_suggested_filename);
base::FilePath GetMailAttachmentDownloadPath(download::DownloadItem* item);
void SetMailAttachmentDownloadPath(download::DownloadItem* item,
                                   base::FilePath download_path);

}  // namespace vivaldi

#endif  // EXTENSIONS_HELPER_MAIL_ATTACHMENTS_UTILS_H_
