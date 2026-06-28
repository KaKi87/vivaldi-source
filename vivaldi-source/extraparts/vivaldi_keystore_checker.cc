// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#include "extraparts/vivaldi_keystore_checker.h"

#include "base/base64.h"
#include "base/logging.h"
#include "base/run_loop.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
#include "chrome/browser/ui/views/message_box_dialog.h"
#include "ui/vivaldi_message_box_dialog.h"
#include "ui/vivaldi_ui_utils.h"
#endif  // !IS_ANDROID && !IS_IOS

#include "components/os_crypt/async/browser/os_crypt_async.h"

#if BUILDFLAG(IS_WIN)
#include "components/os_crypt/async/browser/os_crypt_win.h"
#endif
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "vivaldi/app/grit/vivaldi_native_strings.h"

namespace vivaldi {
namespace {

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_WIN)
constexpr char kCanaryValue[] = "VivaldiKeystoreEncryptionCanary";

bool AskShouldAllowInsecureAccess() {
  if (!ui_tools::IsUIAvailable()) {
    LOG(WARNING) << "KeystoreChecker: AskShouldAllowInsecureAccess: UI Is not "
                    "available yet. Returning NO to insecure access";
    return false;
  }

  VivaldiMessageBoxDialog::Config config{
      l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_FAILED_TITLE),
      l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_UNCRYPTED),
      chrome::MESSAGE_BOX_TYPE_QUESTION,
      l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_CONTINUE_DATALOSS),
      l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_CANCEL),
      u""  // l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_CONTINUE_CHECKBOX)
  };

  // Some extra configs:

  // Use cancel button as default - pressing enter will cause the dialog to
  // cancel.
  config.cancel_default = true;

  // A reasonable sizing for the messagebox.
  config.size = gfx::Size(700, 250);

  auto result = VivaldiMessageBoxDialog::Show(gfx::NativeWindow(), config);

  return result == chrome::MESSAGE_BOX_RESULT_YES;
}

// Describes the result of profile encryption key health analysis.
enum class CanaryStatus {
  kInvalid,
  kValid,
  kNotPresent  // Canary value was not present in the profile.
};

/// Verifies stored canary value, after decrypting, with our canary value.
CanaryStatus VerifyCanary(PrefService* preferences,
                          const os_crypt_async::Encryptor& encryptor) {
  // also decrypt last stored canary value.
  std::string b64 =
      preferences->GetString(vivaldiprefs::kStartupKeystoreCanary);

  // We have no canary, we pretend everything is fine...
  if (b64.empty()) {
    return CanaryStatus::kNotPresent;
  }

  std::string encrypted_canary;
  base::Base64Decode(b64, &encrypted_canary);

  os_crypt_async::Encryptor::DecryptFlags flags;
  std::string decrypted_canary;
  if (!encryptor.DecryptString(encrypted_canary, &decrypted_canary, &flags)) {
    LOG(WARNING) << "KeystoreChecker: Decryption of the canary failed. "
                    "Keystore may have changed!";
    return CanaryStatus::kInvalid;
  }

  // Re-encrypt if the key has been rotated.
  if (flags.should_reencrypt) {
    std::string new_encrypted;
    if (encryptor.EncryptString(kCanaryValue, &new_encrypted)) {
      preferences->SetString(vivaldiprefs::kStartupKeystoreCanary,
                             base::Base64Encode(new_encrypted));
    }
  }

  return (decrypted_canary == kCanaryValue) ? CanaryStatus::kValid
                                            : CanaryStatus::kInvalid;
}

void StoreCanary(PrefService* prefs,
                 const os_crypt_async::Encryptor& encryptor) {
  std::string encrypted_canary;
  if (!encryptor.EncryptString(kCanaryValue, &encrypted_canary)) {
    return;
  }

  LOG(INFO) << "KeystoreChecker: Storing new canary value";
  prefs->SetString(vivaldiprefs::kStartupKeystoreCanary,
                   base::Base64Encode(encrypted_canary));
}

// Runs the actual keystore check and invokes |run_loop.QuitClosure()| so the
// blocking RunLoop in HasLockedKeystore unblocks.
void RunKeystoreCheck(Profile* profile,
                      bool* result,
                      base::OnceClosure quit_closure,
                      const os_crypt_async::Encryptor& encryptor) {
  PrefService* preferences = profile->GetPrefs();

  // We intentionally ignore system profile problems.
  // This is due to the fact that System Profile is fallback
  // when loading in the profile selection screen and we need
  // to be able to show it.
  if (profile->IsSystemProfile()) {
    *result = false;
    std::move(quit_closure).Run();
    return;
  }

  bool had_pref =
      preferences->HasPrefPath(vivaldiprefs::kStartupWasEncryptionUsed);
  bool was_encrypted =
      preferences->GetBoolean(vivaldiprefs::kStartupWasEncryptionUsed);

  bool is_encrypted = encryptor.IsEncryptionAvailable();

  // New profiles start without encryption info, we just store the status.
  if (profile->IsNewProfile() || !had_pref) {
    // Store whether the encryption is now available and a canary.
    preferences->SetBoolean(vivaldiprefs::kStartupWasEncryptionUsed,
                            is_encrypted);
    StoreCanary(preferences, encryptor);
    *result = false;
    std::move(quit_closure).Run();
    return;
  }

  CanaryStatus canary_status = VerifyCanary(preferences, encryptor);

  if (!was_encrypted && is_encrypted) {
    // This profile was previously only used unencrypted. Using secure storage
    // could mean the situation below could happen if the user ever switches
    // back to unencrypted.
    LOG(ERROR)
        << "KeystoreChecker: Profile " << profile->GetBaseName()
        << ": Unencrypted keystore was previously used but encryption is "
           "used now. Upgrading status to secured keystore.";

    // Re-store canary, the encryption key was added and we need have the canary
    // to test for key changes.
    StoreCanary(preferences, encryptor);

    // We test if we dropped out of encryption or the key changed.
  } else if ((was_encrypted && !is_encrypted) ||
             (canary_status == CanaryStatus::kInvalid)) {
    // Was previously encrypted, and is not now. We need to let user know that
    // this will mean logins, cookies and stuff will get lost.
    LOG(ERROR)
        << "KeystoreChecker: Profile " << profile->GetBaseName()
        << ": Encrypted keystore changed or is now unavailable. This may "
           "result in lost cookies and other problems.";

    if (!AskShouldAllowInsecureAccess()) {
      LOG(ERROR) << "KeystoreChecker: Keystore unlock failed and user "
                    "requested profile switch!";
      *result = true;
      std::move(quit_closure).Run();
      return;
    }
  }

  // Store whether the encryption is now available.
  preferences->SetBoolean(vivaldiprefs::kStartupWasEncryptionUsed,
                          is_encrypted);

  // Store a new canary value unless it was valid.
  if (canary_status != CanaryStatus::kValid) {
    StoreCanary(preferences, encryptor);
  }
  *result = false;
  std::move(quit_closure).Run();
}

#endif  // !IS_ANDROID && !IS_IOS && !IS_WIN

}  // namespace

bool HasLockedKeystore(Profile* profile) {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS) || BUILDFLAG(IS_WIN)
  // No verification on mobile platforms and windows (that is done in
  // InitOSCrypt.
  return false;
#else
  bool result = false;

  base::RunLoop run_loop;

  // Fetch an Encryptor instance asynchronously. This may fire synchronously if
  // the Encryptor is already cached (typical case after startup).
  g_browser_process->os_crypt_async()->GetInstance(
      base::BindOnce(
          [](Profile* profile, bool* result, base::OnceClosure quit_closure,
             scoped_refptr<os_crypt_async::Encryptor> encryptor) {
            RunKeystoreCheck(profile, result, std::move(quit_closure),
                             *encryptor);
          },
          profile, &result, run_loop.QuitClosure()),
      os_crypt_async::Encryptor::Option::kEncryptSyncCompat);

  // Block until the callback fires. In practice this is nearly always
  // synchronous because the Encryptor is cached from PreMainMessageLoopRun.
  run_loop.Run();

  return result;
#endif
}

bool InitOSCrypt(PrefService* local_state, bool* should_exit) {
#if BUILDFLAG(IS_WIN)
  *should_exit = false;

  os_crypt_async::InitResult crypt_result =
      os_crypt_async::InitWithExistingKey(local_state);
  if (crypt_result == os_crypt_async::InitResult::kDecryptionFailed) {
    // Ask user if they want the key to be overwritten...
    // This uses native message box internally as vivaldi is not yet
    // prepared to display normal message box.
    VivaldiMessageBoxDialog::Config config{
        l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_FAILED_TITLE),
        l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_UNCRYPTED),
        chrome::MESSAGE_BOX_TYPE_QUESTION,
        l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_CONTINUE_DATALOSS),
        l10n_util::GetStringUTF16(IDS_VIVALDI_KEYSTORE_QUIT),
        u""};
    // This makes the dialog safer and changes the type to warning.
    config.cancel_default = true;
    auto result = VivaldiMessageBoxDialog::Show(gfx::NativeWindow(), config);

    // User requested browser exit. We set the flag and return init failed.
    if (!result) {
      *should_exit = true;
      return false;
    }

    // User does not want to terminate, we will rewrite the key
    // by calling os_crypt_async::Init...
  }

  // Handle normal init in case it is still needed...
  if (crypt_result != os_crypt_async::InitResult::kSuccess) {
    // In case previous call was not successful we still have to init the
    // key - this time potentially rewriting it.
    return os_crypt_async::Init(local_state);
  }

  return true;
#else
  return true;
#endif
}

}  // namespace vivaldi
