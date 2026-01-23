// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "installer/mini_installer/util/google_update_settings.h"

#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "base/task/lazy_thread_pool_task_runner.h"

namespace {

base::LazyThreadPoolSequencedTaskRunner g_collect_stats_consent_task_runner =
    LAZY_THREAD_POOL_SEQUENCED_TASK_RUNNER_INITIALIZER(
        base::TaskTraits(base::MayBlock(),
                         base::TaskPriority::USER_VISIBLE,
                         base::TaskShutdownBehavior::BLOCK_SHUTDOWN));

std::string& GetPosixClientId() {
  static base::NoDestructor<std::string> posix_client_id;
  return *posix_client_id;
}

base::Lock& GetPosixClientIdLock() {
  static base::NoDestructor<base::Lock> posix_client_id_lock;
  return *posix_client_id_lock;
}

}  // namespace

// static
base::SequencedTaskRunner*
GoogleUpdateSettings::CollectStatsConsentTaskRunner() {
  // TODO(fdoray): Use LazyThreadPoolSequencedTaskRunner::GetRaw() here instead
  // of .Get().get() when it's added to the API, http://crbug.com/730170.
  return g_collect_stats_consent_task_runner.Get().get();
}

// static
bool GoogleUpdateSettings::GetCollectStatsConsent() {
  return false;
}

// static
bool GoogleUpdateSettings::SetCollectStatsConsent(bool consented) {
  return false;
}

// static
// TODO(gab): Implement storing/loading for all ClientInfo fields on POSIX.
std::unique_ptr<metrics::ClientInfo>
GoogleUpdateSettings::LoadMetricsClientInfo() {
  auto client_info = std::make_unique<metrics::ClientInfo>();

  base::AutoLock lock(GetPosixClientIdLock());
  if (GetPosixClientId().empty()) {
    return nullptr;
  }
  client_info->client_id = GetPosixClientId();

  return client_info;
}

// static
// TODO(gab): Implement storing/loading for all ClientInfo fields on POSIX.
void GoogleUpdateSettings::StoreMetricsClientInfo(
    const metrics::ClientInfo& client_info) {}

// GetLastRunTime and SetLastRunTime are not implemented for posix. Their
// current return values signal failure which the caller is designed to
// handle.

// static
int GoogleUpdateSettings::GetLastRunTime() {
  return -1;
}

// static
bool GoogleUpdateSettings::SetLastRunTime() {
  return false;
}
