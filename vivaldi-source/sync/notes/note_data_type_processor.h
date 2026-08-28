// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_NOTES_NOTE_DATA_TYPE_PROCESSOR_H_
#define SYNC_NOTES_NOTE_DATA_TYPE_PROCESSOR_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "components/sync/engine/data_type_processor.h"
#include "components/sync/model/data_type_activation_request.h"
#include "components/sync/model/data_type_controller_delegate.h"
#include "components/sync/model/wipe_model_upon_sync_disabled_behavior.h"
#include "sync/notes/synced_note_tracker.h"

namespace file_sync {
class SyncedFileStore;
}

namespace sync_notes {

class NotesModelObserverImpl;
class NoteModelView;

class NoteDataTypeProcessor : public syncer::DataTypeProcessor,
                              public syncer::DataTypeControllerDelegate {
 public:
  NoteDataTypeProcessor(file_sync::SyncedFileStore* synced_file_store,
                        syncer::WipeModelUponSyncDisabledBehavior
                            wipe_model_upon_sync_disabled_behavior);

  NoteDataTypeProcessor(const NoteDataTypeProcessor&) = delete;
  NoteDataTypeProcessor& operator=(const NoteDataTypeProcessor&) = delete;

  ~NoteDataTypeProcessor() override;

  // DataTypeProcessor implementation.
  void ConnectSync(std::unique_ptr<syncer::CommitQueue> worker) override;
  void DisconnectSync() override;
  void GetLocalChanges(size_t max_entries,
                       GetLocalChangesCallback callback) override;
  void OnCommitCompleted(
      const sync_pb::DataTypeState& type_state,
      const syncer::CommitResponseDataList& committed_response_list,
      const syncer::FailedCommitResponseDataList& error_response_list) override;
  void OnUpdateReceived(
      const sync_pb::DataTypeState& type_state,
      syncer::UpdateResponseDataList updates,
      std::optional<sync_pb::GarbageCollectionDirective> gc_directive) override;
  void StorePendingInvalidations(
      std::vector<sync_pb::DataTypeState_Invalidation> invalidations_to_store)
      override;
  // DataTypeControllerDelegate implementation.
  void OnSyncStarting(const syncer::DataTypeActivationRequest& request,
                      StartCallback start_callback) override;
  void OnSyncStopping(syncer::SyncStopMetadataFate metadata_fate) override;
  void GetUnsyncedDataCount(base::OnceCallback<void(size_t)> callback) override;
  void GetAllNodesForDebugging(AllNodesCallback callback) override;
  void GetTypeEntitiesCountForDebugging(
      base::OnceCallback<void(const syncer::TypeEntitiesCount&)> callback)
      const override;
  void RecordMemoryUsageAndCountsHistograms() override;
  void ClearMetadataIfStopped() override;
  void ReportBridgeErrorForTest() override;

  // Encodes all sync metadata into a string, representing a state that can be
  // restored via ModelReadyToSync() below.
  std::string EncodeSyncMetadata() const;

  // It mainly decodes a NoteModelMetadata proto serialized in
  // `metadata_str`, and uses it to fill in the tracker and the data type state
  // objects. `model` must not be null and must outlive this object. It is used
  // to the retrieve the local node ids, and is stored in the processor to be
  // used for further model operations. `schedule_save_closure` is a repeating
  // closure used to schedule a save of the note model together with the
  // metadata.
  void ModelReadyToSync(const std::string& metadata_str,
                        const base::RepeatingClosure& schedule_save_closure,
                        NoteModelView* model);

  bool IsTrackingMetadata() const;

  // Returns the estimate of dynamically allocated memory in bytes.
  size_t EstimateMemoryUsage() const;

  const SyncedNoteTracker* GetTrackerForTest() const;
  bool IsConnectedForTest() const;

  // Reset max notes till which sync is enabled.
  void SetLocalNotesLimitForTesting(size_t limit);

  base::WeakPtr<syncer::DataTypeControllerDelegate> GetWeakPtr();

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  // Migrates the legacy `last_initial_merge_remote_updates_exceeded_limit` bool
  // to the timestamp representation.
  void MigrateLegacyExceededLimitError(
      sync_pb::NotesModelMetadata* model_metadata);

  // Resets the error for exceeding note limit if enough time has passed.
  void MaybeResetExceededLimitError(
      sync_pb::NotesModelMetadata* model_metadata);

  // Handles the error state from the given `model_metadata` if a previous sync
  // cycle reported an error. Returns true if there was an error to be handled.
  [[nodiscard]] bool HandlePreviousErrorState(
      const sync_pb::NotesModelMetadata& model_metadata);

  // Parses and validates the metadata. Returns the metadata if it is valid and
  // there was no previous error.
  std::optional<sync_pb::NotesModelMetadata> ParseAndValidateMetadata(
      const std::string& metadata_str);

  // Initializes the tracker.
  void InitTracker(sync_pb::NotesModelMetadata model_metadata,
                   const std::string& metadata_str);

  // Handles the case where metadata needs to be cleared when the model is
  // ready. Returns true if there was a pending clear metadata operation.
  [[nodiscard]] bool HandlePendingClearMetadata(
      const std::string& metadata_str);

  // If preconditions are met, inform sync that we are ready to connect.
  void ConnectIfReady();

  // Returns true if the given `count` of notes exceeds the sync limit. An
  // `offset` can be provided for cases where the exact count is not known.
  bool DoesCountExceedLocalNotesSyncLimit(size_t count,
                                          size_t offset = 0) const;

  // Returns true if the note count exceeded the limit and an error was
  // reported. Also disconnects sync and resets the `start_callback_`.
  bool MaybeReportLocalNotesCountLimitExceededError(
      syncer::ModelError::Type error_type);

  // Nudges worker if there are any local entities to be committed. Should only
  // be called after initial sync is done and processor is tracking sync
  // entities.
  void NudgeForCommitIfNeeded();

  // Disconnects from sync and reports an error to the controller.
  void DisconnectAndReportError(const syncer::ModelError& error);

  // Performs the required clean up when note model is being deleted.
  void OnNotesModelBeingDeleted();

  // Process specifically calls to OnUpdateReceived() that correspond to the
  // initial merge of notes (e.g. was just enabled).
  void OnInitialUpdateReceived(const sync_pb::DataTypeState& type_state,
                               syncer::UpdateResponseDataList updates);

  // Handles any incremental updates received from the server.
  void OnIncrementalUpdateReceived(const sync_pb::DataTypeState& type_state,
                                   syncer::UpdateResponseDataList updates);

  // Handles a full update (i.e. an update with a "clear all" GC directive) as
  // an incremental update.
  void ApplyFullUpdateAsIncrementalUpdate(
      const sync_pb::DataTypeState& type_state,
      syncer::UpdateResponseDataList updates);

  void OverrideAllServerMetadataToForceApplyUpdates(
      const syncer::UpdateResponseDataList& updates);

  // Returns true if the number of remote updates exceeds the limit.
  bool ExceedsRemoteUpdatesLimit(size_t count) const;

  // Instantiates the required objects to track metadata and starts observing
  // changes from the note model. Note that this does not include tracking
  // of metadata fields managed by the processor but only those tracked by the
  // note tracker.
  void StartTrackingMetadata();

  // Resets note tracker in addition to stopping metadata tracking. Note
  // that unlike StopTrackingMetadata(), this does not disconnect sync and
  // instead the caller must meet this precondition.
  void StopTrackingMetadataAndResetTracker();

  // Honors `wipe_model_upon_sync_disabled_behavior_`, i.e. deletes all
  // notes in the model depending on the selected behavior.
  void TriggerWipeModelUponSyncDisabledBehavior();

  // Creates a DictionaryValue for local and remote debugging information about
  // `node` and appends it to `all_nodes`. It does the same for child nodes
  // recursively. `index` is the index of `node` within its parent. `index`
  // could computed from `node`, however it's much cheaper to pass from outside
  // since we iterate over child nodes already in the calling sites.
  void AppendNodeAndChildrenForDebugging(const vivaldi::NoteNode* node,
                                         int index,
                                         base::ListValue* all_nodes) const;

  const raw_ptr<file_sync::SyncedFileStore> synced_file_store_;

  // Controls whether notes should be wiped when sync is stopped. Not actually
  // used in Vivaldi
  const syncer::WipeModelUponSyncDisabledBehavior
      wipe_model_upon_sync_disabled_behavior_;

  // Stores the start callback in between OnSyncStarting() and
  // ModelReadyToSync().
  StartCallback start_callback_;

  // The request context passed in as part of OnSyncStarting().
  syncer::DataTypeActivationRequest activation_request_;

  // The note model we are processing local changes from and forwarding
  // remote changes to. It is set during ModelReadyToSync(), which is called
  // during startup, as part of the note-loading process.
  raw_ptr<NoteModelView> notes_model_ = nullptr;

  // The callback used to schedule the persistence of note model as well as
  // the metadata to a file during which latest metadata should also be pulled
  // via EncodeSyncMetadata. Processor should invoke it upon changes in the
  // metadata that don't imply changes in the model itself. Persisting updates
  // that imply model changes is the model's responsibility.
  base::RepeatingClosure schedule_save_closure_;

  // Reference to the CommitQueue.
  //
  // The interface hides the posting of tasks across threads as well as the
  // CommitQueue's implementation.  Both of these features are
  // useful in tests.
  std::unique_ptr<syncer::CommitQueue> worker_;

  // Keeps the mapping between server ids and notes nodes together with sync
  // metadata. It is constructed and set during ModelReadyToSync(), if the
  // loaded notes JSON contained previous sync metadata, or upon completion
  // of initial sync, which is called during startup, as part of the
  // note-loading process.
  std::unique_ptr<SyncedNoteTracker> note_tracker_;

  // Stores the timestamp when the number of remote updates downloaded during
  // the latest initial merge exceeded the configured limit. This can be
  // populated from a proto for modern clients, or populated with a recent
  // timestamp for legacy clients (who only stored a boolean). If this is set,
  // `note_tracker_` is not initialized and an error is reported instead.
  std::optional<base::Time>
      initial_merge_remote_updates_exceeded_limit_timestamp_;

  // UUID string that identifies the sync client and is received from the sync
  // engine.
  std::string cache_uuid_;

  std::unique_ptr<NotesModelObserverImpl> notes_model_observer_;

  // This member variable exists only to allow tests to override the limit.
  std::optional<size_t> sync_local_notes_limit_for_tests_;

  // Marks whether metadata should be cleared upon ModelReadyToSync(). True if
  // ClearMetadataIfStopped() is called before ModelReadyToSync().
  bool pending_clear_metadata_ = false;

  // WeakPtrFactory for this processor for DataTypeController.
  base::WeakPtrFactory<NoteDataTypeProcessor> weak_ptr_factory_for_controller_{
      this};

  // WeakPtrFactory for this processor which will be sent to sync thread.
  base::WeakPtrFactory<NoteDataTypeProcessor> weak_ptr_factory_for_worker_{
      this};
};

}  // namespace sync_notes

#endif  // SYNC_NOTES_NOTE_DATA_TYPE_PROCESSOR_H_
