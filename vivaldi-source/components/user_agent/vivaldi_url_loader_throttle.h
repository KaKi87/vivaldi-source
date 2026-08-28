// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef USER_AGENT_VIVALDI_URL_LOADER_THROTTLE_H_
#define USER_AGENT_VIVALDI_URL_LOADER_THROTTLE_H_

#include "content/public/browser/frame_tree_node_id.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace content {
class FrameTreeNode;
}

namespace vivaldi {

class VivaldiURLLoaderThrottle final : public blink::URLLoaderThrottle {
 public:
  explicit VivaldiURLLoaderThrottle(content::FrameTreeNodeId ftni);
  ~VivaldiURLLoaderThrottle() override;

 private:
  // blink::URLLoaderThrottle:
  void DetachFromCurrentSequence() override;
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
  void WillRedirectRequest(
      net::RedirectInfo* redirect_info,
      const network::mojom::URLResponseHead& response_head,
      bool* defer,
      network::HttpRequestHeadersUpdateParams* headers_update_params) override;

  raw_ptr<content::FrameTreeNode> frame_tree_node_ = nullptr;
};
}  // namespace vivaldi
#endif  // USER_AGENT_VIVALDI_URL_LOADER_THROTTLE_H_
