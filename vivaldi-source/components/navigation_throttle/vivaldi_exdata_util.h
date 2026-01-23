// Copyright (c) 2025 Vivaldi. All rights reserved.

#ifndef COMPONENTS_NAVIGATION_THROTTLE_VIVALDI_EXDATA_UTIL_H_
#define COMPONENTS_NAVIGATION_THROTTLE_VIVALDI_EXDATA_UTIL_H_

#include <optional>
#include <string>

namespace content {
class WebContents;
}

namespace vivaldi {

std::optional<std::string> GetFollowerTabExtId(
    content::WebContents* web_contents);
std::optional<std::string> GetExtId(content::WebContents* web_contents);

}  // namespace vivaldi

#endif  // COMPONENTS_NAVIGATION_THROTTLE_VIVALDI_EXDATA_UTIL_H_
