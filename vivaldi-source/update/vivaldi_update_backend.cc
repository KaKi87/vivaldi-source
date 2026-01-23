// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vivaldi_update_backend.h"

#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;

namespace update {

// VivaldiUpdateBackend -----------------------------------------------------------
VivaldiUpdateBackend::VivaldiUpdateBackend(
    VivaldiUpdateDelegate* delegate,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : delegate_(delegate), weak_ptr_factory_(this) {}

VivaldiUpdateBackend::VivaldiUpdateBackend(VivaldiUpdateDelegate* delegate)
    : weak_ptr_factory_(this) {}

VivaldiUpdateBackend::~VivaldiUpdateBackend() {}

VivaldiUpdateBackend::VivaldiUpdateBackend() : weak_ptr_factory_(this) {}

void VivaldiUpdateBackend::Init() {
#if BUILDFLAG(IS_WIN)
  if (!driver_.get()) {
    // The driver is owned by itself, and will self-destruct when its work is
    // done.
    driver_.reset(
        new update::UpdaterCheckVivaldi(weak_ptr_factory_.GetWeakPtr()));
  }
#endif
}

void VivaldiUpdateBackend::Closing() {
  delegate_.reset();
}

void VivaldiUpdateBackend::StartUpdate(bool should_install_update) {
  if (should_install_update) {
    if (update_counter_ > 0) {
      LOG(INFO) << "VivaldiUpdateBackend::StartUpdate aborted. Backend busy ; "
                << update_counter_++;

      NotifyUpdateProgress(AutoUpdateStatus::kError, "", 0);
      return;
    }
    update_counter_ = 1;
  }

#if BUILDFLAG(IS_WIN)
  driver_->BeginUpdateCheck(should_install_update);
#endif
}

void VivaldiUpdateBackend::NotifyUpdateProgress(const AutoUpdateStatus& status,
                                         const std::string& reason,
                                         const int progress) {
  // Note; kNoUpdate is used to block multiple running updates.
  if (status == AutoUpdateStatus::kNoUpdate) {
    update_counter_ = 0;
  }

  if (delegate_) {
    delegate_->NotifyUpdateProgress(status, reason, progress);
  }
}

}  // namespace update
