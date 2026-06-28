// Copyright (c) 2013-2023 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Based on password_manager::password_list_sorter.h that got moved into
// chromium\ios\chrome\browser\autofill\manual_fill\password_list_sorter.h
// during Chromium 120 upgrade

#ifndef EXTENSIONS_API_SAVEDPASSWORDS_PASSWORD_LIST_SORTER_H_
#define EXTENSIONS_API_SAVEDPASSWORDS_PASSWORD_LIST_SORTER_H_

#include <memory>
#include <vector>

namespace password_manager {
struct StoredCredential;
}

namespace extensions {

// Sort entries of |list| based on sort key. The key is the concatenation of
// origin, username
void SortEntriesAndHideDuplicates(
    std::vector<std::unique_ptr<password_manager::StoredCredential>>* list);

}  // namespace extensions

#endif  // EXTENSIONS_API_SAVEDPASSWORDS_PASSWORD_LIST_SORTER_H_
