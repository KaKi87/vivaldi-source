// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_interstitials/core/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace security_interstitials::features {

// Enables a dialog-based UI for HTTPS-First Mode.
// NOTE(ondrej@vivaldi.com): VB-122838
BASE_FEATURE(kHttpsFirstDialogUi, base::FEATURE_DISABLED_BY_DEFAULT); // Vivaldi keep disabled

BASE_FEATURE(kInsecureFormNavigationThrottleForPrerender,
             base::FEATURE_ENABLED_BY_DEFAULT);
}  // namespace security_interstitials::features
