// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/ios/ios_rule_utils.h"
#include "base/containers/fixed_flat_map.h"

namespace adblock_filter {

namespace ios_rule_utils {

namespace {
constexpr auto kResourceTypeMap =
    base::MakeFixedFlatMap<ResourceType, std::string_view>({
        {ResourceType::kDocument, "top-document"},
        {ResourceType::kStylesheet, "style-sheet"},
        {ResourceType::kImage, "image"},
        {ResourceType::kObject, "media"},
        {ResourceType::kScript, "script"},
        {ResourceType::kXmlHttpRequest, "fetch"},
        {ResourceType::kSubDocument, "child-document"},
        {ResourceType::kFont, "font"},
        {ResourceType::kMedia, "media"},
        {ResourceType::kWebSocket, "websocket"},
        {ResourceType::kPing, "ping"},
        {ResourceType::kOther, "other"},
    });
}  // namespace

Trigger::Trigger(std::string url_filter, bool case_sensitive)
    : url_filter_(url_filter), case_sensitive_(case_sensitive) {}
Trigger::~Trigger() = default;

Trigger& Trigger::operator=(Trigger&&) = default;
Trigger::Trigger(Trigger&&) = default;
Trigger::Trigger(const Trigger&) = default;

Trigger Trigger::Clone() const {
  return Trigger(*this);
}

base::DictValue Trigger::ToDict() const {
  base::DictValue result;
  result.Set(kUrlFilter, url_filter_);
  // The default is false
  if (case_sensitive_) {
    result.Set(kUrlFilterIsCaseSensitive, true);
  }
  // If this wasn't set, fall back to default (block all types)
  if (resource_types_) {
    base::ListValue resource_type;
    for (ResourceType type : *resource_types_) {
      resource_type.Append(base::Value(kResourceTypeMap.at(type)));
    }

    // The caller should ensure it isn't making a no-op rule.
    CHECK(!resource_type.empty());
    result.Set(kResourceType, std::move(resource_type));
  }
  if (load_type_) {
    base::ListValue load_type;
    if (*load_type_ == Party::kThird || *load_type_ == Party::kStrictThird) {
      load_type.Append(base::Value(kThirdParty));
    } else {
      load_type.Append(base::Value(kFirstParty));
    }
    result.Set(kLoadType, std::move(load_type));
  }

  if (!top_url_filter_.empty()) {
    base::ListValue top_url_filter;
    for (const auto& url : top_url_filter_) {
      top_url_filter.Append(url);
    }
    result.Set(top_url_filter_is_excluding_ ? kUnlessTopUrl : kIfTopUrl,
               std::move(top_url_filter));
    // The default is false
    if (top_url_filter_is_case_sensitive_) {
      result.Set(kTopUrlFilterIsCaseSensitive, true);
    }
  }

  if (!frame_url_filter_.empty()) {
    base::ListValue frame_url_filter;
    for (const auto& url : frame_url_filter_) {
      frame_url_filter.Append(url);
    }
    result.Set(frame_url_filter_is_excluding_ ? kUnlessFrameUrl : kIfFrameUrl,
               std::move(frame_url_filter));
    // The default is false
    if (frame_url_filter_is_case_sensitive_) {
      result.Set(kFrameUrlFilterIsCaseSensitive, true);
    }
  }

  return result;
}

void Trigger::set_resource_type(ResourceTypes types) {
  resource_types_ = types;
}

void Trigger::set_load_type(std::optional<Party> load_type) {
  load_type_ = load_type;
}

void Trigger::set_top_url_filter(std::string url,
                                 bool is_exclude,
                                 bool case_sensitive) {
  std::vector<std::string> urls;
  urls.push_back(std::move(url));
  set_top_url_filter(urls, is_exclude, case_sensitive);
}
void Trigger::set_top_url_filter(std::vector<std::string> urls,
                                 bool is_exclude,
                                 bool case_sensitive) {
  top_url_filter_ = urls;
  top_url_filter_is_excluding_ = is_exclude;
  top_url_filter_is_case_sensitive_ = case_sensitive;
}

void Trigger::set_frame_url_filter(std::vector<std::string> urls,
                                   bool is_exclude,
                                   bool case_sensitive) {
  frame_url_filter_ = urls;
  frame_url_filter_is_excluding_ = is_exclude;
  frame_url_filter_is_case_sensitive_ = case_sensitive;
}

Action::Action(const char* type) : type_(type) {}
Action::Action(const Action&) = default;

Action::~Action() = default;

Action::Action(Action&&) = default;
Action& Action::operator=(Action&&) = default;
Action& Action::operator=(const Action&) = default;

Action Action::Clone() const {
  return Action(*this);
}

/*static*/
Action Action::BlockAction() {
  return Action(kBlock);
}

/*static*/
Action Action::IgnorePreviousAction() {
  return Action(kIgnorePrevious);
}

/*static*/
Action Action::CssHideAction(std::string selector) {
  Action action(kCssHide);
  action.selector_ = selector;
  return action;
}

base::DictValue Action::ToDict() const {
  base::DictValue result;
  result.Set(kType, type_);
  if (type_ == kCssHide) {
    result.Set(kSelector, selector_);
  } else if (type_ == kRedirect) {
    result.Set(kUrl, redirect_url_);
  } else if (type_ == kCsp) {
    result.Set(kPriority, 0);
    base::DictValue modify_header_info;
    modify_header_info.Set(kOperation, kAppend);
    modify_header_info.Set(kHeader, kCsp);
    modify_header_info.Set(kValue, csp_);
    base::DictValue modify_header_actions;
    modify_header_actions.Set(kResponseHeaders, std::move(modify_header_info));
    result.Set(kModifyHeaders, std::move(modify_header_actions));
  }
  return result;
}

base::DictValue MakeRule(const Trigger& trigger, const Action& action) {
  base::DictValue result;
  result.Set(kTrigger, trigger.ToDict());
  result.Set(kAction, action.ToDict());

  return result;
}

void AppendFromPattern(std::string_view pattern, std::string& result) {
  for (const auto c : pattern) {
    switch (c) {
      case kWildcard.front():
        result.append(kWildcardRegex);
        break;
      case kDelim:
        result.append(kDelimRegex);
        break;
      case '.':
      case '+':
      case '$':
      case '?':
      case '{':
      case '}':
      case '(':
      case ')':
      case '[':
      case ']':
      case '|':
      case '/':
      case '\\':
        result.append("\\");
        [[fallthrough]];
      default:
        result.append(1, c);
    }
  }
}

std::string DomainToIfURL(std::string domain,
                          bool subdomains,
                          bool is_domain_regex) {
  std::string result(kSchemeRegex);
  if (subdomains)
    result.append(kSubdomainRegex);
  if (is_domain_regex) {
    result.append(domain);
  } else {
    ios_rule_utils::AppendFromPattern(domain, result);
  }
  result.append(kEndDomainRegex);

  return result;
}

}  // namespace ios_rule_utils
}  // namespace adblock_filter
