// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Declares functions that implement brand-specific behavior. The definitions
// reside in brand-specific files that are only compiled when their respective
// branding is in use at build time.

#ifndef CHROME_INSTALLER_SETUP_BRAND_BEHAVIORS_H_
#define CHROME_INSTALLER_SETUP_BRAND_BEHAVIORS_H_

namespace base {
class Version;
}  // namespace base

namespace installer {

// Performs brand-specific operations following unintsallation of the browser.
// |version| is the version of the browser being uninstalled.
void DoPostUninstallOperations(const base::Version& version);

}  // namespace installer

#endif  // CHROME_INSTALLER_SETUP_BRAND_BEHAVIORS_H_
