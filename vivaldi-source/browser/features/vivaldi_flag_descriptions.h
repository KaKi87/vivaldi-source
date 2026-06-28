// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#ifndef BROWSER_FLAGS_VIVALDI_FLAG_DEFINITIONS_H_
#define BROWSER_FLAGS_VIVALDI_FLAG_DEFINITIONS_H_

// This file is included to chromium/chrome/browser/about_flags.cc.
// That means you need to add flag descriptions, feature choices, and custom
// types here.

// Includes here might be unused but are required for vivaldi_flags.inc file.
#include "browser/features/vivaldi_features.h"
#include "build/build_config.h"
#include "components/webui/flags/feature_entry.h"

namespace vivaldi_flag {

// Cross-platform
extern const char kChromePagesInternalName[];
extern const char kChromePagesName[];
extern const char kChromePagesDescription[];

extern const char kCssModsInternalName[];
extern const char kCssModsName[];
extern const char kCssModsDescription[];

extern const char kDoubleClickMenuInternalName[];
extern const char kDoubleClickMenuName[];
extern const char kDoubleClickMenuDescription[];

extern const char kHorizontalPinnedTabsInternalName[];
extern const char kHorizontalPinnedTabsName[];
extern const char kHorizontalPinnedTabsDescription[];

extern const char kInternalPageReaderModeInternalName[];
extern const char kInternalPageReaderModeName[];
extern const char kInternalPageReaderModeDescription[];

extern const char kRelatedTabsInternalName[];
extern const char kRelatedTabsName[];
extern const char kRelatedTabsDescription[];

extern const char kNoteEditorName[];
extern const char kNoteEditorOption[];
extern const char kNoteEditorDescription[];

extern const char kSettings20InternalName[];
extern const char kSettings20Name[];
extern const char kSettings20Description[];

#if BUILDFLAG(IS_IOS)
// iOS specific feature flags should be delcared within this block.

extern const char kBankIDDigIDLatencyWorkaroundInternalName[];
extern const char kBankIDDigIDLatencyWorkaroundName[];
extern const char kBankIDDigIDLatencyWorkaroundDescription[];

extern const char kVivaldiIOSCopySanitizedLinkInternalName[];
extern const char kVivaldiIOSCopySanitizedLinkName[];
extern const char kVivaldiIOSCopySanitizedLinkDescription[];

extern const char kVivaldiIOSShowRefactoredStartPageInternalName[];
extern const char kVivaldiIOSShowRefactoredStartPageName[];
extern const char kVivaldiIOSShowRefactoredStartPageDescription[];

#endif  // BUILDFLAG(IS_IOS)

extern const char kVivaldiUseNewUrlSanitizerInternalName[];
extern const char kVivaldiUseNewUrlSanitizerName[];
extern const char kVivaldiUseNewUrlSanitizerDescription[];

extern const char kMailSavingAttachmentsInternalName[];
extern const char kMailSavingAttachmentsName[];
extern const char kMailSavingAttachmentsDescription[];

#if defined(OEM_AUTOMOTIVE_BUILD)
extern const char kCinemaModeInternalName[];
extern const char kCinemaModeName[];
extern const char kCinemaModeDescription[];
#endif  // defined(OEM_AUTOMOTIVE_BUILD)

// Please add names and descriptions above in alphabetical order.

}  // namespace vivaldi_flag

#endif  // BROWSER_FLAGS_VIVALDI_FLAG_DEFINITIONS_H_
