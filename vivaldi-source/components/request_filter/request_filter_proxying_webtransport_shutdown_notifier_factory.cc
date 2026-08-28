// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/request_filter/request_filter_proxying_webtransport_shutdown_notifier_factory.h"

namespace vivaldi {

// static
RequestFilterProxyingWebTransportShutdownNotifierFactory*
RequestFilterProxyingWebTransportShutdownNotifierFactory::GetInstance() {
  static base::NoDestructor<
      RequestFilterProxyingWebTransportShutdownNotifierFactory>
      instance;
  return instance.get();
}

RequestFilterProxyingWebTransportShutdownNotifierFactory::
    RequestFilterProxyingWebTransportShutdownNotifierFactory()
    : BrowserContextKeyedServiceShutdownNotifierFactory(
          "RequestFitlerProxyingWebTransport") {}

}  // namespace vivaldi
