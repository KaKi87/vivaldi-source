// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "browser/kagi_token/kagi_token_cookie_listener.h"

#include "app/vivaldi_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/storage_partition.h"
#include "prefs/vivaldi_pref_names.h"

namespace vivaldi {

namespace {
constexpr char kKagiCookieName[] = "kagi_session";
}  // namespace

void KagiTokenCookieListener::Create(Profile& profile) {
  // Will self-destruct when the profile goes away.
  new KagiTokenCookieListener(profile);
}

KagiTokenCookieListener::KagiTokenCookieListener(Profile& profile)
    : profile_(profile) {
  const GURL kagi_url(vivaldi::kVivaldiKagiURL);

  profile.AddObserver(this);

  network::mojom::CookieManager* cookie_manager =
      profile_->GetDefaultStoragePartition()
          ->GetCookieManagerForBrowserProcess();

  cookie_manager->GetCookieList(
      kagi_url, net::CookieOptions::MakeAllInclusive(),
      net::CookiePartitionKeyCollection(),
      base::BindOnce(&KagiTokenCookieListener::GetCookieListCallback,
                     weak_factory_.GetWeakPtr()));
  cookie_manager->AddCookieChangeListener(kagi_url, kKagiCookieName,
                                          receiver_.BindNewPipeAndPassRemote());
}

KagiTokenCookieListener::~KagiTokenCookieListener() = default;

void KagiTokenCookieListener::GetCookieListCallback(
    const net::CookieAccessResultList& cookie_list,
    const net::CookieAccessResultList& excluded_cookies) {
  for (const net::CookieWithAccessResult& cookie_with_access_result :
       cookie_list) {
    if (cookie_with_access_result.cookie.PartitionKey().has_value()) {
      continue;
    }

    if (cookie_with_access_result.cookie.Name() == kKagiCookieName) {
      profile_->GetPrefs()->SetString(
          vivaldiprefs::kVivaldiSearchEnginesKagiToken,
          cookie_with_access_result.cookie.Value());
    }
  }
}

void KagiTokenCookieListener::OnCookieChange(
    const net::CookieChangeInfo& change) {
  CHECK_EQ(change.cookie.Name(), kKagiCookieName);
  if (net::CookieChangeCauseIsDeletion(change.cause) ||
      change.cookie.Value().empty()) {
    return;
  }
  profile_->GetPrefs()->SetString(vivaldiprefs::kVivaldiSearchEnginesKagiToken,
                                  change.cookie.Value());
}

void KagiTokenCookieListener::OnProfileWillBeDestroyed(Profile* profile) {
  CHECK(profile == &profile_.get());
  profile_->RemoveObserver(this);
  delete this;
}

}  // namespace vivaldi
