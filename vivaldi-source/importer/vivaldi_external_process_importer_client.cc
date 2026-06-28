// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved.

#include "chrome/browser/importer/external_process_importer_client.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "app/vivaldi_resources.h"
#include "build/build_config.h"
#include "chrome/browser/importer/in_process_importer_bridge.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/password_manager/core/browser/password_manager_switches.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "content/public/browser/browser_thread.h"
#include "importer/imported_raw_password_form.h"
#include "importer/vivaldi_profile_import_process_messages.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(IS_LINUX)
#include "components/os_crypt/async/browser/freedesktop_secret_key_provider.h"
#include "components/os_crypt/async/browser/posix_key_provider.h"
#endif  // IS_LINUX

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#include <wincrypt.h>

#include "base/base64.h"
#endif  // IS_WIN

#if BUILDFLAG(IS_MAC)
#include "components/os_crypt/vivaldi_import_keychain_key_provider.h"
#endif  // IS_MAC

namespace {

std::string GetLogSafeOrigin(const GURL& url) {
  if (!url.is_valid()) {
    return std::string();
  }
  return url.DeprecatedGetOriginAsURL().spec();
}

#if BUILDFLAG(IS_WIN)
bool DecryptStringWithDPAPI(const std::string& ciphertext,
                            std::string* plaintext) {
  DATA_BLOB input;
  input.pbData =
      const_cast<BYTE*>(reinterpret_cast<const BYTE*>(ciphertext.data()));
  input.cbData = static_cast<DWORD>(ciphertext.size());

  DATA_BLOB output;
  if (!::CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0,
                            &output)) {
    return false;
  }

  plaintext->assign(reinterpret_cast<char*>(output.pbData), output.cbData);
  LocalFree(output.pbData);
  return true;
}

// Reads the source browser's encrypted key from its Local State file, decrypts
// it with DPAPI, and returns the raw AES key.
std::string GetSourceKeyFromLocalState(const base::FilePath& profile_dir) {
  base::FilePath local_state_file =
      profile_dir.DirName().AppendASCII("Local State");

  std::string file_contents;
  if (!base::ReadFileToString(local_state_file, &file_contents)) {
    return std::string();
  }

  std::optional<base::Value> root = base::JSONReader::Read(
      file_contents, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!root || !root->is_dict()) {
    return std::string();
  }

  const base::Value* os_crypt_dict = root->GetDict().Find("os_crypt");
  if (!os_crypt_dict) {
    return std::string();
  }

  const std::string* base64_key =
      os_crypt_dict->GetDict().FindString("encrypted_key");
  if (!base64_key) {
    return std::string();
  }

  std::string encrypted_key_with_header;
  if (!base::Base64Decode(*base64_key, &encrypted_key_with_header)) {
    return std::string();
  }

  // Key prefix for a key encrypted with DPAPI.
  const char kDPAPIKeyPrefix[] = "DPAPI";
  if (!base::StartsWith(encrypted_key_with_header, kDPAPIKeyPrefix,
                        base::CompareCase::SENSITIVE)) {
    return std::string();
  }

  // Strip the "DPAPI\0" prefix (sizeof includes the null terminator).
  std::string dpapi_blob =
      encrypted_key_with_header.substr(sizeof(kDPAPIKeyPrefix) - 1);

  std::string raw_key;
  if (!DecryptStringWithDPAPI(dpapi_blob, &raw_key)) {
    return std::string();
  }
  return raw_key;
}
#endif  // IS_WIN

// On linux we use async ui thread approach.
#if !BUILDFLAG(IS_LINUX) && !BUILDFLAG(IS_MAC)

std::string GetSourceRawKey(const base::FilePath& profile_dir,
                            user_data_importer::ImporterType importer_type) {
#if BUILDFLAG(IS_WIN)
  return GetSourceKeyFromLocalState(profile_dir);
#else
  NOTREACHED();
#endif
}

#endif  // !BUILDFLAG(IS_LINUX)

user_data_importer::ImportedPasswordForm MakePasswordFormWithoutPassword(
    const ImportedRawPasswordForm& raw) {
  user_data_importer::ImportedPasswordForm form;
  form.signon_realm = raw.signon_realm;
  form.url = raw.url;
  form.action = raw.action;
  form.username_element = raw.username_element;
  form.username_value = raw.username_value;
  form.password_element = raw.password_element;
  form.blocked_by_user = raw.blocked_by_user;
  return form;
}

template <typename DecryptPassword>
bool StoreDecryptedPasswordForms(
    const std::vector<ImportedRawPasswordForm>& raw_passwords,
    DecryptPassword decrypt_password,
    InProcessImporterBridge* bridge) {
  bool failed_decrypt = false;
  size_t succeeded_decrypt_count = 0;

  for (const auto& raw : raw_passwords) {
    user_data_importer::ImportedPasswordForm form =
        MakePasswordFormWithoutPassword(raw);

    if (decrypt_password(raw, &form.password_value)) {
      ++succeeded_decrypt_count;
      if (!form.username_value.empty() || !form.password_value.empty()) {
        bridge->SetPasswordForm(form);
      }
      continue;
    }

    const size_t prefix_length =
        std::min<size_t>(raw.password_value_cipher.size(), 3);
    LOG(WARNING) << "Password import decryption failed for "
                 << GetLogSafeOrigin(raw.url)
                 << " cipher_prefix="
                 << raw.password_value_cipher.substr(0, prefix_length);
    failed_decrypt = true;
  }

  LOG(INFO) << "Password import decrypted " << succeeded_decrypt_count << " of "
            << raw_passwords.size() << " raw password rows.";
  return failed_decrypt;
}

}  // namespace

void ExternalProcessImporterClient::OnImportItemFailed(
    user_data_importer::ImportItem import_item,
    const std::string& error_msg) {
  if (cancelled_)
    return;

  bridge_->NotifyItemFailed(import_item, error_msg);
}

void ExternalProcessImporterClient::OnNotesImportStart(
    const std::u16string& first_folder_name,
    uint32_t total_notes_count) {
  if (cancelled_)
    return;

  notes_first_folder_name_ = first_folder_name;
  total_notes_count_ = total_notes_count;
  notes_.reserve(total_notes_count);
}

void ExternalProcessImporterClient::OnNotesImportGroup(
    const std::vector<ImportedNotesEntry>& notes_group) {
  if (cancelled_)
    return;

  // Collect sets of bookmarks from importer process until we have reached
  // total_bookmarks_count_:
  notes_.insert(notes_.end(), notes_group.begin(), notes_group.end());
  if (notes_.size() == total_notes_count_)
    bridge_->AddNotes(notes_, notes_first_folder_name_);
}

void ExternalProcessImporterClient::OnSpeedDialImportStart(
    uint32_t total_count) {
  if (cancelled_)
    return;

  total_speeddial_count_ = total_count;
  speeddial_.reserve(total_count);
}

void ExternalProcessImporterClient::OnSpeedDialImportGroup(
    const std::vector<ImportedSpeedDialEntry>& group) {
  if (cancelled_)
    return;

  speeddial_.insert(speeddial_.end(), group.begin(), group.end());
  if (speeddial_.size() == total_speeddial_count_)
    bridge_->AddSpeedDial(speeddial_);
}

void ExternalProcessImporterClient::OnExtensionsImportStart(
    uint32_t total_count) {
  if (cancelled_)
    return;

  total_extensions_count_ = total_count;
  extensions_.reserve(total_count);
}

void ExternalProcessImporterClient::OnExtensionsImportGroup(
    const std::vector<std::string>& group) {
  if (cancelled_)
    return;

  extensions_.insert(extensions_.end(), group.begin(), group.end());
  if (extensions_.size() == total_extensions_count_)
    bridge_->AddExtensions(extensions_);
}

void ExternalProcessImporterClient::OnTabImportStart(uint32_t total_count) {
  if (cancelled_)
    return;

  total_tab_count_ = total_count;
  tabs_.reserve(total_count);
}

void ExternalProcessImporterClient::OnTabImportGroup(
    const std::vector<ImportedTabEntry>& group) {
  if (cancelled_)
    return;

  tabs_.insert(tabs_.end(), group.begin(), group.end());
  if (tabs_.size() == total_tab_count_)
    bridge_->AddOpenTabs(tabs_);
}

void ExternalProcessImporterClient::OnRawPasswordsImportStart(
    uint32_t total_count) {
  if (cancelled_)
    return;

  total_raw_passwords_count_ = total_count;
  raw_passwords_.reserve(total_count);
}

void ExternalProcessImporterClient::OnRawPasswordsImportGroup(
    const std::vector<ImportedRawPasswordForm>& group) {
  if (cancelled_)
    return;

  raw_passwords_.insert(raw_passwords_.end(), group.begin(), group.end());
  if (raw_passwords_.size() == total_raw_passwords_count_) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
    if (raw_passwords_.empty())
      return;

    pending_password_decryption_ = true;
    pending_password_decryption_failed_.reset();

    auto raw_passwords_copy = std::move(raw_passwords_);
    if (raw_passwords_copy[0].source_product_name.empty()) {
      OnDecryptorReady(raw_passwords_copy, nullptr);
      return;
    }

    GetDecryptorOnUIThread(
        raw_passwords_copy[0].source_product_name,
        base::BindOnce(&ExternalProcessImporterClient::OnDecryptorReady,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(raw_passwords_copy)));
#else
    std::string raw_key =
        GetSourceRawKey(source_profile_.source_path,
                        source_profile_.importer_type);
    if (raw_key.empty()) {
      LOG(WARNING) << "Password import decryption key unavailable.";
    }

    const bool failed_decrypt = StoreDecryptedPasswordForms(
        raw_passwords_,
        [&raw_key](const ImportedRawPasswordForm& raw,
                   std::u16string* password_value) {
          return !raw_key.empty() &&
                 os_crypt_async::Encryptor::DecryptString16WithRawKey(
                     raw.password_value_cipher, password_value,
                     base::as_byte_span(raw_key));
        },
        bridge_.get());
    if (failed_decrypt) {
      password_import_failed_ = true;
      bridge_->NotifyItemFailed(
          user_data_importer::PASSWORDS,
          l10n_util::GetStringUTF8(
              IDS_IMPORT_ERROR_PASSWORD_DECRYPTION_FAILED));
    }
    std::ranges::fill(raw_key, '\0');
#endif
  }
}

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
void ExternalProcessImporterClient::GetDecryptorOnUIThread(
    const std::string& product_name,
    base::OnceCallback<void(scoped_refptr<os_crypt_async::Encryptor>)>
        callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
#if BUILDFLAG(IS_LINUX)
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string password_store =
      command_line.GetSwitchValueASCII(password_manager::kPasswordStore);
  if (password_store.empty()) {
    password_store = "gnome-libsecret";
  }

  auto freedesktop_provider =
      std::make_unique<os_crypt_async::FreedesktopSecretKeyProvider>(
          password_store, product_name, nullptr /* bus (shared session) */);
  auto posix_provider = std::make_unique<os_crypt_async::PosixKeyProvider>();
#endif
  std::vector<std::pair<os_crypt_async::OSCryptAsync::Precedence,
                        std::unique_ptr<os_crypt_async::KeyProvider>>>
      providers;
#if BUILDFLAG(IS_LINUX)
  providers.emplace_back(/*precedence=*/10u, std::move(freedesktop_provider));
  providers.emplace_back(/*precedence=*/5u, std::move(posix_provider));
#elif BUILDFLAG(IS_MAC)
  auto macos_provider =
      std::make_unique<os_crypt_async::VivaldiImportKeychainKeyProvider>(
          source_profile_.importer_type);
  providers.emplace_back(/*precedence*/10u, std::move(macos_provider));
#endif

  pending_password_os_crypt_ =
      std::make_unique<os_crypt_async::OSCryptAsync>(std::move(providers));
  pending_password_os_crypt_->GetInstance(std::move(callback));
}

void ExternalProcessImporterClient::OnDecryptorReady(
    const std::vector<ImportedRawPasswordForm>& raw_passwords,
    scoped_refptr<os_crypt_async::Encryptor> decryptor) {
  const bool can_decrypt = decryptor && decryptor->IsDecryptionAvailable();

  if (!can_decrypt && !raw_passwords.empty()) {
    LOG(WARNING) << "Password import decryption unavailable. decryptor="
                 << static_cast<bool>(decryptor)
                 << " decryption_available="
                 << (decryptor ? decryptor->IsDecryptionAvailable() : false)
                 << " raw_password_count=" << raw_passwords.size();
  }

  const bool failed_decrypt = StoreDecryptedPasswordForms(
      raw_passwords,
      [&decryptor, can_decrypt](const ImportedRawPasswordForm& raw,
                                std::u16string* password_value) {
        return can_decrypt &&
               decryptor->DecryptString16(raw.password_value_cipher,
                                          password_value);
      },
      bridge_.get());
  CompletePendingPasswordImport(failed_decrypt);
}

void ExternalProcessImporterClient::CompletePendingPasswordImport(
    bool failed_decrypt) {
  pending_password_decryption_failed_ = failed_decrypt;
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ExternalProcessImporterClient::MaybeCompletePendingPasswordImport,
          weak_ptr_factory_.GetWeakPtr()));
}

void ExternalProcessImporterClient::MaybeCompletePendingPasswordImport() {
  if (!pending_password_decryption_failed_.has_value() ||
      !deferred_password_item_finished_) {
    return;
  }

  pending_password_decryption_ = false;
  pending_password_os_crypt_.reset();
  deferred_password_item_finished_ = false;

  const bool failed_decrypt = *pending_password_decryption_failed_;
  pending_password_decryption_failed_.reset();
  if (failed_decrypt) {
    LOG(WARNING) << "Password import completed with decryption failures.";
    bridge_->NotifyItemFailed(
        user_data_importer::PASSWORDS,
        l10n_util::GetStringUTF8(IDS_IMPORT_ERROR_PASSWORD_DECRYPTION_FAILED));
    profile_import_->ReportImportItemFinished(user_data_importer::PASSWORDS);
  } else {
    bridge_->NotifyItemEnded(user_data_importer::PASSWORDS);
    profile_import_->ReportImportItemFinished(user_data_importer::PASSWORDS);
  }

  if (deferred_import_finished_.has_value()) {
    const bool succeeded = deferred_import_finished_->succeeded;
    const std::string error_msg = deferred_import_finished_->error_msg;
    deferred_import_finished_.reset();
    OnImportFinished(succeeded, error_msg);
    return;
  }
}
#endif  // IS_LINUX
