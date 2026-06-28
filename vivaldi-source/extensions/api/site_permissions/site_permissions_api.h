// Copyright (c) 2015-2025 Vivaldi Technologies AS. All rights reserved.

#ifndef EXTENSIONS_API_PERMISSIONS_API_H_
#define EXTENSIONS_API_PERMISSIONS_API_H_

#include <map>
#include <string>

#include "base/functional/callback.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_function.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace permissions {
class PermissionRequest;
}  // namespace permissions

namespace extensions {

class ContentSettingsObserver;

class SitePermissionsAPI : public BrowserContextKeyedAPI,
                           public content_settings::Observer {
 public:
  explicit SitePermissionsAPI(content::BrowserContext* context);
  ~SitePermissionsAPI() override;
  SitePermissionsAPI(const SitePermissionsAPI&) = delete;
  SitePermissionsAPI& operator=(const SitePermissionsAPI&) = delete;

  static SitePermissionsAPI* FromBrowserContext(
      content::BrowserContext* browser_context);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<SitePermissionsAPI>*
  GetFactoryInstance();

  static void SendPermissionChanged(content::BrowserContext* browser_context,
                                    const std::string& origin,
                                    ContentSettingsType type);

  static void SendDeviceGrantChanged(content::BrowserContext* browser_context,
                                     const std::string& origin);

  // Fire onPermissionAccessed event
  static void FirePermissionAccessedEvent(
      content::BrowserContext* browser_context,
      int tab_id,
      ContentSettingsType type,
      const std::string& origin,
      ContentSetting setting,
      const std::vector<std::string>& device_names = {});

  // VB-114658: Fire onPermissionAccessed event with string permission type.
  // Used for device choosers to pass the chooser type string directly.
  static void FirePermissionAccessedEvent(
      content::BrowserContext* browser_context,
      int tab_id,
      const std::string& permission_type,
      const std::string& origin,
      ContentSetting setting,
      const std::vector<std::string>& device_names = {});

  // Fire onPermissionCleared event (e.g., on navigation)
  static void FirePermissionClearedEvent(
      content::BrowserContext* browser_context,
      int tab_id);

  // Fire onPermissionRequest event and store request
  void FirePermissionRequestEvent(
      int tab_id,
      std::unique_ptr<::permissions::PermissionRequest> request,
      ContentSettingsType permission_type,
      const std::string& origin,
      const std::string& protocol_name = "",
      const std::string& protocol_url = "",
      const std::string& embedding_origin = "");

  // VB-114658: Fire onPermissionRequest event for device choosers (async
  // variant). Used by BridgeDeviceChooser to fire initial event with
  // deviceChooserPending=true. Devices will be sent separately via
  // FirePermissionDeviceChooserUpdate.
  void FirePermissionRequestEventForDeviceChooser(
      int tab_id,
      const std::string& request_id,
      const std::string& permission_type,
      const std::string& origin,
      bool allow_multiple_selection);

  // VB-114658: Fire onDeviceChooserUpdate event with devices for a pending
  // chooser request. Sends available devices and whether enumeration is
  // complete.
  void FirePermissionDeviceChooserUpdate(int tab_id,
                                         const std::string& request_id,
                                         const base::ListValue& devices,
                                         bool is_complete);

  // Respond to a permission request (called by respondToPermissionRequest
  // function) VB-114658: For device chooser requests, device_indices can be
  // provided to specify which devices were selected (only used when
  // setting=CONTENT_SETTING_ALLOW).
  bool RespondToPermissionRequest(const std::string& request_id,
                                  ContentSetting setting,
                                  const std::vector<int>& device_indices = {});

  // Clean up pending requests for a specific tab
  void CleanupRequestsForTab(int tab_id);

 private:
  friend class BrowserContextKeyedAPIFactory<SitePermissionsAPI>;
  friend class ContentSettingsObserver;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "SitePermissionsAPI"; }
  static const bool kServiceIsNULLWhileTesting = true;
  static const bool kServiceRedirectedInIncognito = true;

  std::string GenerateRequestId();

  const raw_ptr<Profile> profile_;
  std::unique_ptr<ContentSettingsObserver> settings_observer_;

  // Pending permission request data
  struct PendingRequest {
    PendingRequest() = default;
    PendingRequest(PendingRequest&&) = default;
    PendingRequest& operator=(PendingRequest&&) = default;
    ~PendingRequest() = default;

    int tab_id;
    std::unique_ptr<::permissions::PermissionRequest> request;
  };

  // VB-114658: Pending device chooser request data
  struct PendingDeviceRequest {
    PendingDeviceRequest() = default;
    PendingDeviceRequest(PendingDeviceRequest&&) = default;
    PendingDeviceRequest& operator=(PendingDeviceRequest&&) = default;
    ~PendingDeviceRequest() = default;

    int tab_id;
    std::string permission_type;  // "usbGuard", "serialGuard", etc.
    std::string origin;
    bool allow_multiple_selection = false;
    base::ListValue devices;
  };

  // Pending permission requests: request_id -> unique_ptr to PendingRequest
  std::map<std::string, std::unique_ptr<PendingRequest>>
      pending_permission_requests_;

  // VB-114658: Pending device chooser requests: request_id -> unique_ptr to
  // PendingDeviceRequest
  std::map<std::string, std::unique_ptr<PendingDeviceRequest>>
      pending_device_requests_;

  int next_request_id_ = 0;
};

template <>
struct BrowserContextFactoryDependencies<SitePermissionsAPI> {
  static void DeclareFactoryDependencies(
      BrowserContextKeyedAPIFactory<SitePermissionsAPI>* factory);
};

class SitePermissionsGetAvailablePermissionsFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getAvailablePermissions",
                             SITE_PERMISSIONS_GET_AVAILABLE)
  SitePermissionsGetAvailablePermissionsFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsGetOverriddenSitesFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getOverriddenSites",
                             SITE_PERMISSIONS_GET_OVERRIDDEN_SITES)
  SitePermissionsGetOverriddenSitesFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsGetOverridesForSiteFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getOverridesForSite",
                             SITE_PERMISSIONS_GET_OVERRIDES_FOR_SITE)
  SitePermissionsGetOverridesForSiteFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsGetOverridesForPatternFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getOverridesForPattern",
                             SITE_PERMISSIONS_GET_OVERRIDES_FOR_PATTERN)
  SitePermissionsGetOverridesForPatternFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsSetSitePermissionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.setSitePermission",
                             SITE_PERMISSIONS_SET_SITE_PERMISSION)
  SitePermissionsSetSitePermissionFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsResetSitePermissionsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.resetSitePermissions",
                             SITE_PERMISSIONS_RESET_SITE_PERMISSIONS)
  SitePermissionsResetSitePermissionsFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsRespondToPermissionRequestFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.respondToPermissionRequest",
                             SITE_PERMISSIONS_RESPOND_TO_PERMISSION_REQUEST)
  SitePermissionsRespondToPermissionRequestFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsGetAllowedDevicesFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getAllowedDevices",
                             SITE_PERMISSIONS_GET_ALLOWED_DEVICES)
  SitePermissionsGetAllowedDevicesFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsGetDeviceGrantsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getDeviceGrants",
                             SITE_PERMISSIONS_GET_DEVICE_GRANTS)
  SitePermissionsGetDeviceGrantsFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsRevokeDeviceGrantFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.revokeDeviceGrant",
                             SITE_PERMISSIONS_REVOKE_DEVICE_GRANT)
  SitePermissionsRevokeDeviceGrantFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsGetDeviceGrantSitesFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getDeviceGrantSites",
                             SITE_PERMISSIONS_GET_DEVICE_GRANT_SITES)
  SitePermissionsGetDeviceGrantSitesFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsGetSecurityInfoFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getSecurityInfo",
                             SITE_PERMISSIONS_GET_SECURITY_INFO)
  SitePermissionsGetSecurityInfoFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsGetSiteDataFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.getSiteData",
                             SITE_PERMISSIONS_GET_SITE_DATA)
  SitePermissionsGetSiteDataFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsSetSiteDataFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.setSiteData",
                             SITE_PERMISSIONS_SET_SITE_DATA)
  SitePermissionsSetSiteDataFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsShowCertificateDialogFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.showCertificateDialog",
                             SITE_PERMISSIONS_SHOW_CERTIFICATE_DIALOG)
  SitePermissionsShowCertificateDialogFunction() = default;
  ResponseAction Run() override;
};

class SitePermissionsShowSiteDataDialogFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("sitePermissions.showSiteDataDialog",
                             SITE_PERMISSIONS_SHOW_SITE_DATA_DIALOG)
  SitePermissionsShowSiteDataDialogFunction() = default;
  ResponseAction Run() override;
};

}  // namespace extensions

#endif  // EXTENSIONS_API_PERMISSIONS_API_H_
