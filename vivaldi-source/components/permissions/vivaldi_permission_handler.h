// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#ifndef COMONENTS_PERMISSIONS_VIVALDI_PERMISSION_HANDLER_H
#define COMONENTS_PERMISSIONS_VIVALDI_PERMISSION_HANDLER_H

#include "chromium/components/content_settings/core/common/content_settings.h"

namespace content {
struct GlobalRenderFrameHostId;
class RenderFrameHost;
}  // namespace content

namespace permissions {
class PermissionRequest;
class PermissionRequestID;
class ChooserController;
}  // namespace permissions

namespace vivaldi {
namespace permissions {

/** Called on every notification change. Allows the permission icons to be
 * synchronized. */
void NotifyPermissionSet(const ::permissions::PermissionRequestID& id,
                         ContentSettingsType type,
                         ContentSetting setting);

/** Tries to handle the permission request on vivaldi side.
 * @returns true if permission request will be handled by vivaldi, false
 * otherwise */
bool HandlePermissionRequest(
    const content::GlobalRenderFrameHostId& source_frame_id,
    std::unique_ptr<::permissions::PermissionRequest>& request);

/** VB-114658: Bridge device choosers (USB, Serial, HID, Bluetooth) through
 * sitePermissions API for unified permission UI. Extracts device list from the
 * chooser controller and fires onPermissionRequest event for UI handling.
 * @returns true if device chooser was bridged, false to use default Chromium UI
 */
bool BridgeDeviceChooser(
    content::RenderFrameHost* owner,
    std::unique_ptr<::permissions::ChooserController>* controller);

}  // namespace permissions
}  // namespace vivaldi

#endif /* COMONENTS_PERMISSIONS_VIVALDI_PERMISSION_HANDLER_H */
