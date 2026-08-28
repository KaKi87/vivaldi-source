// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_VIVALDI_USER_AGENT_UA_CONFIG_HOLDER_H_
#define COMPONENTS_VIVALDI_USER_AGENT_UA_CONFIG_HOLDER_H_

#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "components/user_agent/mojom/vivaldi_user_agent_config.mojom.h"

namespace vivaldi_user_agent {

// Common class for use in //components/embedder_support.
class UAConfigHolder {
 public:
  static UAConfigHolder* GetInstance();

  void SetConfig(const mojom::BrandConfiguration& config);

  mojom::BrandConfiguration GetConfig();

 private:
  friend class base::NoDestructor<UAConfigHolder>;

  UAConfigHolder();
  ~UAConfigHolder() = delete;
  UAConfigHolder(const UAConfigHolder&) = delete;
  UAConfigHolder& operator=(const UAConfigHolder&) = delete;

  base::Lock lock_;
  mojom::BrandConfiguration current_config_;  // Guarded by lock_
};

}  // namespace vivaldi_user_agent

#endif  // COMPONENTS_USER_AGENT_UA_CONFIG_HOLDER_H_