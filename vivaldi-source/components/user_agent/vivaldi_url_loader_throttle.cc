// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/user_agent/vivaldi_url_loader_throttle.h"

#include "components/embedder_support/user_agent_utils.h"
#include "components/user_agent/vivaldi_user_agent.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/browser/renderer_host/navigation_request.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"

namespace vivaldi {

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
      return;
    }
  }

  vivaldi_user_agent::ScopedVivaldiThreadURL vivaldi_ua(request->url);
  request->headers.SetHeader("User-Agent", embedder_support::GetUserAgent());
}

void VivaldiURLLoaderThrottle::DetachFromCurrentSequence() {}

}  // namespace vivaldi
