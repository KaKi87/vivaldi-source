// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#include "permissions/vivaldi_permission_handler_impl.h"

#include "components/content_settings/core/common/content_settings.h"
#include "extensions/api/site_permissions/site_permissions_api.h"

#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

#include "components/permissions/permission_request.h"
#include "components/permissions/request_type.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"
#include "extensions/vivaldi_browser_component_wrapper.h"

#include "components/custom_handlers/protocol_handler.h"
#include "components/custom_handlers/register_protocol_handler_permission_request.h"
#include "components/permissions/chooser_controller.h"

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/location.h"

#include "app/vivaldi_apptools.h"

// Device chooser context and factory headers for getting granted devices.
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/usb/usb_chooser_context.h"
#include "chrome/browser/usb/usb_chooser_context_factory.h"
#include "chrome/browser/serial/serial_chooser_context.h"
#include "chrome/browser/serial/serial_chooser_context_factory.h"
#include "chrome/browser/hid/hid_chooser_context.h"
#include "chrome/browser/hid/hid_chooser_context_factory.h"
#include "components/permissions/contexts/bluetooth_chooser_context.h"
#include "chrome/browser/bluetooth/bluetooth_chooser_context_factory.h"

#include <typeinfo>

namespace vivaldi {
namespace permissions {

// View for device chooser controllers that handles async device loading.
// Device choosers (e.g., SerialChooserController, USBChooserController) do
// async device enumeration and notify the view when options become available.
// We send device updates incrementally as they arrive.
class DeviceEnumerationView : public ::permissions::ChooserController::View {
 public:
  // Callback type for when a device is added.
  using OnDeviceAddedCallback =
      base::RepeatingCallback<void(const base::DictValue& device)>;
  // Callback type for when enumeration is complete.
  using OnEnumerationCompleteCallback = base::OnceCallback<void()>;

  DeviceEnumerationView(::permissions::ChooserController* controller,
                        OnDeviceAddedCallback on_device_added,
                        OnEnumerationCompleteCallback on_complete)
      : controller_(controller),
        on_device_added_(on_device_added),
        on_complete_(std::move(on_complete)),
        refreshing_(false),
        last_sent_index_(0),
        sent_complete_(false) {}
  ~DeviceEnumerationView() override = default;

  base::WeakPtr<DeviceEnumerationView> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  // Probe the controller state to handle missed synchronous signals.
  void ProbeInitialState() {
    if (controller_->NumOptions() > 0) {
      // If we already have devices, it means we missed OnOptionAdded or
      // OnOptionsInitialized. Treat this as a signal.
      SendNewDevices();

      // If we have items we can safely assume initial discovery finished
      // before we attached. The UI can now display these devices.
      SignalComplete();
    }
  }

  // Send any new devices that have been added since last check.
  void SendNewDevices() {
    size_t current_count = controller_->NumOptions();
    if (!on_device_added_) {
      return;
    }

    if (current_count <= last_sent_index_) {
      return;
    }

    for (size_t i = last_sent_index_; i < current_count; ++i) {
      base::DictValue device;
      device.Set("index", static_cast<int>(i));
      device.Set("name", base::UTF16ToUTF8(controller_->GetOption(i)));
      device.Set("isPaired", controller_->IsPaired(i));
      on_device_added_.Run(device);
    }
    last_sent_index_ = current_count;
  }

  // Signal that enumeration is complete.
  void SignalComplete() {
    if (!sent_complete_ && on_complete_) {
      sent_complete_ = true;
      std::move(on_complete_).Run();
    }
  }

  // ChooserController::View overrides:
  void OnOptionsInitialized() override {
    // Initial discovery phase is done.
    SendNewDevices();
    SignalComplete();
  }

  void OnOptionAdded(size_t index) override {
    // New device arrived.
    SendNewDevices();
  }

  void OnOptionRemoved(size_t index) override {}

  void OnOptionUpdated(size_t index) override {
    if (index < controller_->NumOptions()) {
      base::DictValue device;
      device.Set("index", static_cast<int>(index));
      device.Set("name", base::UTF16ToUTF8(controller_->GetOption(index)));
      device.Set("isPaired", controller_->IsPaired(index));
      on_device_added_.Run(device);
    }
  }

  void OnAdapterEnabledChanged(bool enabled) override {}

  void OnAdapterAuthorizationChanged(bool authorized) override {}

  void OnRefreshStateChanged(bool refreshing) override {
    if (refreshing_ && !refreshing) {
      SendNewDevices();
      SignalComplete();
    }
    refreshing_ = refreshing;
  }

 private:
  raw_ptr<::permissions::ChooserController> controller_;
  OnDeviceAddedCallback on_device_added_;
  OnEnumerationCompleteCallback on_complete_;
  bool refreshing_ = false;
  size_t last_sent_index_ = 0;
  bool sent_complete_ = false;

  base::WeakPtrFactory<DeviceEnumerationView> weak_ptr_factory_{this};
};


WEB_CONTENTS_USER_DATA_KEY_IMPL(PermissionNavigationObserver);

PermissionNavigationObserver::PermissionNavigationObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<PermissionNavigationObserver>(*web_contents) {
}

void PermissionNavigationObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Same logic as PermissionRequestManager - cleanup on primary main frame
  // cross-document navigation.
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // Get tab ID and cleanup permission state.
  int tab_id = VivaldiBrowserComponentWrapper::GetInstance()
      ->GetTabId(web_contents());
  if (tab_id < 0) {
    return;
  }

  // Cleanup pending requests and fire event to JS.
  extensions::SitePermissionsAPI* api =
      extensions::SitePermissionsAPI::FromBrowserContext(
          web_contents()->GetBrowserContext());
  if (api) {
    api->CleanupRequestsForTab(tab_id);
  }

  // Also clean up any pending device choosers for this tab.
  ::vivaldi::permissions::VivaldiPermissionHandlerImpl* handler =
      ::vivaldi::permissions::VivaldiPermissionHandlerImpl::GetInstance();
  if (handler) {
    handler->CleanupPendingChoosersForTab(tab_id);
  }
}

// Extract granted device names for a given permission type and origin.
std::vector<std::string> ExtractGrantedDeviceNames(
    Profile* profile,
    ContentSettingsType type,
    const url::Origin& origin) {
  std::vector<std::string> names;
  auto get_devices = [&](auto* ctx, const char* field) {
    if (!ctx) return;
    for (const auto& obj : ctx->GetGrantedObjects(origin)) {
      if (!obj) continue;
      auto val = obj->value.FindString(field);
      if (val && !val->empty()) names.push_back(*val);
    }
  };

  switch (type) {
    case ContentSettingsType::USB_CHOOSER_DATA:
      get_devices(UsbChooserContextFactory::GetForProfile(profile), "name");
      break;
    case ContentSettingsType::SERIAL_CHOOSER_DATA:
      get_devices(SerialChooserContextFactory::GetForProfile(profile), "display_name");
      break;
    case ContentSettingsType::HID_CHOOSER_DATA:
      get_devices(HidChooserContextFactory::GetForProfile(profile), "name");
      break;
    case ContentSettingsType::BLUETOOTH_CHOOSER_DATA:
      get_devices(BluetoothChooserContextFactory::GetForProfile(profile), "name");
      break;
    default:
      break;
  }
  return names;
}

// static
VivaldiPermissionHandlerImpl* VivaldiPermissionHandlerImpl::GetInstance() {
  static base::NoDestructor<VivaldiPermissionHandlerImpl> instance;
  return instance.get();
}

void VivaldiPermissionHandlerImpl::NotifyPermissionSet(
    const ::permissions::PermissionRequestID& id,
    ContentSettingsType type,
    ContentSetting setting) {
  if (setting != CONTENT_SETTING_ALLOW && setting != CONTENT_SETTING_BLOCK)
    return;

  auto frame_id = id.global_render_frame_host_id();
  auto* render_frame_host = content::RenderFrameHost::FromID(frame_id);
  if (!render_frame_host) return;

  auto* web_contents = content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) return;

  // Ensure navigation observer is attached so reload cleanup works correctly.
  PermissionNavigationObserver::CreateForWebContents(web_contents);

  if (type == ContentSettingsType::CLIPBOARD_SANITIZED_WRITE) return;

  int tab_id = VivaldiBrowserComponentWrapper::GetInstance()->GetTabId(web_contents);
  if (tab_id < 0) return;

  url::Origin origin = render_frame_host->GetLastCommittedOrigin();
  Profile* profile = Profile::FromBrowserContext(web_contents->GetBrowserContext());
  auto device_names = setting == CONTENT_SETTING_ALLOW && profile
                          ? ExtractGrantedDeviceNames(profile, type, origin)
                          : std::vector<std::string>();

  extensions::SitePermissionsAPI::FirePermissionAccessedEvent(
      web_contents->GetBrowserContext(),
      tab_id,
      type,
      origin.GetURL().spec(),
      setting,
      device_names);
}

bool VivaldiPermissionHandlerImpl::HandlePermissionRequest(
    const content::GlobalRenderFrameHostId& source_frame_id,
    std::unique_ptr<::permissions::PermissionRequest>& request) {


  if (!::vivaldi::IsVivaldiRunning()) {
    return false;
  }

  // Get RenderFrameHost from source_frame_id
  content::RenderFrameHost* source_rfh =
      content::RenderFrameHost::FromID(source_frame_id);
  if (!source_rfh) {
    LOG(ERROR) << "Vivaldi: No RenderFrameHost for source_frame_id";
    return false;
  }

  // Check if this is a GuestView request
  extensions::WebViewGuest* web_view_guest =
      extensions::WebViewGuest::FromRenderFrameHost(source_rfh);
  if (!web_view_guest) {
    return false;
  }

  // Get embedder WebContents (the actual tab)
  content::WebContents* web_contents = web_view_guest->web_contents();
  if (!web_contents) {
    LOG(ERROR) << "Vivaldi: No embedder WebContents";
    return false;
  }

  // Ensure navigation observer is attached to this WebContents.
  PermissionNavigationObserver::CreateForWebContents(web_contents);

  int tab_id = VivaldiBrowserComponentWrapper::GetInstance()->GetTabId(web_contents);

  // Handle protocol handlers and other permission types
  std::optional<ContentSettingsType> content_settings_type;
  std::string protocol_name;
  std::string protocol_url;

  if (request->request_type() == ::permissions::RequestType::kRegisterProtocolHandler) {
    // Extract protocol handler data
    if (ExtractProtocolHandlerData(request.get(), &protocol_name, &protocol_url)) {
      content_settings_type = ContentSettingsType::PROTOCOL_HANDLERS;
    }
  } else {
    // Convert RequestType to ContentSettingsType for other permission types
    content_settings_type =
        ::permissions::RequestTypeToContentSettingsType(request->request_type());
  }

  // Verify we got a valid content settings type
  if (!content_settings_type.has_value()) {
    request->PermissionDenied();
    return true;
  }

  // Get SitePermissionsAPI to fire the event
  extensions::SitePermissionsAPI* api =
      extensions::SitePermissionsAPI::FromBrowserContext(
          web_contents->GetBrowserContext());
  if (!api) {
    LOG(ERROR) << "Vivaldi: No SitePermissionsAPI available";
    request->PermissionDenied();
    return true;
  }

  // Fire onPermissionRequest event
  // JS will call respondToPermissionRequest with the decision
  std::string origin = request->requesting_origin().spec();

  // For two-origin permissions (e.g., storage-access), include the embedding
  // (top-level) origin so the UI can show both.
  std::string embedding_origin;
  if (request->ShouldUseTwoOriginPrompt()) {
    embedding_origin =
        source_rfh->GetMainFrame()->GetLastCommittedOrigin().GetURL().spec();
  }

  api->FirePermissionRequestEvent(
      tab_id,
      std::move(request),
      *content_settings_type,
      origin,
      protocol_name,
      protocol_url,
      embedding_origin);

  return true;
}

bool VivaldiPermissionHandlerImpl::ExtractProtocolHandlerData(
    const ::permissions::PermissionRequest* request,
    std::string* protocol_name_out,
    std::string* protocol_url_out) {
  if (request->request_type() != ::permissions::RequestType::kRegisterProtocolHandler) {
    return false;
  }

  // Cast to access handler_ via friend declaration.
  const custom_handlers::RegisterProtocolHandlerPermissionRequest* protocol_request =
      static_cast<const custom_handlers::RegisterProtocolHandlerPermissionRequest*>(request);

  // Access handler_ directly via friend access.
  const custom_handlers::ProtocolHandler& handler = protocol_request->handler_;
  *protocol_name_out = handler.protocol();
  *protocol_url_out = handler.url().spec();

  return !protocol_name_out->empty();
}

std::string VivaldiPermissionHandlerImpl::GetChooserType(
    ::permissions::ChooserController* controller) {
  // Get device type from controller's GetDeviceType() method.
  std::string type = controller->GetDeviceType();
  return !type.empty() ? type : "unknown";
}

bool VivaldiPermissionHandlerImpl::BridgeDeviceChooser(
    content::RenderFrameHost* owner,
    std::unique_ptr<::permissions::ChooserController>* controller) {
  if (!owner || !controller || !*controller) {
    return false;
  }

  auto* contents = content::WebContents::FromRenderFrameHost(owner);
  if (!contents) {
    return false;
  }

  // Determine chooser type.
  std::string chooser_type = GetChooserType(controller->get());
  if (chooser_type == "unknown") {
    return false;  // Unknown chooser type, let Chromium handle it.
  }

  // Get tab ID.
  int tab_id =
      VivaldiBrowserComponentWrapper::GetInstance()->GetTabId(contents);
  if (tab_id < 0) {
    return false;
  }

  // Get SitePermissionsAPI.
  extensions::SitePermissionsAPI* api =
      extensions::SitePermissionsAPI::FromBrowserContext(
          contents->GetBrowserContext());
  if (!api) {
    return false;
  }

  // Ensure navigation observer is attached so permission icons are cleared
  // on page refresh/navigation.
  PermissionNavigationObserver::CreateForWebContents(contents);

  // Generate request ID.
  std::string request_id = base::NumberToString(++next_chooser_request_id_);

  // Get origin before we move the controller.
  auto origin = owner->GetMainFrame()->GetLastCommittedOrigin().GetURL();
  bool allow_multiple = (*controller)->AllowMultipleSelection();

  // Fire the initial permission request event immediately.
  // This sets up the UI with a spinner (deviceEnumerationComplete = false).
  api->FirePermissionRequestEventForDeviceChooser(
      tab_id, request_id, chooser_type, origin.spec(), allow_multiple);

  // Store the API pointer and tab_id for use in the deferred callback.
  extensions::SitePermissionsAPI* api_ptr = api;

  // Create a view that handles async device enumeration and sends updates.
  auto enumeration_view = std::make_unique<DeviceEnumerationView>(
      controller->get(),
      base::BindRepeating(&VivaldiPermissionHandlerImpl::OnDeviceAdded,
                          base::Unretained(this), request_id, tab_id, api_ptr),
      base::BindOnce(&VivaldiPermissionHandlerImpl::OnEnumerationComplete,
                     base::Unretained(this), request_id, tab_id, api_ptr));

  // Store controller and view for later response handling.
  auto pending = std::make_unique<PendingDeviceChooser>();
  pending->controller = std::move(*controller);
  pending->view = std::move(enumeration_view);
  pending->chooser_type = chooser_type;
  pending->tab_id = tab_id;
  pending->origin = origin.spec();
  pending->browser_context = contents->GetBrowserContext();

  // Set the view on the controller to begin device enumeration.
  // The view is now owned by pending_choosers_ and will stay alive.
  pending_choosers_[request_id] = std::move(*pending);
  pending_choosers_[request_id].controller->set_view(
      pending_choosers_[request_id].view.get());

  // VB-114658: Safety catch-up. If discovery finished synchronously during
  // constructor, we missed the signal. Probe the state in the next task loop.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&DeviceEnumerationView::ProbeInitialState,
                                pending_choosers_[request_id].view->GetWeakPtr()));

  return true;  // Chooser was bridged, prevent default Chromium UI.
}

void VivaldiPermissionHandlerImpl::RespondToDeviceChooser(
    const std::string& request_id,
    bool allow,
    const std::vector<int>& device_indices) {
  auto it = pending_choosers_.find(request_id);
  if (it == pending_choosers_.end()) {
    return;  // Request ID not found or already processed.
  }

  auto& chooser = it->second;
  std::vector<std::string> device_names;

  if (allow && !device_indices.empty()) {
    // Convert int indices to size_t for Select().
    std::vector<size_t> select_indices;
    for (int index : device_indices) {
      if (index >= 0 && index < static_cast<int>(chooser.controller->NumOptions())) {
        select_indices.push_back(static_cast<size_t>(index));
        // Extract device name for permission event.
        device_names.push_back(base::UTF16ToUTF8(chooser.controller->GetOption(index)));
      }
    }

    if (!select_indices.empty()) {
      // Select the devices. The device chooser context automatically
      // saves the selection.
      chooser.controller->Select(select_indices);
      // Fire permission accessed event with device names and chooser type.
      extensions::SitePermissionsAPI::FirePermissionAccessedEvent(
          chooser.browser_context,
          chooser.tab_id,
          chooser.chooser_type,
          chooser.origin,
          CONTENT_SETTING_ALLOW,
          device_names);
    } else {
      chooser.controller->Cancel();
    }
  } else {
    // User denied or selected no devices.
    chooser.controller->Cancel();
  }

  // Clean up the pending chooser.
  pending_choosers_.erase(it);
}

void VivaldiPermissionHandlerImpl::OnDeviceAdded(
    const std::string& request_id,
    int tab_id,
    extensions::SitePermissionsAPI* api,
    const base::DictValue& device) {
  // A device was added during enumeration. Send incremental update.
  // This is called for each device as it arrives (important for Bluetooth).
  if (!api) {
    return;
  }

  base::ListValue devices;
  devices.Append(device.Clone());

  // Send update with one device, not complete yet.
  api->FirePermissionDeviceChooserUpdate(tab_id, request_id, devices,
                                          /*is_complete=*/false);
}

void VivaldiPermissionHandlerImpl::OnEnumerationComplete(
    const std::string& request_id,
    int tab_id,
    extensions::SitePermissionsAPI* api) {
  // Device enumeration is complete. Send final update with all devices.

  auto it = pending_choosers_.find(request_id);
  if (it == pending_choosers_.end()) {
    return;  // Request no longer pending.
  }

  auto& chooser = it->second;

  // Build complete device list.
  base::ListValue devices;
  for (size_t i = 0; i < chooser.controller->NumOptions(); ++i) {
    base::DictValue device;
    device.Set("index", static_cast<int>(i));
    device.Set("name", base::UTF16ToUTF8(chooser.controller->GetOption(i)));
    device.Set("isPaired", chooser.controller->IsPaired(i));
    devices.Append(std::move(device));
  }

  // Fire device chooser update with complete device list and isComplete=true.
  if (api) {
    api->FirePermissionDeviceChooserUpdate(tab_id, request_id, devices,
                                            /*is_complete=*/true);
  }

  // Don't erase pending_choosers_ here. Keep the controller and view
  // alive until the user responds to the permission prompt. The cleanup happens
  // in RespondToDeviceChooser() after the user has made their choice.
  // Erasing here caused a use-after-free bug where the controller/view were
  // destroyed while still potentially in use by JavaScript event handlers.
}

void VivaldiPermissionHandlerImpl::CleanupPendingChoosersForTab(int tab_id) {
  // Clean up all pending device choosers for a tab that was closed
  // or navigated. This prevents memory leaks and use-after-free bugs.
  auto it = pending_choosers_.begin();
  while (it != pending_choosers_.end()) {
    if (it->second.tab_id == tab_id) {
      it = pending_choosers_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace permissions
}  // namespace vivaldi
