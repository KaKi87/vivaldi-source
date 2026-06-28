// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#ifndef IMPORTER_IMPORTED_RAW_PASSWORD_FORM_H_
#define IMPORTER_IMPORTED_RAW_PASSWORD_FORM_H_

#include <string>

#include "url/gurl.h"

// Password form carrying the raw encrypted password blob for decryption in the
// browser process.  On Linux the importer (utility) process cannot use
// OSCryptAsync because it has no running message loop for D-Bus callbacks, so
// the encrypted values are shipped across IPC and decrypted on the browser
// side where a proper RunLoop exists.
struct ImportedRawPasswordForm {
  ImportedRawPasswordForm();
  ImportedRawPasswordForm(const ImportedRawPasswordForm&);
  ~ImportedRawPasswordForm();

  // When changing this, also update
  // profile_vivaldi_import_process_param_traits_macros.h

  std::string signon_realm;
  GURL url;
  GURL action;
  std::u16string username_element;
  std::u16string username_value;
  std::u16string password_element;
  // Raw encrypted cipher text read from the Login Data sqlite table.
  std::string password_value_cipher;
  bool blocked_by_user = false;

  // Source browser product name used to look up the keyring entry
  // (e.g. "Google Chrome", "Chromium", "Vivaldi").
  std::string source_product_name;
};

#endif  // IMPORTER_IMPORTED_RAW_PASSWORD_FORM_H_
