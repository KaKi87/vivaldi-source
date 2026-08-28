// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "extensions/api/pwa/pwa_api.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/web_applications/web_app_filter.h"
#include "chrome/browser/web_applications/web_app_install_manager.h"
#include "chrome/browser/web_applications/web_app_command_scheduler.h"
#include "chrome/browser/web_applications/web_app_provider.h"
#include "chrome/browser/web_applications/web_app_registrar.h"
#include "components/webapps/browser/install_result_code.h"
#include "components/webapps/browser/installable/installable_logging.h"
#include "components/webapps/browser/installable/installable_manager.h"
#include "components/webapps/browser/installable/installable_params.h"
#include "components/webapps/browser/web_app_url_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/schema/pwa.h"
#include "extensions/tools/vivaldi_tools.h"
#include "extensions/vivaldi_browser_component_wrapper.h"

namespace extensions {

namespace pwa = vivaldi::pwa;

// ---------------------------------------------------------------------------
// PwaInstallabilityObserver
// ---------------------------------------------------------------------------

namespace {

webapps::InstallableParams MakeCheckParams() {
  webapps::InstallableParams params;
  params.check_eligibility = true;
  params.fetch_metadata = true;
  params.valid_primary_icon = true;
  params.installable_criteria =
      webapps::InstallableCriteria::kValidManifestWithIcons;
  return params;
}

// Check if a PWA is already installed for the given URL.
bool IsPwaInstalled(content::BrowserContext* context, const GURL& url) {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return false;
  }
  web_app::WebAppProvider* provider =
      web_app::WebAppProvider::GetForWebApps(profile);
  if (!provider) {
    return false;
  }

  // Only consider apps that launch in a standalone window, or were installed
  // by the user.
  web_app::WebAppFilter filter =
      web_app::WebAppFilter::LaunchableFromInstallApi();

  return provider->registrar_unsafe()
      .FindBestAppWithUrlInScope(url, filter)
      .has_value();
}

// Convert InstallableData into the generated schema struct.
// All values are copied here because InstallableData contains raw_refs/raw_ptrs
// owned by InstallableManager that become dangling after this callback returns.
pwa::InstallabilityInfo ToSchemaInfo(
    content::BrowserContext* browser_context, const GURL& page_url,
    const webapps::InstallableData& data, bool installable) {
  pwa::InstallabilityInfo info;

  // Helper = handle url to string conv.
  auto setUrlIfNonEmpty = [](std::optional<std::string>& field,
                             const GURL& url) {
    std::string spec = url.spec();
    if (!spec.empty())
      field = std::move(spec);
  };

  auto setUtfFieldNonEmpty = [](std::optional<std::string>& dst,
                                const std::optional<std::u16string>& src) {
    if (src.has_value() && !src->empty()) {
      dst = base::UTF16ToUTF8(*src);
    }
  };

  if (!page_url.is_valid()) {
    info.status = pwa::InstallabilityStatus::kUnknown;
  } else if (installable) {
    info.status = pwa::InstallabilityStatus::kInstallable;
  } else {
    info.status = pwa::InstallabilityStatus::kNotInstallable;
  }

  info.page_url = page_url.spec();
  info.has_maskable_icon = data.has_maskable_primary_icon;

  setUrlIfNonEmpty(info.manifest_url, *data.manifest_url);
  setUrlIfNonEmpty(info.primary_icon_url, *data.primary_icon_url);
  setUrlIfNonEmpty(info.start_url, data.manifest->start_url);
  setUrlIfNonEmpty(info.scope, data.manifest->scope);

  setUtfFieldNonEmpty(info.name, data.manifest->name);
  setUtfFieldNonEmpty(info.short_name, data.manifest->short_name);

  // Copy errors as readable messages.
  if (!data.errors.empty()) {
    info.errors.emplace();
    for (webapps::InstallableStatusCode code : data.errors) {
      std::string msg = webapps::GetErrorMessage(code);
      if (!msg.empty()) {
        info.errors->push_back(std::move(msg));
      }
    }
  }

  // Check if a PWA is already installed for this page.
  info.is_installed = IsPwaInstalled(browser_context, page_url);

  return info;
}

// Build and fire the onInstallabilityChanged event.
void FireInstallabilityChangedEvent(content::WebContents* contents,
                                    int tab_id,
                                    const pwa::InstallabilityInfo& info) {
  if (!contents)
    return;
  base::ListValue args = pwa::OnInstallabilityChanged::Create(tab_id, info);
  ::vivaldi::BroadcastEvent(pwa::OnInstallabilityChanged::kEventName,
                            std::move(args), contents->GetBrowserContext());
}

}  // namespace

PwaInstallabilityObserver::PwaInstallabilityObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<PwaInstallabilityObserver>(*web_contents) {
  // Ensure InstallableManager exists for this WebContents.
  webapps::InstallableManager::CreateForWebContents(web_contents);

  // Subscribe to install/uninstall events so we can re-check isInstalled
  // when apps are added or removed externally.
  Profile* profile = Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (profile) {
    web_app::WebAppProvider* provider =
        web_app::WebAppProvider::GetForWebApps(profile);
    if (provider) {
      provider->install_manager().AddObserver(this);
    }
  }
}

PwaInstallabilityObserver::~PwaInstallabilityObserver() = default;

void PwaInstallabilityObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Filter out the non-actionables - we're only interested in real document
  // changes.
  if (!navigation_handle->IsInPrimaryMainFrame())
    return;
  if (!navigation_handle->HasCommitted())
    return;
  if (navigation_handle->IsSameDocument())
    return;

  ResetForNewDocument(navigation_handle->GetURL());

  // This is just an act of self defense - if served from cache the
  // DidFinishLoad event may not fire.
  if (navigation_handle->IsServedFromBackForwardCache()) {
    MaybeStartCheck(navigation_handle->GetURL());
  }
}

void PwaInstallabilityObserver::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  // Don't consider iframes and other stuff.
  if (!render_frame_host->IsInPrimaryMainFrame())
    return;
  MaybeStartCheck(validated_url);
}

void PwaInstallabilityObserver::DidUpdateWebManifestURL(
    content::RenderFrameHost* target_frame,
    const GURL& manifest_url) {
  if (!target_frame->IsInPrimaryMainFrame())
    return;

  // Invalidate in-flight callbacks.
  ++generation_;
  weak_factory_.InvalidateWeakPtrs();

  content::WebContents* contents = web_contents();
  if (!contents)
    return;

  if (manifest_url.is_empty()) {
    // Manifest was removed — immediately report not installable.
    int tab_id =
        VivaldiBrowserComponentWrapper::GetInstance()->ExtensionTabUtilGetTabId(
            contents);

    pwa::InstallabilityInfo info;
    info.status = pwa::InstallabilityStatus::kNotInstallable;
    info.page_url = contents->GetLastCommittedURL().spec();

    FireInstallabilityChangedEvent(contents, tab_id, info);
    return;
  }

  // Post a re-check to avoid racing with InstallableManager's own cache reset.
  // InstallableManager also observes DidUpdateWebManifestURL and flushes its
  // state synchronously, so we must not call GetData() directly here.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&PwaInstallabilityObserver::MaybeStartCheck,
                                weak_factory_.GetWeakPtr(),
                                contents->GetLastCommittedURL()));
}

void PwaInstallabilityObserver::WebContentsDestroyed() {
  weak_factory_.InvalidateWeakPtrs();

  // Unregister from install manager before WebContents is torn down.
  content::WebContents* contents = web_contents();
  if (contents) {
    Profile* profile = Profile::FromBrowserContext(contents->GetBrowserContext());
    if (profile) {
      web_app::WebAppProvider* provider =
          web_app::WebAppProvider::GetForWebApps(profile);
      if (provider) {
        provider->install_manager().RemoveObserver(this);
      }
    }
  }
}

void PwaInstallabilityObserver::ResetForNewDocument(const GURL& url) {
  ++generation_;
  weak_factory_.InvalidateWeakPtrs();
  last_page_url_ = url;

  content::WebContents* contents = web_contents();
  if (!contents)
    return;

  int tab_id =
      VivaldiBrowserComponentWrapper::GetInstance()->ExtensionTabUtilGetTabId(
          contents);

  pwa::InstallabilityInfo info;
  info.status = pwa::InstallabilityStatus::kUnknown;
  info.page_url = url.spec();

  FireInstallabilityChangedEvent(contents, tab_id, info);
}

void PwaInstallabilityObserver::MaybeStartCheck(const GURL& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!webapps::IsUrlEligibleForWebApp(url))
    return;

  content::WebContents* contents = web_contents();
  if (!contents || contents->IsBeingDestroyed())
    return;

  auto* manager = webapps::InstallableManager::FromWebContents(contents);
  // The constructor guarantees the manager exists.
  DCHECK(manager);
  if (!manager)
    return;

  manager->GetData(MakeCheckParams(),
                   base::BindOnce(&PwaInstallabilityObserver::OnInstallableData,
                                   weak_factory_.GetWeakPtr(), generation_));
}

void PwaInstallabilityObserver::OnInstallableData(
    uint64_t generation,
    const webapps::InstallableData& data) {
  // If this was already invalidated by navigation, skip it.
  if (generation != generation_)
    return;

  content::WebContents* contents = web_contents();
  if (!contents || contents->IsBeingDestroyed())
    return;

  bool installable = data.errors.empty() && data.installable_check_passed;

  // Copy all needed data inside the callback scope.
  content::BrowserContext* browser_context = contents->GetBrowserContext();
  pwa::InstallabilityInfo info =
      ToSchemaInfo(browser_context, last_page_url_, data, installable);

  int tab_id =
      VivaldiBrowserComponentWrapper::GetInstance()->ExtensionTabUtilGetTabId(
          contents);

  FireInstallabilityChangedEvent(contents, tab_id, info);
}

void PwaInstallabilityObserver::Recheck() {
  MaybeStartCheck(last_page_url_);
}

void PwaInstallabilityObserver::OnWebAppInstalled(const webapps::AppId& app_id) {
  // A web app was installed - re-check to update isInstalled flag.
  Recheck();
}

void PwaInstallabilityObserver::OnWebAppUninstalled(
    const webapps::AppId& app_id, webapps::WebappUninstallSource uninstall_source) {
  // A web app was uninstalled - re-check to update isInstalled flag.
  Recheck();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PwaInstallabilityObserver);

// ---------------------------------------------------------------------------
// PwaInstallFunction
// ---------------------------------------------------------------------------

void PwaInstallFunction::OnInstallComplete(const webapps::AppId& /*app_id*/,
                                           webapps::InstallResultCode code) {
  Respond(ArgumentList(
      vivaldi::pwa::Install::Results::Create(webapps::IsSuccess(code))));
  Release();
}

ExtensionFunction::ResponseAction PwaInstallFunction::Run() {
  using vivaldi::pwa::Install::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  content::WebContents* contents = nullptr;
  bool found =
      VivaldiBrowserComponentWrapper::GetInstance()->ExtensionTabUtilGetTabById(
          params->tab_id, browser_context(), /*include_incognito=*/true,
          &contents);
  if (!found || !contents) {
    return RespondNow(Error("Tab not found"));
  }

  auto* provider = web_app::WebAppProvider::GetForWebContents(contents);
  if (!provider) {
    return RespondNow(Error("Web app provider not found"));
  }

  // Fetch the manifest and install directly without showing the confirmation
  // dialog. The crafted manifest defaults to display: browser, so we
  // override the dialog callback to force kStandalone so that PWA
  // desktop icon launches open in a separate window.
  provider->scheduler().FetchManifestAndInstall(
      webapps::WebappInstallSource::API_BROWSER_TAB,
      contents->GetWeakPtr(),
      base::BindOnce(
          [](base::WeakPtr<web_app::WebAppScreenshotFetcher> screenshot_fetcher,
             content::WebContents* contents,
             std::unique_ptr<web_app::WebAppInstallInfo> install_info,
             web_app::WebAppInstallationAcceptanceCallback
                 acceptance_callback) {
            install_info->user_display_mode =
                web_app::mojom::UserDisplayMode::kStandalone;

            std::move(acceptance_callback)
                .Run(
                    /*user_accepted=*/true,
                    std::move(install_info),
                    base::BindOnce(
                        [](bool install_success,
                           base::OnceClosure reparent_or_launch) {
                          if (install_success && reparent_or_launch) {
                            std::move(reparent_or_launch).Run();
                          }
                        }));
          }),
      base::BindOnce(&PwaInstallFunction::OnInstallComplete, this),
      web_app::FallbackBehavior::kCraftedManifestOnly);
  AddRef(); // Balanced in OnInstallComplete.
  return RespondLater();
}

// ---------------------------------------------------------------------------
// PwaLaunchFunction
// ---------------------------------------------------------------------------

ExtensionFunction::ResponseAction PwaLaunchFunction::Run() {
  using vivaldi::pwa::Launch::Params;

  std::optional<Params> params = Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  content::WebContents* contents = nullptr;
  bool found =
      VivaldiBrowserComponentWrapper::GetInstance()->ExtensionTabUtilGetTabById(
          params->tab_id, browser_context(), /*include_incognito=*/true,
          &contents);
  if (!found || !contents) {
    return RespondNow(Error("Tab not found"));
  }

  const GURL& url = contents->GetLastCommittedURL();

  // Find the installed app for this URL.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile) {
    return RespondNow(Error("Profile not found"));
  }
  web_app::WebAppProvider* provider =
      web_app::WebAppProvider::GetForWebApps(profile);
  if (!provider) {
    return RespondNow(Error("Web app provider not found"));
  }

  web_app::WebAppFilter filter =
      web_app::WebAppFilter::LaunchableFromInstallApi();
  std::optional<webapps::AppId> app_id =
      provider->registrar_unsafe().FindBestAppWithUrlInScope(url, filter);
  if (!app_id) {
    return RespondNow(Error("No installed PWA found for this page"));
  }

  // Launch the PWA using the standard path. The install function sets
  // user_display_mode to kStandalone, so this opens in a separate window.
  provider->scheduler().LaunchApp(
      *app_id, url,
      base::BindOnce(&PwaLaunchFunction::OnLaunchComplete, this),
      /*launch_source=*/std::nullopt);
  AddRef(); // Balanced in OnLaunchComplete.
  return RespondLater();
}

void PwaLaunchFunction::OnLaunchComplete(
    base::WeakPtr<BrowserWindowInterface> browser,
    base::WeakPtr<content::WebContents> web_contents,
    apps::LaunchContainer container) {
  bool success = (bool)browser || (bool)web_contents;
  Respond(ArgumentList(
      vivaldi::pwa::Launch::Results::Create(success)));
  Release();
}

}  // namespace extensions
