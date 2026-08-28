// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "base/no_destructor.h"
#include "components/user_agent/renderer/vivaldi_user_agent_config_receiver.h"
#include "components/user_agent/vivaldi_ua_config_holder.h"

namespace vivaldi_user_agent {

// static
void UAConfigReceiver::BindReceiver(
    mojo::PendingReceiver<mojom::VivaldiUserAgentConfig> receiver) {
  static base::NoDestructor<UAConfigReceiver> instance;
  instance->receiver_.Bind(std::move(receiver));
}

UAConfigReceiver::UAConfigReceiver() = default;
UAConfigReceiver::~UAConfigReceiver() = default;

void UAConfigReceiver::SetPreferences(mojom::BrandConfigurationPtr config) {
  if (config) {
    // Pass the received data to the shared, thread-safe holder
    UAConfigHolder::GetInstance()->SetConfig(*config);
  }
}

}  // namespace vivaldi_user_agent
