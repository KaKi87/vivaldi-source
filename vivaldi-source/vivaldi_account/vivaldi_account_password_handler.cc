// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "vivaldi_account/vivaldi_account_password_handler.h"

#include "base/strings/utf_string_conversions.h"
#include "components/password_manager/core/browser/password_store/stored_credential.h"

namespace {
constexpr char kSyncSignonRealm[] = "vivaldi-sync-login";
constexpr char kSyncOrigin[] = "vivaldi://settings/sync";
}  // namespace

namespace vivaldi {

VivaldiAccountPasswordHandler::VivaldiAccountPasswordHandler(
    scoped_refptr<password_manager::PasswordStoreInterface> password_store,
    Delegate* delegate)
    : delegate_(delegate), password_store_(std::move(password_store)) {
  UpdatePassword();
  if (password_store_)
    password_store_->AddObserver(this);
}

VivaldiAccountPasswordHandler::~VivaldiAccountPasswordHandler() {
  if (password_store_)
    password_store_->RemoveObserver(this);
}

void VivaldiAccountPasswordHandler::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void VivaldiAccountPasswordHandler::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void VivaldiAccountPasswordHandler::SetPassword(const std::string& password) {
  if (!password_store_)
    return;

  DCHECK(!password.empty());

  password_manager::StoredCredential stored_credential = {};
  stored_credential.scheme = password_manager::PasswordForm::Scheme::kOther;
  stored_credential.signon_realm = kSyncSignonRealm;
  stored_credential.url = GURL(kSyncOrigin);
  stored_credential.username_value =
      base::UTF8ToUTF16(delegate_->GetUsername());
  stored_credential.password_value = base::UTF8ToUTF16(password);
  stored_credential.date_created = base::Time::Now();

  password_store_->AddLogin(std::move(stored_credential));
}

void VivaldiAccountPasswordHandler::ForgetPassword() {
  if (!password_store_)
    return;

  password_manager::StoredCredential stored_credential = {};
  stored_credential.scheme = password_manager::PasswordForm::Scheme::kOther;
  stored_credential.signon_realm = kSyncSignonRealm;
  stored_credential.url = GURL(kSyncOrigin);
  stored_credential.username_value =
      base::UTF8ToUTF16(delegate_->GetUsername());

  password_store_->RemoveLogin(FROM_HERE, stored_credential);
}

void VivaldiAccountPasswordHandler::OnGetPasswordStoreResultsOrErrorFrom(
    password_manager::PasswordStoreInterface* store,
    password_manager::LoginsResultOrError results_or_error) {
  const std::vector<password_manager::StoredCredential>* results =
      std::get_if<std::vector<password_manager::StoredCredential>>(
          &results_or_error);
  if (!results) {
    return;
  }
  for (const auto& result : *results) {
    if (base::UTF16ToUTF8(result.username_value) == delegate_->GetUsername()) {
      PasswordReceived(base::UTF16ToUTF8(result.password_value));
    }
  }
}

void VivaldiAccountPasswordHandler::OnLoginsChanged(
    password_manager::PasswordStoreInterface* store,
    const password_manager::PasswordStoreChangeList& changes) {
  for (const password_manager::PasswordStoreChange& change : changes) {
    if (change.credential().signon_realm == kSyncSignonRealm &&
        change.credential().url == GURL(kSyncOrigin) &&
        base::UTF16ToUTF8(change.credential().username_value) ==
            delegate_->GetUsername()) {
      if (change.type() == password_manager::PasswordStoreChange::REMOVE)
        PasswordReceived(std::string());
      else
        UpdatePassword();
    }
  }
}

void VivaldiAccountPasswordHandler::OnLoginsRetained(
    password_manager::PasswordStoreInterface* store,
    const std::vector<password_manager::StoredCredential>&
        retained_credentials) {}

void VivaldiAccountPasswordHandler::UpdatePassword() {
  if (!password_store_)
    return;

  password_manager::PasswordFormDigest form_digest(
      password_manager::PasswordForm::Scheme::kOther, kSyncSignonRealm,
      GURL(kSyncOrigin));

  password_store_->GetLogins(form_digest, weak_ptr_factory_.GetWeakPtr());
}

void VivaldiAccountPasswordHandler::PasswordReceived(
    const std::string& password) {
  bool should_notfify = password.empty() || password_.empty();

  password_ = password;
  if (should_notfify) {
    for (auto& observer : observers_) {
      observer.OnAccountPasswordStateChanged();
    }
  }
}

}  // namespace vivaldi
