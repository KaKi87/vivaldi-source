// Copyright (c) 2020 Vivaldi Technologies AS. All rights reserved

#include "ui/views/video_pip_controller.h"

#include "app/vivaldi_constants.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/browser/media/media_web_contents_observer.h"
#include "content/browser/picture_in_picture/video_picture_in_picture_window_controller_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/media_session_service.h"
#include "content/public/browser/picture_in_picture_window_controller.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/script_executor.h"
#include "extensions/browser/scripting_utils.h"
#include "extensions/common/mojom/code_injection.mojom-forward.h"
#include "extensions/common/mojom/execution_world.mojom-shared.h"
#include "extensions/common/extension.h"

using media_session::mojom::MediaSessionAction;

namespace vivaldi {

VideoPIPController::VideoPIPController(
    vivaldi::VideoPIPController::Delegate* delegate,
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), delegate_(delegate) {
}

VideoPIPController::~VideoPIPController() {
}

void VideoPIPController::WebContentsDestroyed() {
  content::WebContentsObserver::Observe(nullptr);
  delegate_ = nullptr;
}

void VideoPIPController::DidUpdateAudioMutingState(bool muted) {
  if (delegate_) {
    delegate_->AudioMutingStateChanged(muted);
  }
}

// Note this is volume.
void VideoPIPController::SliderValueChanged(views::Slider* sender,
                                            float value,
                                            float old_value,
                                            views::SliderChangeReason reason) {
  SetVolume(value);
}

void VideoPIPController::SliderDragStarted(views::Slider* sender) {
}
void VideoPIPController::SliderDragEnded(views::Slider* sender) {
}

void VideoPIPController::SetVolume(float volume_multiplier) {
  std::string script =
      "var videos = document.querySelectorAll('video'); for (var i = 0; i "
      "< videos.length; i++) { if (videos[i].readyState > 0) { "
      "videos[i].volume = " +
      std::to_string(volume_multiplier) + ";}}";

  extensions::ScriptExecutor* script_executor = nullptr;
  extensions::ScriptExecutor::FrameScope frame_scope =
      extensions::ScriptExecutor::INCLUDE_SUB_FRAMES;
  std::set<int> frame_ids;
  std::string error;

  const extensions::Extension* vivaldi_extension =
      extensions::ExtensionRegistry::Get(web_contents()->GetBrowserContext())
          ->GetExtensionById(vivaldi::kVivaldiAppId,
                             extensions::ExtensionRegistry::EVERYTHING);

  extensions::scripting::InjectionTarget internal_injection_target;
  internal_injection_target.all_frames = true;
  internal_injection_target.tab_id =
      sessions::SessionTabHelper::IdForTab(web_contents()).id();

  if (!extensions::scripting::CanAccessTarget(
          *vivaldi_extension->permissions_data(), internal_injection_target,
          web_contents()->GetBrowserContext(),
          true /*include_incognito_information*/, &script_executor,
          &frame_scope, &frame_ids, &error)) {
    LOG(ERROR) << error;
    return;
  }

  extensions::mojom::ExecutionWorld execution_world =
      extensions::mojom::ExecutionWorld::kUserScript;
  std::optional<std::string> execution_world_id = std::nullopt;

  std::vector<extensions::mojom::JSSourcePtr> sources;
  sources.push_back(
      extensions::mojom::JSSource::New(std::move(script), GURL()));

  extensions::scripting::ExecuteScript(
      vivaldi_extension->id(), std::move(sources), execution_world,
      execution_world_id, script_executor, frame_scope, frame_ids,
      true /*inject_immediately*/, true /*user_gesture*/, base::NullCallback());
}

}  // namespace vivaldi
