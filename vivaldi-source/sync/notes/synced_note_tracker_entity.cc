// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/notes/synced_note_tracker_entity.h"

#include <utility>

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "components/notes/note_node.h"
#include "components/sync/base/deletion_origin.h"
#include "components/sync/engine/commit_and_get_updates_types.h"
#include "components/sync/protocol/entity_metadata.pb.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "components/sync/protocol/unique_position.pb.h"
#include "sync/file_sync/file_store.h"

namespace sync_notes {

SyncedNoteTrackerEntity::SyncedNoteTrackerEntity(
    const vivaldi::NoteNode* note_node,
    syncer::ProcessorEntityMetadata entity_metadata,
    file_sync::SyncedFileStore* synced_file_store)
    : note_node_(note_node),
      metadata_(std::move(entity_metadata)),
      synced_file_store_(synced_file_store) {
  if (note_node) {
    DCHECK(!metadata_.IsDeleted());
  } else {
    DCHECK(metadata_.IsDeleted());
  }
}

SyncedNoteTrackerEntity::~SyncedNoteTrackerEntity() = default;

bool SyncedNoteTrackerEntity::IsDeleted() const {
  return metadata_.IsDeleted();
}

bool SyncedNoteTrackerEntity::IsUnsynced() const {
  return metadata_.IsUnsynced();
}

bool SyncedNoteTrackerEntity::IsUnsyncedLocalCreation() const {
  return metadata_.IsUnsyncedLocalCreation();
}

bool SyncedNoteTrackerEntity::IsVersionAlreadyKnown(
    int64_t update_version) const {
  return metadata_.IsVersionAlreadyKnown(update_version);
}

bool SyncedNoteTrackerEntity::MatchesData(
    const syncer::EntityData& data) const {
  return metadata_.MatchesData(data);
}

bool SyncedNoteTrackerEntity::MatchesBaseData(
    const syncer::EntityData& data) const {
  return metadata_.MatchesBaseData(data);
}

bool SyncedNoteTrackerEntity::MatchesSpecificsHash(
    const sync_pb::EntitySpecifics& specifics) const {
  return metadata_.MatchesSpecificsHash(specifics);
}

syncer::ClientTagHash SyncedNoteTrackerEntity::GetClientTagHash() const {
  return metadata_.GetClientTagHash();
}

size_t SyncedNoteTrackerEntity::EstimateMemoryUsage() const {
  using base::trace_event::EstimateMemoryUsage;
  size_t memory_usage = metadata_.EstimateMemoryUsage();
  memory_usage += sizeof(note_node_);
  return memory_usage;
}

void SyncedNoteTrackerEntity::RecordAcceptedRemoteUpdate(
    const syncer::UpdateResponseData& update) {
  std::optional<sync_pb::UniquePosition> unique_position;
  if (update.entity.specifics.notes().has_unique_position()) {
    unique_position = update.entity.specifics.notes().unique_position();
  }
  if (note_node() && note_node()->is_attachment()) {
    synced_file_store_->SetSyncFileRef(
        metadata().server_id(), syncer::NOTES,
        base::UTF16ToASCII(note_node()->GetContent()));
  }
  metadata_.RecordAcceptedRemoteUpdate(
      update, /*trimmed_specifics=*/sync_pb::EntitySpecifics(),
      std::move(unique_position));
}

void SyncedNoteTrackerEntity::RecordForcedRemoteUpdate(
    const syncer::UpdateResponseData& update) {
  std::optional<sync_pb::UniquePosition> unique_position;
  if (update.entity.specifics.notes().has_unique_position()) {
    unique_position = update.entity.specifics.notes().unique_position();
  }
  metadata_.RecordForcedRemoteUpdate(
      update, /*trimmed_specifics=*/sync_pb::EntitySpecifics(),
      std::move(unique_position));
}

void SyncedNoteTrackerEntity::RecordIgnoredRemoteUpdate(
    const syncer::UpdateResponseData& update) {
  metadata_.RecordIgnoredRemoteUpdate(update);
}

void SyncedNoteTrackerEntity::OverrideServerMetadata(
    const std::string& server_id,
    int64_t server_version) {
  metadata_.OverrideServerMetadata(server_id, server_version);
}

void SyncedNoteTrackerEntity::RecordLocalUpdate(
    const sync_pb::EntitySpecifics& specifics,
    base::Time modification_time) {
  CHECK(!IsDeleted());
  metadata_.UpdateMetadataForLocalUpdate(specifics, modification_time,
                                         specifics.notes().unique_position());
}

void SyncedNoteTrackerEntity::RecordCommitResponse(
    const syncer::CommitResponseData& ack) {
  metadata_.RecordCommitResponse(ack);
}

void SyncedNoteTrackerEntity::RecordLocalDeletion(
    PassKey,
    const syncer::DeletionOrigin& origin) {
  metadata_.RecordLocalDeletion(origin);
}

void SyncedNoteTrackerEntity::IncrementSequenceNumber() {
  metadata_.IncrementSequenceNumber();
}

void SyncedNoteTrackerEntity::UndeleteTombstoneForNoteNode(
    PassKey,
    const vivaldi::NoteNode* node,
    const sync_pb::EntitySpecifics& specifics,
    base::Time modification_time) {
  DCHECK(node);
  DCHECK(IsDeleted());
  note_node_ = node;
  metadata_.UpdateMetadataForLocalUpdate(
      specifics, modification_time, specifics.bookmark().unique_position());
}

}  // namespace sync_notes
