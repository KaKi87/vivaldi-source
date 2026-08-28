// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/user_agent/vivaldi_ua_config_holder.h"

namespace vivaldi_user_agent {

// static
UAConfigHolder* UAConfigHolder::GetInstance() {
  static base::NoDestructor<UAConfigHolder> instance;
  return instance.get();
}

UAConfigHolder::UAConfigHolder() = default;

void UAConfigHolder::SetConfig(const mojom::BrandConfiguration& config) {
  base::AutoLock auto_lock(lock_);
  current_config_ = config;
}

mojom::BrandConfiguration UAConfigHolder::GetConfig() {
  base::AutoLock auto_lock(lock_);
  return current_config_;
}

}  // namespace vivaldi_user_agent
