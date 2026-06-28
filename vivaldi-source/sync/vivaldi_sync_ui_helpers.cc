// Copyright (c) 2015 Vivaldi Technologies AS. All rights reserved

#include "sync/vivaldi_sync_ui_helpers.h"

#include "components/sync/engine/cycle/sync_cycle_snapshot.h"
#include "components/sync/engine/sync_status.h"
#include "components/sync/engine/syncer_error.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_token_status.h"
#include "components/sync/service/sync_user_settings.h"

namespace vivaldi {
namespace sync_ui_helpers {

EngineData::EngineData() = default;
EngineData::EngineData(const EngineData& other) = default;
EngineData::~EngineData() = default;

CycleData GetCycleData(syncer::SyncService* sync_service) {
  CHECK(sync_service);
  CycleData cycle_data;

  syncer::SyncCycleSnapshot cycle_snapshot =
      sync_service->GetLastCycleSnapshotForDebugging();
  syncer::SyncStatus status;
  sync_service->QueryDetailedSyncStatusForDebugging(&status);

  cycle_data.cycle_start_time = cycle_snapshot.sync_start_time();
  cycle_data.next_retry_time = status.retry_time;

  if (!cycle_snapshot.is_initialized()) {
    cycle_data.download_updates_status = NOT_SYNCED;
    cycle_data.commit_status = NOT_SYNCED;
    return cycle_data;
  }

  switch (cycle_snapshot.model_neutral_state()
              .last_download_updates_result.type()) {
    case syncer::SyncerError::Type::kSuccess:
      cycle_data.download_updates_status = SUCCESS;
      break;
    case syncer::SyncerError::Type::kHttpError:
      if (cycle_snapshot.model_neutral_state()
              .last_download_updates_result.GetHttpErrorOrDie() ==
          net::HTTP_UNAUTHORIZED) {
        cycle_data.download_updates_status = AUTH_ERROR;
      } else {
        cycle_data.download_updates_status = SERVER_ERROR;
      }
      break;
    case syncer::SyncerError::Type::kNetworkError:
      cycle_data.download_updates_status = NETWORK_ERROR;
      break;
    case syncer::SyncerError::Type::kProtocolError:
      switch (cycle_snapshot.model_neutral_state()
                  .last_download_updates_result.GetProtocolErrorOrDie()) {
        case syncer::SyncProtocolErrorType::THROTTLED:
          cycle_data.download_updates_status = THROTTLED;
          break;
        default:
          cycle_data.download_updates_status = OTHER_ERROR;
      }
      break;
    case syncer::SyncerError::Type::kProtocolViolationError:
      cycle_data.download_updates_status = OTHER_ERROR;
      break;
  }

  switch (cycle_snapshot.model_neutral_state().commit_result.type()) {
    case syncer::SyncerError::Type::kSuccess:
      if (cycle_data.download_updates_status != SUCCESS)
        cycle_data.commit_status = NOT_SYNCED;
      else
        cycle_data.commit_status = SUCCESS;
      break;
    case syncer::SyncerError::Type::kHttpError:
      if (cycle_snapshot.model_neutral_state()
              .commit_result.GetHttpErrorOrDie() == net::HTTP_UNAUTHORIZED) {
        cycle_data.commit_status = AUTH_ERROR;
      } else {
        cycle_data.commit_status = SERVER_ERROR;
      }
      break;
    case syncer::SyncerError::Type::kNetworkError:
      cycle_data.commit_status = NETWORK_ERROR;
      break;
    case syncer::SyncerError::Type::kProtocolError:
      switch (cycle_snapshot.model_neutral_state()
                  .commit_result.GetProtocolErrorOrDie()) {
        case syncer::SyncProtocolErrorType::THROTTLED:
          cycle_data.commit_status = THROTTLED;
          break;
        case syncer::SyncProtocolErrorType::CONFLICT:
          cycle_data.commit_status = CONFLICT;
          break;
        default:
          cycle_data.commit_status = OTHER_ERROR;
      }
      break;
    case syncer::SyncerError::Type::kProtocolViolationError:
      cycle_data.commit_status = OTHER_ERROR;
  }

  return cycle_data;
}

EngineData GetEngineData(syncer::SyncService* sync_service) {
  CHECK(sync_service);
  EngineData engine_data;
  if (sync_service->is_clearing_sync_data()) {
    engine_data.engine_state = EngineState::CLEARING_DATA;
  } else if (!sync_service->HasSyncConsent() ||
             sync_service->GetTransportState() ==
                 syncer::SyncService::TransportState::START_DEFERRED) {
    engine_data.engine_state = EngineState::STOPPED;
  } else if (!sync_service->CanSyncFeatureStart()) {
    engine_data.engine_state = EngineState::FAILED;
  } else if (sync_service->IsEngineInitialized()) {
    if (sync_service->GetTransportState() ==
            syncer::SyncService::TransportState::
                PENDING_DESIRED_CONFIGURATION ||
        !sync_service->GetUserSettings()->IsInitialSyncFeatureSetupComplete()) {
      engine_data.engine_state = EngineState::CONFIGURATION_PENDING;
    } else {
      engine_data.engine_state = EngineState::STARTED;
    }
  } else if (sync_service->GetSyncTokenStatusForDebugging().connection_status ==
             syncer::CONNECTION_SERVER_ERROR) {
    engine_data.engine_state = EngineState::STARTING_SERVER_ERROR;
  } else {
    engine_data.engine_state = EngineState::STARTING;
  }

  engine_data.disable_reasons = sync_service->GetDisableReasons();

  syncer::SyncStatus status;
  sync_service->QueryDetailedSyncStatusForDebugging(&status);

  engine_data.protocol_error_type = status.sync_protocol_error.error_type;
  engine_data.protocol_error_description =
      status.sync_protocol_error.error_description;
  engine_data.protocol_error_client_action = status.sync_protocol_error.action;

  engine_data.is_encrypting_everything =
      sync_service->IsEngineInitialized()
          ? sync_service->GetUserSettings()->IsEncryptEverythingEnabled()
          : false;
  engine_data.uses_encryption_password =
      sync_service->GetUserSettings()->IsUsingExplicitPassphrase();
  engine_data.needs_decryption_password =
      sync_service->GetUserSettings()
          ->IsPassphraseRequiredForPreferredDataTypes();
  engine_data.is_setup_in_progress = sync_service->IsSetupInProgress();
  engine_data.is_first_setup_complete =
      sync_service->GetUserSettings()->IsInitialSyncFeatureSetupComplete();

  engine_data.sync_everything =
      sync_service->GetUserSettings()->IsSyncEverythingEnabled();

  engine_data.data_types = sync_service->GetUserSettings()->GetSelectedTypes();

  return engine_data;
}

bool SetEncryptionPassword(syncer::SyncService* sync_service,
                           const std::string& password) {
  CHECK(sync_service);
  if (sync_service->GetUserSettings()->IsPassphraseRequired()) {
    if (password.empty())
      return false;
    return sync_service->GetUserSettings()->SetDecryptionPassphrase(password);
  }

  if (sync_service->GetUserSettings()->IsUsingExplicitPassphrase())
    return false;

  if (!password.empty()) {
    sync_service->GetUserSettings()->SetEncryptionPassphrase(password);
    return true;
  }

  return false;
}
}  // namespace sync_ui_helpers
}  // namespace vivaldi
