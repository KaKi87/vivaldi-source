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

    {vivaldi_flag::kInternalPageReaderModeInternalName,
     vivaldi_flag::kInternalPageReaderModeName,
     vivaldi_flag::kInternalPageReaderModeDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kInternalPageReaderMode)},

    {vivaldi_flag::kLayoutsInternalName, vivaldi_flag::kLayoutsName,
     vivaldi_flag::kLayoutsDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kLayouts)},

    {vivaldi_flag::kNoteEditorName, vivaldi_flag::kNoteEditorOption,
     vivaldi_flag::kNoteEditorDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kNoteEditor)},

    {vivaldi_flag::kPanelOnboardingInternalName,
     vivaldi_flag::kPanelOnboardingName,
     vivaldi_flag::kPanelOnboardingDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kPanelOnboarding)},

    {vivaldi_flag::kRelatedTabsInternalName, vivaldi_flag::kRelatedTabsName,
     vivaldi_flag::kRelatedTabsDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kRelatedTabs)},

    {vivaldi_flag::kSettings20InternalName, vivaldi_flag::kSettings20Name,
     vivaldi_flag::kSettings20Description,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux,
     FEATURE_VALUE_TYPE(vivaldi_features::kSettings20)},

    {vivaldi_flag::kThemeUnifiedInternalName, vivaldi_flag::kThemeUnifiedName,
     vivaldi_flag::kThemeUnifiedDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux,
     FEATURE_VALUE_TYPE(vivaldi_features::kThemeUnified)},

#if BUILDFLAG(IS_IOS)
    // iOS specific feature flags should be delcared within this block.

    {vivaldi_flag::kBankIDDigIDLatencyWorkaroundInternalName,
     vivaldi_flag::kBankIDDigIDLatencyWorkaroundName,
     vivaldi_flag::kBankIDDigIDLatencyWorkaroundDescription, flags_ui::kOsIos,
     FEATURE_VALUE_TYPE(vivaldi_features::kBankIDDigIDLatencyWorkaround)},

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

    {vivaldi_flag::kShowNewDeviceChooserInternalName,
     vivaldi_flag::kShowNewDeviceChooserName,
     vivaldi_flag::kShowNewDeviceChooserDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kShowNewDeviceChooser)},

    {vivaldi_flag::kShowUnifiedSiteDialogInternalName,
     vivaldi_flag::kShowUnifiedSiteDialogName,
     vivaldi_flag::kShowUnifiedSiteDialogDescription,
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsLinux |
         flags_ui::kOsCrOS,
     FEATURE_VALUE_TYPE(vivaldi_features::kShowUnifiedSiteDialog)},
