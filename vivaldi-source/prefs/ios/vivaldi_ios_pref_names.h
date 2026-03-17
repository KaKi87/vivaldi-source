// Copyright (c) 2026 Vivaldi Technologies. All Rights Reserved.

#ifndef PREFS_IOS_VIVALDI_IOS_PREF_NAMES_H_
#define PREFS_IOS_VIVALDI_IOS_PREF_NAMES_H_

#include "chromium/build/build_config.h"

namespace vivaldiprefs {

#if BUILDFLAG(IS_IOS)
// Dict preference which tracks the current elements and order of the overflow
// menu's vivaldi actions.
inline constexpr char kOverflowMenuVivaldiActionsOrder[] =
    "overflow_menu.vivaldi_actions_order";

// Notes
inline constexpr char kVivaldiNoteFolderDefault[] =
    "vivaldi.note.default_folder";
inline constexpr char kVivaldiNoteCachedTopMostRow[] =
    "vivaldi.note.cached_top_most_row";
inline constexpr char kVivaldiNoteCachedFolderId[] =
    "vivaldi.note.cached_folder_id";
inline constexpr char kVivaldiNotesSortingMode[] = "vivaldi.notes.sorting_mode";
inline constexpr char kVivaldiNotesSortingOrder[] =
    "vivaldi.notes.sorting_order";
inline constexpr char kVivaldiNotesShowMarkdownEditor[] =
    "vivaldi.notes.show_markdown_editor";

// Setting for folder visiblity on bookmark folder page.
inline constexpr char kVivaldiBookmarkFoldersViewMode[] =
    "vivaldi.bookmark_folders.view_mode";

inline constexpr char kVivaldiBookmarksSortingMode[] =
    "vivaldi.bookmarks.sorting_mode";
inline constexpr char kVivaldiBookmarksSortingOrder[] =
    "vivaldi.bookmarks.sorting_order";

// Search Engine
inline constexpr char kVivaldiEnableSearchEngineNickname[] =
    "vivaldi.search_engine.enable_nickname";

// Address bar
inline constexpr char kVivaldiShowFullAddressEnabled[] =
    "vivaldi.addressbar.show_full_address";
inline constexpr char kVivaldiShowXForSuggestionEnabled[] =
    "vivaldi.addressbar.show_x_suggestion";
// Enable swipe gesture to open tab switcher from address bar.
inline constexpr char kVivaldiAddressBarSwipeGestureEnabled[] =
    "vivaldi.addressbar.swipe_gesture.enabled";
// DEPRECATED 02/2026. Legacy key used as migration source only.
inline constexpr char kVivaldiAddressBarSwipeGestureEnabledLegacy[] =
    "vivaldi.addressbar.swipe_gesture_enabled";

// Tabs
// Desktop style tabs enabled status.
inline constexpr char kVivaldiDesktopTabsEnabled[] =
    "vivaldi.desktop_tabs.mode";
// Reverse search suggestion results order state for bottom address bar.
inline constexpr char kVivaldiReverseSearchResultsEnabled[] =
    "vivaldi.tabs.reverse_search_results";
// Tab stack use status.
inline constexpr char kVivaldiTabStackEnabled[] =
    "vivaldi.desktop_tabs.tab_stack";
// Show X button in background tabs.
inline constexpr char kVivaldiShowXButtonBackgroundTabsEnabled[] =
    "vivaldi.tabs.show_x_button_background_tabs";
// Open new tab when last open tab is closed.
inline constexpr char kVivaldiOpenNTPOnClosingLastTabEnabled[] =
    "vivaldi.tabs.open_ntp_on_closing_last_tab";
// Focus omnibox on new tab page.
inline constexpr char kVivaldiFocusOmniboxOnNTPEnabled[] =
    "vivaldi.tabs.focus_omnibox_on_ntp";
// Swipe to close tabs in tab switcher.
inline constexpr char kVivaldiSwipeToCloseTabEnabled[] =
    "vivaldi.tabs.swipe_to_close_tab";
// New Tab URL.
inline constexpr char kVivaldiNewTabURL[] = "vivaldi.tabs.newtab.url";
// New Tab Setting.
inline constexpr char kVivaldiNewTabSetting[] = "vivaldi.tabs.newtab.setting";

// General Settings
// Homepage URL.
inline constexpr char kVivaldiHomepageURL[] = "vivaldi.general.homepage.url";
// Show Homepage Button.
inline constexpr char kVivaldiHomepageEnabled[] =
    "vivaldi.general.homepage.enabled";
// Allow audio to play in background tab.
inline constexpr char kVivaldiBackgroundAudioEnabled[] =
    "vivaldi.general.backgroundaudio.enabled";
// Enable/Disable Translate Infobar Banner.
inline constexpr char kVivaldiTranslateInfobarBannerDisabled[] =
    "vivaldi.translate.infobar_banner.disabled.ios";
// DEPRECATED 02/2026. Legacy key used as migration source only.
inline constexpr char kVivaldiTranslateInfobarBannerDisabledLegacy[] =
    "vivaldi.translate.infobar_banner.disabled";

// Appearance
// Selected browser theme, e.g. Light, Dark, System.
inline constexpr char kVivaldiAppearanceMode[] =
    "vivaldi.appearance.selected.mode";
// Selected website appearance style, e.g. Light, Dark, Auto.
inline constexpr char kVivaldiWebsiteAppearanceStyle[] =
    "vivaldi.appearance.website_appearance.style";
// Force dark theme on website.
inline constexpr char kVivaldiWebsiteAppearanceForceDarkTheme[] =
    "vivaldi.appearance.website_appearance.force_dark_theme";
// Custom accent color selected either from preloaded colors or manual entry.
inline constexpr char kVivaldiCustomAccentColor[] =
    "vivaldi.appearance.custom.accent_color";
// Dynamic accent color from webpage.
inline constexpr char kVivaldiDynamicAccentColorEnabled[] =
    "vivaldi.appearance.dynamic.accent_color";

// Start page
// Speed dial/Bookmarks sorting mode.
inline constexpr char kVivaldiSpeedDialSortingMode[] =
    "vivaldi.speed_dial.sorting_mode";
// Speed dial/Bookmarks sorting order, e.g. ascending/descending.
inline constexpr char kVivaldiSpeedDialSortingOrder[] =
    "vivaldi.speed_dial.sorting_order";
// Start page layout.
inline constexpr char kVivaldiStartPageLayoutStyle[] =
    "vivaldi.start_page.layout_style";
// Start page speed dial maximum column.
inline constexpr char kVivaldiStartPageSDMaximumColumns[] =
    "vivaldi.start_page.speed_dial.maximum_columns";
// Start page show/hide frequently visited.
inline constexpr char kVivaldiStartPageShowFrequentlyVisited[] =
    "vivaldi.start_page.show_frequently_visited";
// Start page show/hide speed dials.
inline constexpr char kVivaldiStartPageShowSpeedDials[] =
    "vivaldi.start_page.show_speed_dials";
// Start page show/hide customize button.
inline constexpr char kVivaldiStartPageShowCustomizeButton[] =
    "vivaldi.start_page.show_customize_button";
// Start page show/hide Add button.
inline constexpr char kVivaldiStartPageShowAddButton[] =
    "vivaldi.start_page.show_add_button";
// Start page custom background image.
inline constexpr char kVivaldiStartpagePortraitImage[] =
    "vivaldi.appearance.startpage.image";
inline constexpr char kVivaldiStartpageLandscapeImage[] =
    "vivaldi.appearance.startpage_image.landscape";
// Start page Daily Mix background image.
inline constexpr char kVivaldiStartPageDailyMixImage[] =
    "vivaldi.start_page.daily_mix.image";
inline constexpr char kVivaldiStartPageDailyMixLastFetchDate[] =
    "vivaldi.start_page.daily_mix.last_fetch_date";
// Start page Daily Mix metadata JSON.
inline constexpr char kVivaldiStartPageDailyMixMetadata[] =
    "vivaldi.start_page.daily_mix.metadata";
// Preloaded selected wallpaper name.
inline constexpr char kVivaldiStartupWallpaper[] =
    "vivaldi.startup.wallpaper.name";
// Start page reopen with item.
inline constexpr char kVivaldiStartPageOpenWithItem[] =
    "vivaldi.start_page.open_with.item";
// Start page last visited group.
inline constexpr char kVivaldiStartPageLastVisitedGroup[] =
    "vivaldi.start_page.last_visited_group";
// Safari import entry point shown on start page.
inline constexpr char kVivaldiSafariImportEntryPointShown[] =
    "vivaldi.start_page.safari_import_entry_point_shown";

// Panels
// Setting for opening panel instead of dialog for partial translate from
// webpage.
inline constexpr char kVivaldiPreferTranslatePanel[] =
    "vivaldi.translate.prefer_panel";
// Content Settings
// Global page zoom value.
inline constexpr char kVivaldiPageZoomLevel[] =
    "vivaldi.content_setting.pagezoom.level";
// Privacy & Security Settings
inline constexpr char kVivaldiBlockExternalApps[] =
    "vivaldi.privacy.block_external_apps";
// Reader Mode [iOS]
inline constexpr char kVivaldiReaderModeEnabled[] =
    "vivaldi.content_setting.reader_mode.enabled";
inline constexpr char kReaderModeFontSize[] =
    "vivaldi.content_setting.reader_mode.font_size";
inline constexpr char kReaderModeFontFamily[] =
    "vivaldi.content_setting.reader_mode.font_family";
inline constexpr char kReaderModeTheme[] =
    "vivaldi.content_setting.reader_mode.theme";
#endif  // BUILDFLAG(IS_IOS)

}  // namespace vivaldiprefs

#endif  // PREFS_IOS_VIVALDI_IOS_PREF_NAMES_H_
