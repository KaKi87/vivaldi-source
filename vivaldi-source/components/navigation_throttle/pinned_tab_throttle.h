// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_NAVIGATION_THROTTLE_PINNED_TAB_THROTTLE_H_
#define COMPONENTS_NAVIGATION_THROTTLE_PINNED_TAB_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}

class PinnedTabsThrottle : public content::NavigationThrottle {
 public:
  explicit PinnedTabsThrottle(content::NavigationThrottleRegistry& registry);
  ~PinnedTabsThrottle() override;

  ThrottleCheckResult WillStartRequest() override;

  const char* GetNameForLogging() override;
};

#endif  // COMPONENTS_NAVIGATION_THROTTLE_PINNED_TAB_THROTTLE_H_
