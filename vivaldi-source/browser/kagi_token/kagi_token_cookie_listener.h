// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef BROWSER_KAGI_TOKEN_KAGI_TOKEN_COOKIE_LISTENER_H_
#define BROWSER_KAGI_TOKEN_KAGI_TOKEN_COOKIE_LISTENER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

namespace vivaldi {
class KagiTokenCookieListener : public network::mojom::CookieChangeListener,
                                public ProfileObserver {
 public:
  static void Create(Profile& profile);

 private:
  explicit KagiTokenCookieListener(Profile& profile);
  ~KagiTokenCookieListener();

  KagiTokenCookieListener(const KagiTokenCookieListener&) = delete;
  KagiTokenCookieListener& operator=(const KagiTokenCookieListener&) = delete;

  // Implementing network::mojom::CookieChangeListener
  void OnCookieChange(const net::CookieChangeInfo& change) override;

  // Implementing ProfileObserver;
  void OnProfileWillBeDestroyed(Profile* profile) override;

  void GetCookieListCallback(
      const net::CookieAccessResultList& cookie_list,
      const net::CookieAccessResultList& excluded_cookies);

  const raw_ref<Profile> profile_;

  mojo::Receiver<network::mojom::CookieChangeListener> receiver_{this};
  base::WeakPtrFactory<KagiTokenCookieListener> weak_factory_{this};
};
}  // namespace vivaldi

#endif  // BROWSER_KAGI_TOKEN_KAGI_TOKEN_COOKIE_LISTENER_H_
