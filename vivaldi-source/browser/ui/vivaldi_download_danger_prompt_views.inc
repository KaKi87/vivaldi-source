// Copyright (c) 2024-2026 Vivaldi Technologies AS. All rights reserved

void DownloadDangerDialogDelegate::RunScanAndClose(const ui::Event& event) {
  ui::DialogModelPasswordField* password_field =
      dialog_model()->GetPasswordFieldByUniqueId(kVivaldiFilePasswordFieldId);
  std::u16string password = password_field->text();
  if (password.empty()) {
    password_field->Invalidate();
    return;
  }

#if BUILDFLAG(SAFE_BROWSING_AVAILABLE)
  if (download_ &&
      download_->GetDangerType() ==
          download::DOWNLOAD_DANGER_TYPE_PROMPT_FOR_LOCAL_PASSWORD_SCANNING) {
    LOG(ERROR) << " checking danger";
    safe_browsing::DownloadProtectionService::CheckDownloadWithLocalDecryption(
        download_, base::UTF16ToUTF8(password));
  }
#endif
}
