// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved.

#ifndef UI_VIEWS_VIVALDI_WINDOW_FRAME_VIEW_H_
#define UI_VIEWS_VIVALDI_WINDOW_FRAME_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/views/window/native_frame_view.h"
#include "ui/gfx/geometry/point.h"

class VivaldiBrowserWindow;

class VivaldiNativeFrameView : public views::NativeFrameView {
 public:
  VivaldiNativeFrameView(views::Widget* widget, VivaldiBrowserWindow* window);
  VivaldiNativeFrameView(const VivaldiNativeFrameView&) = delete;
  VivaldiNativeFrameView& operator=(VivaldiNativeFrameView&) = delete;
  ~VivaldiNativeFrameView() override;

 private:
  // views::FrameView
  int NonClientHitTest(const gfx::Point& point) override;

  raw_ptr<VivaldiBrowserWindow> window_ = nullptr;
};

std::unique_ptr<views::FrameView> CreateVivaldiWindowFrameView(
    VivaldiBrowserWindow* window);

#endif  // UI_VIEWS_VIVALDI_WINDOW_FRAME_VIEW_H_
