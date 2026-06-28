// Copyright (c) 2015 Vivaldi Technologies

#ifndef PREFS_NATIVE_SETTINGS_OBSERVER_MAC_H_
#define PREFS_NATIVE_SETTINGS_OBSERVER_MAC_H_

#include "prefs/native_settings_observer.h"

class Profile;
@class AppkitSettingsObserver;

namespace vivaldi {

class NativeSettingsObserverMac : public NativeSettingsObserver {
 public:
  explicit NativeSettingsObserverMac(Profile* profile);
  ~NativeSettingsObserverMac() override;

 private:
  AppkitSettingsObserver* appkitObserver;
};

}  // namespace vivaldi

#endif  // PREFS_NATIVE_SETTINGS_OBSERVER_MAC_H_
