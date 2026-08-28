// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_NOTES_SYNCED_NOTE_TRACKER_ENTITY_H_
#define SYNC_NOTES_SYNCED_NOTE_TRACKER_ENTITY_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/types/pass_key.h"
#include "components/sync/base/client_tag_hash.h"
#include "components/sync/model/processor_entity_metadata.h"
#include "components/sync/protocol/entity_metadata.pb.h"

namespace sync_pb {
class EntitySpecifics;
}  // namespace sync_pb

namespace vivaldi {
class NoteNode;
}  // namespace vivaldi

namespace file_sync {
class SyncedFileStore;
}

namespace syncer {
struct EntityData;
struct UpdateResponseData;
struct CommitResponseData;
class DeletionOrigin;
}  // namespace syncer

namespace sync_notes {

class SyncedNoteTracker;

// This class manages the metadata corresponding to an individual NoteNode
// instance. It is analogous to the more generic syncer::ProcessorEntity, which
// is not reused for notes for historic reasons.
class SyncedNoteTrackerEntity {
 public:
  using PassKey = base::PassKey<SyncedNoteTracker>;

  // |note_node| can be null for tombstones.
  SyncedNoteTrackerEntity(const vivaldi::NoteNode* note_node,
                          syncer::ProcessorEntityMetadata entity_metadata,
                          file_sync::SyncedFileStore* synced_file_store);
  SyncedNoteTrackerEntity(const SyncedNoteTrackerEntity&) = delete;
  SyncedNoteTrackerEntity(SyncedNoteTrackerEntity&&) = delete;
  ~SyncedNoteTrackerEntity();

  SyncedNoteTrackerEntity& operator=(const SyncedNoteTrackerEntity&) = delete;
  SyncedNoteTrackerEntity& operator=(SyncedNoteTrackerEntity&&) = delete;

  // Returns true if this entity is deleted (tombstone).
  bool IsDeleted() const;

  // Returns true if this data is out of sync with the server.
  // A commit may or may not be in progress at this time.
  bool IsUnsynced() const;

  // Returns true if this entity was created locally and not yet committed to
  // the server (including while the commit is in flight, until a response is
  // received from the server).
  bool IsUnsyncedLocalCreation() const;

  // Returns true if the specified `update_version` is already known, i.e. is
  // smaller or equal to the last known server version.
  bool IsVersionAlreadyKnown(int64_t update_version) const;

  // Check whether |data| matches the stored specifics hash. It also compares
  // parent information (which is included in specifics).
  bool MatchesData(const syncer::EntityData& data) const;

  // Check whether |data| matches the stored base specifics hash.
  bool MatchesBaseData(const syncer::EntityData& data) const;

  // Check whether |specifics| matches the stored specifics_hash.
  bool MatchesSpecificsHash(const sync_pb::EntitySpecifics& specifics) const;

  // Returns null for tombstones.
  const vivaldi::NoteNode* note_node() const { return note_node_; }

  const sync_pb::EntityMetadata& metadata() const { return metadata_.proto(); }

  bool commit_may_have_started() const { return commit_may_have_started_; }
  void MarkCommitMayHaveStarted() { commit_may_have_started_ = true; }

  syncer::ClientTagHash GetClientTagHash() const;

  void RecordLocalUpdate(const sync_pb::EntitySpecifics& specifics,
                         base::Time modification_time);

  void RecordAcceptedRemoteUpdate(const syncer::UpdateResponseData& update);
  void RecordForcedRemoteUpdate(const syncer::UpdateResponseData& update);
  void RecordIgnoredRemoteUpdate(const syncer::UpdateResponseData& update);
  void OverrideServerMetadata(const std::string& server_id,
                              int64_t server_version);

  void RecordCommitResponse(const syncer::CommitResponseData& ack);

  void IncrementSequenceNumber();

  // Returns the estimate of dynamically allocated memory in bytes.
  size_t EstimateMemoryUsage() const;

  // Semi-private functions exclusively available to SyncedNoteTracker:
  // Used in local deletions to mark an entity as a tombstone.
  void clear_note_node(PassKey) { note_node_ = nullptr; }

  // Used when replacing a node in order to update its otherwise immutable
  // UUID.
  void set_note_node(PassKey, const vivaldi::NoteNode* note_node) {
    note_node_ = note_node;
  }

  void RecordLocalDeletion(PassKey, const syncer::DeletionOrigin& origin);

  // Re-associates a placeholder tombstone with a real note node (e.g. undo
  // deletion).
  void UndeleteTombstoneForNoteNode(PassKey,
                                    const vivaldi::NoteNode* node,
                                    const sync_pb::EntitySpecifics& specifics,
                                    base::Time modification_time);

 private:
  // Null for tombstones.
  raw_ptr<const vivaldi::NoteNode, AcrossTasksDanglingUntriaged> note_node_;

  // Serializable Sync metadata.
  syncer::ProcessorEntityMetadata metadata_;

  // Whether there could be a commit sent to the server for this entity. It's
  // used to protect against sending tombstones for entities that have never
  // been sent to the server. It's only briefly false between the time was
  // first added to the tracker until the first commit request is sent to the
  // server. The tracker sets it to true in the constructor because this code
  // path is only executed in production when loading from disk.
  bool commit_may_have_started_ = false;

  const raw_ptr<file_sync::SyncedFileStore> synced_file_store_;
};

}  // namespace sync_notes

#endif  // SYNC_NOTES_SYNCED_NOTE_TRACKER_ENTITY_H_
