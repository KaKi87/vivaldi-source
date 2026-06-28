// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_USER_AGENT_VIVALDI_IOS_USER_AGENT_OVERRIDE_H_
#define COMPONENTS_USER_AGENT_VIVALDI_IOS_USER_AGENT_OVERRIDE_H_

#include <string>

#include "ios/web/common/user_agent.h"

class GURL;

namespace vivaldi_ios_user_agent {

std::string GetUserAgentForURL(web::UserAgentType type,
                               const GURL& url,
                               std::string default_user_agent);

}  // namespace vivaldi_ios_user_agent

#endif  // COMPONENTS_USER_AGENT_VIVALDI_IOS_USER_AGENT_OVERRIDE_H_
