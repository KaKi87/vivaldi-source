// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.
//
#include "extensions/helper/ext_data_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"

#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"

namespace vivaldi {
namespace ext_data_helper {
void SetGroupExtData(const std::string& group_id,
                     TabExtKey key,
                     const std::optional<std::string>& value) {
  for (BrowserWindowInterface* browser : GetAllBrowserWindowInterfaces()) {
    TabStripModel* tab_strip = browser->GetTabStripModel();
    for (tabs::TabInterface* tab : *tab_strip) {
      content::WebContents* contents = tab->GetContents();

      if (group_id != TabExtData::Get(contents)->GetGroupId())
        continue;

      auto* ext = TabExtData::Get(contents);
      if (value) {
        ext->SetUnsafe(key, base::Value(std::string(*value)));
      } else {
        ext->Remove(key);
      }
    }
  }
}

void SetGroupTitle(const std::string& group_id,
                   const std::optional<std::string>& title) {
  SetGroupExtData(group_id, TabExtKey::kFixedGroupTitle, title);
}

void SetGroupColor(const std::string& group_id,
                   const std::optional<std::string>& color) {
  SetGroupExtData(group_id, TabExtKey::kGroupColor, color);
}
}  // namespace ext_data_helper
}  // namespace vivaldi
