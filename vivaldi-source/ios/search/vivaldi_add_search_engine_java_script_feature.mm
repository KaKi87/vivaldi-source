// Copyright 2025 Vivaldi Technologies. All rights reserved.

#include "ios/search/vivaldi_add_search_engine_java_script_feature.h"

#include "base/values.h"
#include "ios/web/public/js_messaging/java_script_feature_util.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace {
constexpr char kHandlerName[] = "VivaldiSearchableForm";
constexpr char kCommandKey[] = "command";
constexpr char kFocusedFormCommand[] = "focusedForm";
constexpr char kValidKey[] = "valid";
constexpr char kActionUrlKey[] = "actionUrl";
constexpr char kQueryTemplateKey[] = "queryTemplate";
constexpr char kInputNameKey[] = "inputName";
constexpr char kApiEntryPoint[] =
    "vivaldiSearchableForm.requestFocusedSearchableForm";
}  // namespace

namespace vivaldi {

VivaldiAddSearchEngineJavaScriptFeature*
VivaldiAddSearchEngineJavaScriptFeature::GetInstance() {
  static base::NoDestructor<VivaldiAddSearchEngineJavaScriptFeature> instance;
  return instance.get();
}

VivaldiAddSearchEngineJavaScriptFeature::
    VivaldiAddSearchEngineJavaScriptFeature()
    : web::JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                             {FeatureScript::CreateWithFilename(
                                 "vivaldi_searchable_form",  // builder target
                                 FeatureScript::InjectionTime::kDocumentStart,
                                 FeatureScript::TargetFrames::kAllFrames,
                                 FeatureScript::ReinjectionBehavior::
                                     kReinjectOnDocumentRecreation)},
                             {}) {}

VivaldiAddSearchEngineJavaScriptFeature::~
    VivaldiAddSearchEngineJavaScriptFeature() = default;

void VivaldiAddSearchEngineJavaScriptFeature::AddObserver(
    Observer* observer) {
  observers_.AddObserver(observer);
}

void VivaldiAddSearchEngineJavaScriptFeature::RemoveObserver(
    Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool VivaldiAddSearchEngineJavaScriptFeature::RequestFocusedSearchFormData(
    web::WebState* web_state) {
  if (!web_state) {
    return false;
  }
  web::WebFrame* frame =
      web_state->GetPageWorldWebFramesManager()->GetMainWebFrame();
  if (!frame) {
    return false;
  }
  return CallJavaScriptFunction(frame, kApiEntryPoint, base::Value::List());
}

std::optional<std::string>
VivaldiAddSearchEngineJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kHandlerName;
}

void VivaldiAddSearchEngineJavaScriptFeature::ScriptMessageReceived(
    web::WebState* web_state,
    const web::ScriptMessage& script_message) {
  if (!web_state) {
    return;
  }
  if (observers_.empty()) {
    return;
  }
  const base::Value* body = script_message.body();
  if (!body) {
    return;
  }
  const base::Value::Dict* dict = body->GetIfDict();
  if (!dict) {
    return;
  }
  const std::string* command = dict->FindString(kCommandKey);
  if (!command || *command != kFocusedFormCommand) {
    return;
  }

  bool valid = dict->FindBool(kValidKey).value_or(true);
  std::optional<SearchableFormData> payload;
  if (valid) {
    const std::string* action_url = dict->FindString(kActionUrlKey);
    const std::string* query_template = dict->FindString(kQueryTemplateKey);
    const std::string* input_name = dict->FindString(kInputNameKey);
    if (action_url && query_template && input_name) {
      SearchableFormData data;
      data.action_url = *action_url;
      data.query_template = *query_template;
      data.input_name = *input_name;
      payload = data;
    }
  }

  for (Observer& observer : observers_) {
    observer.OnFocusedSearchableForm(web_state, payload);
  }
}

}  // namespace vivaldi
