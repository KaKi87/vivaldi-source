// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_COMPOSITOR_STRIP_TAB_UNDERLINE_MANAGER_H_
#define CHROME_BROWSER_ANDROID_COMPOSITOR_STRIP_TAB_UNDERLINE_MANAGER_H_

#include <map>
#include <memory>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#if BUILDFLAG(GOOGLE_CHROME_BRANDING) // Vivaldi: keep disabled
#include "chrome/browser/glic/browser_ui/tab_underline_controller.h"
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)  // Vivaldi: keep disabled
#include "third_party/jni_zero/jni_zero.h"

namespace android {

class StripTabUnderlineManager {
 public:
  StripTabUnderlineManager(JNIEnv* env, const jni_zero::JavaRef<jobject>& obj);
  ~StripTabUnderlineManager();

  StripTabUnderlineManager(const StripTabUnderlineManager&) = delete;
  StripTabUnderlineManager& operator=(const StripTabUnderlineManager&) = delete;

  // Destroy the native manager.
  void Destroy(JNIEnv* env);

  // Register a tab to be tracked for underlines.
  void RegisterTab(JNIEnv* env, const jni_zero::JavaRef<jobject>& jtab);

  // Unregister a tab from being tracked.
  void UnregisterTab(JNIEnv* env, int32_t tab_id);

  void SetUnderlineState(int tab_id, bool is_underlined);

 private:
  class UiDelegateImpl;

#if BUILDFLAG(GOOGLE_CHROME_BRANDING) // Vivaldi: keep disabled
  struct TabUnderlineContext {
    TabUnderlineContext();
    TabUnderlineContext(
        std::unique_ptr<glic::TabUnderlineController> controller,
        std::unique_ptr<UiDelegateImpl> delegate);
    ~TabUnderlineContext();

    std::unique_ptr<glic::TabUnderlineController> controller;
    std::unique_ptr<UiDelegateImpl> delegate;
  };
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)  // Vivaldi: keep disabled

  base::android::ScopedJavaGlobalRef<jobject> java_obj_;
#if BUILDFLAG(GOOGLE_CHROME_BRANDING) // Vivaldi: keep disabled
  std::map<int, TabUnderlineContext> tracked_tabs_;
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)  // Vivaldi: keep disabled
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_COMPOSITOR_STRIP_TAB_UNDERLINE_MANAGER_H_
