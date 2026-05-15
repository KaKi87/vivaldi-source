// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include "extensions/helper/vivaldi_tab_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chromium/chrome/browser/ui/browser_list.h"
#include "components/ext_data/tab_ext_data.h"
#include "extensions/api/guest_view/parent_tab_user_data.h"

namespace vivaldi {

TabType GetVivaldiPanelType(content::WebContents* web_content) {
  CHECK(web_content);
  std::optional<int> parent_id =
      ::vivaldi::ParentTabUserData::GetParentTabId(web_content);
  if (parent_id) {
    if ((*parent_id) > 0) {
      return TabType::WIDGET;
    }
  }
  std::optional<std::string> panel_id =
      TabExtData::Get(web_content)->GetPanelId();
  if (!panel_id)
    return TabType::PAGE;

  if (panel_id->rfind("WEBPANEL_", 0) == 0) {
    return TabType::WEBPANEL;
  }

  if (panel_id->rfind("EXT_PANEL_", 0) == 0) {
    return TabType::SIDEPANEL;
  }

  if (panel_id->rfind("WebWidget_", 0) == 0) {
    return TabType::WIDGET;
  }

  return TabType::INVALID;
}

bool IsPanel(TabType type) {
  return type == TabType::WEBPANEL || type == TabType::SIDEPANEL;
}

bool IsPage(TabType type) {
  return type == TabType::PAGE;
}

bool IsWidget(TabType type) {
  return type == TabType::WIDGET;
}

}  // namespace vivaldi
