// Copyright 2025 Vivaldi Technologies. All rights reserved.

#include "components/os_crypt/vivaldi_import_keychain_key_provider.h"

#include <array>
#include <memory>

#include "base/apple/osstatus_logging.h"
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "components/os_crypt/async/common/algorithm.mojom.h"
#include "components/os_crypt/common/os_crypt_switches.h"
#include "crypto/apple/fake_keychain_v2.h"
#include "crypto/apple/keychain_v2.h"
#include "crypto/kdf.h"
#include "crypto/subtle_passkey.h"

namespace os_crypt_async {

namespace {

  void GetSourceKeychainNames(user_data_importer::ImporterType importer_type,
                              std::string* service_name,
                              std::string* account_name) {
    switch (importer_type) {
      case user_data_importer::TYPE_CHROME:
      case user_data_importer::TYPE_CHROMIUM:
        *service_name = "Chrome Safe Storage";
        *account_name = "Chrome";
        break;
      case user_data_importer::TYPE_BRAVE:
        *service_name = "Brave Safe Storage";
        *account_name = "Brave";
        break;
      case user_data_importer::TYPE_EDGE_CHROMIUM:
        *service_name = "Microsoft Edge Safe Storage";
        *account_name = "Microsoft Edge";
        break;
      case user_data_importer::TYPE_OPERA_OPIUM:
      case user_data_importer::TYPE_OPERA_OPIUM_BETA:
      case user_data_importer::TYPE_OPERA_OPIUM_DEV:
        *service_name = "Opera Safe Storage";
        *account_name = "Opera";
        break;
      case user_data_importer::TYPE_OPERA_GX:
        *service_name = "Opera GX Safe Storage";
        *account_name = "Opera GX";
        break;
      case user_data_importer::TYPE_VIVALDI:
        *service_name = "Vivaldi Safe Storage";
        *account_name = "Vivaldi";
        break;
      case user_data_importer::TYPE_YANDEX:
        *service_name = "Yandex Safe Storage";
        *account_name = "Yandex";
        break;
      case user_data_importer::TYPE_ARC:
        *service_name = "Arc Safe Storage";
        *account_name = "Arc";
        break;
      default:
        *service_name = "Chrome Safe Storage";
        *account_name = "Chrome";
        break;
    }
  }

  // See chromium/components/os_crypt/async/browser/keychain_key_provider.mm
  constexpr char kKeyTag[] = "v10";
  constexpr size_t kDerivedKeySize = 16;
  constexpr auto kSalt =
      std::to_array<uint8_t>({'s', 'a', 'l', 't', 'y', 's', 'a', 'l', 't'});
  constexpr size_t kIterations = 1003;

  base::expected<Encryptor::Key, KeyProvider::KeyError> GetKeyTask(
      crypto::SubtlePassKey subtle_passkey,
      user_data_importer::ImporterType importer_type) {
    std::string service_name, account_name;
    GetSourceKeychainNames(importer_type, &service_name, &account_name);

    if (service_name.empty() || account_name.empty()) {
        return base::unexpected(KeyProvider::KeyError::kPermanentlyUnavailable);
    }

    crypto::apple::KeychainV2* keychain_to_use =
        &crypto::apple::KeychainV2::GetInstance();

    auto password =
      keychain_to_use->FindGenericPassword(service_name, account_name);

    // `password` can be an empty string if keychain access is denied by the user
    // or some other error occurs.
    if (!password.has_value()) {
        return base::unexpected(KeyProvider::KeyError::kTemporarilyUnavailable);
    }

    std::array<uint8_t, kDerivedKeySize> key_bytes;
    crypto::kdf::Pbkdf2HmacSha1({.iterations = kIterations},
                                base::as_byte_span(*password), kSalt, key_bytes,
                                subtle_passkey);

    return Encryptor::Key(key_bytes, mojom::Algorithm::kAES128CBC);
  }
}

VivaldiImportKeychainKeyProvider::VivaldiImportKeychainKeyProvider(
    user_data_importer::ImporterType importer_type)
    : importer_type_(importer_type) {}

VivaldiImportKeychainKeyProvider::~VivaldiImportKeychainKeyProvider() {}

// os_crypt_async::KeyProvider interface.
void VivaldiImportKeychainKeyProvider::GetKey(KeyCallback callback) {
  // Apple's documentation [1] recommends accessing the keychain on a worker
  // thread. [1]
  // https://developer.apple.com/documentation/security/secitemcopymatching%28_%3A_%3A%29#Performance-considerations
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(&GetKeyTask, crypto::SubtlePassKey{}, importer_type_),
      base::BindOnce(std::move(callback), kKeyTag));
}

bool VivaldiImportKeychainKeyProvider::UseForEncryption() {
  return false;
}

bool VivaldiImportKeychainKeyProvider::IsCompatibleWithOsCryptSync() {
  return true;
}

}  // namespace os_crypt_async
