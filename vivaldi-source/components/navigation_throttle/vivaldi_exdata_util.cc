// Copyright (c) 2025 Vivaldi. All rights reserved.

#include "vivaldi_exdata_util.h"

#include "base/json/json_reader.h"
#include "content/public/browser/web_contents.h"

namespace vivaldi {

std::optional<std::string> GetFollowerTabExtId(
    content::WebContents* web_contents) {
  std::string viv_ext_data = web_contents->GetVivExtData();
  std::optional<base::Value> json =
      base::JSONReader::Read(viv_ext_data, base::JSON_PARSE_RFC);
  std::optional<std::string> follower_tab_ext_id;
  if (json && json->is_dict()) {
    const std::string* ext_ptr = json->GetDict().FindString("followerTabExtId");
    follower_tab_ext_id = ext_ptr ? std::make_optional(*ext_ptr) : std::nullopt;
  }
  return follower_tab_ext_id;
}

std::optional<std::string> GetExtId(content::WebContents* web_contents) {
  std::string viv_ext_data = web_contents->GetVivExtData();
  std::optional<base::Value> json =
      base::JSONReader::Read(viv_ext_data, base::JSON_PARSE_RFC);
  std::optional<std::string> ext_id;
  if (json && json->is_dict()) {
    const std::string* ext_ptr = json->GetDict().FindString("ext_id");
    ext_id = ext_ptr ? std::make_optional(*ext_ptr) : std::nullopt;
  }
  return ext_id;
}

}  // namespace vivaldi
