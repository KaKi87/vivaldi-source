// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UPDATE_UPDATE_SERVICE_H_
#define UPDATE_UPDATE_SERVICE_H_

#include "base/observer_list.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/api/auto_update/auto_update_status.h"
#include "vivaldi_update_model_observer.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace update {

class VivaldiUpdateBackend;

class VivaldiUpdateService : public KeyedService {
 public:
  VivaldiUpdateService();
  ~VivaldiUpdateService() override;
  VivaldiUpdateService(const VivaldiUpdateService&) = delete;
  VivaldiUpdateService& operator=(const VivaldiUpdateService&) = delete;

  bool Init();

  // Called from shutdown service before shutting down the browser
  void Shutdown() override;

  void AddObserver(VivaldiUpdateModelObserver* observer);
  void RemoveObserver(VivaldiUpdateModelObserver* observer);

  // Call to schedule a given task for running on the update thread with the
  // specified priority. The task will have ownership taken.
  void ScheduleTask(base::OnceClosure task);

  base::CancelableTaskTracker::TaskId StartUpdate(
      bool should_install_update,
      base::OnceCallback<void()> callback,
      base::CancelableTaskTracker* tracker);

 private:
  class VivaldiUpdateBackendDelegate;
  friend class base::RefCountedThreadSafe<VivaldiUpdateService>;
  friend class VivaldiUpdateBackendDelegate;
  friend class VivaldiUpdateBackend;

  void Cleanup();

  void OnUpdgradeProgress(const AutoUpdateStatus& msg,
                          const std::string& reason,
                          const int progress);

  SEQUENCE_CHECKER(sequence_checker_);

  // The observers.
  base::ObserverList<VivaldiUpdateModelObserver> observers_;

  scoped_refptr<base::SequencedTaskRunner> backend_task_runner_;

  // This pointer will be null once Cleanup() has been called, meaning no
  // more calls should be made to the udpate thread.
  scoped_refptr<VivaldiUpdateBackend> update_backend_;

  // All vended weak pointers are invalidated in Cleanup().
  base::WeakPtrFactory<VivaldiUpdateService> weak_ptr_factory_;
};

}  // namespace update

#endif  // UPDATE_UPDATE_SERVICE_H_
