// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_TABS_TAB_HELPERS_H_
#define COMPONENTS_TABS_TAB_HELPERS_H_

#include <optional>

#include "components/sessions/core/session_types.h"

namespace vivaldi {

bool IsTabInAWorkspace(const sessions::SessionTab& session_tab);
std::optional<double> GetTabWorkspaceId(const sessions::SessionTab& session_tab);

}  // namespace vivaldi

#endif  // COMPONENTS_TABS_TAB_HELPERS_H_
