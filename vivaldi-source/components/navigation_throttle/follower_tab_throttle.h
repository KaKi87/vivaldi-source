// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_NAVIGATION_THROTTLE_FOLLOWER_TAB_THROTTLE_H_
#define COMPONENTS_NAVIGATION_THROTTLE_FOLLOWER_TAB_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

class FollowerTabThrottle : public content::NavigationThrottle {
 public:
  explicit FollowerTabThrottle(content::NavigationThrottleRegistry& registry);
  ~FollowerTabThrottle() override;

  ThrottleCheckResult WillStartRequest() override;

  const char* GetNameForLogging() override;
};

#endif  //  COMPONENTS_NAVIGATION_THROTTLE_FOLLOWER_TAB_THROTTLE_H_
