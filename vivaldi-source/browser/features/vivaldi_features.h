// Copyright 2025 Vivaldi Technologies. All rights reserved.

#ifndef BROWSER_FEATURES_VIVALDI_FEATURES_H_
#define BROWSER_FEATURES_VIVALDI_FEATURES_H_

#include "base/feature_list.h"
#include "build/build_config.h"

// Flags must be in alphabetical order.
namespace vivaldi_features {

BASE_DECLARE_FEATURE(kChromePages);

BASE_DECLARE_FEATURE(kCssMods);

BASE_DECLARE_FEATURE(kDoubleClickMenu);

BASE_DECLARE_FEATURE(kInternalPageReaderMode);

BASE_DECLARE_FEATURE(kLayouts);

BASE_DECLARE_FEATURE(kNoteEditor);

BASE_DECLARE_FEATURE(kPanelOnboarding);

BASE_DECLARE_FEATURE(kRelatedTabs);

BASE_DECLARE_FEATURE(kSettings20);

BASE_DECLARE_FEATURE(kShowNewDeviceChooser);
bool IsNewDeviceChooserEnabled();

BASE_DECLARE_FEATURE(kThemeUnified);

#if BUILDFLAG(IS_IOS)
// iOS specific feature flags should be delcared within this block.

BASE_DECLARE_FEATURE(kBankIDDigIDLatencyWorkaround);

BASE_DECLARE_FEATURE(kVivaldiIOSShowRefactoredStartPage);
bool IsVivaldiIOSShowRefactoredStartPageEnabled();

#endif  // BUILDFLAG(IS_IOS)

#if defined(OEM_AUTOMOTIVE_BUILD)
BASE_DECLARE_FEATURE(kCinemaMode);
#endif  // defined(OEM_AUTOMOTIVE_BUILD)

BASE_DECLARE_FEATURE(kShowUnifiedSiteDialog);

}  // namespace vivaldi_features

#endif  // BROWSER_FEATURES_VIVALDI_FEATURES_H_
