// Copyright (c) 2020 Vivaldi Technologies AS. All rights reserved

#include "base/containers/flat_set.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/views/controls/slider.h"
#include "content/browser/picture_in_picture/picture_in_picture_session.h"

namespace vivaldi {
class VideoProgress;
class MuteButton;

class VideoPIPController
    : public content::WebContentsObserver,
      public views::SliderListener {
 public:
  class Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}

    // Mute state for the whole WebContents.
    virtual void AudioMutingStateChanged(bool muted) = 0;
  };

  ~VideoPIPController() override;
  VideoPIPController(vivaldi::VideoPIPController::Delegate* delegate,
                     content::WebContents* web_contents);
  VideoPIPController(const VideoPIPController&) = delete;
  VideoPIPController& operator=(const VideoPIPController&) = delete;

  void SetVolume(float volume_multiplier);

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;
  void DidUpdateAudioMutingState(bool muted) override;

  void SliderValueChanged(views::Slider* sender,
                                  float value,
                                  float old_value,
                                  views::SliderChangeReason reason) override;
  // Invoked when a drag starts or ends (more specifically, when the mouse
  // button is pressed or released).
  void SliderDragStarted(views::Slider* sender) override;
  void SliderDragEnded(views::Slider* sender) override;

 private:
  raw_ptr<Delegate> delegate_ = nullptr;
};

}  // namespace vivaldi
