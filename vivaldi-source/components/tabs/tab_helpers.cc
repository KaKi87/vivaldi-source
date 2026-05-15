// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "components/tabs/tab_helpers.h"

#include "base/json/json_reader.h"

namespace vivaldi {
constexpr char kVivaldiWorkspace[] = "workspaceId";

namespace {
std::optional<double> GetTabWorkspaceId(const std::string& viv_extdata) {
  base::JSONParserOptions options = base::JSON_PARSE_RFC;
  std::optional<base::Value> json =
      base::JSONReader::Read(viv_extdata, options);
  std::optional<double> value;
  if (json && json->is_dict()) {
    value = json->GetDict().FindDouble(kVivaldiWorkspace);
  }
  return value;
}
}  // namespace

bool IsTabInAWorkspace(const sessions::SessionTab& tab) {
  return GetTabWorkspaceId(tab).has_value();
}

std::optional<double> GetTabWorkspaceId(const sessions::SessionTab& tab) {
  return GetTabWorkspaceId(tab.viv_ext_data);
}
}  // namespace vivaldi
