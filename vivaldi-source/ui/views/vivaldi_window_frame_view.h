// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved.

#ifndef UI_VIEWS_VIVALDI_WINDOW_FRAME_VIEW_H_
#define UI_VIEWS_VIVALDI_WINDOW_FRAME_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/events/event_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/window/native_frame_view.h"

class VivaldiBrowserWindow;
namespace views {
class FrameView;
class EventMonitor;
}  // namespace views

namespace ui {
class MouseEvent;
}

class VivaldiWindowFrameEventObserver : public ui::EventObserver {
 public:
  explicit VivaldiWindowFrameEventObserver(VivaldiBrowserWindow* window);
  VivaldiWindowFrameEventObserver(const VivaldiWindowFrameEventObserver&) =
      delete;
  VivaldiWindowFrameEventObserver& operator=(VivaldiWindowFrameEventObserver&) =
      delete;
  ~VivaldiWindowFrameEventObserver() override;

 private:
  // ui::EventObserver:
  void OnEvent(const ui::Event& event) override;

  void OnMouseEvent(const ui::MouseEvent& event);

  raw_ptr<VivaldiBrowserWindow> window_ = nullptr;
  std::unique_ptr<views::EventMonitor> event_monitor_;
};

class VivaldiNativeFrameView : public views::NativeFrameView {
 public:
  explicit VivaldiNativeFrameView(VivaldiBrowserWindow* window);
  VivaldiNativeFrameView(const VivaldiNativeFrameView&) = delete;
  VivaldiNativeFrameView& operator=(VivaldiNativeFrameView&) = delete;
  ~VivaldiNativeFrameView() override;

 private:
  raw_ptr<VivaldiBrowserWindow> window_ = nullptr;

  std::unique_ptr<VivaldiWindowFrameEventObserver> window_frame_event_observer_;
};

std::unique_ptr<views::FrameView> CreateVivaldiWindowFrameView(
    VivaldiBrowserWindow* window);

#endif  // UI_VIEWS_VIVALDI_WINDOW_FRAME_VIEW_H_
