// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#ifndef EXTENSIONS_EXT_DATA_TAB_POSITIONING_HELPER_H_
#define EXTENSIONS_EXT_DATA_TAB_POSITIONING_HELPER_H_

#include "chrome/common/extensions/api/tabs.h"
#include "components/ext_data/tab_positioning_params.h"

namespace vivaldi {

TabPositioningParams GetTabPositioningParamsFromCreateProperties(
    const extensions::api::tabs::Create::Params::CreateProperties&
        create_properties);

}  // namespace vivaldi

#endif  // EXTENSIONS_EXT_DATA_TAB_POSITIONING_HELPER_H_
