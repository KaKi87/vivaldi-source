// Copyright (c) 2015 Vivaldi Technologies AS. All rights reserved

#include "extensions/api/savedpasswords/savedpasswords_api.h"

#include <string>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/passwords_private/passwords_private_delegate.h"
#include "chrome/browser/extensions/api/passwords_private/passwords_private_delegate_factory.h"
#include "chrome/browser/password_manager/factories/profile_password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_store/password_form_converters.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/web_ui.h"
#include "extensions/api/savedpasswords/password_list_sorter.h"
#include "extensions/api/vivaldi_utilities/vivaldi_utilities_api.h"
#include "extensions/schema/savedpasswords.h"
#include "extensions/vivaldi_browser_component_wrapper.h"
#include "ui/vivaldi_browser_window.h"

#if BUILDFLAG(IS_MAC)
#include "extraparts/vivaldi_keychain_util.h"
#endif

namespace extensions {

using vivaldi::savedpasswords::SavedPasswordItem;
namespace {

void FilterAndSortPasswords(
    std::vector<std::unique_ptr<password_manager::StoredCredential>>*
        password_list) {
  password_list->erase(
      std::remove_if(password_list->begin(), password_list->end(),
                     [](const auto& form) { return form->blocked_by_user; }),
      password_list->end());

  extensions::SortEntriesAndHideDuplicates(password_list);
}

}  // namespace

SavedpasswordsGetListFunction::SavedpasswordsGetListFunction() {}

SavedpasswordsGetListFunction::~SavedpasswordsGetListFunction() {}

ExtensionFunction::ResponseAction SavedpasswordsGetListFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  scoped_refptr<password_manager::PasswordStoreInterface> password_store =
      ProfilePasswordStoreFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS);

  AddRef();  // Balanced in OnGetPasswordStoreResults
  password_store->GetAllLoginsWithAffiliationAndBrandingInformation(
      weak_ptr_factory_.GetWeakPtr());
  return RespondLater();
}

void SavedpasswordsGetListFunction::OnGetPasswordStoreResultsOrErrorFrom(
    password_manager::PasswordStoreInterface* store,
    password_manager::LoginsResultOrError results_or_error) {
  if (auto* error = std::get_if<password_manager::PasswordStoreBackendError>(
          &results_or_error)) {
    LOG(WARNING) << "Password store backend error: "
                 << static_cast<int>(error->type);
    Respond(Error("Password store backend error"));
    Release();
    return;
  }

  auto credentials_vector =
      std::get<std::vector<password_manager::StoredCredential>>(
          std::move(results_or_error));

  std::vector<std::unique_ptr<password_manager::StoredCredential>> credentials;

  for (auto& credential : credentials_vector) {
    credentials.push_back(std::make_unique<password_manager::StoredCredential>(
        std::move(credential)));
  }

  FilterAndSortPasswords(&credentials);

  std::vector<SavedPasswordItem> svd_pwd_entries;
  int index = 0;
  for (const auto& credential : credentials) {
    SavedPasswordItem notes_tree_node;
    notes_tree_node.username = base::UTF16ToUTF8(credential->username_value);
    notes_tree_node.password = base::UTF16ToUTF8(credential->password_value);
    notes_tree_node.origin =
        base::UTF16ToUTF8(url_formatter::FormatUrl(credential->url));
    notes_tree_node.index = base::NumberToString(index++);
    svd_pwd_entries.push_back(std::move(notes_tree_node));
  }
  namespace Results = vivaldi::savedpasswords::GetList::Results;
  Respond(ArgumentList(Results::Create(svd_pwd_entries)));

  Release();  // Balanced in Run().
}

SavedpasswordsRemoveFunction::SavedpasswordsRemoveFunction() {}

SavedpasswordsRemoveFunction::~SavedpasswordsRemoveFunction() {}

ExtensionFunction::ResponseAction SavedpasswordsRemoveFunction::Run() {
  using vivaldi::savedpasswords::Remove::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  if (!base::StringToSizeT(params->id, &id_to_remove_)) {
    return RespondNow(Error("id is not a valid index - " + params->id));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  password_store_ = ProfilePasswordStoreFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);

  AddRef();  // Balanced in OnGetPasswordStoreResults
  password_store_->GetAllLoginsWithAffiliationAndBrandingInformation(
      weak_ptr_factory_.GetWeakPtr());
  return RespondLater();
}

void SavedpasswordsRemoveFunction::OnGetPasswordStoreResultsOrErrorFrom(
    password_manager::PasswordStoreInterface* store,
    password_manager::LoginsResultOrError results_or_error) {
  if (auto* error = std::get_if<password_manager::PasswordStoreBackendError>(
          &results_or_error)) {
    LOG(WARNING) << "Password store backend error: "
                 << static_cast<int>(error->type);
    Respond(Error("Password store backend error"));
    Release();
    return;
  }

  namespace Results = vivaldi::savedpasswords::Remove::Results;

  auto credentials_vector =
      std::get<std::vector<password_manager::StoredCredential>>(
          std::move(results_or_error));
  std::vector<std::unique_ptr<password_manager::StoredCredential>> credentials;

  for (auto& credential : credentials_vector) {
    credentials.push_back(std::make_unique<password_manager::StoredCredential>(
        std::move(credential)));
  }

  FilterAndSortPasswords(&credentials);
  if (id_to_remove_ >= credentials.size()) {
    Respond(Error("id is outside the allowed range"));
  } else {
    password_manager::StoredCredential* credential_ptr =
        (credentials)[id_to_remove_].get();

    password_store_->RemoveLogin(FROM_HERE, *credential_ptr);
    Respond(ArgumentList(Results::Create()));
  }

  Release();  // Balanced in Run().
}

ExtensionFunction::ResponseAction SavedpasswordsAddFunction::Run() {
  using vivaldi::savedpasswords::Add::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  if (!params->password_form.password.has_value()) {
    return RespondNow(Error("No password"));
  }

#if BUILDFLAG(IS_MAC)
  if (!::vivaldi::HasKeychainAccess()) {
    return RespondNow(Error("No keychain access, unable to store password."));
  }
#endif

  password_manager::PasswordForm password_form = {};
  password_form.scheme = password_manager::PasswordForm::Scheme::kOther;
  password_form.signon_realm = params->password_form.signon_realm;
  password_form.url = GURL(params->password_form.origin);
  password_form.username_value =
      base::UTF8ToUTF16(params->password_form.username);
  password_form.password_value =
      base::UTF8ToUTF16(params->password_form.password.value());
  password_form.date_created = base::Time::Now();

  Profile* profile = Profile::FromBrowserContext(browser_context());
  scoped_refptr<password_manager::PasswordStoreInterface> password_store =
      ProfilePasswordStoreFactory::GetForProfile(
          profile, params->is_explicit ? ServiceAccessType::EXPLICIT_ACCESS
                                       : ServiceAccessType::IMPLICIT_ACCESS);
  password_store->AddLogin(password_manager::FromPasswordForm(password_form));

  return RespondNow(NoArguments());
}

SavedpasswordsGetFunction::SavedpasswordsGetFunction() {}

SavedpasswordsGetFunction::~SavedpasswordsGetFunction() {}

ExtensionFunction::ResponseAction SavedpasswordsGetFunction::Run() {
  using vivaldi::savedpasswords::Get::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  // EXPLICIT_ACCESS used as this is a read operation that must work in
  // incognito too.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  scoped_refptr<password_manager::PasswordStoreInterface> password_store =
      ProfilePasswordStoreFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS);

  username_ = params->password_form.username;

  password_manager::PasswordFormDigest form_digest(
      password_manager::PasswordForm::Scheme::kOther,
      params->password_form.signon_realm, GURL(params->password_form.origin));

  // Adding a ref on the behalf of the password store, which expects us to
  // remain alive
  AddRef();
  password_store->GetLogins(form_digest, weak_ptr_factory_.GetWeakPtr());

  return RespondLater();
}

void SavedpasswordsGetFunction::OnGetPasswordStoreResultsOrErrorFrom(
    password_manager::PasswordStoreInterface* store,
    password_manager::LoginsResultOrError results_or_error) {
  if (auto* error = std::get_if<password_manager::PasswordStoreBackendError>(
          &results_or_error)) {
    LOG(WARNING) << "Password store backend error: "
                 << static_cast<int>(error->type);
    Respond(Error("Password store backend error"));
    Release();
    return;
  }

  namespace Results = vivaldi::savedpasswords::Get::Results;
  auto results = Results::Create(false, "");

  auto credentials_vector =
      std::get<std::vector<password_manager::StoredCredential>>(
          std::move(results_or_error));

  for (const auto& result : credentials_vector) {
    if (base::UTF16ToUTF8(result.username_value) == username_) {
      results = Results::Create(true, base::UTF16ToUTF8(result.password_value));
      break;
    }
  }

  Respond(ArgumentList(std::move(results)));

  // Balance the AddRef in Run
  Release();
}

ExtensionFunction::ResponseAction SavedpasswordsCreateDelegateFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  // We only need to create the delegate once.
  // There is no process for deleting the delegate,
  //  so once it's created it lives until browser shutdown.
  if (!extensions::PasswordsPrivateDelegateFactory::GetForBrowserContext(
          profile, false)) {
    scoped_refptr<extensions::PasswordsPrivateDelegate>*
        passwords_private_delegate;
    passwords_private_delegate =
        new scoped_refptr<extensions::PasswordsPrivateDelegate>;
    *passwords_private_delegate =
        extensions::PasswordsPrivateDelegateFactory::GetForBrowserContext(
            profile, true);
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction SavedpasswordsDeleteFunction::Run() {
  using vivaldi::savedpasswords::Delete::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  scoped_refptr<password_manager::PasswordStoreInterface> password_store(
      ProfilePasswordStoreFactory::GetForProfile(
          profile, params->is_explicit ? ServiceAccessType::EXPLICIT_ACCESS
                                       : ServiceAccessType::IMPLICIT_ACCESS));
  if (!password_store.get()) {
    return RespondNow(Error("No such passwordstore for profile"));
  }

  password_manager::PasswordForm password_form = {};
  password_form.scheme = password_manager::PasswordForm::Scheme::kOther;
  password_form.signon_realm = params->password_form.signon_realm;
  password_form.url = GURL(params->password_form.origin);
  password_form.username_value =
      base::UTF8ToUTF16(params->password_form.username);

  password_store->RemoveLogin(
      FROM_HERE, password_manager::FromPasswordForm(password_form));

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction SavedpasswordsAuthenticateFunction::Run() {
  using vivaldi::savedpasswords::Authenticate::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  VivaldiBrowserWindow* window =
      VivaldiBrowserComponentWrapper::GetInstance()->VivaldiBrowserWindowFromId(
          params->window_id);
  if (!window) {
    return RespondNow(Error("No such window"));
  }

  PasswordsPrivateDelegateFactory::GetForBrowserContext(
      window->web_contents()->GetBrowserContext(),
      /*create=*/true)
      ->AuthenticateUser(
          base::BindOnce(
              &SavedpasswordsAuthenticateFunction::AuthenticationComplete,
              this),
          window->web_contents());

  return RespondLater();
}

void SavedpasswordsAuthenticateFunction::AuthenticationComplete(
    bool authenticated) {
  namespace Results = vivaldi::savedpasswords::Authenticate::Results;
  Respond(ArgumentList(Results::Create(authenticated)));
}

}  // namespace extensions
