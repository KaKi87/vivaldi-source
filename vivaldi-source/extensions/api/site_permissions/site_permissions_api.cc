// Copyright (c) 2015-2025 Vivaldi Technologies AS. All rights reserved.
#include "extensions/api/site_permissions/site_permissions_api.h"

#include <set>
#include <string_view>

#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/permissions/permission_request.h"
#include "extensions/browser/api/content_settings/content_settings_helpers.h"
#include "extensions/browser/extension_function.h"
#include "extensions/schema/site_permissions.h"
#include "extensions/tools/vivaldi_tools.h"
#include "url/gurl.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bluetooth/bluetooth_chooser_context_factory.h"
#include "chrome/browser/hid/hid_chooser_context.h"
#include "chrome/browser/hid/hid_chooser_context_factory.h"
#include "chrome/browser/serial/serial_chooser_context.h"
#include "chrome/browser/serial/serial_chooser_context_factory.h"
#include "chrome/browser/usb/usb_chooser_context.h"
#include "chrome/browser/usb/usb_chooser_context_factory.h"
#include "components/permissions/contexts/bluetooth_chooser_context.h"
#include "components/permissions/object_permission_context_base.h"

#include "extensions/vivaldi_browser_component_wrapper.h"

// VB-114658: Device chooser bridge support
#include "permissions/vivaldi_permission_handler_impl.h"

// For security info
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/page_info/chrome_page_info_delegate.h"
#include "chrome/browser/ui/page_info/page_info_infobar_delegate.h"
#include "chrome/browser/ui/tab_contents/tab_contents_iterator.h"
#include "chrome/browser/ui/tab_dialogs.h"
#include "chrome/browser/ui/views/site_data/page_specific_site_data_dialog_controller.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/security_state/content/security_state_tab_helper.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/x509_certificate.h"

// For site data
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "components/browsing_data/content/browsing_data_model.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "net/cookies/cookie_setting_override.h"
#include "net/cookies/site_for_cookies.h"

namespace extensions {

constexpr std::string_view kBluetoothGuard = "bluetooth-guard";
constexpr std::string_view kUsbGuard = "usb-guard";
constexpr std::string_view kSerialGuard = "serial-guard";
constexpr std::string_view kHidGuard = "hid-guard";

constexpr std::array<ContentSetting, 4> kAllContentSettings = {
    CONTENT_SETTING_BLOCK,
    CONTENT_SETTING_ALLOW,
    CONTENT_SETTING_ASK,
    CONTENT_SETTING_SESSION_ONLY,
    // NOTE: CONTENT_SETTING_DETECT_IMPORTANT_CONTENT
};

// Returns true if the scoping type requires a secondary origin.
bool RequiresSecondaryOrigin(
    content_settings::WebsiteSettingsInfo::ScopingType scoping_type) {
  return scoping_type == content_settings::WebsiteSettingsInfo::
                             REQUESTING_AND_TOP_SCHEMEFUL_SITE_SCOPE ||
         scoping_type == content_settings::WebsiteSettingsInfo::
                             REQUESTING_ORIGIN_AND_TOP_SCHEMEFUL_SITE_SCOPE;
}

// Show a "reload to apply changes" infobar on tabs matching the given pattern.
void ShowReloadInfoBarForPattern(const ContentSettingsPattern& primary_pattern,
                                 Profile* profile) {
  tabs::ForEachTabInterface([&](tabs::TabInterface* tab) {
    content::WebContents* web_contents = tab->GetContents();
    const GURL tab_url = web_contents->GetLastCommittedURL();
    if (primary_pattern.Matches(tab_url) &&
        tab->GetBrowserWindowInterface()->GetProfile()->GetOriginalProfile() ==
            profile->GetOriginalProfile()) {
      infobars::ContentInfoBarManager* infobar_manager =
          infobars::ContentInfoBarManager::FromWebContents(web_contents);
      if (infobar_manager) {
        PageInfoInfoBarDelegate::CreateForVivaldi(infobar_manager);
      }
    }
    return true;
  });
}

vivaldi::site_permissions::PermissionSetting toPermissionSetting(
    ContentSetting setting) {
  using vivaldi::site_permissions::PermissionSetting;
  switch (setting) {
    case CONTENT_SETTING_ALLOW:
      return PermissionSetting::kAllow;
    case CONTENT_SETTING_BLOCK:
      return PermissionSetting::kBlock;
    case CONTENT_SETTING_ASK:
      return PermissionSetting::kAsk;
    case CONTENT_SETTING_SESSION_ONLY:
      return PermissionSetting::kSessionOnly;
    case CONTENT_SETTING_DEFAULT:
      return PermissionSetting::kDefault;
    default:
      break;
  }

  return PermissionSetting::kNone;
}

std::optional<ContentSetting> fromPermissionSetting(
    vivaldi::site_permissions::PermissionSetting setting) {
  switch (setting) {
    case vivaldi::site_permissions::PermissionSetting::kAllow:
      return CONTENT_SETTING_ALLOW;
    case vivaldi::site_permissions::PermissionSetting::kBlock:
      return CONTENT_SETTING_BLOCK;
    case vivaldi::site_permissions::PermissionSetting::kAsk:
      return CONTENT_SETTING_ASK;
    case vivaldi::site_permissions::PermissionSetting::kSessionOnly:
      return CONTENT_SETTING_SESSION_ONLY;
    case vivaldi::site_permissions::PermissionSetting::kDefault:
      return CONTENT_SETTING_DEFAULT;
    case vivaldi::site_permissions::PermissionSetting::kNone:
      return std::nullopt;
  }
  return std::nullopt;
}

inline vivaldi::site_permissions::SafeBrowsingStatus toSafeBrowsingStatus(
    security_state::MaliciousContentStatus status) {
  switch (status) {
    case security_state::MALICIOUS_CONTENT_STATUS_NONE:
      return vivaldi::site_permissions::SafeBrowsingStatus::kNone;
    case security_state::MALICIOUS_CONTENT_STATUS_MALWARE:
      return vivaldi::site_permissions::SafeBrowsingStatus::kMalware;
    case security_state::MALICIOUS_CONTENT_STATUS_UNWANTED_SOFTWARE:
      return vivaldi::site_permissions::SafeBrowsingStatus::kUnwantedSoftware;
    case security_state::MALICIOUS_CONTENT_STATUS_SOCIAL_ENGINEERING:
      return vivaldi::site_permissions::SafeBrowsingStatus::kSocialEngineering;
    case security_state::MALICIOUS_CONTENT_STATUS_SAVED_PASSWORD_REUSE:
      return vivaldi::site_permissions::SafeBrowsingStatus::kSavedPasswordReuse;
    case security_state::MALICIOUS_CONTENT_STATUS_SIGNED_IN_SYNC_PASSWORD_REUSE:
      return vivaldi::site_permissions::SafeBrowsingStatus::
          kSignedInSyncPasswordReuse;
    case security_state::
        MALICIOUS_CONTENT_STATUS_SIGNED_IN_NON_SYNC_PASSWORD_REUSE:
      return vivaldi::site_permissions::SafeBrowsingStatus::
          kSignedInNonSyncPasswordReuse;
    case security_state::MALICIOUS_CONTENT_STATUS_ENTERPRISE_PASSWORD_REUSE:
      return vivaldi::site_permissions::SafeBrowsingStatus::
          kEnterprisePasswordReuse;
    case security_state::MALICIOUS_CONTENT_STATUS_BILLING:
      return vivaldi::site_permissions::SafeBrowsingStatus::kBilling;
    case security_state::MALICIOUS_CONTENT_STATUS_MANAGED_POLICY_WARN:
      return vivaldi::site_permissions::SafeBrowsingStatus::kManagedPolicyWarn;
    case security_state::MALICIOUS_CONTENT_STATUS_MANAGED_POLICY_BLOCK:
      return vivaldi::site_permissions::SafeBrowsingStatus::kManagedPolicyBlock;
    default:
      return vivaldi::site_permissions::SafeBrowsingStatus::kNone;
  }
}

// Returns the chooser context for a given device permission type string.
permissions::ObjectPermissionContextBase* GetChooserContextForType(
    Profile* profile,
    const std::string& permission_type) {
  if (permission_type == kUsbGuard) {
    return UsbChooserContextFactory::GetForProfile(profile);
  } else if (permission_type == kSerialGuard) {
    return SerialChooserContextFactory::GetForProfile(profile);
  } else if (permission_type == kHidGuard) {
    return HidChooserContextFactory::GetForProfile(profile);
  } else if (permission_type == kBluetoothGuard) {
    return BluetoothChooserContextFactory::GetForProfile(profile);
  }
  return nullptr;
}

// All supported device chooser permission types.
constexpr std::array<const char*, 4> kAllDeviceTypes = {
    "usb-guard", "serial-guard", "hid-guard", "bluetooth-guard"};

// Various device chooser contexts crash in GetKeyForObject when devices
// have alternative identifiers instead of the expected key fields.
std::string GetSafeKeyForObject(
    permissions::ObjectPermissionContextBase* context,
    const base::DictValue& object) {
  // Ephemeral USB devices have a GUID instead of a serial number.
  const std::string* ephemeral_guid = object.FindString("ephemeral-guid");
  if (ephemeral_guid) {
    return *ephemeral_guid;
  }
  // Token-based serial ports don't have vendor/product fields.
  const std::string* token = object.FindString("token");
  if (token) {
    return *token;
  }
  // HID devices may have a GUID instead of a serial number.
  const std::string* hid_guid = object.FindString("guid");
  if (hid_guid && !hid_guid->empty()) {
    return *hid_guid;
  }
  return context->GetKeyForObject(object);
}

class ContentSettingsObserver : public content_settings::Observer {
 public:
  ContentSettingsObserver(SitePermissionsAPI* owner)
      : owner_{owner},
        host_content_settings_map_{
            HostContentSettingsMapFactory::GetForProfile(owner->profile_)} {
    observer_.Observe(host_content_settings_map_);
  }

  // Per profile observation of content settings.
  base::ScopedObservation<HostContentSettingsMap, content_settings::Observer>
      observer_{this};

  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override {
    DCHECK(owner_);
    SitePermissionsAPI::SendPermissionChanged(
        static_cast<content::BrowserContext*>(owner_->profile_.get()),
        primary_pattern.ToString(), content_type);
  }

  SitePermissionsAPI* owner_;
  HostContentSettingsMap* host_content_settings_map_;
};

SitePermissionsAPI::SitePermissionsAPI(content::BrowserContext* context)
    : profile_(Profile::FromBrowserContext(context)) {
  settings_observer_.reset(new ContentSettingsObserver(this));
}

SitePermissionsAPI::~SitePermissionsAPI() {}

void BrowserContextFactoryDependencies<SitePermissionsAPI>::
    DeclareFactoryDependencies(
        BrowserContextKeyedAPIFactory<SitePermissionsAPI>* factory) {
  factory->DependsOn(
      ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
  factory->DependsOn(HostContentSettingsMapFactory::GetInstance());
  factory->DependsOn(BluetoothChooserContextFactory::GetInstance());
  factory->DependsOn(HidChooserContextFactory::GetInstance());
  factory->DependsOn(SerialChooserContextFactory::GetInstance());
  factory->DependsOn(UsbChooserContextFactory::GetInstance());
}

SitePermissionsAPI* SitePermissionsAPI::FromBrowserContext(
    content::BrowserContext* browser_context) {
  SitePermissionsAPI* api = GetFactoryInstance()->Get(browser_context);
  return api;
}

// static
BrowserContextKeyedAPIFactory<SitePermissionsAPI>*
SitePermissionsAPI::GetFactoryInstance() {
  static base::NoDestructor<BrowserContextKeyedAPIFactory<SitePermissionsAPI>>
      instance;
  return instance.get();
}

// static
void SitePermissionsAPI::SendPermissionChanged(
    content::BrowserContext* browser_context,
    const std::string& origin,
    ContentSettingsType type) {
  vivaldi::site_permissions::PermissionChangeEvent event;

  if (!content_settings::mojom::IsKnownEnumValue(type))
    return;

  // Don't propagate DEFAULT or types not registered in WebsiteSettingsRegistry
  // as ContentSettingsTypeToString will crash on null registry entry.
  if (type == ContentSettingsType::DEFAULT)
    return;

  if (!content_settings::WebsiteSettingsRegistry::GetInstance()->Get(type))
    return;

  event.origin = origin;
  event.permission =
      content_settings_helpers::ContentSettingsTypeToString(type);

  ::vivaldi::BroadcastEvent(
      vivaldi::site_permissions::OnPermissionChanged::kEventName,
      vivaldi::site_permissions::OnPermissionChanged::Create(event),
      browser_context);
}

// static
void SitePermissionsAPI::SendDeviceGrantChanged(
    content::BrowserContext* browser_context,
    const std::string& origin) {
  vivaldi::site_permissions::DeviceGrantChangeEvent event;
  event.origin = origin;

  ::vivaldi::BroadcastEvent(
      vivaldi::site_permissions::OnDeviceGrantChanged::kEventName,
      vivaldi::site_permissions::OnDeviceGrantChanged::Create(event),
      browser_context);
}

// static
void SitePermissionsAPI::FirePermissionAccessedEvent(
    content::BrowserContext* browser_context,
    int tab_id,
    ContentSettingsType type,
    const std::string& origin,
    ContentSetting setting,
    const std::vector<std::string>& device_names) {
  if (!content_settings::mojom::IsKnownEnumValue(type))
    return;

  // Don't propagate DEFAULT or types not registered in WebsiteSettingsRegistry
  // as ContentSettingsTypeToString will crash on null registry entry.
  if (type == ContentSettingsType::DEFAULT)
    return;

  if (!content_settings::WebsiteSettingsRegistry::GetInstance()->Get(type))
    return;

  // Convert enum to string and delegate to string-based version.
  std::string permission_type = base::ToLowerASCII(
      content_settings_helpers::ContentSettingsTypeToString(type));
  FirePermissionAccessedEvent(browser_context, tab_id, permission_type, origin,
                              setting, device_names);
}

// static
void SitePermissionsAPI::FirePermissionAccessedEvent(
    content::BrowserContext* browser_context,
    int tab_id,
    const std::string& permission_type,
    const std::string& origin,
    ContentSetting setting,
    const std::vector<std::string>& device_names) {
  vivaldi::site_permissions::PermissionAccessedEvent event;

  event.tab_id = tab_id;
  event.permission = permission_type;
  event.origin = origin;

  switch (setting) {
    case CONTENT_SETTING_ALLOW:
      event.setting = "allow";
      break;
    case CONTENT_SETTING_ASK:
      event.setting = "ask";
      break;
    case CONTENT_SETTING_BLOCK:
      event.setting = "block";
      break;
    case CONTENT_SETTING_DEFAULT:
    default:
      event.setting = "default";
      break;
  }

  // Add device names if provided.
  if (!device_names.empty()) {
    event.devices = device_names;
  }

  ::vivaldi::BroadcastEvent(
      vivaldi::site_permissions::OnPermissionAccessed::kEventName,
      vivaldi::site_permissions::OnPermissionAccessed::Create(event),
      browser_context);
}

// static
void SitePermissionsAPI::FirePermissionClearedEvent(
    content::BrowserContext* browser_context,
    int tab_id) {
  ::vivaldi::BroadcastEvent(
      vivaldi::site_permissions::OnPermissionCleared::kEventName,
      vivaldi::site_permissions::OnPermissionCleared::Create(tab_id),
      browser_context);
}

std::string SitePermissionsAPI::GenerateRequestId() {
  return base::NumberToString(++next_request_id_);
}

void SitePermissionsAPI::FirePermissionRequestEvent(
    int tab_id,
    std::unique_ptr<::permissions::PermissionRequest> request,
    ContentSettingsType permission_type,
    const std::string& origin,
    const std::string& protocol_name,
    const std::string& protocol_url,
    const std::string& embedding_origin) {
  // Generate unique request ID
  std::string request_id = GenerateRequestId();

  // Store request object and tab_id for later
  auto pending_request = std::make_unique<PendingRequest>();
  pending_request->tab_id = tab_id;
  pending_request->request = std::move(request);
  pending_permission_requests_[request_id] = std::move(pending_request);

  // Fire event with request ID
  vivaldi::site_permissions::PermissionRequestEvent event;
  event.request_id = request_id;
  event.tab_id = tab_id;
  event.permission =
      content_settings_helpers::ContentSettingsTypeToString(permission_type);
  event.origin = origin;

  // For two-origin permissions (e.g., storage-access), include the embedding
  // origin so the UI can display both.
  if (!embedding_origin.empty()) {
    event.embedding_origin = embedding_origin;
  }

  // Add protocol handler specific fields if provided
  if (!protocol_name.empty()) {
    event.protocol_name = protocol_name;
  }
  if (!protocol_url.empty()) {
    event.protocol_url = protocol_url;
  }

  ::vivaldi::BroadcastEvent(
      vivaldi::site_permissions::OnPermissionRequest::kEventName,
      vivaldi::site_permissions::OnPermissionRequest::Create(event), profile_);
}

void SitePermissionsAPI::CleanupRequestsForTab(int tab_id) {
  // Find and remove all regular permission requests for this tab
  auto it = pending_permission_requests_.begin();
  while (it != pending_permission_requests_.end()) {
    if (it->second->tab_id == tab_id) {
      // Cancel the permission request directly
      if (it->second->request) {
        it->second->request->Cancelled();
      }
      it = pending_permission_requests_.erase(it);
    } else {
      ++it;
    }
  }

  // VB-114658: Find and remove all device chooser requests for this tab
  auto device_it = pending_device_requests_.begin();
  while (device_it != pending_device_requests_.end()) {
    if (device_it->second->tab_id == tab_id) {
      device_it = pending_device_requests_.erase(device_it);
    } else {
      ++device_it;
    }
  }

  // Fire event to JS to clear permission UI state.
  FirePermissionClearedEvent(profile_, tab_id);
}

bool SitePermissionsAPI::RespondToPermissionRequest(
    const std::string& request_id,
    ContentSetting setting,
    const std::vector<int>& device_indices) {
  // VB-114658: Check if this is a device chooser request first
  auto device_it = pending_device_requests_.find(request_id);
  if (device_it != pending_device_requests_.end()) {
    // Device chooser request - forward to handler
    bool allow = (setting == CONTENT_SETTING_ALLOW);
    ::vivaldi::permissions::VivaldiPermissionHandlerImpl* handler =
        static_cast<::vivaldi::permissions::VivaldiPermissionHandlerImpl*>(
            ::vivaldi::permissions::VivaldiPermissionHandlerBase::Get());
    if (handler) {
      handler->RespondToDeviceChooser(request_id, allow, device_indices);
    }
    pending_device_requests_.erase(device_it);
    return true;
  }

  // Regular permission request
  auto it = pending_permission_requests_.find(request_id);
  if (it == pending_permission_requests_.end()) {
    return false;  // Request not found
  }

  // Call the appropriate method on the permission request
  auto* perm = it->second->request.get();
  if (perm) {
    switch (setting) {
      case CONTENT_SETTING_ALLOW:
        perm->PermissionGranted(std::monostate(), false);  // Remember decision
        break;
      case CONTENT_SETTING_SESSION_ONLY:
        perm->PermissionGranted(std::monostate(),
                                true);  // One-time/this session
        break;
      case CONTENT_SETTING_BLOCK:
        perm->PermissionDenied();
        break;
      case CONTENT_SETTING_ASK:
      case CONTENT_SETTING_DEFAULT:
        perm->Cancelled();
        break;
      default:
        perm->PermissionDenied();
        break;
    }
  }

  // Clean up
  pending_permission_requests_.erase(it);
  return true;
}

void SitePermissionsAPI::FirePermissionRequestEventForDeviceChooser(
    int tab_id,
    const std::string& request_id,
    const std::string& permission_type,
    const std::string& origin,
    bool allow_multiple_selection) {
  // VB-114658: Fire initial permission request event for device chooser with
  // deviceChooserPending=true. Devices will be sent separately via
  // FirePermissionDeviceChooserUpdate.

  // Store device chooser request for later response handling.
  auto pending_request = std::make_unique<PendingDeviceRequest>();
  pending_request->tab_id = tab_id;
  pending_request->permission_type = permission_type;
  pending_request->origin = origin;
  pending_request->allow_multiple_selection = allow_multiple_selection;
  pending_request->devices = base::ListValue();  // Initially empty.
  pending_device_requests_[request_id] = std::move(pending_request);

  // Fire initial event with deviceChooserPending=true, no devices yet.
  vivaldi::site_permissions::PermissionRequestEvent event;
  event.request_id = request_id;
  event.tab_id = tab_id;
  event.permission = permission_type;
  event.origin = origin;
  event.allow_multiple_selection = allow_multiple_selection;
  event.device_chooser_pending = true;
  // devices field is not set (will be populated via onDeviceChooserUpdate).

  ::vivaldi::BroadcastEvent(
      vivaldi::site_permissions::OnPermissionRequest::kEventName,
      vivaldi::site_permissions::OnPermissionRequest::Create(event), profile_);
}

void SitePermissionsAPI::FirePermissionDeviceChooserUpdate(
    int tab_id,
    const std::string& request_id,
    const base::ListValue& devices,
    bool is_complete) {
  // VB-114658: Fire device chooser update with newly discovered devices.

  // Update the stored request with new devices.
  auto it = pending_device_requests_.find(request_id);
  if (it != pending_device_requests_.end()) {
    it->second->devices = devices.Clone();
  }

  // Convert base::Value::List to DeviceInfo vector.
  std::vector<vivaldi::site_permissions::DeviceInfo> device_list;
  for (size_t i = 0; i < devices.size(); ++i) {
    const auto& device_value = devices[i];
    if (!device_value.is_dict()) {
      continue;
    }

    const auto& device_dict = device_value.GetDict();
    vivaldi::site_permissions::DeviceInfo device_info;

    // Extract index.
    auto index = device_dict.FindInt("index");
    if (index) {
      device_info.index = *index;
    }

    // Extract name.
    auto name = device_dict.FindString("name");
    if (name) {
      device_info.name = *name;
    }

    // Extract isPaired.
    auto is_paired = device_dict.FindBool("isPaired");
    if (is_paired) {
      device_info.is_paired = *is_paired;
    }

    device_list.push_back(std::move(device_info));
  }

  // Fire the device chooser update event.
  vivaldi::site_permissions::DeviceChooserUpdate update;
  update.request_id = request_id;
  update.devices = std::move(device_list);
  update.is_complete = is_complete;

  ::vivaldi::BroadcastEvent(
      vivaldi::site_permissions::OnDeviceChooserUpdate::kEventName,
      vivaldi::site_permissions::OnDeviceChooserUpdate::Create(update),
      profile_);

  // VB-114658: Don't erase pending_device_requests_ here even when enumeration
  // is complete. The request needs to stay alive until the user responds via
  // respondToPermissionRequest (which calls RespondToPermissionRequest).
  // Erasing here caused immediate auto-denial because the request became "not
  // found" when the user tried to respond, and the permission system
  // auto-denies unknown requests.
}

ExtensionFunction::ResponseAction
SitePermissionsGetAvailablePermissionsFunction::Run() {
  using vivaldi::site_permissions::PermissionOptions;
  using vivaldi::site_permissions::PermissionSetting;
  namespace Results =
      vivaldi::site_permissions::GetAvailablePermissions::Results;

  std::vector<PermissionOptions> permissions;

  content_settings::ContentSettingsRegistry* registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  for (const content_settings::ContentSettingsInfo* info : *registry) {
    auto website_info = info->website_settings_info();

    PermissionOptions permission_entry;

    permission_entry.permission =
        content_settings_helpers::ContentSettingsTypeToString(
            website_info->type());

    // Iterate all possible options (hardcoded)
    for (ContentSetting setting : kAllContentSettings) {
      if (!info->IsSettingValid(setting)) {
        continue;
      }
      auto jsetting = toPermissionSetting(setting);
      if (jsetting != PermissionSetting::kNone) {
        permission_entry.options.push_back(jsetting);
      }
    }

    // Only call GetInitialDefaultSetting() if the initial_default_value is an
    // integer. Some settings have none or dict values.
    if (website_info->initial_default_value().is_int()) {
      auto jsetting = toPermissionSetting(info->GetInitialDefaultSetting());
      if (jsetting != PermissionSetting::kNone) {
        permission_entry.default_ = jsetting;
      } else {
        permission_entry.default_ = PermissionSetting::kDefault;
      }
    } else {
      permission_entry.default_ = PermissionSetting::kDefault;
    }

    // Indicate if this permission requires a secondary origin
    permission_entry.requires_secondary_origin =
        RequiresSecondaryOrigin(website_info->scoping_type());

    permissions.push_back(std::move(permission_entry));
  }

  return RespondNow(ArgumentList(Results::Create(std::move(permissions))));
}

ExtensionFunction::ResponseAction
SitePermissionsGetOverriddenSitesFunction::Run() {
  using vivaldi::site_permissions::SitePermissionInfo;

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile)
    return RespondNow(Error("Could not get profile"));

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map)
    return RespondNow(Error("Could not access content settings"));

  // Map of origin -> count of overridden permissions
  base::flat_map<std::string, int> origin_counts;

  const content_settings::ContentSettingsRegistry* registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  for (const auto* info : *registry) {
    auto website_info = info->website_settings_info();
    ContentSettingsType type = website_info->type();

    auto entries = settings_map->GetSettingsForOneType(type);

    for (const auto& entry : entries) {
      ContentSetting setting =
          content_settings::ValueToContentSetting(entry.setting_value);
      if (setting == CONTENT_SETTING_DEFAULT)
        continue;

      std::string origin = entry.primary_pattern.ToString();

      // filter out extension permissions for now... it even shows vivaldi ext
      // and does NOT resolve the ext name.
      GURL url(origin);
      if (url.SchemeIs(extensions::kExtensionScheme))
        continue;

      // For wildcard pattern, compare to initial default (only count actual
      // global overrides). For specific sites, count all non-default settings
      // (they're all overrides).
      bool is_wildcard =
          (entry.primary_pattern == ContentSettingsPattern::Wildcard());
      if (is_wildcard && setting == info->GetInitialDefaultSetting())
        continue;

      if (!origin.empty()) {
        origin_counts[origin]++;
      }
    }
  }

  // Convert map to vector of SitePermissionInfo objects
  std::vector<SitePermissionInfo> sites;
  for (const auto& [origin, count] : origin_counts) {
    SitePermissionInfo site_info;
    site_info.origin = origin;
    site_info.count = count;
    sites.push_back(std::move(site_info));
  }

  return RespondNow(ArgumentList(
      vivaldi::site_permissions::GetOverriddenSites::Results::Create(sites)));
}

ExtensionFunction::ResponseAction
SitePermissionsGetOverridesForSiteFunction::Run() {
  using vivaldi::site_permissions::GetOverridesForSite::Params;
  namespace Results = vivaldi::site_permissions::GetOverridesForSite::Results;
  using vivaldi::site_permissions::Permission;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  const std::string& origin_str = params->origin;

  // Parse the origin as a URL for matching against content settings patterns.
  GURL origin_url(origin_str);

  // Also parse as a pattern to handle pattern queries like "*" or
  // "[*.]example.com" (e.g. from onPermissionChanged events).
  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromString(origin_str);

  bool is_url_query = origin_url.is_valid();
  bool is_pattern_query = !is_url_query && primary_pattern.IsValid();

  if (!is_url_query && !is_pattern_query) {
    return RespondNow(Error("Invalid origin."));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile)
    return RespondNow(Error("Could not get profile."));

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map)
    return RespondNow(Error("Could not access content settings."));

  const auto* registry =
      content_settings::ContentSettingsRegistry::GetInstance();
  std::vector<Permission> result;

  for (const auto* info : *registry) {
    auto website_info = info->website_settings_info();
    ContentSettingsType type = website_info->type();

    // Get all settings for this type
    auto entries = settings_map->GetSettingsForOneType(type);

    for (const auto& entry : entries) {
      if (is_url_query) {
        // For URL queries, include any pattern that matches the origin URL
        // (global overrides, wildcard subdomains, exact matches, etc.)
        // Also include entries where the secondary pattern matches the URL
        // (e.g. storage-access where the current site is the embedder).
        bool primary_matches = entry.primary_pattern.Matches(origin_url);
        bool secondary_matches =
            entry.secondary_pattern != ContentSettingsPattern::Wildcard() &&
            entry.secondary_pattern.Matches(origin_url);
        if (!primary_matches && !secondary_matches)
          continue;
      } else {
        // For pattern queries (e.g. "*", "[*.]example.com"), match exactly.
        if (entry.primary_pattern != primary_pattern)
          continue;
      }

      ContentSetting setting =
          content_settings::ValueToContentSetting(entry.setting_value);
      if (setting == CONTENT_SETTING_DEFAULT)
        continue;

      // Filter out wildcard entries that match the initial default — these
      // are not overrides.
      if (entry.primary_pattern == ContentSettingsPattern::Wildcard() &&
          setting == info->GetInitialDefaultSetting())
        continue;

      Permission permission;
      permission.origin = entry.primary_pattern.ToString();

      // Include secondary origin if it's not a wildcard
      if (website_info->SupportsSecondaryPattern() &&
          entry.secondary_pattern != ContentSettingsPattern::Wildcard()) {
        permission.secondary_origin = entry.secondary_pattern.ToString();
      }

      permission.permission =
          content_settings_helpers::ContentSettingsTypeToString(type);
      permission.setting = toPermissionSetting(setting);

      result.push_back(std::move(permission));
    }
  }

  return RespondNow(ArgumentList(Results::Create(result)));
}

ExtensionFunction::ResponseAction
SitePermissionsGetOverridesForPatternFunction::Run() {
  using vivaldi::site_permissions::GetOverridesForPattern::Params;
  namespace Results = vivaldi::site_permissions::GetOverridesForSite::Results;
  using vivaldi::site_permissions::Permission;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  const std::string& pattern_str = params->pattern;

  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromString(pattern_str);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile)
    return RespondNow(Error("Could not get profile."));

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map)
    return RespondNow(Error("Could not access content settings."));

  const auto* registry =
      content_settings::ContentSettingsRegistry::GetInstance();
  std::vector<Permission> result;

  for (const auto* info : *registry) {
    auto website_info = info->website_settings_info();
    ContentSettingsType type = website_info->type();

    // Get all settings for this type
    auto entries = settings_map->GetSettingsForOneType(type);

    for (const auto& entry : entries) {
      // For pattern queries (e.g. "*", "[*.]example.com"), match exactly.
      if (entry.primary_pattern != primary_pattern) {
        continue;
      }

      ContentSetting setting =
          content_settings::ValueToContentSetting(entry.setting_value);
      if (setting == CONTENT_SETTING_DEFAULT)
        continue;

      Permission permission;
      permission.origin = entry.primary_pattern.ToString();

      // Include secondary origin if it's not a wildcard
      if (website_info->SupportsSecondaryPattern() &&
          entry.secondary_pattern != ContentSettingsPattern::Wildcard()) {
        permission.secondary_origin = entry.secondary_pattern.ToString();
      }

      permission.permission =
          content_settings_helpers::ContentSettingsTypeToString(type);
      permission.setting = toPermissionSetting(setting);

      result.push_back(std::move(permission));
    }
  }

  return RespondNow(ArgumentList(Results::Create(result)));
}

ExtensionFunction::ResponseAction
SitePermissionsSetSitePermissionFunction::Run() {
  using vivaldi::site_permissions::SetSitePermission::Params;
  namespace Results = vivaldi::site_permissions::SetSitePermission::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  // Parse primary origin as a pattern (supports wildcards like "*" or
  // "[*.]example.com")
  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromString(params->origin);
  if (!primary_pattern.IsValid()) {
    return RespondNow(Error("Invalid origin pattern."));
  }

  // Parse secondary origin as a pattern
  // Empty string is treated as wildcard for convenience
  std::string secondary_origin_str = params->secondary_origin;
  if (secondary_origin_str.empty()) {
    secondary_origin_str = "*";
  }
  ContentSettingsPattern secondary_pattern =
      ContentSettingsPattern::FromString(secondary_origin_str);
  if (!secondary_pattern.IsValid()) {
    return RespondNow(Error("Invalid secondary origin pattern."));
  }

  // Convert permission string to content setting type
  std::optional<ContentSettingsType> type_opt =
      content_settings_helpers::StringToContentSettingsType(params->permission);
  if (!type_opt) {
    return RespondNow(Error("Unknown permission type: " + params->permission));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile) {
    return RespondNow(Error("Could not get profile."));
  }

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map) {
    return RespondNow(Error("Could not access content settings."));
  }

  content_settings::ContentSettingsRegistry* registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  auto info = registry->Get(*type_opt);
  if (!info) {
    return RespondNow(Error("Could not retrieve the content setting info."));
  }

  auto website_info = info->website_settings_info();

  // Convert setting first, so we can check if it's DEFAULT in validation
  std::optional<ContentSetting> setting_opt =
      fromPermissionSetting(params->setting);
  if (!setting_opt)
    return RespondNow(Error("Invalid setting value"));

  // Validate secondary origin based on permission type requirements
  if (RequiresSecondaryOrigin(website_info->scoping_type())) {
    // Allow wildcard secondary pattern in two cases:
    // 1. Setting to DEFAULT (erases the override)
    // 2. Both patterns are wildcards (setting global default, like Chrome's
    // SetDefaultContentSetting)
    bool is_setting_default = (*setting_opt == CONTENT_SETTING_DEFAULT);
    bool is_global_default =
        (primary_pattern == ContentSettingsPattern::Wildcard() &&
         secondary_pattern == ContentSettingsPattern::Wildcard());

    if (!is_setting_default && !is_global_default) {
      // For non-default settings that aren't global, require a specific
      // secondary origin
      if (params->secondary_origin.empty() ||
          secondary_pattern == ContentSettingsPattern::Wildcard()) {
        return RespondNow(
            Error("Secondary origin is required for permission type: " +
                  params->permission));
      }
    }
  }

  if (!info->IsSettingValid(*setting_opt) &&
      (*setting_opt != CONTENT_SETTING_DEFAULT)) {
    return RespondNow(Error("Invalid setting type for given permission type."));
  }

  // Use SetContentSettingCustomScope to set the exact patterns
  settings_map->SetContentSettingCustomScope(primary_pattern, secondary_pattern,
                                             *type_opt, *setting_opt);

  ShowReloadInfoBarForPattern(primary_pattern, profile);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
SitePermissionsResetSitePermissionsFunction::Run() {
  using vivaldi::site_permissions::ResetSitePermissions::Params;
  namespace Results = vivaldi::site_permissions::GetOverridesForSite::Results;
  using vivaldi::site_permissions::Permission;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  const std::string& origin_str = params->origin;

  // Parse the origin string as a pattern (not just a URL)
  // This allows resetting patterns like "[*.]example.com" or "*"
  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromString(origin_str);

  if (!primary_pattern.IsValid()) {
    return RespondNow(Error("Invalid origin pattern."));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile) {
    return RespondNow(Error("Could not get profile."));
  }

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map) {
    return RespondNow(Error("Could not access content settings."));
  }

  const auto* registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  for (const auto* info : *registry) {
    auto website_info = info->website_settings_info();
    ContentSettingsType type = website_info->type();

    // Get all settings for this type
    auto entries = settings_map->GetSettingsForOneType(type);

    for (const auto& entry : entries) {
      // Only reset entries where primary pattern exactly matches
      // This includes all secondary origins for that pattern
      if (entry.primary_pattern != primary_pattern)
        continue;

      ContentSetting setting =
          content_settings::ValueToContentSetting(entry.setting_value);
      if (setting == CONTENT_SETTING_DEFAULT)
        continue;

      // Reset this specific permission by setting it to default
      // This will use the correct patterns (primary and secondary)
      settings_map->SetContentSettingCustomScope(entry.primary_pattern,
                                                 entry.secondary_pattern, type,
                                                 CONTENT_SETTING_DEFAULT);
    }
  }

  // Also revoke any device grants (USB, Serial, HID, Bluetooth) for this
  // origin.
  GURL origin_url(origin_str);
  if (origin_url.is_valid()) {
    auto origin = url::Origin::Create(origin_url);
    bool had_grants = false;
    for (const char* type : kAllDeviceTypes) {
      auto* context = GetChooserContextForType(profile, type);
      if (!context) {
        continue;
      }
      auto objects = context->GetGrantedObjects(origin);
      for (const auto& object : objects) {
        context->RevokeObjectPermission(origin, object->value);
        had_grants = true;
      }
    }
    if (had_grants) {
      SitePermissionsAPI::SendDeviceGrantChanged(browser_context(), origin_str);
    }
  }

  ShowReloadInfoBarForPattern(primary_pattern, profile);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
SitePermissionsRespondToPermissionRequestFunction::Run() {
  using vivaldi::site_permissions::RespondToPermissionRequest::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  SitePermissionsAPI* api =
      SitePermissionsAPI::FromBrowserContext(browser_context());
  if (!api) {
    return RespondNow(Error("API not available"));
  }

  // Convert PermissionRequestResponse enum to ContentSetting
  ContentSetting setting;
  switch (params->setting) {
    case vivaldi::site_permissions::PermissionRequestResponse::kAllow:
      setting = CONTENT_SETTING_ALLOW;
      break;
    case vivaldi::site_permissions::PermissionRequestResponse::kAllowThisTime:
      setting = CONTENT_SETTING_SESSION_ONLY;
      break;
    case vivaldi::site_permissions::PermissionRequestResponse::kDeny:
      setting = CONTENT_SETTING_BLOCK;
      break;
    case vivaldi::site_permissions::PermissionRequestResponse::kAsk:
      setting = CONTENT_SETTING_ASK;
      break;
    default:
      return RespondNow(Error("Invalid setting value"));
  }

  // VB-114658: Extract device indices if provided (for device chooser requests)
  std::vector<int> device_indices;
  if (params->device_indices) {
    for (const auto& index : *params->device_indices) {
      device_indices.push_back(index);
    }
  }

  bool success = api->RespondToPermissionRequest(params->request_id, setting,
                                                 device_indices);

  if (!success) {
    return RespondNow(Error("Request not found or already completed"));
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
SitePermissionsGetAllowedDevicesFunction::Run() {
  using vivaldi::site_permissions::GetAllowedDevices::Params;
  namespace Results = vivaldi::site_permissions::GetAllowedDevices::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  std::vector<std::string> device_names;
  auto profile = Profile::FromBrowserContext(browser_context());
  GURL origin_url(params->origin);

  auto* context = GetChooserContextForType(profile, params->permission_type);
  if (context) {
    auto objects = context->GetGrantedObjects(url::Origin::Create(origin_url));
    std::set<std::string> seen;
    for (const auto& object : objects) {
      std::string name =
          base::UTF16ToUTF8(context->GetObjectDisplayName(object->value));
      // Deduplicate (HID can have multiple entries for the same device).
      if (seen.insert(name).second) {
        device_names.push_back(std::move(name));
      }
    }
  }

  return RespondNow(ArgumentList(Results::Create(std::move(device_names))));
}

ExtensionFunction::ResponseAction
SitePermissionsGetDeviceGrantsFunction::Run() {
  using vivaldi::site_permissions::GetDeviceGrants::Params;
  namespace Results = vivaldi::site_permissions::GetDeviceGrants::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto profile = Profile::FromBrowserContext(browser_context());
  GURL origin_url(params->origin);
  auto origin = url::Origin::Create(origin_url);

  std::vector<vivaldi::site_permissions::DeviceGrant> grants;

  // Collect grants from specified type or all device types.
  auto collect_grants = [&](const std::string& perm_type) {
    auto* context = GetChooserContextForType(profile, perm_type);
    if (!context) {
      return;
    }
    auto objects = context->GetGrantedObjects(origin);
    for (const auto& object : objects) {
      vivaldi::site_permissions::DeviceGrant grant;
      grant.name =
          base::UTF16ToUTF8(context->GetObjectDisplayName(object->value));
      grant.key = GetSafeKeyForObject(context, object->value);
      grant.origin = params->origin;
      grant.permission_type = perm_type;
      grant.is_policy =
          object->source == content_settings::SettingSource::kPolicy;
      grants.push_back(std::move(grant));
    }
  };

  if (params->permission_type) {
    collect_grants(*params->permission_type);
  } else {
    for (const char* type : kAllDeviceTypes) {
      collect_grants(type);
    }
  }

  return RespondNow(ArgumentList(Results::Create(std::move(grants))));
}

ExtensionFunction::ResponseAction
SitePermissionsRevokeDeviceGrantFunction::Run() {
  using vivaldi::site_permissions::RevokeDeviceGrant::Params;
  namespace Results = vivaldi::site_permissions::RevokeDeviceGrant::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto profile = Profile::FromBrowserContext(browser_context());
  auto* context = GetChooserContextForType(profile, params->permission_type);
  if (!context) {
    return RespondNow(ArgumentList(Results::Create(false)));
  }

  auto origin = url::Origin::Create(GURL(params->origin));

  // Find the granted object matching the key, then revoke by object.
  // This is necessary because UsbChooserContext overrides
  // RevokeObjectPermission to handle ephemeral (GUID-based) devices that aren't
  // in the base class's object store.
  auto objects = context->GetGrantedObjects(origin);
  for (const auto& object : objects) {
    if (GetSafeKeyForObject(context, object->value) == params->key) {
      context->RevokeObjectPermission(origin, object->value);
      SitePermissionsAPI::SendDeviceGrantChanged(browser_context(),
                                                 params->origin);
      return RespondNow(ArgumentList(Results::Create(true)));
    }
  }
  return RespondNow(ArgumentList(Results::Create(false)));
}

ExtensionFunction::ResponseAction
SitePermissionsGetDeviceGrantSitesFunction::Run() {
  using vivaldi::site_permissions::GetDeviceGrantSites::Params;
  namespace Results = vivaldi::site_permissions::GetDeviceGrantSites::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto profile = Profile::FromBrowserContext(browser_context());
  std::set<std::string> origins;

  // Collect origins from specified type or all device types.
  auto collect_origins = [&](const std::string& perm_type) {
    auto* context = GetChooserContextForType(profile, perm_type);
    if (!context) {
      return;
    }
    auto objects = context->GetAllGrantedObjects();
    for (const auto& object : objects) {
      origins.insert(object->origin.spec());
    }
  };

  if (params->permission_type) {
    collect_origins(*params->permission_type);
  } else {
    for (const char* type : kAllDeviceTypes) {
      collect_origins(type);
    }
  }

  std::vector<std::string> result(origins.begin(), origins.end());
  return RespondNow(ArgumentList(Results::Create(std::move(result))));
}

ExtensionFunction::ResponseAction
SitePermissionsGetSecurityInfoFunction::Run() {
  using vivaldi::site_permissions::GetSecurityInfo::Params;
  namespace Results = vivaldi::site_permissions::GetSecurityInfo::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  // Get WebContents from tab ID.
  content::WebContents* web_contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(params->tab_id, browser_context(), true,
                                    &web_contents)) {
    return RespondNow(Error("Tab not found"));
  }

  if (!web_contents) {
    return RespondNow(Error("WebContents not available"));
  }

  vivaldi::site_permissions::SecurityInfo security_info;

  // Get security state from the tab helper.
  auto* helper = SecurityStateTabHelper::FromWebContents(web_contents);
  if (!helper) {
    // Tab doesn't have security state yet, return defaults.
    security_info.connection_status =
        vivaldi::site_permissions::ConnectionStatus::kUnencrypted;
    security_info.identity_status =
        vivaldi::site_permissions::IdentityStatus::kNoCert;
    security_info.issuer_name = "";
    security_info.is_secure = false;
    return RespondNow(ArgumentList(Results::Create(std::move(security_info))));
  }

  auto visible_security_state = helper->GetVisibleSecurityState();

  // Map connection status based on certificate and cert_status.
  if (visible_security_state->certificate &&
      !(visible_security_state->cert_status & net::CERT_STATUS_ALL_ERRORS)) {
    security_info.connection_status =
        vivaldi::site_permissions::ConnectionStatus::kEncrypted;
    security_info.is_secure = true;
  } else if (!visible_security_state->certificate) {
    security_info.connection_status =
        vivaldi::site_permissions::ConnectionStatus::kUnencrypted;
    security_info.is_secure = false;
  } else {
    security_info.connection_status =
        vivaldi::site_permissions::ConnectionStatus::kError;
    security_info.is_secure = false;
  }

  // Map identity status based on certificate.
  if (!visible_security_state->certificate) {
    security_info.identity_status =
        vivaldi::site_permissions::IdentityStatus::kNoCert;
  } else if (visible_security_state->two_qwac) {
    security_info.identity_status =
        vivaldi::site_permissions::IdentityStatus::kQwacCert;
  } else if (visible_security_state->cert_status & net::CERT_STATUS_IS_EV) {
    security_info.identity_status =
        vivaldi::site_permissions::IdentityStatus::kEvCert;
  } else if (visible_security_state->cert_status &
             net::CERT_STATUS_ALL_ERRORS) {
    security_info.identity_status =
        vivaldi::site_permissions::IdentityStatus::kError;
  } else {
    security_info.identity_status =
        vivaldi::site_permissions::IdentityStatus::kCert;
  }

  // Extract issuer name from certificate.
  security_info.issuer_name = "";
  if (visible_security_state->certificate) {
    security_info.issuer_name =
        visible_security_state->certificate->issuer().GetDisplayName();
  }

  auto malicious_state = helper->GetMaliciousContentStatus();
  security_info.safe_browsing_status = toSafeBrowsingStatus(malicious_state);

  return RespondNow(ArgumentList(Results::Create(std::move(security_info))));
}

ExtensionFunction::ResponseAction SitePermissionsGetSiteDataFunction::Run() {
  using vivaldi::site_permissions::GetSiteData::Params;
  namespace Results = vivaldi::site_permissions::GetSiteData::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  // Get WebContents from tab ID.
  content::WebContents* web_contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(params->tab_id, browser_context(), true,
                                    &web_contents)) {
    return RespondNow(Error("Tab not found"));
  }

  if (!web_contents) {
    return RespondNow(Error("WebContents not available"));
  }

  using vivaldi::site_permissions::CookieSetting;

  vivaldi::site_permissions::SiteDataSummary site_data;
  site_data.cookie_count = 0;
  site_data.storage_count = 0;
  site_data.total_size = 0;
  site_data.third_party_cookies_allowed = true;
  site_data.cookie_setting = CookieSetting::kAllow;  // Default.

  // Get PageSpecificContentSettings which tracks site data.
  auto* pscs = content_settings::PageSpecificContentSettings::GetForFrame(
      web_contents->GetPrimaryMainFrame());

  if (!pscs) {
    return RespondNow(ArgumentList(Results::Create(std::move(site_data))));
  }

  // Get the browsing data models (allowed and blocked).
  auto* allowed_model = pscs->allowed_browsing_data_model();
  auto* blocked_model = pscs->blocked_browsing_data_model();

  // Helper lambda to count data entries.
  auto count_entries = [&site_data](BrowsingDataModel* model) {
    if (!model)
      return;
    for (auto entry : *model) {
      const auto& storage_types = entry.data_details->storage_types;
      if (storage_types.Has(BrowsingDataModel::StorageType::kCookie)) {
        site_data.cookie_count++;
      }
      if (storage_types.Has(BrowsingDataModel::StorageType::kLocalStorage) ||
          storage_types.Has(BrowsingDataModel::StorageType::kSessionStorage) ||
          storage_types.Has(BrowsingDataModel::StorageType::kQuotaStorage)) {
        site_data.storage_count++;
      }
      site_data.total_size += entry.data_details->storage_size;
    }
  };

  // Count cookies and storage in allowed and blocked models.
  count_entries(allowed_model);
  count_entries(blocked_model);

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  scoped_refptr<content_settings::CookieSettings> cookie_settings =
      CookieSettingsFactory::GetForProfile(profile);

  if (cookie_settings) {
    bool globally_blocked = cookie_settings->ShouldBlockThirdPartyCookies();
    site_data.third_party_cookies_allowed = !globally_blocked;

    if (globally_blocked) {
      // Check whether a per-site exception overrides the global block.
      content_settings::SettingInfo bypass_info;
      GURL first_party_url = web_contents->GetLastCommittedURL();
      if (cookie_settings->IsThirdPartyAccessAllowed(first_party_url,
                                                     &bypass_info)) {
        site_data.third_party_cookies_bypass_active = true;
        if (!bypass_info.metadata.expiration().is_null()) {
          site_data.third_party_cookies_bypass_expiration = static_cast<double>(
              bypass_info.metadata.expiration().InMillisecondsSinceUnixEpoch());
        }
      }
    }
  } else {
    site_data.third_party_cookies_allowed = true;
  }

  // Get the cookie setting for this site.
  if (cookie_settings) {
    GURL site_url = web_contents->GetLastCommittedURL();
    ContentSetting setting = cookie_settings->GetCookieSetting(
        site_url, net::SiteForCookies::FromUrl(site_url), site_url,
        net::CookieSettingOverrides());
    if (setting == CONTENT_SETTING_BLOCK) {
      site_data.cookie_setting = CookieSetting::kBlock;
    } else if (setting == CONTENT_SETTING_SESSION_ONLY) {
      site_data.cookie_setting = CookieSetting::kSessionOnly;
    } else {
      site_data.cookie_setting = CookieSetting::kAllow;
    }
  }

  return RespondNow(ArgumentList(Results::Create(std::move(site_data))));
}

ExtensionFunction::ResponseAction SitePermissionsSetSiteDataFunction::Run() {
  using vivaldi::site_permissions::SetSiteData::Params;
  namespace Results = vivaldi::site_permissions::SetSiteData::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile) {
    return RespondNow(Error("Profile not available"));
  }

  GURL origin(params->origin);
  if (!origin.is_valid()) {
    return RespondNow(Error("Invalid origin URL"));
  }

  scoped_refptr<content_settings::CookieSettings> cookie_settings =
      CookieSettingsFactory::GetForProfile(profile);

  if (!cookie_settings) {
    return RespondNow(Error("Cookie settings not available"));
  }

  // Handle third-party cookie settings if provided.
  if (params->settings.third_party_cookies_allowed.has_value()) {
    if (params->settings.third_party_cookies_allowed.value()) {
      // Enabling bypass: clear any older origin-scoped BLOCK exception that
      // would shadow the new schemeful-site-scoped ALLOW from UserBypass.
      // ResetThirdPartyCookieSetting is only safe when a per-site rule exists
      // — without one it falls through to the global * -> * rule and resets
      // it to DEFAULT, wiping the user's global cookie setting.  So guard by
      // checking the secondary pattern is not the all-hosts wildcard.
      HostContentSettingsMap* hcsm =
          HostContentSettingsMapFactory::GetForProfile(profile);
      if (hcsm) {
        content_settings::SettingInfo existing_info;
        hcsm->GetContentSetting(GURL(), origin, ContentSettingsType::COOKIES,
                                &existing_info);
        if (!existing_info.secondary_pattern.MatchesAllHosts()) {
          cookie_settings->ResetThirdPartyCookieSetting(origin);
        }
      }
      // Grant a temporary (90-day) user bypass exception, site-scoped.
      cookie_settings->SetCookieSettingForUserBypass(origin);
    } else {
      // Disabling bypass: the UI only reaches here when a bypass is active, so
      // a per-site rule always exists and Reset is safe (matches Chrome's
      // flow).
      cookie_settings->ResetThirdPartyCookieSetting(origin);
    }
  }

  return RespondNow(ArgumentList(Results::Create()));
}

ExtensionFunction::ResponseAction
SitePermissionsShowCertificateDialogFunction::Run() {
  using vivaldi::site_permissions::ShowCertificateDialog::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  // Get WebContents from tab ID.
  content::WebContents* web_contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(params->tab_id, browser_context(), true,
                                    &web_contents)) {
    return RespondNow(Error("Tab not found"));
  }

  if (!web_contents) {
    return RespondNow(Error("WebContents not available"));
  }

  // Get the certificate from security state.
  auto* helper = SecurityStateTabHelper::FromWebContents(web_contents);
  if (!helper) {
    return RespondNow(Error("No security information available"));
  }

  auto visible_security_state = helper->GetVisibleSecurityState();
  if (!visible_security_state->certificate) {
    return RespondNow(Error("No certificate available for this page"));
  }

  // Use ChromePageInfoDelegate to show the certificate dialog.
  auto delegate = std::make_unique<ChromePageInfoDelegate>(web_contents);
  delegate->OpenCertificateDialog(visible_security_state->certificate.get());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
SitePermissionsShowSiteDataDialogFunction::Run() {
  using vivaldi::site_permissions::ShowSiteDataDialog::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  // Get WebContents from tab ID.
  content::WebContents* web_contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(params->tab_id, browser_context(), true,
                                    &web_contents)) {
    return RespondNow(Error("Tab not found"));
  }

  if (!web_contents) {
    return RespondNow(Error("WebContents not available"));
  }

  // Show the on-device site data dialog via TabDialogs.
  TabDialogs::CreateForWebContents(web_contents);
  auto* tab_dialogs = TabDialogs::FromWebContents(web_contents);
  if (!tab_dialogs) {
    return RespondNow(Error("TabDialogs not available"));
  }

  tab_dialogs->ShowCollectedCookies();

  return RespondNow(NoArguments());
}

}  // namespace extensions
