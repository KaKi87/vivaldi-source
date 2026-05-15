// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/ext_data/tab_ext_data_observer.h"
#include "app/vivaldi_apptools.h"

namespace vivaldi {
TabExtDataObserver::TabExtDataObserver(content::WebContents* web_contents,
                                       OnKeysChangedCallback callback)
    : web_contents_(web_contents), callback_(std::move(callback)) {

  if (!vivaldi::IsVivaldiRunning())
    return;

  vivaldi::TabExtData::Get(web_contents_)->AddObserver(this);
}

TabExtDataObserver::~TabExtDataObserver() {

  if (!vivaldi::IsVivaldiRunning())
    return;

  if (vivaldi::TabExtData::Get(web_contents_)) {
    vivaldi::TabExtData::Get(web_contents_)->RemoveObserver(this);
  };
}

void TabExtDataObserver::OnKeysChanged(
    vivaldi::TabExtData* tab_ext_data,
    const std::set<std::string>& changed_keys) {

  if (!vivaldi::IsVivaldiRunning())
    return;

  callback_.Run(tab_ext_data, changed_keys);
}
}  // namespace vivaldi
