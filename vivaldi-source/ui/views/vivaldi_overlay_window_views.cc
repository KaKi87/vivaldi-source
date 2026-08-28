// Copyright (c) 2020-2022 Vivaldi Technologies AS. All rights reserved

#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/overlay/video_overlay_window_views.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/picture_in_picture_window_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/mute_button.h"
#include "ui/views/controls/volume_slider.h"

#include "app/vivaldi_apptools.h"
#include "prefs/vivaldi_pref_names.h"

// The file contains the vivaldi specific code for the VideoOverlayWindowViews
// class used for the Picture-in-Picture window.

constexpr int kVolumeSliderHeight = 30;
constexpr gfx::Size kVivaldiButtonControlSize(20, 20);

class VideoPipControllerDelegate
    : public vivaldi::VideoPIPController::Delegate {
 public:
  VideoPipControllerDelegate(vivaldi::MuteButton* mute_button,
                             vivaldi::VolumeSlider* volume_slider)
      : vivaldi::VideoPIPController::Delegate(),
        mute_button_(mute_button),
        volume_slider_(volume_slider) {}
  ~VideoPipControllerDelegate() override {}

  void AudioMutingStateChanged(bool muted) override {
    DCHECK(mute_button_);
    if (mute_button_) {
      mute_button_->ChangeMode(muted ? vivaldi::MuteButton::Mode::kMute
                                     : vivaldi::MuteButton::Mode::kAudible,
                               false);
    }
  }

 private:
  // ownership tied to the VideoOverlayWindowViews class
  const raw_ptr<vivaldi::MuteButton> mute_button_ = nullptr;
  const raw_ptr<vivaldi::VolumeSlider> volume_slider_ = nullptr;
};

void VideoOverlayWindowViews::HandleVivaldiMuteButton() {
  content::WebContents* contents = controller_->GetWebContents();

  DCHECK_EQ(contents->IsAudioMuted(),
            mute_button_->muted_mode() == vivaldi::MuteButton::Mode::kMute);

  if (contents->IsAudioMuted()) {
    contents->SetAudioMuted(false);
    mute_button_->ChangeMode(vivaldi::MuteButton::Mode::kAudible, false);
  } else {
    contents->SetAudioMuted(true);
    mute_button_->ChangeMode(vivaldi::MuteButton::Mode::kMute, false);
  }
}

void VideoOverlayWindowViews::InitVivaldiControls() {
  if (!vivaldi::IsVivaldiRunning())
    return;

  video_pip_controller_ = std::make_unique<vivaldi::VideoPIPController>(
      video_pip_delegate_.get(), controller_->GetWebContents());

  auto mute_button = std::make_unique<vivaldi::MuteButton>(base::BindRepeating(
      [](VideoOverlayWindowViews* overlay) {
        overlay->HandleVivaldiMuteButton();
      },
      base::Unretained(this)));

  mute_button->SetPaintToLayer(ui::LAYER_TEXTURED);
  mute_button->layer()->SetFillsBoundsOpaquely(false);
  mute_button->layer()->SetName("MuteControlsView");
  mute_button_ =
      GetControlsContainerView()->AddChildView(std::move(mute_button));

  content::WebContents* contents = controller_->GetWebContents();

  mute_button_->ChangeMode(contents->IsAudioMuted()
                               ? vivaldi::MuteButton::Mode::kMute
                               : vivaldi::MuteButton::Mode::kAudible,
                           true);

  auto volume_slider_view =
      std::make_unique<vivaldi::VolumeSlider>(video_pip_controller_.get());

  volume_slider_ =
      GetControlsContainerView()->AddChildView(std::move(volume_slider_view));
}

void VideoOverlayWindowViews::UpdateVivaldiControlsVisibility(bool is_visible) {
  if (mute_button_) {
    mute_button_->SetVisible(is_visible);
  }
  if (volume_slider_) {
    volume_slider_->SetVisible(is_visible);
  }
}

void VideoOverlayWindowViews::UpdateVivaldiControlsBounds(int primary_control_y,
                                                          int margin) {
  if (!vivaldi::IsVivaldiRunning())
    return;

  //  <MUTE> #######volume####### <Prev>[<PLAY/PAUSE>]

  gfx::Point mutebutton_position = gfx::Point(
      margin,
      GetBounds().size().height() -
          ((kVivaldiButtonControlSize.height()) + (kVolumeSliderHeight) / 2));

  mute_button_->SetSize(kVivaldiButtonControlSize);
  mute_button_->SetPosition(mutebutton_position);

  int slider_end = show_previous_track_button_
                       ? GetPreviousTrackControlsBounds().x()
                       : GetPlayPauseControlsBounds().x();
  gfx::Point volumeslider_position = gfx::Point(
      kVivaldiButtonControlSize.width() + margin,
      GetBounds().size().height() - (kVivaldiButtonControlSize.height() * 2));

  volume_slider_->SetSize(
      gfx::Size((slider_end - volumeslider_position.x()), kVolumeSliderHeight));
  volume_slider_->SetPosition(volumeslider_position);
}

void VideoOverlayWindowViews::HandleVivaldiGestureEvent(
    ui::GestureEvent* event) {
  if (!vivaldi::IsVivaldiRunning())
    return;

  bool handled = false;
  if (!handled) {
    HandleVivaldiMuteButton();
  }
}

bool VideoOverlayWindowViews::IsPointInVivaldiControl(const gfx::Point& point) {
  if (!vivaldi::IsVivaldiRunning())
    return false;

  if ((mute_button_ && mute_button_->GetMirroredBounds().Contains(point)) ||
      (volume_slider_ && volume_slider_->GetMirroredBounds().Contains(point))) {
    return true;
  }
  return false;
}
