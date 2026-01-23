// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved
//
// Based on Chromium code with the following copyright:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_ASYNC_SECRET_PORTAL_KEY_PROVIDER_VIVALDI_H_
#define COMPONENTS_OS_CRYPT_ASYNC_SECRET_PORTAL_KEY_PROVIDER_VIVALDI_H_

#include "components/os_crypt/async/browser/secret_portal_key_provider.h"

class PrefRegistrySimple;
class PrefService;

namespace os_crypt_async {

// Vivaldi-specific implementation of SecretPortalKeyProvider that uses
// PBKDF2-HMAC-SHA1 with AES-128-CBC instead of HKDF-SHA-256 with AES-256-GCM
// for compatibility with existing sync implementation which was derived from
// the other linux backend handling implementations (i.e. same key derivation is
// done in freedesktop_secret_key_provider.cc in async).
//
// This class inherits from SecretPortalKeyProvider and overrides only the
// ReceivedSecret() method to apply Vivaldi's custom key derivation logic.
class VivaldiSecretPortalKeyProvider : public SecretPortalKeyProvider {
 public:
  static void RegisterLocalPrefs(PrefRegistrySimple* registry);

  VivaldiSecretPortalKeyProvider(PrefService* local_state,
                                 bool use_for_encryption);

  ~VivaldiSecretPortalKeyProvider() override;

 protected:
  // Override to use PBKDF2-HMAC-SHA1 derivation instead of HKDF-SHA-256
  // for compatibility with existing Vivaldi encrypted data.
  void ReceivedSecret() override;
};

}  // namespace os_crypt_async

#endif  // COMPONENTS_OS_CRYPT_ASYNC_SECRET_PORTAL_KEY_PROVIDER_VIVALDI_H_
