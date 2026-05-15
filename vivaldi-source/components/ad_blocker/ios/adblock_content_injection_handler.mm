// Copyright (c) 2023 Vivaldi Technologies AS. All rights reserved

#import "components/ad_blocker/ios/adblock_content_injection_handler.h"

#import <vector>

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#import "base/containers/adapters.h"
#import "base/containers/lru_cache.h"
#import "base/json/json_string_value_serializer.h"
#import "base/strings/string_number_conversions.h"
#import "base/strings/string_split.h"
#import "base/strings/string_util.h"
#import "base/time/time.h"
#import "components/ad_blocker/core/parser/utils.h"
#import "components/ad_blocker/ios/ios_rule_utils.h"
#import "components/ad_blocker/ios/utils.h"
#import "ios/web/js_messaging/scoped_wk_script_message_handler.h"
#import "ios/web/public/browser_state.h"
#import "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#import "net/base/registry_controlled_domains/registry_controlled_domain.h"
#import "third_party/re2/src/re2/re2.h"
#import "third_party/re2/src/re2/stringpiece.h"

namespace adblock_filter {

namespace {

// We keep the result of the script argument lookups for this amount of hosts.
// Complex websites will have more frames with different hosts than others,
// but hopefully this should cover a few tens of pages on different hosts,
// which should allow a fair amount of navigations without cache misses.
constexpr size_t kScriptArgumentCacheSize = 200;

constexpr char kMessageNamePrefix[] = "vivaldi_adblock_scriptlet_";
constexpr size_t kMessageNamePrefixLength =
    std::string_view(kMessageNamePrefix).length();

/*constexpr char kJSScriptArgRequestPart1[] = "window.webkit.messageHandlers['";
constexpr char kJSScriptArgRequestPart2[] =
    R"JsSource('].postMessage({}).then((scriptlet_arguments) => {
      if(Array.isArray(scriptlet_arguments) && scriptlet_arguments.length != 0) {
        const source=)JsSource";
constexpr char kJSScriptArgRequestPart3[] = R"JsSource(;
        new Function(source)();
  }
});)JsSource";

std::string SourceToTemplatedHexString(std::string_view source) {
  std::string result("`");
  // 200 should be able to hold the expansion of 9 placeholders.
  result.reserve(source.length() * 4 + 200);

  const int kPlaceHolderLength = 5;

  for (auto c = source.begin(); c != source.end(); ++c) {
    if (*c == '{' && source.end() - c >= kPlaceHolderLength) {
      if (*(c + 1) == '{' && base::IsAsciiDigit(*(c + 2)) && *(c + 3) == '}' &&
          *(c + 4) == '}') {
        int index = -1;
        base::StringToInt(std::string_view(c + 2, c + 3), &index);
        result.append("${scriptlet_arguments[");
        result.append(base::NumberToString(index - 1));
        result.append("]}");
        c += 5;
      }
    }

    result.append("\\x");
    result.append(base::HexEncode(std::string_view(c, c + 1)));
  }

  result.push_back('`');
  return result;
}*/

void AddIncludedScriptletIndices(const base::ListValue& included_list,
                                 std::set<int>& included_indices,
                                 std::set<int>& excluded_indices) {
  for (const auto& scriptlet_index : included_list) {
    CHECK(scriptlet_index.is_int());
    if (!excluded_indices.contains(scriptlet_index.GetInt())) {
      included_indices.insert(scriptlet_index.GetInt());
    }
  }
}

void AddExcludedScriptletIndices(const base::ListValue& excluded_list,
                                 std::set<int>& included_indices,
                                 std::set<int>& excluded_indices) {
  for (const auto& scriptlet_index : excluded_list) {
    CHECK(scriptlet_index.is_int());
    excluded_indices.insert(scriptlet_index.GetInt());
    included_indices.erase(scriptlet_index.GetInt());
  }
}

void UpdateScriptletIndices(const base::DictValue& node,
                            std::set<int>& included_indices,
                            std::set<int>& excluded_indices) {
  if (const auto* included = node.FindList(ios_rule_utils::kIncluded)) {
    AddIncludedScriptletIndices(*included, included_indices, excluded_indices);
  }

  if (const auto* excluded = node.FindList(ios_rule_utils::kExcluded)) {
    AddExcludedScriptletIndices(*excluded, included_indices, excluded_indices);
  }
}

class ContentInjectionHandlerImpl : public ContentInjectionHandler,
                                    public Resources::Observer {
 public:
  explicit ContentInjectionHandlerImpl(web::BrowserState* browser_state,
                                       Resources* resources);
  ~ContentInjectionHandlerImpl() override;
  ContentInjectionHandlerImpl(const ContentInjectionHandlerImpl&) = delete;
  ContentInjectionHandlerImpl& operator=(const ContentInjectionHandlerImpl&) =
      delete;

  // Implementing ContentInjectionHandler
  void SetIncognitoBrowserState(web::BrowserState* browser_state) override;
  void SetScriptletInjectionRules(RuleGroup group,
                                  base::DictValue injection_rules) override;

  // Implementing Resources::Observer
  void OnResourcesLoaded() override;

 private:
  void OnNewConfigurationCreated(
      web::WKWebViewConfigurationProvider* config_provider,
      WKWebViewConfiguration* new_config);
  void InjectUserScripts();
  void InjectUserScriptsForController(
      __weak WKUserContentController* user_content_controller);
  void HandlePlaceholderRequest(WKScriptMessage* message,
                                ScriptMessageReplyHandler reply_handler);

  base::WeakPtr<web::WKWebViewConfigurationProvider> config_provider_;
  base::WeakPtr<web::WKWebViewConfigurationProvider> incognito_config_provider_;
  RuleGroupArray<std::optional<base::DictValue>> injection_rules_;
  base::LRUCache<std::string, base::DictValue> script_arguments_cache_;

  Resources* resources_;

  __weak WKUserContentController* user_content_controller_;
  __weak WKUserContentController* incognito_user_content_controller_;

  // CallbackListSubscription members
  base::CallbackListSubscription main_config_subscription_;
  base::CallbackListSubscription incognito_config_subscription_;

  // NOTE(julien): Unclear why, but I am not managing to use a straight vector
  // of ScopedWKScriptMessageHandler and insert with emplace_back.
  // Doing so leads to a compilation error that I can't figure out.
  std::vector<std::unique_ptr<ScopedWKScriptMessageHandler>>
      script_message_handlers_;

  base::WeakPtrFactory<ContentInjectionHandlerImpl> weak_ptr_factory_{this};
};

ContentInjectionHandlerImpl::ContentInjectionHandlerImpl(
    web::BrowserState* browser_state,
    Resources* resources)
    : config_provider_(
          web::WKWebViewConfigurationProvider::FromBrowserState(browser_state)
              .AsWeakPtr()),
      script_arguments_cache_(kScriptArgumentCacheSize),
      resources_(resources) {
  // Register callback for configuration changes in the main profile
  if (config_provider_) {
    // Apply for the existing configuration
    OnNewConfigurationCreated(config_provider_.get(),
                              config_provider_->GetWebViewConfiguration());

    main_config_subscription_ =
        config_provider_->RegisterConfigurationCreatedCallback(
            base::BindRepeating(
                &ContentInjectionHandlerImpl::OnNewConfigurationCreated,
                weak_ptr_factory_.GetWeakPtr(), config_provider_.get()));
  }

  if (!resources_->loaded())
    resources->AddObserver(this);
}

void ContentInjectionHandlerImpl::SetIncognitoBrowserState(
    web::BrowserState* browser_state) {
  if (incognito_config_provider_) {
    incognito_config_subscription_ = base::CallbackListSubscription();
    incognito_config_provider_.reset();
    incognito_user_content_controller_ = nullptr;
  }

  if (!browser_state) {
    return;
  }

  incognito_config_provider_ =
      web::WKWebViewConfigurationProvider::FromBrowserState(browser_state)
          .AsWeakPtr();
  if (incognito_config_provider_) {
    // Apply for the existing configuration
    OnNewConfigurationCreated(
        incognito_config_provider_.get(),
        incognito_config_provider_->GetWebViewConfiguration());

    incognito_config_subscription_ =
        incognito_config_provider_->RegisterConfigurationCreatedCallback(
            base::BindRepeating(
                &ContentInjectionHandlerImpl::OnNewConfigurationCreated,
                weak_ptr_factory_.GetWeakPtr(),
                incognito_config_provider_.get()));
  }
}

ContentInjectionHandlerImpl::~ContentInjectionHandlerImpl() {
  // No need to manually remove observers as
  // subscriptions are handled by CallbackListSubscription.
}

void ContentInjectionHandlerImpl::OnResourcesLoaded() {
  resources_->RemoveObserver(this);
  InjectUserScripts();
}

void ContentInjectionHandlerImpl::OnNewConfigurationCreated(
    web::WKWebViewConfigurationProvider* config_provider,
    WKWebViewConfiguration* new_config) {
  if (config_provider == config_provider_.get()) {
    user_content_controller_ = new_config.userContentController;
    if (resources_->loaded())
      InjectUserScriptsForController(user_content_controller_);
  } else if (config_provider == incognito_config_provider_.get()) {
    incognito_user_content_controller_ = new_config.userContentController;
    if (resources_->loaded())
      InjectUserScriptsForController(incognito_user_content_controller_);
  }
}

void ContentInjectionHandlerImpl::SetScriptletInjectionRules(
    RuleGroup group,
    base::DictValue injection_rules) {
  injection_rules_[group] = std::move(injection_rules);
  script_arguments_cache_.Clear();
}

void ContentInjectionHandlerImpl::InjectUserScripts() {
  if (user_content_controller_)
    InjectUserScriptsForController(user_content_controller_);
  if (incognito_user_content_controller_)
    InjectUserScriptsForController(incognito_user_content_controller_);
}

void ContentInjectionHandlerImpl::InjectUserScriptsForController(
    __weak WKUserContentController* user_content_controller) {
/*  std::map<std::string, Resources::InjectableResource> injections =
      resources_->GetInjections();

  WKContentWorld* content_world =
      [WKContentWorld worldWithName:@"vivaldi_adblock_user_scripts"];

  for (const auto& [name, injectable_resource] : injections) {
    // We don't support other scriptlets yet.
    if (name != kAbpSnippetsMainScriptletName &&
        name != kAbpSnippetsIsolatedScriptletName) {
      continue;
    }
    std::string message_name(kMessageNamePrefix);
    message_name.append(name);

    std::string patched_source(kJSScriptArgRequestPart1);
    patched_source.append(message_name);
    patched_source.append(kJSScriptArgRequestPart2);
    patched_source.append(SourceToTemplatedHexString(injectable_resource.code));
    patched_source.append(kJSScriptArgRequestPart3);

    script_message_handlers_.emplace_back(
        std::make_unique<ScopedWKScriptMessageHandler>(
            user_content_controller,
            [NSString stringWithUTF8String:message_name.c_str()],
            injectable_resource.use_main_world ? WKContentWorld.pageWorld
                                               : content_world,
            base::BindRepeating(
                &ContentInjectionHandlerImpl::HandlePlaceholderRequest,
                weak_ptr_factory_.GetWeakPtr())));
    WKUserScript* user_script = [[WKUserScript alloc]
          initWithSource:[NSString stringWithUTF8String:patched_source.c_str()]
           injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:false
          inContentWorld:injectable_resource.use_main_world
                             ? WKContentWorld.pageWorld
                             : content_world];
    [user_content_controller addUserScript:user_script];
  }*/
}

void ContentInjectionHandlerImpl::HandlePlaceholderRequest(
    WKScriptMessage* message,
    ScriptMessageReplyHandler reply_handler) {
  std::string host([message.frameInfo.securityOrigin.host
      cStringUsingEncoding:NSUTF8StringEncoding]);

  std::string message_name(
      [message.name cStringUsingEncoding:NSUTF8StringEncoding]);
  CHECK(base::StartsWith(message_name, kMessageNamePrefix));
  std::string scriptlet_name = message_name.substr(kMessageNamePrefixLength);

  // We don't support other scriptlets yet.
  CHECK(scriptlet_name == kAbpSnippetsMainScriptletName ||
        scriptlet_name == kAbpSnippetsIsolatedScriptletName);

  auto cached = script_arguments_cache_.Get(host);
  if (cached != script_arguments_cache_.end()) {
    reply_handler(cached->second.Find(scriptlet_name), nil);
    return;
  }

  const auto labels = base::SplitStringPiece(host, ".", base::TRIM_WHITESPACE,
                                             base::SPLIT_WANT_NONEMPTY);
  size_t registry_length =
      net::registry_controlled_domains::GetCanonicalHostRegistryLength(
          host, net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  std::map<std::string,
           std::set<const base::ListValue*, ContentInjectionArgumentsCompare>>
      selected_arguments_lists_per_scriptlet;

  for (auto [group, injection_rules] : injection_rules_) {
    if (!injection_rules) {
      continue;
    }

    const base::DictValue* domain_tree =
        injection_rules->FindDict(ios_rule_utils::kDomainConstraints);

    const base::DictValue* hostname_node =
        domain_tree->FindDict(ios_rule_utils::kDomainTreeHostnameNode);
    const base::DictValue* entities_node = nullptr;
    const base::DictValue* included_regexes =
        domain_tree->FindDict(ios_rule_utils::kDomainTreeIncludedRegexes);
    const base::DictValue* excluded_regexes =
        domain_tree->FindDict(ios_rule_utils::kDomainTreeExcludedRegexes);

    std::set<int> included_scriptlet_indices;
    std::set<int> excluded_scriptlet_indices;
    size_t seen_length = 0;

    for (const auto& label : base::Reversed(labels)) {
      if (hostname_node) {
        hostname_node = hostname_node->FindDict(label);
      }
      if (entities_node) {
        entities_node = entities_node->FindDict(label);
      }

      if (hostname_node) {
        UpdateScriptletIndices(*hostname_node, included_scriptlet_indices,
                               excluded_scriptlet_indices);
      }
      if (entities_node) {
        UpdateScriptletIndices(*entities_node, included_scriptlet_indices,
                               excluded_scriptlet_indices);
      }

      seen_length += label.length() + 1;
      if (seen_length == registry_length + 1) {
        entities_node =
            domain_tree->FindDict(ios_rule_utils::kDomainTreeEntityNode);
      }
    }

    for (const auto [regex, snippet_indices] : *included_regexes) {
      if (re2::RE2::PartialMatch(host, re2::RE2(regex))) {
        CHECK(snippet_indices.is_list());
        AddIncludedScriptletIndices(snippet_indices.GetList(),
                                    included_scriptlet_indices,
                                    excluded_scriptlet_indices);
      }
    }

    for (const auto [regex, snippet_indices] : *excluded_regexes) {
      if (re2::RE2::PartialMatch(host, re2::RE2(regex))) {
        CHECK(snippet_indices.is_list());
        AddExcludedScriptletIndices(snippet_indices.GetList(),
                                    included_scriptlet_indices,
                                    excluded_scriptlet_indices);
      }
    }

    const base::ListValue* scriptlets =
        injection_rules->FindList(ios_rule_utils::kScriptletRules);

    for (int scriptlet_index : included_scriptlet_indices) {
      const base::Value& scriptlet = (*scriptlets)[scriptlet_index];
      CHECK(scriptlet.is_dict());
      for (const auto [name, arguments] : scriptlet.GetDict()) {
        CHECK(arguments.is_list());
        selected_arguments_lists_per_scriptlet[name].insert(
            &arguments.GetList());
      }
    }
  }

  base::DictValue result;
  for (const auto& [name, selected_arguments_lists] :
       selected_arguments_lists_per_scriptlet) {
    std::string abp_argument;
    for (const auto& selected_arguments_list : selected_arguments_lists) {
      // The ABP snippet arguments were purposefully left with a trailing
      // comma at the parsing stage. We can just concatenate them here.
      abp_argument.append(selected_arguments_list->front().GetString());
    }
    if (!abp_argument.empty()) {
      // Remove extra comma
      abp_argument.pop_back();
    }
    result.Set(name, base::ListValue().Append(abp_argument));
  }

  reply_handler(result.Find(scriptlet_name), nil);

  script_arguments_cache_.Put(host, std::move(result));
}
}  // namespace

ContentInjectionHandler::~ContentInjectionHandler() = default;
/*static*/
std::unique_ptr<ContentInjectionHandler> ContentInjectionHandler::Create(
    web::BrowserState* browser_state,
    Resources* resources) {
  return std::make_unique<ContentInjectionHandlerImpl>(browser_state,
                                                       resources);
}

}  // namespace adblock_filter
