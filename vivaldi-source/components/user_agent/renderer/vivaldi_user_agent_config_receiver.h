// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_USER_AGENT_RENDERER_UA_CONFIG_RECEIVER_H_
#define COMPONENTS_USER_AGENT_RENDERER_UA_CONFIG_RECEIVER_H_

#include "components/user_agent/mojom/vivaldi_user_agent_config.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace vivaldi_user_agent {

// Renderer-only class to handle IPC from the browser.
class UAConfigReceiver : public mojom::VivaldiUserAgentConfig {
 public:
  static void BindReceiver(
      mojo::PendingReceiver<mojom::VivaldiUserAgentConfig> receiver);

  UAConfigReceiver();
  ~UAConfigReceiver() override;

  // mojom::VivaldiUserAgentConfig override:
  void SetPreferences(mojom::BrandConfigurationPtr config) override;

 private:
  mojo::Receiver<mojom::VivaldiUserAgentConfig> receiver_{this};
};

}  // namespace vivaldi_user_agent

#endif  // COMPONENTS_USER_AGENT_RENDERER_UA_CONFIG_RECEIVER_H_
