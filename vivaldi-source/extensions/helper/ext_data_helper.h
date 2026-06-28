// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_EXT_DATA_EXT_DATA_HELPER_H_
#define COMPONENTS_EXT_DATA_EXT_DATA_HELPER_H_

#include <optional>
#include <string>

namespace vivaldi {
enum class TabExtKey;

namespace ext_data_helper {
void SetGroupTitle(const std::string& groupId,
                   const std::optional<std::string>& title);
void SetGroupColor(const std::string& groupId,
                   const std::optional<std::string>& color);

void SetGroupExtData(const std::string& group_id,
                     ::vivaldi::TabExtKey key,
                     const std::optional<std::string>& value);
}  // namespace ext_data_helper
}  // namespace vivaldi

#endif  // COMPONENTS_EXT_DATA_EXT_DATA_HELPER_H_
