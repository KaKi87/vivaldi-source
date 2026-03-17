// Copyright (c) 2015-2022 Vivaldi Technologies. All Rights Reserved.

#ifndef PREFS_VIVALDI_PREF_NAMES_H_
#define PREFS_VIVALDI_PREF_NAMES_H_

#include "chromium/build/build_config.h"

namespace vivaldiprefs {

// Profile prefs go here.
extern const char kAutoUpdateEnabled[];
extern const char kVivaldiAccountPendingRegistration[];
extern const char kVivaldiLastTopSitesVacuumDate[];
extern const char kVivaldiSearchEnginesKagiToken[];

// Deprecated profile prefs go here.

// DEPRECATED 12/2025
extern const char kVivaldiPIPPlacement[];

// DEPRECATED 06/2025
extern const char kVivaldiExperiments[];

// DEPRECATED 08/2025
extern const char kSyncedDefaultPrivateSearchProviderGUID[];
extern const char kSyncedDefaultSearchFieldProviderGUID[];
extern const char kSyncedDefaultPrivateSearchFieldProviderGUID[];
extern const char kSyncedDefaultSpeedDialsSearchProviderGUID[];
extern const char kSyncedDefaultSpeedDialsPrivateSearchProviderGUID[];
extern const char kSyncedDefaultImageSearchProviderGUID[];

// Local state prefs go here.
extern const char kVivaldiAutoUpdateStandalone[];
extern const char kVivaldiUniqueUserId[];
extern const char kVivaldiStatsNextDailyPing[];
extern const char kVivaldiStatsNextWeeklyPing[];
extern const char kVivaldiStatsNextMonthlyPing[];
extern const char kVivaldiStatsNextTrimestrialPing[];
extern const char kVivaldiStatsNextSemestrialPing[];
extern const char kVivaldiStatsNextYearlyPing[];
extern const char kVivaldiStatsExtraPing[];
extern const char kVivaldiStatsExtraPingTime[];
extern const char kVivaldiStatsPingsSinceLastMonth[];
extern const char kVivaldiProfileImagePath[];
extern const char kVivaldiTranslateLanguageList[];
extern const char kVivaldiTranslateLanguageListLastUpdate[];
extern const char kVivaldiAccountServerUrlIdentity[];
extern const char kVivaldiSyncServerUrl[];
extern const char kVivaldiSyncNotificationsServerUrl[];

// sync error
extern const char kVivaldiLastSyncErrorDialogShownDate[];
extern const char kVivaldiShouldAskSyncErrorAgain[];

extern const char kVivaldiClientHintsBrand[];
extern const char kVivaldiClientHintsBrandAppendVivaldi[];
extern const char kVivaldiClientHintsBrandCustomBrand[];
extern const char kVivaldiClientHintsBrandCustomBrandVersion[];

extern const char kVivaldiCrashReportingConsentGranted[];
extern const char kVivaldiCrashReportLastUuidSeen[];
extern const char kVivaldiCrashReportingConsentDialogLastSeenTime[];

extern const char kVivaldiPreferredColorScheme[];
#if BUILDFLAG(IS_ANDROID)
// Background media playback for YouTube.
extern const char kBackgroundMediaPlaybackAllowed[];
extern const char kPWADisabled[];
extern const char kAddressBarDeleteDirectMatch[];
#endif
}  // namespace vivaldiprefs

#endif  // PREFS_VIVALDI_PREF_NAMES_H_
