#pragma once

#include <map>
#include "chrome/browser/extensions/api/side_panel/side_panel_service.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"

class BrowserWindowInterface;
class Profile;

namespace vivaldi {

class SidePanelCoordinator : public SidePanelUI,
                             public extensions::SidePanelService::Observer {
 public:
  SidePanelCoordinator(BrowserWindowInterface*);

  ~SidePanelCoordinator() override;

  void Show(SidePanelEntryId entry_id,
            std::optional<SidePanelOpenTrigger> open_trigger,
            bool suppress_animations) override;

  void Show(SidePanelEntryKey entry_id,
            std::optional<SidePanelOpenTrigger> open_trigger,
            bool suppress_animations) override;

  void ShowFrom(SidePanelEntryKey entry_key,
                gfx::Rect starting_bounds) override {}

  void Close(SidePanelEntry::PanelType panel_type,
             SidePanelEntryHideReason hide_reason,
             bool suppress_animations) override;

  void Toggle(SidePanelEntryKey key,
              SidePanelOpenTrigger open_trigger) override;

  // void UpdatePinState() override;

  std::optional<SidePanelEntryId> GetCurrentEntryId(
      SidePanelEntry::PanelType panel_type) const override;

  int GetCurrentEntryDefaultContentWidth(
      SidePanelEntry::PanelType panel_type) const override;

  bool IsSidePanelShowing(SidePanelEntry::PanelType panel_type) const override;

  bool IsSidePanelEntryShowing(
      const SidePanelEntryKey& entry_key) const override;

  bool IsSidePanelEntryShowing(const SidePanelEntryKey& entry_key,
                               bool for_tab) const override;

  base::CallbackListSubscription RegisterSidePanelShown(
      SidePanelEntry::PanelType type,
      ShownCallback callback) override;

  content::WebContents* GetWebContentsForTest(SidePanelEntryId id) override;

  void OnPanelOptionsChanged(const extensions::ExtensionId& extension_id,
                             const extensions::api::side_panel::PanelOptions&
                                 updated_options) override;
  void OnSidePanelServiceShutdown() override;

  void DisableAnimationsForTesting() override {}

  void SetNoDelaysForTesting(bool no_delays_for_testing) override {}

  void OnActiveTabChanged(content::WebContents* old_contents,
                          content::WebContents* new_contents,
                          bool tab_removed_for_deletion) override {}

 private:
  Profile* GetProfile();

  BrowserWindowInterface* browser_;
};

}  // namespace vivaldi
