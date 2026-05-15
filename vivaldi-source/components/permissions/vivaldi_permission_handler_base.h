// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_PERMISSIONS_VIVALDI_PERMISSION_HANDLER_BASE_H
#define COMPONENTS_PERMISSIONS_VIVALDI_PERMISSION_HANDLER_BASE_H

#include "components/permissions/vivaldi_permission_handler.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace vivaldi {
namespace permissions {

/// Base class for overrides in site permission handling.
class VivaldiPermissionHandlerBase {
 public:
  /// Returns the active permission handler instance
  static VivaldiPermissionHandlerBase* Get();

  VivaldiPermissionHandlerBase();
  ~VivaldiPermissionHandlerBase();

  /// Called when permission changed (ALLOW/DENY, etc) from chromium, forwards
  /// the events to JS
  virtual void NotifyPermissionSet(const ::permissions::PermissionRequestID& id,
                                   ContentSettingsType type,
                                   ContentSetting setting);

  /** Called to handle a queued permission request via our overrides (and not by
   * builtin chromium dialog).
   * @return true if the request was handled by the override */
  virtual bool HandlePermissionRequest(
      const content::GlobalRenderFrameHostId& source_frame_id,
      std::unique_ptr<::permissions::PermissionRequest>& request);

  /** VB-114658: Bridge device choosers through sitePermissions API.
   * @return true if device chooser was bridged via sitePermissions API */
  virtual bool BridgeDeviceChooser(
      content::RenderFrameHost* owner,
      std::unique_ptr<class ::permissions::ChooserController>* controller);
};

}  // namespace permissions
}  // namespace vivaldi

#endif /* COMPONENTS_PERMISSIONS_VIVALDI_PERMISSION_HANDLER_BASE_H */
