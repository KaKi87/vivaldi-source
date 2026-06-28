// Copyright (c) 2017 Vivaldi Technologies AS. All rights reserved

#include "extensions/api/window/window_private_api.h"

#include <cstdint>
#include <memory>
#include <string>

#include "base/json/json_string_value_serializer.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/tabs/windows_util.h"
#include "chrome/browser/extensions/browser_extension_window_controller.h"
#include "chrome/browser/extensions/window_controller.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window/public/browser_collection_observer.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/window_sizer/window_sizer.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/color_parser.h"
#include "extensions/common/image_util.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "ui/base/ui_base_types.h"

#include "browser/related_tab_strip_helper.h"
#include "browser/tab_positioning.h"
#include "browser/vivaldi_browser_finder.h"
#include "extensions/api/extension_action_utils/extension_action_utils_api.h"
#include "extensions/api/tabs/tabs_private_api.h"
#include "extensions/api/zoom/zoom_api.h"
#include "extensions/tools/vivaldi_tools.h"
#include "extensions/vivaldi_associated_tabs.h"
#include "extensions/vivaldi_browser_component_wrapper.h"
#include "ui/vivaldi_browser_window.h"
#include "ui/vivaldi_ui_utils.h"
#include "ui/window_registry_service.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

#include "components/ext_data/tab_ext_data.h"

namespace extensions {

namespace vivaldi {
using vivaldi::window_private::WindowState;
WindowState ConvertToJSWindowState(ui::mojom::WindowShowState state) {
  switch (state) {
    case ui::mojom::WindowShowState::kFullscreen:
      return WindowState::kFullscreen;
    case ui::mojom::WindowShowState::kMaximized:
      return WindowState::kMaximized;
    case ui::mojom::WindowShowState::kMinimized:
      return WindowState::kMinimized;
    default:
      return WindowState::kNormal;
  }
  NOTREACHED();
  // return WindowState::kNormal;
}

ui::mojom::WindowShowState ConvertToWindowShowState(
    vivaldi::window_private::WindowState state) {
  using vivaldi::window_private::WindowState;
  switch (state) {
    case WindowState::kNormal:
      return ui::mojom::WindowShowState::kNormal;
    case WindowState::kMinimized:
      return ui::mojom::WindowShowState::kMinimized;
    case WindowState::kMaximized:
      return ui::mojom::WindowShowState::kMaximized;
    case WindowState::kFullscreen:
      return ui::mojom::WindowShowState::kFullscreen;
    case WindowState::kNone:
      return ui::mojom::WindowShowState::kDefault;
  }
  NOTREACHED();
  // return  ui::mojom::WindowShowState::kDefault;
}
}  // namespace vivaldi

namespace {

class VivaldiBrowserObserver : public TabStripModelObserver,
                               public BrowserCollectionObserver {
 public:
  VivaldiBrowserObserver();

  // This should not be called.
  ~VivaldiBrowserObserver() override { NOTREACHED(); }

  static VivaldiBrowserObserver& GetInstance();

  void WindowsForProfileClosing(Profile* profile);

  // If the browser is closing due to the profile closure, return the profile
  // index. Otherwise return SIZE_MAX.
  size_t FindClosingWindow(int32_t browser_id);

 private:
  friend class ::extensions::VivaldiWindowsAPI;

  // TabStripModelObserver implementation
  void OnTabChangedAt(tabs::TabInterface* tab,
                      int index,
                      TabChangeType change_type) override;
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // BrowserCollectionObserver overrides:
  void OnBrowserCreated(BrowserWindowInterface* browser) override;
  void OnBrowserClosed(BrowserWindowInterface* browser) override;
  void OnBrowserActivated(BrowserWindowInterface* browser) override;

  base::ScopedObservation<GlobalBrowserCollection, BrowserCollectionObserver>
      browser_collection_observation_{this};

  // Used to track windows being closed by profiles being closed, they should
  // not have any confirmation dialogs.
  std::vector<int32_t> closing_windows_;
};

VivaldiBrowserObserver& VivaldiBrowserObserver::GetInstance() {
  static base::NoDestructor<VivaldiBrowserObserver> instance;
  return *instance;
}

VivaldiBrowserObserver::VivaldiBrowserObserver() {
  browser_collection_observation_.Observe(
      GlobalBrowserCollection::GetInstance());
}

void VivaldiBrowserObserver::WindowsForProfileClosing(Profile* profile) {
  if (profile->IsGuestSession()) {
    // We don't care about guest windows.
    return;
  }

  ForEachCurrentAndNewBrowserWindowInterfaceOrderedByActivation(
      [&](BrowserWindowInterface* browser) {
        // If this is the last normal window, close the settings
        // window so shutdown can progress normally.
        if (browser->GetProfile()->GetOriginalProfile() ==
            profile->GetOriginalProfile()) {
          closing_windows_.push_back(browser->GetSessionID().id());
        }
        return true;  // continue iterating
      });
}

size_t VivaldiBrowserObserver::FindClosingWindow(int32_t browser_id) {
  // STL is too verbose not to use a one-letter abbreviation.
  const auto& v = closing_windows_;
  auto i = std::find(v.begin(), v.end(), browser_id);
  return (i != v.end()) ? std::distance(v.begin(), i) : SIZE_MAX;
}

void VivaldiBrowserObserver::OnTabChangedAt(tabs::TabInterface* tab,
                                            int index,
                                            TabChangeType change_type) {
  // Ignore 'loading' and 'title' changes.
  if (change_type != TabChangeType::kAll)
    return;

  TabsPrivateAPI::FromBrowserContext(tab->GetContents()->GetBrowserContext())
      ->NotifyTabChange(tab->GetContents());
}

void VivaldiBrowserObserver::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  // Ext data maintanance
  if (change.type() == TabStripModelChange::Type::kInserted) {
    for (const auto& contents : change.GetInsert()->contents) {
      ::vivaldi::TabExtData::Get(contents.contents)->OnTabAdded();
    }
  } else if (change.type() == TabStripModelChange::Type::kRemoved) {
    for (const auto& contents : change.GetRemove()->contents) {
      ::vivaldi::TabExtData::Get(contents.contents)->OnTabRemoved();
    }
  }

  // The kReplaced case is handled in TabModel::DiscardContents - VB-127571
  // Copying ExtData here from the old to new WebContents is too late since the
  // browser already started telling other components that the tab had changed.

  ::vivaldi::HandleAssociatedTabs(tab_strip_model, change);
  ::vivaldi::related_tabs::HandleOrphans(tab_strip_model, change);
  ::vivaldi::tab_positioning::HandleStacking(tab_strip_model, change);
  ::vivaldi::related_tabs::HandleGroups(tab_strip_model, change);

  if (!selection.active_tab_changed() || !selection.new_contents)
    return;

  TabsPrivateAPI::FromBrowserContext(
      selection.new_contents->GetBrowserContext())
      ->NotifyTabSelectionChange(selection.new_contents);
}

void VivaldiBrowserObserver::OnBrowserCreated(BrowserWindowInterface* browser) {
  browser->GetTabStripModel()->AddObserver(this);

  if (browser->is_vivaldi()) {
    ZoomAPI::AddZoomObserver(browser);
  }
}

void VivaldiBrowserObserver::OnBrowserClosed(BrowserWindowInterface* browser) {
  browser->GetTabStripModel()->RemoveObserver(this);

  if (browser->is_vivaldi()) {
    ZoomAPI::RemoveZoomObserver(browser);
  }

  size_t i = FindClosingWindow(browser->GetSessionID().id());
  if (i != SIZE_MAX) {
    closing_windows_.erase(closing_windows_.begin() + i);
  }

  if (chrome::GetBrowserCount(browser->GetProfile()) == 1) {
    ForEachCurrentAndNewBrowserWindowInterfaceOrderedByActivation(
        [&](BrowserWindowInterface* browser) {
          // If this is the last normal window, close the settings
          // window so shutdown can progress normally.
          if (browser->is_vivaldi() &&
              static_cast<VivaldiBrowserWindow*>(browser->GetWindow())
                      ->window_type() ==
                  VivaldiBrowserWindow::WindowType::SETTINGS) {
            browser->GetWindow()->Close();
          }
          return true;  // continue iterating
        });
  }
}

void VivaldiBrowserObserver::OnBrowserActivated(
    BrowserWindowInterface* browser) {
  TabsPrivateAPI::FromBrowserContext(browser->GetProfile())
      ->NotifyTabSelectionChange(
          browser->GetTabStripModel()->GetActiveWebContents());
}

}  // namespace

// static
void VivaldiWindowsAPI::Init() {
  VivaldiBrowserObserver::GetInstance();
}

// static
void VivaldiWindowsAPI::WindowsForProfileClosing(Profile* profile) {
  VivaldiBrowserObserver::GetInstance().WindowsForProfileClosing(profile);
}

// static
bool VivaldiWindowsAPI::IsWindowClosingBecauseProfileClose(Browser* browser) {
  size_t i = VivaldiBrowserObserver::GetInstance().FindClosingWindow(
      browser->session_id().id());
  return i != SIZE_MAX;
}

ExtensionFunction::ResponseAction WindowPrivateCreateFunction::Run() {
  using vivaldi::window_private::Create::Params;
  namespace Results = vivaldi::window_private::Create::Results;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  bool incognito = false;
  std::string tab_url;
  std::string viv_ext_data;
  std::string window_key;

  if (params->options.incognito.has_value()) {
    incognito = params->options.incognito.value();
  }
  if (params->options.tab_url.has_value()) {
    tab_url = params->options.tab_url.value();
  }
  if (params->options.viv_ext_data.has_value()) {
    viv_ext_data = params->options.viv_ext_data.value();
  }
  if (params->options.window_key.has_value()) {
    window_key = params->options.window_key.value();
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());

  if (profile->IsGuestSession() && !incognito) {
    // Opening a new window from a guest session is only allowed for
    // incognito windows.  It will crash on purpose otherwise.
    // See Browser::Browser() for the CHECKs.
    return RespondNow(
        Error("New guest window can only be opened from incognito window"));
  }

  VivaldiBrowserWindow* named_window =
      ::vivaldi::WindowRegistryService::Get(profile)->GetNamedWindow(
          window_key);

  if (named_window) {
    named_window->Activate();
    return RespondNow(ArgumentList(Results::Create(named_window->id())));
  }

  if (incognito) {
    profile = profile->GetOffTheRecordProfile(
        Profile::OTRProfileID::PrimaryID(), true);
  } else {
    profile = profile->GetOriginalProfile();
  }

  gfx::Rect window_bounds;

  ui::mojom::WindowShowState ignored_show_state =
      ui::mojom::WindowShowState::kDefault;
  WindowSizer::GetBrowserWindowBoundsAndShowState(
      gfx::Rect(), nullptr, &window_bounds, &ignored_show_state);

  if (params->options.bounds) {
    if (params->options.bounds->top) {
      window_bounds.set_y(*params->options.bounds->top);
    }
    if (params->options.bounds->left) {
      window_bounds.set_x(*params->options.bounds->left);
    }
    if (params->options.bounds->width) {
      window_bounds.set_width(params->options.bounds->width);
    }
    if (params->options.bounds->height) {
      window_bounds.set_height(params->options.bounds->height);
    }
  }

  ui::mojom::WindowShowState window_state =
      params->options.state != vivaldi::window_private::WindowState::kNone
          ? vivaldi::ConvertToWindowShowState(params->options.state)
          : ui::mojom::WindowShowState::kDefault;

  VivaldiBrowserComponentWrapper::GetInstance()->WindowPrivateCreate(
      profile, params->type, window_state, window_bounds, window_key,
      viv_ext_data, tab_url,
      base::BindOnce(&WindowPrivateCreateFunction::OnAppUILoaded, this));

  return RespondLater();
}

void WindowPrivateCreateFunction::OnAppUILoaded(VivaldiBrowserWindow* window) {
  namespace Results = vivaldi::window_private::Create::Results;

  DCHECK(!did_respond());

  int window_id = window ? window->id() : -1;
  Respond(ArgumentList(Results::Create(window_id)));
}

ExtensionFunction::ResponseAction WindowPrivateGetCurrentIdFunction::Run() {
  namespace Results = vivaldi::window_private::GetCurrentId::Results;

  // It is OK to use GetSenderWebContents() as JS will call this function from
  // the proper window.
  content::WebContents* web_contents = GetSenderWebContents();
  Browser* browser = VivaldiBrowserComponentWrapper::GetInstance()
                         ->FindBrowserForEmbedderWebContents(web_contents);
  if (!browser)
    return RespondNow(Error("No Browser instance"));

  return RespondNow(ArgumentList(Results::Create(browser->session_id().id())));
}

ExtensionFunction::ResponseAction WindowPrivateSetStateFunction::Run() {
  using vivaldi::window_private::SetState::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  extensions::WindowController* controller;
  std::string error;
  bool was_fullscreen;
  if (!VivaldiBrowserComponentWrapper::GetInstance()->GetControllerFromWindowID(
          this, params->window_id, &controller, &error)) {
    return RespondNow(Error(error));
  }
  Browser* browser = controller->GetBrowser();
  ui::mojom::WindowShowState show_state =
      vivaldi::ConvertToWindowShowState(params->state);

  // Don't trigger onStateChanged event for changes coming from JS. The
  // assumption is that JS updates its state as needed after each
  // windowPrivate.setState() call.
  static_cast<VivaldiBrowserWindow*>(browser->window())
      ->SetWindowState(show_state);

  switch (show_state) {
    case ui::mojom::WindowShowState::kMinimized:
      was_fullscreen = browser->window()->IsFullscreen();
      extensions::BrowserExtensionWindowController::From(browser)
          ->window()
          ->Minimize();
      if (was_fullscreen) {
        extensions::BrowserExtensionWindowController::From(browser)
            ->SetFullscreenMode(false, extension()->url());
      }
      break;
    case ui::mojom::WindowShowState::kMaximized:
      was_fullscreen = browser->window()->IsFullscreen();
#if BUILDFLAG(IS_MAC)
      // NOTE(bjorgvin@vivaldi.com): VB-82933 SetFullscreenMode has to be after
      // Maximize on macOS.
      extensions::BrowserExtensionWindowController::From(browser)
          ->window()
          ->Maximize();
      if (was_fullscreen) {
        extensions::BrowserExtensionWindowController::From(browser)
            ->SetFullscreenMode(false, extension()->url());
      }
#else
      // NOTE(bjorgvin@vivaldi.com): VB-83626 SetFullscreenMode has to be before
      // Maximize on Linux to prevent triggering of extra onStateChanged events.
      if (was_fullscreen) {
        extensions::BrowserExtensionWindowController::From(browser)
            ->SetFullscreenMode(false, extension()->url());
      }
      extensions::BrowserExtensionWindowController::From(browser)
          ->window()
          ->Maximize();
#endif
      break;
    case ui::mojom::WindowShowState::kFullscreen:
      extensions::BrowserExtensionWindowController::From(browser)
          ->SetFullscreenMode(true, extension()->url());
      break;
    case ui::mojom::WindowShowState::kNormal:
      was_fullscreen = browser->window()->IsFullscreen();
      if (was_fullscreen) {
        extensions::BrowserExtensionWindowController::From(browser)
            ->SetFullscreenMode(false, extension()->url());
      } else {
        browser->window()->Restore();
      }
      break;
    default:
      break;
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
WindowPrivateUpdateMaximizeButtonPositionFunction::Run() {
  using vivaldi::window_private::UpdateMaximizeButtonPosition::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  extensions::WindowController* controller;
  std::string error;
  if (!windows_util::GetControllerFromWindowID(
          this, params->window_id, WindowController::GetAllWindowFilter(),
          &controller, &error)) {
    return RespondNow(Error(error));
  }
  VivaldiBrowserWindow* window =
      VivaldiBrowserWindow::FromBrowser(controller->GetBrowser());
  if (window) {
    gfx::RectF rect(params->left, params->top, params->width, params->height);
    ::vivaldi::FromUICoordinates(window->web_contents(), &rect);
    gfx::Rect int_rect(std::round(rect.x()), std::round(rect.y()),
                       std::round(rect.width()), std::round(rect.height()));
    window->UpdateMaximizeButtonPosition(int_rect);
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
WindowPrivateGetFocusedElementInfoFunction::Run() {
  using vivaldi::window_private::GetFocusedElementInfo::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  VivaldiBrowserWindow* window =
      VivaldiBrowserComponentWrapper::GetInstance()->VivaldiBrowserWindowFromId(
          params->window_id);
  if (!window) {
    return RespondNow(Error("No such window"));
  }

  content::WebContentsImpl* web_contents =
      static_cast<content::WebContentsImpl*>(window->web_contents());
  content::RenderFrameHostImpl* render_frame_host =
      web_contents->GetFocusedFrame();
  if (!render_frame_host) {
    render_frame_host = web_contents->GetPrimaryMainFrame();
  }
  render_frame_host->GetVivaldiFrameService()->GetFocusedElementInfo(
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&WindowPrivateGetFocusedElementInfoFunction::
                             FocusedElementInfoReceived,
                         this),
          "", "", false, ""));

  return RespondLater();
}

void WindowPrivateGetFocusedElementInfoFunction::FocusedElementInfoReceived(
    const std::string& tag_name,
    const std::string& type,
    bool editable,
    const std::string& role) {
  namespace Results = vivaldi::window_private::GetFocusedElementInfo::Results;

  vivaldi::window_private::FocusedElementInfo info;
  info.tag_name = tag_name;
  info.type = type;
  info.editable = editable;
  info.role = role;
  return Respond(ArgumentList(Results::Create(info)));
}

ExtensionFunction::ResponseAction
WindowPrivateIsOnScreenWithNotchFunction::Run() {
  namespace Results = vivaldi::window_private::IsOnScreenWithNotch::Results;
  using vivaldi::window_private::IsOnScreenWithNotch::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  VivaldiBrowserWindow* window =
      VivaldiBrowserComponentWrapper::GetInstance()->VivaldiBrowserWindowFromId(
          params->window_id);
  if (!window) {
    return RespondNow(Error("No such window"));
  }

  return RespondNow(
      ArgumentList(Results::Create(IsWindowOnScreenWithNotch(window))));
}

ExtensionFunction::ResponseAction
WindowPrivateSetControlButtonsPositionFunction::Run() {
  using vivaldi::window_private::SetControlButtonsPosition::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  extensions::WindowController* controller;
  std::string error;
  if (!windows_util::GetControllerFromWindowID(
          this, params->window_id, WindowController::GetAllWindowFilter(),
          &controller, &error)) {
    return RespondNow(Error(error));
  }
  VivaldiBrowserWindow* window =
      VivaldiBrowserWindow::FromBrowser(controller->GetBrowser());
  if (!window) {
    return RespondNow(Error("No window for browser."));
  }

  RequestChange(window->GetNativeWindow(), params->position);
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
WindowPrivatePerformHapticFeedbackFunction::Run() {
  PerformHapticFeedback();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction WindowPrivateSetHotSpotFunction::Run() {
  using vivaldi::window_private::SetHotSpot::Params;
  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  extensions::WindowController* controller;
  std::string error;
  if (!windows_util::GetControllerFromWindowID(
          this, params->window_id, WindowController::GetAllWindowFilter(),
          &controller, &error)) {
    return RespondNow(Error(error));
  }
  VivaldiBrowserWindow* window =
      VivaldiBrowserWindow::FromBrowser(controller->GetBrowser());
  if (!window) {
    return RespondNow(Error("No window for browser."));
  }

  VivaldiBrowserWindow::HotSpot hotspot;

  hotspot.location = params->location;
  hotspot.width = params->width;
  hotspot.height = params->height;

  window->SetHotSpot(hotspot);

  return RespondNow(NoArguments());
}

}  // namespace extensions
