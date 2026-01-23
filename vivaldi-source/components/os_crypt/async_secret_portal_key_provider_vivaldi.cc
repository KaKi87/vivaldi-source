// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved
//
// Based on Chromium code with the following copyright:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/async_secret_portal_key_provider_vivaldi.h"

#include <array>

#include "base/logging.h"
#include "components/os_crypt/async/common/algorithm.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "crypto/kdf.h"

namespace os_crypt_async {

namespace {

// Vivaldi: Match sync os_crypt's PBKDF2 parameters for compatibility.
constexpr crypto::kdf::Pbkdf2HmacSha1Params kParams{
    .iterations = 1,
};

const auto kSalt = base::byte_span_from_cstring("saltysalt");

}  // namespace

// static
void VivaldiSecretPortalKeyProvider::RegisterLocalPrefs(
    PrefRegistrySimple* registry) {
  // Delegate to base class implementation.
  SecretPortalKeyProvider::RegisterLocalPrefs(registry);
}

VivaldiSecretPortalKeyProvider::VivaldiSecretPortalKeyProvider(
    PrefService* local_state,
    bool use_for_encryption)
    : SecretPortalKeyProvider(local_state, use_for_encryption) {}

VivaldiSecretPortalKeyProvider::~VivaldiSecretPortalKeyProvider() = default;

void VivaldiSecretPortalKeyProvider::ReceivedSecret() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (secret_.empty()) {
    LOG(ERROR) << "Xdg-Desktop-Portal: Retrieved secret is empty.";
    return Finalize(InitStatus::kEmptySecret);
  }

  // Use PBKDF2-HMAC-SHA1 derivation to match sync os_crypt
  // implementation for compatibility with existing encrypted data.
  // Sync os_crypt derives a 16-byte key from the portal secret using PBKDF2.
  std::array<uint8_t, 16> derived_key;
  crypto::kdf::DeriveKeyPbkdf2HmacSha1(kParams, base::span(secret_), kSalt,
                                       derived_key, crypto::SubtlePassKey{});
  secret_.clear();

  VLOG(1) << "Xdg-Desktop-Portal: Successfully derived key for encryption.";

  // Sync os_crypt uses AES-128-CBC, not AES-256-GCM.
  Encryptor::Key key(derived_key, mojom::Algorithm::kAES128CBC);
  Finalize(InitStatus::kSuccess, kKeyTag, std::move(key));
}

}  // namespace os_crypt_async
