// Copyright (c) 2015-2022 Vivaldi Technologies

#include "prefs/vivaldi_pref_names.h"

namespace vivaldiprefs {

// Profile prefs go here.

const char kAutoUpdateEnabled[] = "vivaldi.autoupdate_enabled";
const char kVivaldiLastTopSitesVacuumDate[] =
    "vivaldi.last_topsites_vacuum_date";
const char kVivaldiSearchEnginesKagiToken[] =
    "vivaldi.search_engines.kagi_token";

// Deprecated profile prefs go here.
// DEPRECATED 12/2025
const char kVivaldiPIPPlacement[] = "vivaldi.pip_placement";

// DEPRECATED 06/2025
const char kVivaldiExperiments[] = "vivaldi.experiments";

// DEPRECATED 08/2025
const char kSyncedDefaultPrivateSearchProviderGUID[] =
    "default_search_provider.synced_guid_private";
const char kSyncedDefaultSearchFieldProviderGUID[] =
    "default_search_provider.synced_guid_search_field";
const char kSyncedDefaultPrivateSearchFieldProviderGUID[] =
    "default_search_provider.synced_guid_search_field_private";
const char kSyncedDefaultSpeedDialsSearchProviderGUID[] =
    "default_search_provider.synced_guid_speeddials";
const char kSyncedDefaultSpeedDialsPrivateSearchProviderGUID[] =
    "default_search_provider.synced_guid_speeddials_private";
const char kSyncedDefaultImageSearchProviderGUID[] =
    "default_search_provider.synced_guid_image";

// Local state prefs go here
const char kVivaldiAutoUpdateStandalone[] = "vivaldi.autoupdate.standalone";
const char kVivaldiUniqueUserId[] = "vivaldi.unique_user_id";
const char kVivaldiStatsNextDailyPing[] = "vivaldi.stats.next_daily_ping";
const char kVivaldiStatsNextWeeklyPing[] = "vivaldi.stats.next_weekly_ping";
const char kVivaldiStatsNextMonthlyPing[] = "vivaldi.stats.next_monthly_ping";
const char kVivaldiStatsNextTrimestrialPing[] =
    "vivaldi.stats.next_trimestrial_ping";
const char kVivaldiStatsNextSemestrialPing[] =
    "vivaldi.stats.next_semestrial_ping";
const char kVivaldiStatsNextYearlyPing[] = "vivaldi.stats.next_yearly_ping";
const char kVivaldiStatsExtraPing[] = "vivaldi.stats.extra_ping";
const char kVivaldiStatsExtraPingTime[] = "vivaldi.stats.extra_ping_time";
const char kVivaldiStatsPingsSinceLastMonth[] =
    "vivaldi.stats.pings_since_last_month";
const char kVivaldiProfileImagePath[] = "vivaldi.profile_image_path";
const char kVivaldiTranslateLanguageList[] = "vivaldi.translate.language_list";
const char kVivaldiTranslateLanguageListLastUpdate[] =
    "vivaldi.translate.language_list_last_update";
const char kVivaldiAccountServerUrlIdentity[] =
    "vivaldi.vivaldi.account.server_url.identity";
const char kVivaldiSyncServerUrl[] = "vivaldi.sync.server_url";
const char kVivaldiSyncNotificationsServerUrl[] =
    "vivaldi.sync.notifications.server_url";
// sync error
const char kVivaldiLastSyncErrorDialogShownDate[] =
    "vivaldi.sync.error_dialog.prompt_date";
const char kVivaldiShouldAskSyncErrorAgain[] =
    "vivaldi.sync.error_dialog.ask_again";

const char kVivaldiClientHintsBrand[] = "vivaldi.ClientHintsBrand";
const char kVivaldiClientHintsBrandAppendVivaldi[] =
    "vivaldi.ClientHintsBrandAppendVivaldi";
const char kVivaldiClientHintsBrandCustomBrand[] =
    "vivaldi.ClientHintsCustomBrand";
const char kVivaldiClientHintsBrandCustomBrandVersion[] =
    "vivaldi.ClientHintsCustomBrandVersion";

const char kVivaldiCrashReportingConsentGranted[] =
    "vivaldi.CrashReportingConsentGranted";
const char kVivaldiCrashReportLastUuidSeen[] =
    "vivaldi.CrashReportLastUuidSeen";
const char kVivaldiCrashReportingConsentDialogLastSeenTime[] =
    "vivaldi.CrashReportingConsentDialogLastSeenTime";

const char kVivaldiPreferredColorScheme[] = "vivaldi.PreferredColorScheme";
#if BUILDFLAG(IS_ANDROID)
const char kBackgroundMediaPlaybackAllowed[] =
    "vivaldi.background.media_playback.allowed";
const char kPWADisabled[] = "vivaldi.site.PWADisabled.disabled";
const char kAddressBarDeleteDirectMatch[] =
    "vivaldi.address.field.direct.match.deletion.enabled";
#endif
}  // namespace vivaldiprefs
