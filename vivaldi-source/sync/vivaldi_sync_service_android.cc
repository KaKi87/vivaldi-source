// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "sync/vivaldi_sync_service_android.h"

#include "app/vivaldi_apptools.h"
#include "base/android/jni_string.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/android/chrome_jni_headers/VivaldiSyncService_jni.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "components/sync/service/sync_token_status.h"
#include "sync/vivaldi_sync_service_impl.h"
#include "sync/vivaldi_sync_ui_helpers.h"

static jlong JNI_VivaldiSyncService_Init(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj) {
  VivaldiSyncServiceAndroid* sync_service_android =
      new VivaldiSyncServiceAndroid(env, obj);
  if (!sync_service_android->Init(env)) {
    delete sync_service_android;
    return 0;
  }
  return reinterpret_cast<intptr_t>(sync_service_android);
}

VivaldiSyncServiceAndroid::VivaldiSyncServiceAndroid(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj)
    : weak_java_ref_(env, obj) {}

VivaldiSyncServiceAndroid::~VivaldiSyncServiceAndroid() {
  if (sync_service_)
    sync_service_->RemoveObserver(this);
}

bool VivaldiSyncServiceAndroid::Init(JNIEnv* env) {
  // VAB-12889: Guard against launch before native init is complete.
  Profile* profile = ProfileManager::GetActiveUserProfile();
  if (!profile)
    return false;
  sync_service_ = SyncServiceFactory::GetForProfile(profile);
  if (!sync_service_)
    return false;
  sync_service_->AddObserver(this);
  SendCycleData();
  return true;
}

jboolean VivaldiSyncServiceAndroid::SetEncryptionPassword(
    JNIEnv* env,
    const base::android::JavaRef<jstring>& password) {
  if (!sync_service_)
    return false;
  return vivaldi::sync_ui_helpers::SetEncryptionPassword(
      sync_service_, base::android::ConvertJavaStringToUTF8(env, password));
}

void VivaldiSyncServiceAndroid::ClearServerData(JNIEnv* env) {
  if (sync_service_ && vivaldi::IsVivaldiRunning())
    sync_service_->ClearSyncData();
}

jboolean VivaldiSyncServiceAndroid::HasServerError(JNIEnv* env) {
  if (!sync_service_)
    return false;
  return sync_service_->GetSyncTokenStatusForDebugging().connection_status ==
         syncer::CONNECTION_SERVER_ERROR;
}

jboolean VivaldiSyncServiceAndroid::IsSetupInProgress(JNIEnv* env) {
  if (!sync_service_)
    return false;
  return sync_service_->IsSetupInProgress();
}

base::android::ScopedJavaLocalRef<jstring>
VivaldiSyncServiceAndroid::GetBackupEncryptionToken(JNIEnv* env) {
  if (!sync_service_)
    return base::android::ConvertUTF8ToJavaString(env, "");
  return base::android::ConvertUTF8ToJavaString(
      env, sync_service_->GetEncryptionBootstrapTokenForBackup().value_or(""));
}

jboolean VivaldiSyncServiceAndroid::RestoreEncryptionToken(
    JNIEnv* env,
    const base::android::JavaRef<jstring>& token) {
  if (!sync_service_)
    return false;
  return sync_service_->ResetEncryptionBootstrapTokenFromBackup(
      base::android::ConvertJavaStringToUTF8(env, token));
}

jboolean VivaldiSyncServiceAndroid::CanSyncFeatureStart(JNIEnv* env) {
  if (!sync_service_)
    return false;
  return sync_service_->CanSyncFeatureStart();
}

void VivaldiSyncServiceAndroid::SendCycleData() {
  if (!sync_service_)
    return;
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_ref_.get(env);

  vivaldi::sync_ui_helpers::CycleData cycle_data =
      vivaldi::sync_ui_helpers::GetCycleData(sync_service_);

  Java_VivaldiSyncService_onCycleData(
      env, obj, cycle_data.download_updates_status, cycle_data.commit_status,
      cycle_data.cycle_start_time.InMillisecondsSinceUnixEpoch(),
      cycle_data.next_retry_time.InMillisecondsSinceUnixEpoch());
}

void VivaldiSyncServiceAndroid::OnSyncCycleCompleted(
    syncer::SyncService* sync) {
  SendCycleData();
}

void VivaldiSyncServiceAndroid::OnSyncShutdown(syncer::SyncService* sync) {
  sync->RemoveObserver(this);
  sync_service_ = nullptr;
}

DEFINE_JNI_FOR_VivaldiSyncService()
