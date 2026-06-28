// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#import "ios/bankid/vivaldi_bankid_form_navigation_java_script_feature.h"

#import "base/no_destructor.h"

namespace {
const char kScriptName[] = "vivaldi_bankid_form_navigation_workaround";
}  // namespace

namespace vivaldi {

// static
VivaldiBankIDFormNavigationJavaScriptFeature*
VivaldiBankIDFormNavigationJavaScriptFeature::GetInstance() {
  static base::NoDestructor<VivaldiBankIDFormNavigationJavaScriptFeature>
      instance;
  return instance.get();
}

VivaldiBankIDFormNavigationJavaScriptFeature::
    VivaldiBankIDFormNavigationJavaScriptFeature()
    : web::JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                             {FeatureScript::CreateWithFilename(
                                 kScriptName,
                                 FeatureScript::InjectionTime::kDocumentStart,
                                 FeatureScript::TargetFrames::kAllFrames,
                                 FeatureScript::ReinjectionBehavior::
                                     kReinjectOnDocumentRecreation)}) {}

VivaldiBankIDFormNavigationJavaScriptFeature::
    ~VivaldiBankIDFormNavigationJavaScriptFeature() = default;

}  // namespace vivaldi
