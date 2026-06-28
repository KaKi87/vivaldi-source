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

const char kHorizontalPinnedTabsInternalName[] =
    "vivaldi-horizontal-pinned-tabs";
const char kHorizontalPinnedTabsName[] =
    "Horizontal pinned tabs in vertical tab bars";
const char kHorizontalPinnedTabsDescription[] =
    "Makes pinned tabs smaller and ordered in a grid when the tab bars are to "
    "the left or right.";

extern const char kInternalPageReaderModeInternalName[] =
    "vivaldi-internal-page-reader-mode";
extern const char kInternalPageReaderModeName[] =
    "Use internal page for reader mode";
extern const char kInternalPageReaderModeDescription[] =
    "Should give better results on most pages but is lacking in translation "
    "features.";

const char kNoteEditorName[] = "vivaldi-note-editor";
const char kNoteEditorOption[] = "Use the new note editor";
const char kNoteEditorDescription[] =
    "Give access to a new note editor with more features.";

const char kRelatedTabsInternalName[] = "vivaldi-related-tabs";
const char kRelatedTabsName[] = "Show related tabs sort option.";
const char kRelatedTabsDescription[] =
    "Display tabs in Window and Tabs panel as a opener-tree structure.";

const char kSettings20InternalName[] = "vivaldi-settings20";
const char kSettings20Name[] = "Settings 2.0";
const char kSettings20Description[] = "New layout and features for Settings";

#if BUILDFLAG(IS_IOS)
// iOS specific feature flags should be delcared within this block.

const char kBankIDDigIDLatencyWorkaroundInternalName[] =
    "vivaldi-bankid-digid-latency-workaround";
const char kBankIDDigIDLatencyWorkaroundName[] =
    "BankID/DigID latency workaround";
const char kBankIDDigIDLatencyWorkaroundDescription[] =
    "Delays automatic POST form submission on affected BankID/DigID flows to "
    "avoid a WKWebView navigation timing issue.";

const char kVivaldiIOSCopySanitizedLinkInternalName[] =
    "vivaldi-ios-copy-sanitized-link";
const char kVivaldiIOSCopySanitizedLinkName[] = "Copy sanitized link";
const char kVivaldiIOSCopySanitizedLinkDescription[] =
    "When enabled it shows Copy sanitized link menu"
    "on address bar and share sheet and which copies link without trackers";

const char kVivaldiIOSShowRefactoredStartPageInternalName[] =
    "vivaldi-ios-refactored-startpage";
const char kVivaldiIOSShowRefactoredStartPageName[] =
    "Show Refactored StartPage for Vivaldi iOS";
const char kVivaldiIOSShowRefactoredStartPageDescription[] =
    "When enabled show refactored StartPage instead of current production "
    "version.";

#endif  // BUILDFLAG(IS_IOS)

const char kVivaldiUseNewUrlSanitizerInternalName[] =
    "vivaldi-use-new-url-sanitizer";
const char kVivaldiUseNewUrlSanitizerName[] =
    "Use new url sanitizer for copy-link operations.";
const char kVivaldiUseNewUrlSanitizerDescription[] =
    "When enabled, a new url sanitizer for clean url copy will be used.";

const char kMailSavingAttachmentsInternalName[] =
    "vivaldi-use-new-save-mail-attachments";
const char kMailSavingAttachmentsName[] =
    "Configure directory for mail attachments.";
const char kMailSavingAttachmentsDescription[] =
    "Allow users to select directory for saving mail attachments.";

#if defined(OEM_AUTOMOTIVE_BUILD)
const char kCinemaModeInternalName[] = "vivaldi-cinema-mode";
const char kCinemaModeName[] = "Vivaldi experimental fullscreen/immersive mode";
const char kCinemaModeDescription[] =
    "This enables Vivaldi's experimental fullscreen/immersive mode for "
    "streaming-sites.";
#endif  // defined(OEM_AUTOMOTIVE_BUILD)

// Please insert your name/description above in alphabetical order.

}  // namespace vivaldi_flag
