// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "brave/components/query_filter/common/features.h"

namespace query_filter {
namespace features {

// Alias to the vivaldi-side feature so the verbatim query_filter code
// shares a single canonical flag with Vivaldi.
const base::Feature& kQueryFilterComponent =
    ::vivaldi_features::kVivaldiUseNewUrlSanitizer;

}  // namespace features
}  // namespace query_filter
