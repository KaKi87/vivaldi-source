// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_REQUEST_FILTER_REQUEST_FILTER_PROXYING_WEBTRANSPORT_SHUTDOWN_NOTIFIER_FACTORY_H_
#define COMPONENTS_REQUEST_FILTER_REQUEST_FILTER_PROXYING_WEBTRANSPORT_SHUTDOWN_NOTIFIER_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_shutdown_notifier_factory.h"

namespace vivaldi {

// Notifies WebTransport proxy instances when their associated BrowserContext
// is actively shutting down.
//
// This separate notifier is required because RequestFilterManager spans across
// regular and Incognito profiles. If proxies relied on RequestFilterManager's
// KeyedService shutdown, they would miss the Incognito profile's destruction
// and leak.
//
// Subscribing here allows the proxies to cleanly destroy themselves and
// their Mojo pipes before the context pointer becomes invalid, preventing
// Use-After-Free crashes during asynchronous network teardowns.
class RequestFilterProxyingWebTransportShutdownNotifierFactory
    : public BrowserContextKeyedServiceShutdownNotifierFactory {
 public:
  static RequestFilterProxyingWebTransportShutdownNotifierFactory*
  GetInstance();

 private:
  friend base::NoDestructor<
      RequestFilterProxyingWebTransportShutdownNotifierFactory>;

  RequestFilterProxyingWebTransportShutdownNotifierFactory();
  ~RequestFilterProxyingWebTransportShutdownNotifierFactory() override =
      default;
};

}  // namespace vivaldi

#endif  // COMPONENTS_REQUEST_FILTER_REQUEST_FILTER_PROXYING_WEBTRANSPORT_SHUTDOWN_NOTIFIER_FACTORY_H_
