// Copyright (c) 2018 Vivaldi Technologies AS. All rights reserved

#ifndef UI_VIVALDI_LOCATION_BAR_H_
#define UI_VIVALDI_LOCATION_BAR_H_

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/location_bar/location_bar.h"

namespace content {
class WebContents;
}

class VivaldiBrowserWindow;

/*
This is the Vivaldi implementation of the LocationBar class, but
for the most part it's empty.
*/
class VivaldiLocationBar : public LocationBar {
 public:
  VivaldiLocationBar(VivaldiBrowserWindow& window);
  ~VivaldiLocationBar() override;
  VivaldiLocationBar(const VivaldiLocationBar&) = delete;
  VivaldiLocationBar& operator=(const VivaldiLocationBar&) = delete;

  // The details necessary to open the user's desired omnibox match.
  void FocusLocation(bool select_all) override {}
  void FocusSearch() override {}
  void UpdateContentSettingsIcons() override;

  // TODO?
  void SaveStateToContents(content::WebContents* contents) override {}
  void Revert() override {}

  OmniboxView* GetOmniboxView() override;

  OmniboxController* GetOmniboxController() override;

  content::WebContents* GetWebContents() override;

  LocationBarModel* GetLocationBarModel() override;

  std::optional<bubble_anchor_util::AnchorConfiguration> GetChipAnchor()
      override;

  void OnChanged() override {}

  void UpdateWithoutTabRestore() override {}

  ChipController* GetChipController() override;

  ui::TrackedElement* GetAnchorOrNull() override;

  Browser* GetBrowser() override;

  bool IsVisible() const override;

  bool IsDrawn() const override;

  bool IsTopLevelFullscreen() const override;

  bool IsEditingOrEmpty() const override;

  void InvalidateLayout() override {}

  gfx::Rect Bounds() const override;

  gfx::Size MinimumSize() const override;

  gfx::Size PreferredSize() const override;

  void Update(content::WebContents* contents) override {}

  void ResetTabState(content::WebContents* contents) override {}

  bool HasSecurityStateChanged() override;

  LocationBarTesting* GetLocationBarForTesting() override;

 private:
  // Owner
  const raw_ref<const VivaldiBrowserWindow> window_;
};

#endif  // UI_VIVALDI_LOCATION_BAR_H_
