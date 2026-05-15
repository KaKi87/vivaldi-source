// Copyright (c) 2018 Vivaldi Technologies AS. All rights reserved

#include "chrome/browser/ui/sync/tab_contents_synced_tab_delegate.h"

#include "components/ext_data/tab_ext_data.h"

std::string TabContentsSyncedTabDelegate::GetVivExtData() const {
#if BUILDFLAG(IS_ANDROID)
  if (!vivaldi::TabExtData::Has(web_contents_))
    return "";
#endif
  return vivaldi::TabExtData::Get(web_contents_)->ToString();
}
