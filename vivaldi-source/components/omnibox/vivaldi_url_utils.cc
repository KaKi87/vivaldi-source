// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved
#include "vivaldi_url_utils.h"

#if !BUILDFLAG(IS_IOS)
#include "thirdparty/brave/common/url_cleaning.h"
#endif  // BUILDFLAG(IS_IOS)
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "url/android/gurl_android.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "chrome/browser/ui/android/toolbar/jni_headers/VivaldiUrlUtils_jni.h"
#endif  // End IS_ANDROID

GURL CopyUrlWithoutParameters(const GURL& gurl) {
  if (!gurl.is_valid() || !gurl.has_query()) {
    return gurl;
  }

  // TODO: Needs profile ptr...: return CleanURL(profile, gurl);
  return gurl;
}

#if BUILDFLAG(IS_ANDROID)
static jni_zero::ScopedJavaLocalRef<jobject>
JNI_VivaldiUrlUtils_CopyUrlWithoutParameters(
    JNIEnv* env,
    const jni_zero::JavaRef<jobject>& j_url) {
  GURL gurl =
      CopyUrlWithoutParameters(url::GURLAndroid::ToNativeGURL(env, j_url));
  return url::GURLAndroid::FromNativeGURL(env, gurl);
}

DEFINE_JNI_FOR_VivaldiUrlUtils()
#endif  // End IS_ANDROID
