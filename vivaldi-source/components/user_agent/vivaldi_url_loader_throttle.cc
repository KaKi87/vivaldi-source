// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/user_agent/vivaldi_url_loader_throttle.h"

#include "components/embedder_support/user_agent_utils.h"
#include "components/user_agent/vivaldi_user_agent.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/browser/renderer_host/navigation_request.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"

namespace vivaldi {

namespace {

void DecideBrandingHeader(network::ResourceRequest* request) {
  if (!request->url.SchemeIsCryptographic())
    return; // No branding in non-secure

  auto meta_data = blink::UserAgentOverride::GetUaMetaDataOverrideGlobal(
      request->url.GetHost());
  if (meta_data.has_value()) {
    std::string brands = meta_data->SerializeBrandFullVersionList();
    if (!brands.empty()) {
      request->headers.SetHeader("Sec-CH-UA", brands);
    }
  }
}

}  // namespace

VivaldiURLLoaderThrottle::VivaldiURLLoaderThrottle(
    content::FrameTreeNodeId frame_tree_node_id) {
  // If we are coming from URLLoaderThrottleProviderImpl there is no
  // frametreenode, and we will always try to spoof.
  if (frame_tree_node_id) {
    frame_tree_node_ =
        content::FrameTreeNode::GloballyFindByID(frame_tree_node_id);
  }
}

VivaldiURLLoaderThrottle::~VivaldiURLLoaderThrottle() = default;

void VivaldiURLLoaderThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  bool check_domain_meta = true;

  if (frame_tree_node_) {
    content::NavigationRequest* navigation_request =
        frame_tree_node_->navigation_request();

    content::WebContents* web_contents =
        content::WebContents::FromFrameTreeNodeId(
            frame_tree_node_->frame_tree_node_id());

    const blink::UserAgentOverride& ua_override =
        web_contents->GetUserAgentOverride();

    if (!ua_override.ua_string_override.empty() ||
        navigation_request->is_overriding_user_agent()) {
      if (check_domain_meta) {
        DecideBrandingHeader(request);
      }
      return;
    }
  }

#if BUILDFLAG(IS_ANDROID)
  // Without a FrameTreeNode we cannot consult the per-WebContents UA
  // override, so honor any User-Agent the renderer has already set.
  // Ref. VAB-12975.
  if (request->headers.HasHeader(net::HttpRequestHeaders::kUserAgent)) {
    return;
  }
#endif

  vivaldi_user_agent::ScopedVivaldiThreadURL vivaldi_ua(request->url);
  // Note this might have been set when the renderer was created and
  // webpreferences is updated, but to capture when this is not the case, like a
  // reload, we need to update user agent here. We check if the Vivaldi postfix
  // has been already added to avoid multiple instances in UpdateAgentString.
  request->headers.SetHeader(net::HttpRequestHeaders::kUserAgent,
                             embedder_support::GetUserAgent());

  if (check_domain_meta) {
    DecideBrandingHeader(request);
  }
}

void VivaldiURLLoaderThrottle::WillRedirectRequest(
  net::RedirectInfo* redirect_info,
  const network::mojom::URLResponseHead& response_head,
  bool* defer,
  network::HttpRequestHeadersUpdateParams* headers_update_params) {
  vivaldi_user_agent::ScopedVivaldiThreadURL vivaldi_ua(redirect_info->new_url);
  headers_update_params->modified_headers.SetHeader(
      net::HttpRequestHeaders::kUserAgent, embedder_support::GetUserAgent());
}

void VivaldiURLLoaderThrottle::DetachFromCurrentSequence() {}

}  // namespace vivaldi
