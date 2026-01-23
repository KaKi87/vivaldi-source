// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#include "ui/base/hit_test.h"
#include "ui/views/vivaldi_window_frame_view.h"
#include "ui/views/widget/widget.h"
#include "ui/vivaldi_browser_window.h"

VivaldiNativeFrameView::VivaldiNativeFrameView(views::Widget* widget,
                                               VivaldiBrowserWindow* window)
    : NativeFrameView(widget), window_(window) {}

VivaldiNativeFrameView::~VivaldiNativeFrameView() = default;

int VivaldiNativeFrameView::NonClientHitTest(const gfx::Point& point) {
  views::Widget* widget = window_->GetWidget();
  if (!widget) {
    return HTNOWHERE;
  }

  window_->ReportNCMousePosition(point);

  return HTNOWHERE;
}
