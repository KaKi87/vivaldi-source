// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_EXT_DATA_TAB_POSITIONING_PARAMS_H_
#define COMPONENTS_EXT_DATA_TAB_POSITIONING_PARAMS_H_

#include <optional>
#include <string>

namespace vivaldi {

// You may want to also update:
// extensions/ext_data/tab_positioning_helper.cc
// chromium/chrome/common/extensions/api/tabs.json

enum class TabInvokedBy {
  kNone = 0,  // Default vivaldi position.
  kMainStrip = 1,
  kSubStrip = 2,  // [+] in substrip
  kKeyboard = 3,
  kAccordion = 4,
  kTabBarButton = 5,  // [+] in main strip
  kChromiumExtension = 6,
  kEmailUi = 7,     // "Compose" button so far
  kHtml = 8,        // link click
  kBackground = 9,  // open in background
  kBookmarks = 10,
  kSpeedDial = 11,
  kCommand = 12,
  kEmailLinkBackground = 13,
  kEmailLink = 14,
  kPanelLinkBackground = 15,
  kPanelLink = 16,
  kVivaldiUi = 17,
  kDownload = 18,
};

struct TabPositioningParams {
  TabInvokedBy invoked_by = TabInvokedBy::kNone;
  std::optional<std::string> invoked_by_extra_arg;
};

}  // namespace vivaldi

#endif  // COMPONENTS_EXT_DATA_TAB_POSITIONING_PARAMS_H_
