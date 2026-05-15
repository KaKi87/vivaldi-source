//
// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.
//

#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tabs/public/tab_strip_collection.h"

bool TabStripModel::IsMovable(int index) const {
  return contents_data_->IsMovable(index);
}

double TabStripModel::GetActiveWorkspace() const {
  return active_workspace_;
}

void TabStripModel::SetActiveWorkspace(double workspace_id) {
  active_workspace_ = workspace_id;
}

void TabStripModel::AddVivaldiSanitizerGuardRef(int n) {
  vivaldi_sanitizer_refs_ += n;
  CHECK(vivaldi_sanitizer_refs_ >= 0);
}

bool TabStripModel::IsVivaldiSanitizerEnabled() const {
  return vivaldi_sanitizer_refs_ == 0;
}
