// Copyright (c) 2015-2017 Vivaldi Technologies AS. All rights reserved

#include "components/browser_sync/common_controller_builder.h"

// Vivaldi
#include "app/vivaldi_apptools.h"
#include "components/sync/model/forwarding_data_type_controller_delegate.h"
#include "components/sync/service/data_type_controller.h"
#include "sync/notes/note_sync_service.h"

namespace browser_sync {

std::unique_ptr<syncer::DataTypeController>
CommonControllerBuilder::CreateNotesDataTypeController() {

  // Notes sync is enabled by default.  Register unless explicitly
  // disabled.
  if (!(vivaldi::IsVivaldiRunning() || vivaldi::ForcedVivaldiRunning()))
    return nullptr;

  // Services can be null in tests.
  if (!note_sync_service_.value())
    return nullptr;

  return std::make_unique<syncer::DataTypeController>(
      syncer::NOTES,
      std::make_unique<syncer::ForwardingDataTypeControllerDelegate>(
          note_sync_service_.value()->GetNoteSyncControllerDelegate().get()),
      /*delegate_for_transport_mode=*/nullptr);
}

// Vivaldi
void CommonControllerBuilder::SetNoteSyncService(
    sync_notes::NoteSyncService* note_sync_service) {
  note_sync_service_.Set(note_sync_service);
}

}  // namespace browser_sync
