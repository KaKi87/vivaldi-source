// Copyright (c) 2013-2023 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/api/savedpasswords/password_list_sorter.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "components/password_manager/core/browser/password_store/stored_credential.h"
#include "url/gurl.h"
namespace extensions {
namespace {
std::string CreateSortKey(const password_manager::StoredCredential& cred) {
  std::string key = cred.url.spec();

  if (!cred.username_value.empty()) {
    key += base::UTF16ToUTF8(cred.username_value);
  }

  return key;
}
}  // namespace

void SortEntriesAndHideDuplicates(
    std::vector<std::unique_ptr<password_manager::StoredCredential>>* list) {
  using SortPair =
      std::pair<std::string,
                std::unique_ptr<password_manager::StoredCredential>>;
  std::vector<SortPair> keys_to_forms;

  keys_to_forms.reserve(list->size());
  for (auto& form : *list) {
    std::string key = CreateSortKey(*form);
    keys_to_forms.emplace_back(std::move(key), std::move(form));
  }

  std::sort(
      keys_to_forms.begin(), keys_to_forms.end(),
      [](const SortPair& a, const SortPair& b) { return a.first < b.first; });

  list->clear();

  std::string previous_key;
  for (auto& key_to_form : keys_to_forms) {
    if (key_to_form.first != previous_key) {
      list->push_back(std::move(key_to_form.second));
      previous_key = key_to_form.first;
    }
  }
}

}  // namespace extensions
