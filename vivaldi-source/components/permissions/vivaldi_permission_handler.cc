// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#include "components/permissions/vivaldi_permission_handler.h"
#include "components/permissions/vivaldi_permission_handler_base.h"

#include "components/permissions/permission_request_manager.h"
#include "components/permissions/chooser_controller.h"

namespace permissions {

bool PermissionRequestManager::VivaldiHandleAddRequest(
    content::RenderFrameHost* source_frame,
    std::unique_ptr<PermissionRequest>& request) {
  return vivaldi::permissions::HandlePermissionRequest(
      source_frame->GetGlobalId(), request);
}

}  // namespace permissions

namespace vivaldi {
namespace permissions {

void NotifyPermissionSet(const ::permissions::PermissionRequestID& id,
                         ContentSettingsType type,
                         ContentSetting setting) {
#if !BUILDFLAG(IS_ANDROID)
  VivaldiPermissionHandlerBase* instance = VivaldiPermissionHandlerBase::Get();
  if (instance) {
    instance->NotifyPermissionSet(id, type, setting);
  }
#endif
}

bool HandlePermissionRequest(
    const content::GlobalRenderFrameHostId& source_frame_id,
    std::unique_ptr<::permissions::PermissionRequest>& request) {
#if !BUILDFLAG(IS_ANDROID)
  VivaldiPermissionHandlerBase* instance = VivaldiPermissionHandlerBase::Get();
  if (instance) {
    return instance->HandlePermissionRequest(source_frame_id, request);
  }
#endif
  return false;
}

bool BridgeDeviceChooser(
    content::RenderFrameHost* owner,
    std::unique_ptr<::permissions::ChooserController>* controller) {
#if !BUILDFLAG(IS_ANDROID)
  VivaldiPermissionHandlerBase* instance = VivaldiPermissionHandlerBase::Get();
  if (instance) {
    return instance->BridgeDeviceChooser(owner, controller);
  }
#endif
  return false;
}

}  // namespace permissions
}  // namespace vivaldi
