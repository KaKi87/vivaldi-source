// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_NOTES_CONSTANTS_H_
#define SYNC_NOTES_CONSTANTS_H_

#include <stddef.h>

namespace sync_notes {

// The maximum number of notes that are allowed to be synced. If the number of
// local notes exceeds this limit, sync will enter a "notes limit exceeded"
// error state.
// LINT.IfChange(SyncNotesLimit)
inline constexpr size_t kSyncNotesLimit = 100000;
// LINT.ThenChange(//components/sync/android/java/src/org/chromium/components/sync/SyncService.java:SyncNotesLimit)

}  // namespace sync_notes

#endif  // SYNC_NOTES_CONSTANTS_H_
