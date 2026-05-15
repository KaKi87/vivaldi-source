// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_EXT_DATA_TAB_EXT_DATA_OBSERVER_H_
#define COMPONENTS_EXT_DATA_TAB_EXT_DATA_OBSERVER_H_

#include <set>
#include <string>

#include "base/functional/callback.h"
#include "components/ext_data/tab_ext_data.h"

namespace content {
class WebContents;
}

namespace vivaldi {

// This class provides a compact way to set up an observer with just a
// web_contents and a callback. Mostly useful for avoiding complex patches on
// chromium.
class TabExtDataObserver : public vivaldi::TabExtData::Observer {
 public:
  using OnKeysChangedCallback =
      base::RepeatingCallback<void(vivaldi::TabExtData* tab_ext_data,
                                   const std::set<std::string>& changed_keys)>;
  TabExtDataObserver(content::WebContents* web_contents,
                     OnKeysChangedCallback callback);
  ~TabExtDataObserver();

  // Implementing TabExtData::Observer
  void OnKeysChanged(vivaldi::TabExtData* tab_ext_data,
                     const std::set<std::string>& changed_keys) override;

 private:
  raw_ptr<content::WebContents> web_contents_;
  OnKeysChangedCallback callback_;
};
}  // namespace vivaldi

#endif  // COMPONENTS_EXT_DATA_TAB_EXT_DATA_OBSERVER_H_
