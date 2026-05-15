// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved
#include "browser/features/vivaldi_flag_descriptions.h"

namespace vivaldi_flag {

// Internal names of the features MUST be globally unique, thus prefix those
// with 'vivaldi-'. Also, please follow naming convension, k...InternalName,
// k...Name, k...Description.

const char kChromePagesInternalName[] = "vivaldi-chrome-pages";
const char kChromePagesName[] = "Enable Chrome Pages";
const char kChromePagesDescription[] =
    "Do not redirect chrome pages to vivaldi pages.";

const char kCssModsInternalName[] = "vivaldi-css-mods";
const char kCssModsName[] = "Allow CSS modifications";
const char kCssModsDescription[] =
    "Allow user interface CSS modifications set in Appearance settings.";

const char kDoubleClickMenuInternalName[] = "vivaldi-double-click-menu";
const char kDoubleClickMenuName[] = "Show context menu on a double click";
const char kDoubleClickMenuDescription[] =
    "Use a double click anywhere in the page to show its context menu. "
    "Requires a restart of Vivaldi to take effect.";

extern const char kInternalPageReaderModeInternalName[] =
    "vivaldi-internal-page-reader-mode";
extern const char kInternalPageReaderModeName[] =
    "Use internal page for reader mode";
extern const char kInternalPageReaderModeDescription[] =
    "Should give better results on most pages but is lacking in translation "
    "features.";

const char kLayoutsInternalName[] = "vivaldi-layouts";
const char kLayoutsName[] = "Vivaldi Layouts";
const char kLayoutsDescription[] =
    "Select one of several predefined browser layouts";

const char kShowNewDeviceChooserInternalName[] =
    "vivaldi-show-new-device-chooser";
const char kShowNewDeviceChooserName[] = "Show new device chooser dialog";
const char kShowNewDeviceChooserDescription[] =
    "Replaces chromium builtin device chooser dialogs with new unified "
    "permission bubbles";

const char kShowUnifiedSiteDialogInternalName[] =
    "vivaldi-show-unified-site-dialog";
const char kShowUnifiedSiteDialogName[] = "Show unified site dialog";
const char kShowUnifiedSiteDialogDescription[] =
    "Consolidates separate Chrome and Vivaldi site dialogs into a single "
    "unified interface";

const char kNoteEditorName[] = "vivaldi-note-editor";
const char kNoteEditorOption[] = "Use the new note editor";
const char kNoteEditorDescription[] =
    "Give access to a new note editor with more features.";

const char kPanelOnboardingInternalName[] = "vivaldi-panel-onboarding";
const char kPanelOnboardingName[] = "Onboarding Panel Step";
const char kPanelOnboardingDescription[] =
    "Add 2 steps in onboarding to select internal and web panels";

const char kRelatedTabsInternalName[] = "vivaldi-related-tabs";
const char kRelatedTabsName[] = "Show related tabs sort option.";
const char kRelatedTabsDescription[] =
    "Display tabs in Window and Tabs panel as a opener-tree structure.";

const char kSettings20InternalName[] = "vivaldi-settings20";
const char kSettings20Name[] = "Settings 2.0";
const char kSettings20Description[] = "New layout and features for Settings";

const char kThemeUnifiedInternalName[] = "vivaldi-theme-unified";
const char kThemeUnifiedName[] = "Enable Theme Unified Color";
const char kThemeUnifiedDescription[] =
    "Allow the Unified color position for Themes.";

#if BUILDFLAG(IS_IOS)
// iOS specific feature flags should be delcared within this block.

const char kBankIDDigIDLatencyWorkaroundInternalName[] =
    "vivaldi-bankid-digid-latency-workaround";
const char kBankIDDigIDLatencyWorkaroundName[] =
    "BankID/DigID latency workaround";
const char kBankIDDigIDLatencyWorkaroundDescription[] =
    "Delays automatic POST form submission on affected BankID/DigID flows to "
    "avoid a WKWebView navigation timing issue.";

const char kVivaldiIOSShowRefactoredStartPageInternalName[] =
    "vivaldi-ios-refactored-startpage";
const char kVivaldiIOSShowRefactoredStartPageName[] =
    "Show Refactored StartPage for Vivaldi iOS";
const char kVivaldiIOSShowRefactoredStartPageDescription[] =
    "When enabled show refactored StartPage instead of current production "
    "version.";

#endif  // BUILDFLAG(IS_IOS)

#if defined(OEM_AUTOMOTIVE_BUILD)
const char kCinemaModeInternalName[] = "vivaldi-cinema-mode";
const char kCinemaModeName[] = "Vivaldi experimental fullscreen/immersive mode";
const char kCinemaModeDescription[] =
    "This enables Vivaldi's experimental fullscreen/immersive mode for "
    "streaming-sites.";
#endif  // defined(OEM_AUTOMOTIVE_BUILD)

// Please insert your name/description above in alphabetical order.

}  // namespace vivaldi_flag
