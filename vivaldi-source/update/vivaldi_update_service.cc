// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vivaldi_update_service.h"

#include <string>
#include <utility>

#include "base/task/thread_pool.h"
#include "update/vivaldi_update_backend.h"
#include "update/vivaldi_update_model_observer.h"

using update::VivaldiUpdateBackend;

namespace update {
class VivaldiUpdateService::VivaldiUpdateBackendDelegate
    : public VivaldiUpdateBackend::VivaldiUpdateDelegate {
 public:
  VivaldiUpdateBackendDelegate(
      const base::WeakPtr<VivaldiUpdateService>& update_service,
      const scoped_refptr<base::SequencedTaskRunner>& service_task_runner)
      : update_service_(update_service),
        service_task_runner_(service_task_runner) {}

  void NotifyUpdateProgress(const AutoUpdateStatus& status,
                            const std::string& reason,
                            const int progress) {
    service_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&VivaldiUpdateService::OnUpdgradeProgress,
                                  update_service_, status, reason, progress));
  }

 private:
  const base::WeakPtr<VivaldiUpdateService> update_service_;
  const scoped_refptr<base::SequencedTaskRunner> service_task_runner_;
};

VivaldiUpdateService::VivaldiUpdateService() : weak_ptr_factory_(this) {}

VivaldiUpdateService::~VivaldiUpdateService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Shutdown the backend. This does nothing if Cleanup was already invoked.
  Cleanup();
}

void VivaldiUpdateService::Shutdown() {
  Cleanup();
}

bool VivaldiUpdateService::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!backend_task_runner_);
  backend_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  // Create the update backend.
  scoped_refptr<VivaldiUpdateBackend> backend(new VivaldiUpdateBackend(
      new VivaldiUpdateBackendDelegate(
                            weak_ptr_factory_.GetWeakPtr(),
                            base::SingleThreadTaskRunner::GetCurrentDefault()),
                        backend_task_runner_));
  update_backend_.swap(backend);
  ScheduleTask(base::BindOnce(&VivaldiUpdateBackend::Init, update_backend_));

  return true;
}

base::CancelableTaskTracker::TaskId VivaldiUpdateService::StartUpdate(
    bool should_install_update,
    base::OnceCallback<void()> callback,
    base::CancelableTaskTracker* tracker) {
  DCHECK(backend_task_runner_) << "Update service being called after cleanup";
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return tracker->PostTaskAndReply(
      backend_task_runner_.get(), FROM_HERE,
      base::BindOnce(&VivaldiUpdateBackend::StartUpdate, update_backend_,
                     should_install_update),
      base::BindOnce(std::move(callback)));
}

void VivaldiUpdateService::ScheduleTask(base::OnceClosure task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(backend_task_runner_);

  backend_task_runner_->PostTask(FROM_HERE, std::move(task));
}

void VivaldiUpdateService::AddObserver(VivaldiUpdateModelObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void VivaldiUpdateService::RemoveObserver(
    VivaldiUpdateModelObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void VivaldiUpdateService::Cleanup() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!backend_task_runner_) {
    // We've already cleaned up.
    return;
  }

  weak_ptr_factory_.InvalidateWeakPtrs();

  // Unload the backend.
  if (update_backend_) {
    ScheduleTask(base::BindOnce(&VivaldiUpdateBackend::Closing,
                                std::move(update_backend_)));
  }

  backend_task_runner_ = nullptr;
}

void VivaldiUpdateService::OnUpdgradeProgress(const AutoUpdateStatus& status,
                                       const std::string& reason,
                                       const int progress) {
  for (VivaldiUpdateModelObserver& observer : observers_) {
    observer.OnUpdateProgress(this, status, reason, progress);
  }
}

}  // namespace update
