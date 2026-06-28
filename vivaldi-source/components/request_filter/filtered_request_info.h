// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_REQUEST_FILTER_FILTERED_REQUEST_INFO_H_
#define COMPONENTS_REQUEST_FILTER_FILTERED_REQUEST_INFO_H_

#include <stdint.h>
#include <optional>

#include "content/public/browser/content_browser_client.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace vivaldi {
// A URL request representation used by the request filter. This structure
// carries information about an in-progress request.
struct FilteredRequestInfo {
  explicit FilteredRequestInfo(
      uint64_t request_id,
      content::GlobalRenderFrameHostId global_id,
      const network::ResourceRequest& request,
      content::ContentBrowserClient::URLLoaderFactoryType loader_factory_type,
      bool is_async,
      bool is_webtransport,
      std::optional<int64_t> navigation_id);

  FilteredRequestInfo(FilteredRequestInfo&&);
  FilteredRequestInfo& operator=(FilteredRequestInfo&&) = delete;

  ~FilteredRequestInfo();

  void AddSslInfo(const std::optional<net::SSLInfo>& ssl_info);

  // Fill in response data for this request.
  void AddResponse(const network::mojom::URLResponseHead& added_response);

  // A unique identifier for this request.
  const uint64_t id;

  const network::ResourceRequest request;
  network::mojom::URLResponseHeadPtr response;

  // The ID and frame routing ID of the render process which initiated the
  // request, or invalid if not applicable (i.e. if initiated by the browser).
  const content::GlobalRenderFrameHostId global_id;

  const content::ContentBrowserClient::URLLoaderFactoryType loader_factory_type;

  // Indicates if this request is asynchronous.
  const bool is_async;

  // Indicate whether this is a web transport request.
  const bool is_webtransport;

  // Valid if this request corresponds to a navigation.
  const std::optional<int64_t> navigation_id;

  // For SecurityInfo object.
  std::optional<net::SSLInfo> ssl_info;
};

}  // namespace vivaldi

#endif  // COMPONENTS_REQUEST_FILTER_FILTERED_REQUEST_INFO_H_
