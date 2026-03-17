// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#ifndef VIVALDI_ACCOUNT_VIVALDI_ACCOUNT_MANAGER_ANDROID_H_
#define VIVALDI_ACCOUNT_VIVALDI_ACCOUNT_MANAGER_ANDROID_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "vivaldi_account/vivaldi_account_manager.h"

class Profile;

class VivaldiAccountManagerAndroid
    : public vivaldi::VivaldiAccountManager::Observer {
 public:
  VivaldiAccountManagerAndroid(JNIEnv* env,
                               const base::android::JavaRef<jobject>& obj);
  ~VivaldiAccountManagerAndroid() override;
  VivaldiAccountManagerAndroid(const VivaldiAccountManagerAndroid&) = delete;
  VivaldiAccountManagerAndroid& operator=(const VivaldiAccountManagerAndroid&) =
      delete;

  static void CreateNow();

  void Login(JNIEnv* env,
             const base::android::JavaRef<jobject>& obj,
             const base::android::JavaRef<jstring>& username,
             const base::android::JavaRef<jstring>& password,
             jboolean save_password);
  void Logout(JNIEnv* env, const base::android::JavaRef<jobject>& obj);
  void SetSessionName(JNIEnv* env,
                      const base::android::JavaRef<jobject>& obj,
                      const base::android::JavaRef<jstring>& session_name);

  base::android::ScopedJavaLocalRef<jobject> GetPendingRegistration(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& obj);
  jboolean SetPendingRegistration(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& obj,
      const base::android::JavaRef<jstring>& username,
      const base::android::JavaRef<jstring>& password,
      const base::android::JavaRef<jstring>& recovery_email);
  void ResetPendingRegistration(JNIEnv* env,
                                const base::android::JavaRef<jobject>& obj);

  // VivaldiAccountManager::Observer implementation
  void OnVivaldiAccountUpdated() override;
  void OnTokenFetchSucceeded() override;
  void OnTokenFetchFailed() override;
  void OnVivaldiAccountShutdown() override;

 private:
  const raw_ptr<Profile> profile_;
  raw_ptr<vivaldi::VivaldiAccountManager> account_manager_;

  void SendStateUpdate();

  JavaObjectWeakGlobalRef weak_java_ref_;
};

#endif  // VIVALDI_ACCOUNT_VIVALDI_ACCOUNT_MANAGER_H_
