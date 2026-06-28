// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "chrome/browser/notifications/notification_permission_context.h"
#include "components/permissions/permission_context_base.h"

#include "app/vivaldi_apptools.h"
#include "base/functional/callback.h"
#include "components/permissions/permission_request_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/api/site_permissions/site_permissions_api.h"
#include "extensions/browser/guest_view/web_view/web_view_constants.h"
#include "extensions/vivaldi_browser_component_wrapper.h"
#endif

void NotificationPermissionContext::UpdatePrivateTabContext(
    const permissions::PermissionRequestID& id,
    const GURL& requesting_frame,
    bool allowed) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(
          content::RenderFrameHost::FromID(id.global_render_frame_host_id()));

  if (web_contents) {
    ContentSetting content_setting =
        allowed ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;

    // Get tab ID
    int tab_id =
        VivaldiBrowserComponentWrapper::GetInstance()->GetTabId(web_contents);
    if (tab_id >= 0) {
      // Fire onPermissionAccessed event
      extensions::SitePermissionsAPI::FirePermissionAccessedEvent(
          web_contents->GetBrowserContext(), tab_id,
          ContentSettingsType::NOTIFICATIONS, requesting_frame.spec(),
          content_setting);
    }
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
}
