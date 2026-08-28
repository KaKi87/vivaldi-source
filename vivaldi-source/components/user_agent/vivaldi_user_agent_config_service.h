// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENT_VIVALDI_USER_AGENT_CONFIG_SERVICE_H_
#define COMPONENT_VIVALDI_USER_AGENT_CONFIG_SERVICE_H_

#include <map>
#include <string>
#include "base/memory/raw_ptr.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/user_agent/mojom/vivaldi_user_agent_config.mojom.h"
#include "content/public/browser/render_process_host_creation_observer.h"

namespace content {
class BrowserContext;
}

namespace vivaldi_user_agent {

class VivaldiUserAgentConfigService
    : public KeyedService,
      public content::RenderProcessHostCreationObserver {
 public:

  VivaldiUserAgentConfigService(PrefService* prefs);
  ~VivaldiUserAgentConfigService() override;
  VivaldiUserAgentConfigService(const VivaldiUserAgentConfigService&) = delete;
  VivaldiUserAgentConfigService& operator=(
      const VivaldiUserAgentConfigService&) =
      delete;

  static VivaldiUserAgentConfigService* Get(content::BrowserContext* context);

  // content::RenderProcessHostCreationObserver
  void OnRenderProcessHostCreated(
      content::RenderProcessHost* process_host) override;

 private:

  void OnPrefChanged(const std::string& path);

  void UpdatePrefs();
  // process independant prefs
  std::unique_ptr<mojom::BrandConfiguration> current_config_;

  PrefService* prefs_;
  PrefChangeRegistrar pref_registrar_;
  mojo::RemoteSet<mojom::VivaldiUserAgentConfig> remotes_;
};

}  // namespace vivaldi_user_agent

#endif  // COMPONENT_VIVALDI_USER_AGENT_CONFIG_SERVICE_H_
