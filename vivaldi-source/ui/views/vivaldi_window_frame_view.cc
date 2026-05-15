// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved.

#include "ui/views/vivaldi_window_frame_view.h"
#include "ui/views/event_monitor.h"
#include "ui/views/widget/widget.h"
#include "ui/vivaldi_browser_window.h"

VivaldiWindowFrameEventObserver::VivaldiWindowFrameEventObserver(
    VivaldiBrowserWindow* window)
    : window_(window) {
  event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      this, window_->GetNativeWindow(),
      {ui::EventType::kMouseMoved, ui::EventType::kMouseDragged});
}

VivaldiWindowFrameEventObserver::~VivaldiWindowFrameEventObserver() {
  event_monitor_.reset();
}

void VivaldiWindowFrameEventObserver::OnEvent(const ui::Event& event) {
  if (event.IsMouseEvent()) {
    OnMouseEvent(*event.AsMouseEvent());
  }
}

void VivaldiWindowFrameEventObserver::OnMouseEvent(
    const ui::MouseEvent& event) {
  if (event.type() != ui::EventType::kMouseMoved &&
      event.type() != ui::EventType::kMouseDragged) {
    return;
  }

  const gfx::Point point(event.x(), event.y());
  window_->ReportMousePosition(point,
                               event.type() == ui::EventType::kMouseDragged);
}

VivaldiNativeFrameView::VivaldiNativeFrameView(VivaldiBrowserWindow* window)
    : NativeFrameView(window->GetWidget()),
      window_(window),
      window_frame_event_observer_(
          std::make_unique<VivaldiWindowFrameEventObserver>(window_)) {}

VivaldiNativeFrameView::~VivaldiNativeFrameView() = default;
