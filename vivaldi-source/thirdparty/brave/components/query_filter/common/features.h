// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_COMMON_FEATURES_H_

#include "browser/features/vivaldi_features.h"

#include "base/feature_list.h"

namespace query_filter {
namespace features {

// Vivaldi: Alias to the vivaldi-side feature so the verbatim query_filter code
// shares a single canonical flag with Vivaldi.
extern const base::Feature& kQueryFilterComponent;

}  // namespace features
}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_COMMON_FEATURES_H_
