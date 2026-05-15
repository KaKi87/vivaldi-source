// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef IOS_BANKID_VIVALDI_BANKID_FORM_NAVIGATION_JAVA_SCRIPT_FEATURE_H_
#define IOS_BANKID_VIVALDI_BANKID_FORM_NAVIGATION_JAVA_SCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace vivaldi {

class VivaldiBankIDFormNavigationJavaScriptFeature
    : public web::JavaScriptFeature {
 public:
  static VivaldiBankIDFormNavigationJavaScriptFeature* GetInstance();

 private:
  friend class base::NoDestructor<
      VivaldiBankIDFormNavigationJavaScriptFeature>;

  VivaldiBankIDFormNavigationJavaScriptFeature();
  ~VivaldiBankIDFormNavigationJavaScriptFeature() override;

  VivaldiBankIDFormNavigationJavaScriptFeature(
      const VivaldiBankIDFormNavigationJavaScriptFeature&) = delete;
  VivaldiBankIDFormNavigationJavaScriptFeature& operator=(
      const VivaldiBankIDFormNavigationJavaScriptFeature&) = delete;
};

}  // namespace vivaldi

#endif  // IOS_BANKID_VIVALDI_BANKID_FORM_NAVIGATION_JAVA_SCRIPT_FEATURE_H_
