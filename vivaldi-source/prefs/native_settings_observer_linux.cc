// Copyright (c) 2016 Vivaldi Technologies. All Rights Reserved.

#include "prefs/native_settings_observer_linux.h"

#include "base/logging.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "prefs/native_settings_observer_linux.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"
namespace vivaldi {

// static
NativeSettingsObserver* NativeSettingsObserver::Create(Profile* profile) {
  return new NativeSettingsObserverLinux(profile);
}

NativeSettingsObserverLinux::NativeSettingsObserverLinux(Profile* profile)
    : NativeSettingsObserver(profile) {}

void NativeSettingsObserverLinux::HandleThemeChange(
    ui::NativeTheme::PreferredColorScheme color_scheme) {
  PrefService* prefs = ProfileManager::GetLastUsedProfile()->GetPrefs();
  switch (color_scheme) {
    case ui::NativeTheme::PreferredColorScheme::kDark:
      prefs->SetInteger(vivaldiprefs::kSystemDesktopThemeColor, 1);
      break;
    default:
      prefs->SetInteger(vivaldiprefs::kSystemDesktopThemeColor, 0);
  }
}
}  // namespace vivaldi
