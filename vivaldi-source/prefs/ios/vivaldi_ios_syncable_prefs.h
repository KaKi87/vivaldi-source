// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef PREFS_IOS_VIVALDI_IOS_SYNCABLE_PREFS_H_
#define PREFS_IOS_VIVALDI_IOS_SYNCABLE_PREFS_H_

#include <optional>
#include <string_view>

#include "components/sync_preferences/syncable_prefs_database.h"

namespace vivaldi {

// Returns iOS-only sync metadata for a Vivaldi pref, if present.
std::optional<sync_preferences::SyncablePrefMetadata>
GetIOSSyncablePrefMetadata(std::string_view pref_name);

}  // namespace vivaldi

#endif  // PREFS_IOS_VIVALDI_IOS_SYNCABLE_PREFS_H_
