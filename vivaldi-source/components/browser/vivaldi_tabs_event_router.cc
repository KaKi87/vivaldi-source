// Copyright (c) 2021 Vivaldi Technologies AS. All rights reserved

#include "chrome/browser/extensions/api/tabs/tabs_event_router.h"

#include "app/vivaldi_apptools.h"
#include "app/vivaldi_constants.h"
#include "browser/tab_probe.h"
#include "components/ext_data/tab_ext_data.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "extensions/schema/tabs_private.h"
#include "extensions/tools/vivaldi_tools.h"

namespace extensions {

bool TabsEventRouter::TabEntry::SetDiscarded(bool new_val) {
  if (was_discarded_ == new_val)
    return false;
  was_discarded_ = new_val;
  return true;
}

void TabsEventRouter::VivExtDataUpdated(
    ::vivaldi::TabExtData* tab_ext_data,
    const std::set<std::string>& changed_keys) {
  int tab_id =
      sessions::SessionTabHelper::IdForTab(tab_ext_data->GetWebContents()).id();
  base::DictValue value;
  for (const std::string& key : changed_keys) {
    auto* inner = tab_ext_data->Get(key);
    if (inner) {
      value.Set(key, inner->Clone());
    } else {
      value.Set(key, base::Value(base::Value::Type::NONE));
    }
  }

  ::vivaldi::BroadcastEvent(
      vivaldi::tabs_private::OnExtDataChanged::kEventName,
      vivaldi::tabs_private::OnExtDataChanged::Create(
          tab_id, base::Value(std::move(value))),
      tab_ext_data->GetWebContents()->GetBrowserContext());

  std::set<std::string> changed_property_names;
  changed_property_names.insert(::vivaldi::kExtDataKey);
  DispatchTabUpdatedEvent(tab_ext_data->GetWebContents(),
                          std::move(changed_property_names));
}

}  // namespace extensions
