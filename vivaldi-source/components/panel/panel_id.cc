// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#include "components/panel/panel_id.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

namespace vivaldi {
namespace {
static const char* kPanelId = "panelId";
}

std::optional<std::string> ParseVivPanelId(const std::string& viv_ext_data) {
  std::optional<base::Value> json =
      base::JSONReader::Read(viv_ext_data, base::JSON_PARSE_RFC);

  if (!json || !json->is_dict()) {
    return std::nullopt;
  }

  const std::string* panel_id = json->GetDict().FindString(kPanelId);

  return panel_id ? *panel_id : std::optional<std::string>();
}
}  // namespace vivaldi
