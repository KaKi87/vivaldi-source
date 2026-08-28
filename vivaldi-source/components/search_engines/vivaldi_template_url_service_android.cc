// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved
#include "components/search_engines/android/template_url_service_android.h"

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/strings/utf_ostream_operators.h"
#include "base/strings/utf_string_conversions.h"
#include "components/search_engines/android/template_url_android.h"
#include "components/search_engines/search_engines_helper.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"

using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

void TemplateUrlServiceAndroid::VivaldiSetDefaultOverride(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj,
    const base::android::JavaRef<jstring>& jkeyword) {
  std::u16string keyword(
      base::android::ConvertJavaStringToUTF16(env, jkeyword));
  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForKeyword(keyword);
  template_url_service_->VivaldiSetDefaultOverride(template_url);
}

void TemplateUrlServiceAndroid::VivaldiResetDefaultOverride(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj) {
  template_url_service_->VivaldiResetDefaultOverride();
}

base::android::ScopedJavaLocalRef<jobject>
TemplateUrlServiceAndroid::VivaldiGetDefaultSearchEngine(
    JNIEnv* env,
    const JavaRef<jobject>& obj,
    jint type) {
  const TemplateURL* default_search_provider =
      template_url_service_->GetDefaultSearchProvider(
          TemplateURLService::DefaultSearchType(type));
  if (default_search_provider == nullptr) {
    return base::android::ScopedJavaLocalRef<jobject>();
  }
  return CreateTemplateUrlAndroid(env, default_search_provider);
}

base::android::ScopedJavaLocalRef<jobject>
TemplateUrlServiceAndroid::VivaldiGetSearchEngineForHost(
    JNIEnv* env,
    const JavaRef<jobject>& obj,
    const JavaRef<jstring>& jhost) {
  std::string host = base::android::ConvertJavaStringToUTF8(jhost);
  const TemplateURL* search_provider =
      template_url_service_->GetTemplateURLForHost(host);
  if (search_provider == nullptr) {
    return base::android::ScopedJavaLocalRef<jobject>();
  }
  return CreateTemplateUrlAndroid(env, search_provider);
}

// Adds a custom search engine to TemplateURLService.
void TemplateUrlServiceAndroid::AddCustomSearchEngine(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj,
    const JavaRef<jstring>& short_name,
    const JavaRef<jstring>& nickname,
    const JavaRef<jstring>& searchable_jurl,
    const JavaRef<jstring>& suggestion_jurl,
    const JavaRef<jstring>& jsearch_url_post_params,
    const JavaRef<jstring>& jimage_url,
    const JavaRef<jstring>& jimage_url_post_params) {
  std::string searchable_url =
      base::android::ConvertJavaStringToUTF8(env, searchable_jurl);
  std::string suggest_url =
      base::android::ConvertJavaStringToUTF8(env, suggestion_jurl);

  TemplateURLData data;
  data.safe_for_autoreplace = false;
  // VAB-13360
  data.is_active = TemplateURLData::ActiveStatus::kTrue;
  data.SetShortName(base::android::ConvertJavaStringToUTF16(short_name));
  data.SetKeyword(base::android::ConvertJavaStringToUTF16(nickname));
  data.SetURL(searchable_url);
  data.suggestions_url = suggest_url;

  std::string search_url_post_params;
  if (jsearch_url_post_params) {
    search_url_post_params =
        base::android::ConvertJavaStringToUTF8(env, jsearch_url_post_params);
    data.search_url_post_params = search_url_post_params;
  }
  std::string image_url;
  if (jimage_url) {
    image_url = base::android::ConvertJavaStringToUTF8(env, jimage_url);
    data.image_url = image_url;
  }
  std::string image_url_post_params;
  if (jimage_url_post_params) {
    image_url_post_params =
        base::android::ConvertJavaStringToUTF8(env, jimage_url_post_params);
    data.image_url_post_params = image_url_post_params;
  }
  template_url_service_->Add(std::make_unique<TemplateURL>(data));
}

base::android::ScopedJavaLocalRef<jobject>
TemplateUrlServiceAndroid::GetTemplateUrlFromKeywordNative(
    JNIEnv* env,
    const JavaRef<jobject>& obj,
    const JavaRef<jstring>& jkeyword) {
  std::u16string keyword =
      base::android::ConvertJavaStringToUTF16(env, jkeyword);
  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForKeyword(keyword);
  if (!template_url)
    return base::android::ScopedJavaLocalRef<jstring>();
  if (template_url == nullptr) {
    return base::android::ScopedJavaLocalRef<jobject>();
  }
  return CreateTemplateUrlAndroid(env, template_url);
}

void TemplateUrlServiceAndroid::RemoveTemplateUrl(
    JNIEnv* env,
    const JavaRef<jobject>& obj,
    const JavaRef<jstring>& jkeyword) {
  std::u16string keyword =
      base::android::ConvertJavaStringToUTF16(env, jkeyword);
  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForKeyword(keyword);
  template_url_service_->Remove(template_url);
}

// Updates TemplateUrl with new information to TemplateURLService.
jboolean TemplateUrlServiceAndroid::UpdateTemplateUrl(
    JNIEnv* env,
    const JavaRef<jobject>& obj,
    const JavaRef<jstring>& jkeyword,
    const JavaRef<jstring>& jnew_keyword,
    const JavaRef<jstring>& jshort_name,
    const JavaRef<jstring>& searchable_jurl,
    const JavaRef<jstring>& suggestion_jurl,
    const JavaRef<jstring>& jsearch_url_post_params,
    const JavaRef<jstring>& jimage_url,
    const JavaRef<jstring>& jimage_url_post_params) {
  std::u16string keyword =
      base::android::ConvertJavaStringToUTF16(env, jkeyword);
  std::string searchable_url =
      base::android::ConvertJavaStringToUTF8(env, searchable_jurl);
  std::string suggest_url =
      base::android::ConvertJavaStringToUTF8(env, suggestion_jurl);
  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForKeyword(keyword);
  TemplateURLData data =
      template_url_service_->GetTemplateURLForKeyword(keyword)->data();

  data.SetShortName(base::android::ConvertJavaStringToUTF16(env, jshort_name));
  data.SetKeyword(base::android::ConvertJavaStringToUTF16(env, jnew_keyword));
  data.SetURL(searchable_url);
  data.suggestions_url = suggest_url;
  std::string search_url_post_params;
  if (jsearch_url_post_params) {
    search_url_post_params =
        base::android::ConvertJavaStringToUTF8(env, jsearch_url_post_params);
    data.search_url_post_params = search_url_post_params;
  }
  std::string image_url;
  if (jimage_url) {
    image_url = base::android::ConvertJavaStringToUTF8(env, jimage_url);
    data.image_url = image_url;
  }
  std::string image_url_post_params;
  if (jimage_url_post_params) {
    image_url_post_params =
        base::android::ConvertJavaStringToUTF8(env, jimage_url_post_params);
    data.image_url_post_params = image_url_post_params;
  }
  bool updated = template_url_service_->UpdateData(template_url, data);
  return updated;
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::GetUrlToDisplayBridge(JNIEnv* env,
                                                 const JavaRef<jobject>& obj,
                                                 const JavaRef<jstring>& jurl) {
  std::string url = base::android::ConvertJavaStringToUTF8(jurl);
  return base::android::ConvertUTF8ToJavaString(env, GetUrlToDisplay(url));
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::GetUrlFromDisplayBridge(
    JNIEnv* env,
    const JavaRef<jobject>& obj,
    const JavaRef<jstring>& jurl) {
  std::string url = base::android::ConvertJavaStringToUTF8(jurl);
  return base::android::ConvertUTF8ToJavaString(env, GetUrlFromDisplay(url));
}
