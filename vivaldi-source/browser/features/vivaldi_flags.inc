// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

// This file is included to chromium/chrome/browser/about_flags.cc and
// chromium/ios/chrome/browser/flags/about_flags.mm at the end of the definition
// of kFeatureEntries.

// Instructions on how to add new feature flags:
// https://wiki.viv.int/doku.php?id=featuresystem#the_feature_flag_system

#include "browser/features/vivaldi_features.h"
{vivaldi_flag::kChromePagesInternalName, vivaldi_flag::kChromePagesName,
 vivaldi_flag::kChromePagesDescription,
 flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux | flags_ui::kOsCrOS,
 FEATURE_VALUE_TYPE(vivaldi_features::kChromePages)},

    {vivaldi_flag::kCssModsInternalName, vivaldi_flag::kCssModsName,
     vivaldi_flag::kCssModsDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kCssMods)},

    {vivaldi_flag::kDoubleClickMenuInternalName,
     vivaldi_flag::kDoubleClickMenuName,
     vivaldi_flag::kDoubleClickMenuDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kDoubleClickMenu)},

    {vivaldi_flag::kHorizontalPinnedTabsInternalName,
     vivaldi_flag::kHorizontalPinnedTabsName,
     vivaldi_flag::kHorizontalPinnedTabsDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kHorizontalPinnedTabs)},

    {vivaldi_flag::kInternalPageReaderModeInternalName,
     vivaldi_flag::kInternalPageReaderModeName,
     vivaldi_flag::kInternalPageReaderModeDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kInternalPageReaderMode)},

    {vivaldi_flag::kLocalCalculatorInternalName,
     vivaldi_flag::kLocalCalculatorName,
     vivaldi_flag::kLocalCalculatorDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux
     | flags_ui::kOsCrOS | flags_ui::kOsAndroid | flags_ui::kOsIos,
     FEATURE_VALUE_TYPE(vivaldi_features::kLocalCalculator)},

    {vivaldi_flag::kNoteEditorName, vivaldi_flag::kNoteEditorOption,
     vivaldi_flag::kNoteEditorDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kNoteEditor)},

    {vivaldi_flag::kSettings20InternalName, vivaldi_flag::kSettings20Name,
     vivaldi_flag::kSettings20Description,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux,
     FEATURE_VALUE_TYPE(vivaldi_features::kSettings20)},

    {vivaldi_flag::kToolbarOverflowInternalName, vivaldi_flag::kToolbarOverflowName,
     vivaldi_flag::kToolbarOverflowDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux,
     FEATURE_VALUE_TYPE(vivaldi_features::kToolbarOverflow)},

#if BUILDFLAG(IS_IOS)
    // iOS specific feature flags should be delcared within this block.

    {vivaldi_flag::kVivaldiIOSCopySanitizedLinkInternalName,
     vivaldi_flag::kVivaldiIOSCopySanitizedLinkName,
     vivaldi_flag::kVivaldiIOSCopySanitizedLinkDescription, flags_ui::kOsIos,
     FEATURE_VALUE_TYPE(vivaldi_features::kVivaldiIOSCopySanitizedLink)},

    {vivaldi_flag::kVivaldiIOSShowRefactoredStartPageInternalName,
     vivaldi_flag::kVivaldiIOSShowRefactoredStartPageName,
     vivaldi_flag::kVivaldiIOSShowRefactoredStartPageDescription,
     flags_ui::kOsIos,
     FEATURE_VALUE_TYPE(vivaldi_features::kVivaldiIOSShowRefactoredStartPage)},

#endif  // BUILDFLAG(IS_IOS)

#if defined(OEM_AUTOMOTIVE_BUILD)
    {vivaldi_flag::kCinemaModeInternalName, vivaldi_flag::kCinemaModeName,
     vivaldi_flag::kCinemaModeDescription, flags_ui::kOsAndroid,
     FEATURE_VALUE_TYPE(vivaldi_features::kCinemaMode)},
#endif  // defined(OEM_AUTOMOTIVE_BUILD)

    {vivaldi_flag::kVivaldiUseNewUrlSanitizerInternalName,
     vivaldi_flag::kVivaldiUseNewUrlSanitizerName,
     vivaldi_flag::kVivaldiUseNewUrlSanitizerDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kVivaldiUseNewUrlSanitizer)},

    {vivaldi_flag::kMailSavingAttachmentsInternalName,
     vivaldi_flag::kMailSavingAttachmentsName,
     vivaldi_flag::kMailSavingAttachmentsDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kMailSavingAttachments)},
