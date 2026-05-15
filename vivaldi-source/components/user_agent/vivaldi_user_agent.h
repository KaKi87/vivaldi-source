// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_USER_AGENT_VIVALDI_USER_AGENT_H_
#define COMPONENTS_USER_AGENT_VIVALDI_USER_AGENT_H_

#include <string>
#include <string_view>
#include <vector>

#include "url/gurl.h"

namespace vivaldi_user_agent {

extern const char kVivaldiSuffix[];

bool SpoofStableChromiumVersion(GURL url);

// Should we spoof the useragent on the URL?
bool IsUrlAllowed(const GURL& url);

// Update the user agent string based on the current scoped thread URL
void UpdateAgentString(bool reduced, std::string& user_agent);

std::vector<std::string> GetVivaldiAllowlist();
std::vector<std::string> GetVivaldiEdgeList();

std::string UpdateReducedChromeProductString(std::string actual_product);
std::string_view UpdateChromeProductString(std::string_view actual_product);

std::string UpdateChromeFullVersionString(std::string_view actual_product);
std::string UpdateChromeMajorVersionString(
    std::string_view actual_product);

class ScopedVivaldiThreadURL {
 public:
  ScopedVivaldiThreadURL() : ScopedVivaldiThreadURL(GURL()) {}
  ScopedVivaldiThreadURL(GURL url);
  ~ScopedVivaldiThreadURL();

 static std::optional<GURL> GetURLForThread();

 private:
  ScopedVivaldiThreadURL(ScopedVivaldiThreadURL&) = delete;
  ScopedVivaldiThreadURL& operator=(ScopedVivaldiThreadURL&) = delete;

  static std::optional<GURL>& GetInstanceForThread();

  GURL url_;
  std::optional<GURL> old_status;
};

}  // namespace vivaldi_user_agent

#endif  // COMPONENTS_USER_AGENT_VIVALDI_USER_AGENT_H_
