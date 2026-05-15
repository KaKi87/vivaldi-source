// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#ifndef PERMISSIONS_VIVALDI_PERMISSION_HANDLER_IMPL_H
#define PERMISSIONS_VIVALDI_PERMISSION_HANDLER_IMPL_H

#include <map>
#include <memory>

#include "chromium/components/content_settings/core/common/content_settings.h"
#include "components/permissions/vivaldi_permission_handler_base.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace permissions {
class PermissionRequest;
class PermissionRequestID;
class ChooserController;
}  // namespace permissions

namespace extensions {
class SitePermissionsAPI;
}  // namespace extensions

namespace vivaldi {
namespace permissions {

// Forward declarations for view.
class DeviceEnumerationView;

// VB-114658: Storage for pending device chooser with associated controller.
struct PendingDeviceChooser {
  std::unique_ptr<::permissions::ChooserController> controller;
  std::unique_ptr<DeviceEnumerationView> view;
  std::string chooser_type;  // "usb", "serial", "hid", "bluetooth"
  int tab_id;
  std::string origin;  // Origin URL for permission event.
  content::BrowserContext* browser_context = nullptr;  // Browser context for API access.
};

// Observer that cleans up permission state on navigation.
class PermissionNavigationObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PermissionNavigationObserver> {
 public:
  ~PermissionNavigationObserver() override = default;

  // WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  friend class content::WebContentsUserData<PermissionNavigationObserver>;
  explicit PermissionNavigationObserver(content::WebContents* web_contents);

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

/** This is UI only (ie. no unittest) permission handler implementation, spawned
 * at vivaldi startup, handles the bridging from Permission request sources to
 * our permission prompts in JS.
 * @note In unit tests, this class is not spawned, as the chromium instance
 * itself isn't and we're dependent on that. This is handled by
 * vivaldi::NotifyPermissionSet and vivaldi::HandlePermissionRequest functions
 * (noop in no-instance situations).
 */
class VivaldiPermissionHandlerImpl : public VivaldiPermissionHandlerBase {
 public:
  VivaldiPermissionHandlerImpl() = default;

  // Mostly used for initialization.
  static VivaldiPermissionHandlerImpl* GetInstance();

  /// Called when permission changed (ALLOW/DENY, etc) from chromium, forwards
  /// the events to JS.
  void NotifyPermissionSet(const ::permissions::PermissionRequestID& id,
                           ContentSettingsType type,
                           ContentSetting setting) override;

  /** Called to handle a queued permission request via our overrides (and not by
   * builtin chromium dialog).
   * @return true if the request was handled by the override */
  bool HandlePermissionRequest(
      const content::GlobalRenderFrameHostId& source_frame_id,
      std::unique_ptr<::permissions::PermissionRequest>& request) override;

  /** VB-114658: Bridge device choosers through sitePermissions API.
   * @return true if device chooser was bridged via sitePermissions API */
  bool BridgeDeviceChooser(
      content::RenderFrameHost* owner,
      std::unique_ptr<::permissions::ChooserController>* controller) override;

  /** VB-114658: Respond to a device chooser selection from the sitePermissions API.
   * Called by SitePermissionsAPI when user selects/denies a device choice.
   * @param request_id The request ID from FirePermissionRequestEventWithDevices.
   * @param allow Whether the user allowed access (true) or denied (false).
   * @param device_indices List of device indices the user allowed (only used if allow=true). */
  void RespondToDeviceChooser(const std::string& request_id,
                              bool allow,
                              const std::vector<int>& device_indices);

  /** VB-114658: Callback when a device is added during enumeration.
   * Called by DeviceEnumerationView via OnOptionAdded(). */
  void OnDeviceAdded(const std::string& request_id,
                     int tab_id,
                     extensions::SitePermissionsAPI* api,
                     const base::DictValue& device);

  /** VB-114658: Callback when device enumeration completes.
   * Called by DeviceEnumerationView when enumeration signals completion. */
  void OnEnumerationComplete(const std::string& request_id,
                            int tab_id,
                            extensions::SitePermissionsAPI* api);

  /** VB-114658: Clean up all pending device choosers for a tab.
   * Called when a tab is closed or navigated. */
  void CleanupPendingChoosersForTab(int tab_id);

 private:
  // Extract protocol handler data from a protocol handler permission request.
  // Returns true if protocol data was extracted successfully.
  static bool ExtractProtocolHandlerData(
      const ::permissions::PermissionRequest* request,
      std::string* protocol_name_out,
      std::string* protocol_url_out);

  // Extract chooser type from ChooserController instance.
  static std::string GetChooserType(
      ::permissions::ChooserController* controller);

  // VB-114658: Storage for pending device choosers indexed by request ID.
  std::map<std::string, PendingDeviceChooser> pending_choosers_;
  int next_chooser_request_id_ = 0;
};

}  // namespace permissions
}  // namespace vivaldi

#endif /* PERMISSIONS_VIVALDI_PERMISSION_HANDLER_IMPL_H */
