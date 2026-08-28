// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef EXTENSIONS_API_PWA_PWA_API_H_
#define EXTENSIONS_API_PWA_PWA_API_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/web_applications/web_app_install_manager_observer.h"
#include "components/services/app_service/public/cpp/app_launch_util.h"
#include "components/webapps/browser/install_result_code.h"
#include "components/webapps/browser/installable/installable_data.h"
#include "components/webapps/common/web_app_id.h"
#include "components/webapps/browser/installable/installable_logging.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/extension_function.h"
#include "url/gurl.h"

namespace extensions {

// ---------------------------------------------------------------------------
// Per-WebContents observer: watches navigation / manifest events and checks
// PWA installability via InstallableManager.  Also subscribes to
// WebAppInstallManager so that external install/uninstall events trigger a
// re-check, keeping the isInstalled flag up-to-date without polling.
// ---------------------------------------------------------------------------
class PwaInstallabilityObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PwaInstallabilityObserver>,
      public web_app::WebAppInstallManagerObserver {
 public:
  ~PwaInstallabilityObserver() override;

  // Manually trigger a re-check for the current page.
  void Recheck();

  const GURL& last_page_url() const { return last_page_url_; }

 private:
  friend class content::WebContentsUserData<PwaInstallabilityObserver>;

  explicit PwaInstallabilityObserver(content::WebContents* web_contents);

  // content::WebContentsObserver overrides.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidUpdateWebManifestURL(content::RenderFrameHost* target_frame,
                               const GURL& manifest_url) override;
  void WebContentsDestroyed() override;

  void ResetForNewDocument(const GURL& url);
  void MaybeStartCheck(const GURL& url);
  void OnInstallableData(uint64_t generation,
                         const webapps::InstallableData& data);

  // web_app::WebAppInstallManagerObserver overrides.
  void OnWebAppInstalled(const webapps::AppId& app_id) override;
  void OnWebAppUninstalled(const webapps::AppId& app_id,
                           webapps::WebappUninstallSource uninstall_source)
      override;

  GURL last_page_url_;
  uint64_t generation_ = 0;

  base::WeakPtrFactory<PwaInstallabilityObserver> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

class PwaInstallFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pwa.install", PWA_INSTALL)
  PwaInstallFunction() = default;

 private:
  ~PwaInstallFunction() override = default;
  ResponseAction Run() override;
  void OnInstallComplete(const webapps::AppId& /*app_id*/,
                         webapps::InstallResultCode code);
};

class PwaLaunchFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pwa.launch", PWA_LAUNCH)
  PwaLaunchFunction() = default;

 private:
  ~PwaLaunchFunction() override = default;
  ResponseAction Run() override;
  void OnLaunchComplete(base::WeakPtr<BrowserWindowInterface> browser,
                        base::WeakPtr<content::WebContents> web_contents,
                        apps::LaunchContainer container);
};

}  // namespace extensions

#endif  // EXTENSIONS_API_PWA_PWA_API_H_
