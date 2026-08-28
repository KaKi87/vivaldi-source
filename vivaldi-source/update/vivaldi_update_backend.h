// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UPDATE_UPDATE_BACKEND_H_
#define UPDATE_UPDATE_BACKEND_H_

#include "base/cancelable_callback.h"
#include "base/observer_list.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/task/single_thread_task_runner.h"
#include "extensions/api/autoupdate/auto_update_status.h"
#include "vivaldi_update_backend_notifier.h"

#if BUILDFLAG(IS_WIN)
#include "update/win/vivaldi_updater_win.h"
#endif

namespace base {
class SingleThreadTaskRunner;
}

namespace update {

class VivaldiUpdateBackend
    : public base::RefCountedThreadSafe<update::VivaldiUpdateBackend>,
      public VivaldiUpdateBackendNotifier {
 public:
  class VivaldiUpdateDelegate {
   public:
    virtual ~VivaldiUpdateDelegate() {}

    virtual void NotifyUpdateProgress(const AutoUpdateStatus& status,
                                      const std::string& reason,
                                      const int progress) = 0;
  };

  explicit VivaldiUpdateBackend(VivaldiUpdateDelegate* delegate);

  VivaldiUpdateBackend();
  VivaldiUpdateBackend(VivaldiUpdateDelegate* delegate,
                       scoped_refptr<base::SequencedTaskRunner> task_runner);

  void Init();
  void Closing();

  void StartUpdate(bool should_install_update);
  void NotifyUpdateProgress(const AutoUpdateStatus& status,
                            const std::string& reason,
                            const int progress) override;

 protected:
  ~VivaldiUpdateBackend() override;
  VivaldiUpdateBackend(const VivaldiUpdateBackend&) = delete;
  VivaldiUpdateBackend& operator=(const VivaldiUpdateBackend&) = delete;

 private:
  friend class base::RefCountedThreadSafe<VivaldiUpdateBackend>;

  std::unique_ptr<VivaldiUpdateDelegate> delegate_;

#if BUILDFLAG(IS_WIN)
  std::unique_ptr<update::UpdaterCheckVivaldi> driver_;
#endif

  // Used to restrict to one running update task.
  int update_counter_ = 0;

  base::WeakPtrFactory<VivaldiUpdateBackendNotifier> weak_ptr_factory_;
};

}  // namespace update

#endif  // UPDATE_UPDATE_BACKEND_H_
