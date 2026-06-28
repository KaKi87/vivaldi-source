// Copyright 2025 Vivaldi Technologies. All rights reserved.

#ifndef VIVALDI_COMPONENTS_OS_CRYPT_VIVALDI_IMPORT_KEYCHAIN_KEY_PROVIDER_H_
#define VIVALDI_COMPONENTS_OS_CRYPT_VIVALDI_IMPORT_KEYCHAIN_KEY_PROVIDER_H_

#include "components/os_crypt/async/browser/key_provider.h"
#include "components/user_data_importer/common/importer_type.h"

namespace os_crypt_async {

// Provides an encryption key derived from a password stored in the macOS/iOS
// Keychain. This is compatible with the synchronous OSCrypt implementation.
class VivaldiImportKeychainKeyProvider
    : public KeyProvider {
 public:
  VivaldiImportKeychainKeyProvider(
      user_data_importer::ImporterType importer_type);
  ~VivaldiImportKeychainKeyProvider() override;

 private:
  // os_crypt_async::KeyProvider interface.
  void GetKey(KeyCallback callback) override;
  bool UseForEncryption() override;
  bool IsCompatibleWithOsCryptSync() override;

  user_data_importer::ImporterType importer_type_;
};

}  // namespace os_crypt_async

#endif  // VIVALDI_COMPONENTS_OS_CRYPT_VIVALDI_IMPORT_KEYCHAIN_KEY_PROVIDER_H_
