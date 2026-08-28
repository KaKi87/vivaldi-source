// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#include "extensions/ext_data/tab_positioning_helper.h"

namespace vivaldi {

TabPositioningParams GetTabPositioningParamsFromCreateProperties(
    const extensions::api::tabs::Create::Params::CreateProperties&
        create_properties) {
  using extensions::api::tabs::VivaldiInvokedBy;

  TabPositioningParams params;
  switch (create_properties.invoked_by) {
    case VivaldiInvokedBy::kMainStrip:
      params.invoked_by = TabInvokedBy::kMainStrip;
      break;

    case VivaldiInvokedBy::kSubStrip:
      params.invoked_by = TabInvokedBy::kSubStrip;
      break;

    case VivaldiInvokedBy::kKeyboard:
      params.invoked_by = TabInvokedBy::kKeyboard;
      break;

    case VivaldiInvokedBy::kAccordion:
      params.invoked_by = TabInvokedBy::kAccordion;
      break;

    case VivaldiInvokedBy::kTabBarButton:
      params.invoked_by = TabInvokedBy::kTabBarButton;
      break;

    case VivaldiInvokedBy::kNone:
      params.invoked_by = TabInvokedBy::kNone;
      break;

    case VivaldiInvokedBy::kEmailUi:
      params.invoked_by = TabInvokedBy::kEmailUi;
      break;

    case VivaldiInvokedBy::kBookmarks:
      params.invoked_by = TabInvokedBy::kBookmarks;
      break;

    case VivaldiInvokedBy::kSpeedDial:
      params.invoked_by = TabInvokedBy::kSpeedDial;
      break;

    case VivaldiInvokedBy::kCommand:
      params.invoked_by = TabInvokedBy::kCommand;
      break;

    case VivaldiInvokedBy::kVivaldiUi:
      params.invoked_by = TabInvokedBy::kVivaldiUi;
      break;
  }

  params.invoked_by_extra_arg = create_properties.invoked_by_extra_arg;
  return params;
}
}  // namespace vivaldi
