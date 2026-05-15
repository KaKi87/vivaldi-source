// Copyright 2026 Vivaldi Technologies. All rights reserved.

#include "content/browser/renderer_host/render_widget_host_view_mac.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#import "content/browser/renderer_host/text_input_client_mac.h"
#include "content/public/browser/render_process_host.h"
#include "ui/base/mojom/attributed_string.mojom.h"

namespace content {

void RenderWidgetHostViewMac::VivaldiOnAsyncDictionaryHitTestCompleted(
    base::WeakPtr<RenderWidgetHostViewInput> view,
    std::optional<gfx::PointF> transformed_point) {
  if (!view || !transformed_point.has_value()) {
    return;
  }
  auto* view_base = static_cast<RenderWidgetHostViewBase*>(view.get());

  RenderWidgetHostImpl* widget_host = RenderWidgetHostImpl::From(
      view_base->GetRenderWidgetHost());

  if (!widget_host)
    return;

  // For popups, do not support QuickLook.
  if (popup_parent_host_view_)
    return;

  int32_t target_widget_process_id =
      widget_host->GetProcess()->GetDeprecatedID();
  int32_t target_widget_routing_id = widget_host->GetRoutingID();

  // VB-95233: Vivaldi needs to do the scaling after transforming to the
  // webpage view
  transformed_point->Scale(GetDeviceScaleFactor());

  TextInputClientMac::GetInstance()->GetStringAtPoint(
      widget_host, gfx::ToFlooredPoint(transformed_point.value()),
      base::BindOnce(&RenderWidgetHostViewMac::OnGotStringForDictionaryOverlay,
                     weak_factory_.GetWeakPtr(), target_widget_process_id,
                     target_widget_routing_id));
}

}  // namespace content
