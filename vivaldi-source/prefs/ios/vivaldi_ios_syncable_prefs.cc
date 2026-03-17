// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "prefs/ios/vivaldi_ios_syncable_prefs.h"

#include "base/containers/fixed_flat_map.h"
#include "prefs/ios/vivaldi_ios_pref_names.h"

namespace vivaldi {

namespace {

// Keep ranges in sync with prefs/prefs_sync_ids.md.
constexpr int kVivaldiIOSSyncablePrefIdStart = 1400000;

struct IOSSyncablePrefMetadata final
    : public sync_preferences::SyncablePrefMetadata {
  constexpr IOSSyncablePrefMetadata(
      int syncable_pref_id,
      sync_preferences::MergeBehavior merge_behavior)
      : SyncablePrefMetadata(syncable_pref_id,
                             syncer::PREFERENCES,
                             sync_preferences::PrefSensitivity::kNone,
                             merge_behavior) {}
};

constexpr IOSSyncablePrefMetadata MakeIOSSyncablePrefMetadata(
    int syncable_pref_id,
    sync_preferences::MergeBehavior merge_behavior =
        sync_preferences::MergeBehavior::kNone) {
  return IOSSyncablePrefMetadata(syncable_pref_id, merge_behavior);
}

// Not an enum class to ease cast to int.
namespace syncable_prefs_ids {
// These values are persisted in sync metadata. Entries should never be
// renumbered and numeric values should never be reused.
enum {
  // Starts with 1,400,000 (iOS range), see prefs/prefs_sync_ids.md.
  kVivaldiAddressBarSwipeGestureEnabled = kVivaldiIOSSyncablePrefIdStart,
  kVivaldiTranslateInfobarBannerDisabled = 1400001,
  kVivaldiBookmarksSortingMode = 1400002,
  kVivaldiBookmarksSortingOrder = 1400003,
  kVivaldiBookmarkFoldersViewMode = 1400004,
  kVivaldiNotesSortingMode = 1400005,
  kVivaldiNotesSortingOrder = 1400006,
  kVivaldiNotesShowMarkdownEditor = 1400007,
  kVivaldiSpeedDialSortingMode = 1400008,
  kVivaldiSpeedDialSortingOrder = 1400009,
  kVivaldiStartPageLayoutStyle = 1400010,
  kVivaldiStartPageShowFrequentlyVisited = 1400011,
  kVivaldiStartPageShowSpeedDials = 1400012,
  kVivaldiStartPageShowCustomizeButton = 1400013,
  kVivaldiStartupWallpaper = 1400014,
  kVivaldiAppearanceMode = 1400015,
  kVivaldiWebsiteAppearanceStyle = 1400016,
  kVivaldiWebsiteAppearanceForceDarkTheme = 1400017,
  kVivaldiCustomAccentColor = 1400018,
  kVivaldiDynamicAccentColorEnabled = 1400019,
  kVivaldiHomepageEnabled = 1400020,
  kVivaldiHomepageURL = 1400021,
  kVivaldiBackgroundAudioEnabled = 1400022,
  kVivaldiDesktopTabsEnabled = 1400023,
  kVivaldiTabStackEnabled = 1400024,
  kVivaldiReverseSearchResultsEnabled = 1400025,
  kVivaldiShowXButtonBackgroundTabsEnabled = 1400026,
  kVivaldiOpenNTPOnClosingLastTabEnabled = 1400027,
  kVivaldiFocusOmniboxOnNTPEnabled = 1400028,
  kVivaldiSwipeToCloseTabEnabled = 1400029,
  kVivaldiNewTabURL = 1400030,
  kVivaldiNewTabSetting = 1400031,
  kReaderModeFontSize = 1400032,
  kReaderModeFontFamily = 1400033,
  kReaderModeTheme = 1400034,
  kVivaldiReaderModeEnabled = 1400035,
  kVivaldiPageZoomLevel = 1400036,
  kVivaldiBlockExternalApps = 1400037,
};
}  // namespace syncable_prefs_ids

// Keep IDs stable forever once shipped.
//
// Below is the list of iOS-only syncable prefs which are expected to be
// synced across iOS devices but not registered as SYNCABLE_PREF in pref
// registry.
// Having these prefs here allows them to be in synced prefs allow list. But
// as long as they are not registered as SYNCABLE_PREF,
// they won't be synced. Therefore, syncable prefs must be
// added here and registered as SYNCABLE_PREF in pref registry at the same
// time to be synced across iOS devices.
// IMPORTANT! When adding a new syncable pref, make sure to assign it a unique
// id and never reuse or change the id once assigned and shipped. Also never
// register any pref as SYNCABLE_PREF in pref registry without adding it here,
// which will cause a crash when syncing to older versions because the pref
// won't be in the allow list for older builds.
constexpr auto kIOSSyncablePrefsAllowlist = base::MakeFixedFlatMap<
    std::string_view,
    sync_preferences::SyncablePrefMetadata>({
    {vivaldiprefs::kVivaldiAddressBarSwipeGestureEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiAddressBarSwipeGestureEnabled)},
    {vivaldiprefs::kVivaldiTranslateInfobarBannerDisabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiTranslateInfobarBannerDisabled)},
    {vivaldiprefs::kVivaldiBookmarksSortingMode,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiBookmarksSortingMode)},
    {vivaldiprefs::kVivaldiBookmarksSortingOrder,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiBookmarksSortingOrder)},
    {vivaldiprefs::kVivaldiBookmarkFoldersViewMode,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiBookmarkFoldersViewMode)},
    {vivaldiprefs::kVivaldiNotesSortingMode,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiNotesSortingMode)},
    {vivaldiprefs::kVivaldiNotesSortingOrder,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiNotesSortingOrder)},
    {vivaldiprefs::kVivaldiNotesShowMarkdownEditor,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiNotesShowMarkdownEditor)},
    {vivaldiprefs::kVivaldiSpeedDialSortingMode,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiSpeedDialSortingMode)},
    {vivaldiprefs::kVivaldiSpeedDialSortingOrder,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiSpeedDialSortingOrder)},
    {vivaldiprefs::kVivaldiStartPageLayoutStyle,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiStartPageLayoutStyle)},
    {vivaldiprefs::kVivaldiStartPageShowFrequentlyVisited,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiStartPageShowFrequentlyVisited)},
    {vivaldiprefs::kVivaldiStartPageShowSpeedDials,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiStartPageShowSpeedDials)},
    {vivaldiprefs::kVivaldiStartPageShowCustomizeButton,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiStartPageShowCustomizeButton)},
    {vivaldiprefs::kVivaldiStartupWallpaper,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiStartupWallpaper)},
    {vivaldiprefs::kVivaldiAppearanceMode,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiAppearanceMode)},
    {vivaldiprefs::kVivaldiWebsiteAppearanceStyle,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiWebsiteAppearanceStyle)},
    {vivaldiprefs::kVivaldiWebsiteAppearanceForceDarkTheme,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiWebsiteAppearanceForceDarkTheme)},
    {vivaldiprefs::kVivaldiCustomAccentColor,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiCustomAccentColor)},
    {vivaldiprefs::kVivaldiDynamicAccentColorEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiDynamicAccentColorEnabled)},
    {vivaldiprefs::kVivaldiHomepageEnabled,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiHomepageEnabled)},
    {vivaldiprefs::kVivaldiHomepageURL,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiHomepageURL)},
    {vivaldiprefs::kVivaldiBackgroundAudioEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiBackgroundAudioEnabled)},
    {vivaldiprefs::kVivaldiDesktopTabsEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiDesktopTabsEnabled)},
    {vivaldiprefs::kVivaldiTabStackEnabled,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiTabStackEnabled)},
    {vivaldiprefs::kVivaldiReverseSearchResultsEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiReverseSearchResultsEnabled)},
    {vivaldiprefs::kVivaldiShowXButtonBackgroundTabsEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiShowXButtonBackgroundTabsEnabled)},
    {vivaldiprefs::kVivaldiOpenNTPOnClosingLastTabEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiOpenNTPOnClosingLastTabEnabled)},
    {vivaldiprefs::kVivaldiFocusOmniboxOnNTPEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiFocusOmniboxOnNTPEnabled)},
    {vivaldiprefs::kVivaldiSwipeToCloseTabEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiSwipeToCloseTabEnabled)},
    {vivaldiprefs::kVivaldiNewTabURL,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiNewTabURL)},
    {vivaldiprefs::kVivaldiNewTabSetting,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiNewTabSetting)},
    {vivaldiprefs::kReaderModeFontSize,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kReaderModeFontSize)},
    {vivaldiprefs::kReaderModeFontFamily,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kReaderModeFontFamily)},
    {vivaldiprefs::kReaderModeTheme,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kReaderModeTheme)},
    {vivaldiprefs::kVivaldiReaderModeEnabled,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiReaderModeEnabled)},
    {vivaldiprefs::kVivaldiPageZoomLevel,
     MakeIOSSyncablePrefMetadata(syncable_prefs_ids::kVivaldiPageZoomLevel)},
    {vivaldiprefs::kVivaldiBlockExternalApps,
     MakeIOSSyncablePrefMetadata(
         syncable_prefs_ids::kVivaldiBlockExternalApps)},
});

}  // namespace

std::optional<sync_preferences::SyncablePrefMetadata>
GetIOSSyncablePrefMetadata(std::string_view pref_name) {
  const auto it = kIOSSyncablePrefsAllowlist.find(pref_name);
  if (it != kIOSSyncablePrefsAllowlist.end()) {
    return it->second;
  }
  return std::nullopt;
}

}  // namespace vivaldi
