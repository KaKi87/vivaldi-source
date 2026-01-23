// Copyright (c) 2018 Vivaldi Technologies AS. All rights reserved
// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "installer/mini_installer/setup/brand_behaviors.h"
#include "installer/util/vivaldi_setup_util.h"

namespace installer {

void DoPostUninstallOperations(const base::Version& version) {
  vivaldi::DoPostUninstallOperations(version);
}

}  // namespace installer
