// Copyright 2025 Vivaldi Technologies. All rights reserved.

#ifndef IOS_SEARCH_VIVALDI_ADD_SEARCH_ENGINE_JAVA_SCRIPT_FEATURE_H_
#define IOS_SEARCH_VIVALDI_ADD_SEARCH_ENGINE_JAVA_SCRIPT_FEATURE_H_

#include <stdint.h>
#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class WebState;
}  // namespace web

namespace vivaldi {

struct SearchableFormData {
  std::string action_url;
  std::string query_template;
  std::string input_name;
};

class VivaldiAddSearchEngineJavaScriptFeature : public web::JavaScriptFeature {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnFocusedSearchableForm(web::WebState* web_state,
                                         std::optional<SearchableFormData> data,
                                         std::optional<int64_t> request_id) = 0;

   protected:
    ~Observer() override = default;
  };

  static VivaldiAddSearchEngineJavaScriptFeature* GetInstance();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Requests JS to evaluate the currently focused element. Returns true if the
  // JS call was dispatched.
  bool RequestFocusedSearchFormData(web::WebState* web_state,
                                    int64_t request_id);

 private:
  friend class base::NoDestructor<VivaldiAddSearchEngineJavaScriptFeature>;

  VivaldiAddSearchEngineJavaScriptFeature();
  ~VivaldiAddSearchEngineJavaScriptFeature() override;

  VivaldiAddSearchEngineJavaScriptFeature(
      const VivaldiAddSearchEngineJavaScriptFeature&) = delete;
  VivaldiAddSearchEngineJavaScriptFeature& operator=(
      const VivaldiAddSearchEngineJavaScriptFeature&) = delete;

  // web::JavaScriptFeature
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceived(web::WebState* web_state,
                             const web::ScriptMessage& script_message) override;

  base::ObserverList<Observer, true> observers_;
};

}  // namespace vivaldi

#endif  // IOS_SEARCH_VIVALDI_ADD_SEARCH_ENGINE_JAVA_SCRIPT_FEATURE_H_
