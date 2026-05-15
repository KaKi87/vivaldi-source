// Copyright (c) 2018 Vivaldi Technologies AS. All rights reserved

#include "extensions/browser/guest_view/web_view/web_view_permission_helper_delegate.h"

#include "base/values.h"
#include "components/custom_handlers/register_protocol_handler_permission_request.h"
#include "components/guest_view/vivaldi_guest_view_constants.h"
#include "content/public/browser/web_contents_delegate.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"
#include "extensions/vivaldi_browser_component_wrapper.h"

namespace extensions {

void WebViewPermissionHelper::SetDownloadInformation(
    const content::DownloadInformation& info) {
  download_info_ = info;
}

void WebViewPermissionHelperDelegate::SetDownloadInformation(
    const content::DownloadInformation& info) {
  download_info_ = info;
}

void WebViewPermissionHelper::RegisterProtocolHandler(
    content::RenderFrameHost* requesting_frame,
    const std::string& protocol,
    const GURL& url,
    bool user_gesture) {
}

void WebViewPermissionHelper::OnProtocolPermissionResponse(
    bool allow,
    const std::string& user_input) {
  VivaldiBrowserComponentWrapper::GetInstance()->SetOrRollbackProtocolHandler(
      web_view_guest()->web_contents(), allow);
}

}  // namespace extensions
