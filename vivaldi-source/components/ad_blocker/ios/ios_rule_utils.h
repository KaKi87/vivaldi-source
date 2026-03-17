// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_AD_BLOCKER_IOS_IOS_RULES_TRIGGER_H_
#define COMPONENTS_AD_BLOCKER_IOS_IOS_RULES_TRIGGER_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "components/ad_blocker/public/core/adblock_request_filter_rule_types.h"

namespace adblock_filter {

namespace ios_rule_utils {
inline constexpr char kDelim = '^';
inline constexpr std::string_view kWildcard = "*";
inline constexpr char kRegexBegin[] = "^";
inline constexpr char kRegexEnd[] = "$";
inline constexpr char kRegexOptional[] = "?";
// The three next regexes are identical to those WebKit uses to tranform
// if-domain to if-top-url
inline constexpr char kSchemeRegex[] = "^[a-z][a-z+.-]*:\\/\\/";
inline constexpr char kSubdomainRegex[] = "([^/]*\\.)*";
inline constexpr char kEndDomainRegex[] = "[:/]";
inline constexpr char kDelimRegex[] = "[^a-zA-Z0-9_.%-]";
inline constexpr char kLastDelimRegex[] = "([^a-zA-Z0-9_.%-].*)?$";
inline constexpr char kWildcardRegex[] = ".*";

inline constexpr char kVersion[] = "version";
inline constexpr char kPartnerListAllowedDocuments[] =
    "partner-list-allowed-documents";
inline constexpr char kNetworkRules[] = "network";
inline constexpr char kCosmeticRules[] = "cosmetic";
inline constexpr char kScriptletRules[] = "scriptlets";
inline constexpr char kDomainConstraints[] = "domain-constraints";
inline constexpr char kBlockRules[] = "block";
inline constexpr char kAllowRules[] = "allow";
inline constexpr char kBlockImportantRules[] = "block-important";
inline constexpr char kGeneric[] = "generic";
inline constexpr char kSpecific[] = "specific";
inline constexpr char kGenericAllowRules[] = "generic-allow";
inline constexpr char kBlockAllowPairs[] = "block-allow-pairs";
// These names are used in a dict otherwise containing domain name labels
// Prefixing them with a dot ensures they won't collide with an actual fragemnt.
inline constexpr char kIncluded[] = ".included";
inline constexpr char kExcluded[] = ".excluded";

inline constexpr char kTrigger[] = "trigger";
inline constexpr char kUrlFilter[] = "url-filter";
inline constexpr char kUrlFilterIsCaseSensitive[] =
    "url-filter-is-case-sensitive";
inline constexpr char kResourceType[] = "resource-type";
inline constexpr char kLoadType[] = "load-type";
inline constexpr char kFirstParty[] = "first-party";
inline constexpr char kThirdParty[] = "third-party";
inline constexpr char kLoadContext[] = "load-context";
inline constexpr char kTopFrame[] = "top-frame";
inline constexpr char kChildFrame[] = "child-frame";
inline constexpr char kIfTopUrl[] = "if-top-url";
inline constexpr char kUnlessTopUrl[] = "unless-top-url";
inline constexpr char kTopUrlFilterIsCaseSensitive[] =
    "top-url-filter-is-case-sensitive";
inline constexpr char kIfFrameUrl[] = "if-frame-url";
inline constexpr char kUnlessFrameUrl[] = "unless-frame-url";
inline constexpr char kFrameUrlFilterIsCaseSensitive[] =
    "frame-url-filter-is-case-sensitive";

inline constexpr char kAction[] = "action";
inline constexpr char kBlock[] = "block";
inline constexpr char kType[] = "type";
inline constexpr char kIgnorePrevious[] = "ignore-previous-rules";
inline constexpr char kCssHide[] = "css-display-none";
inline constexpr char kRedirect[] = "redirect";
inline constexpr char kModifyHeaders[] = "modify-headers";
inline constexpr char kSelector[] = "selector";
inline constexpr char kUrl[] = "url";
inline constexpr char kPriority[] = "priority";
inline constexpr char kResponseHeaders[] = "response-headers";
inline constexpr char kOperation[] = "operation";
inline constexpr char kAppend[] = "append";
inline constexpr char kHeader[] = "header";
inline constexpr char kCsp[] = "Content-Security-Policy";
inline constexpr char kValue[] = "value";

inline constexpr std::string_view kDomainTreeNodeType = ".type";
inline constexpr std::string_view kDomainTreeHostnameNode = ".";
inline constexpr std::string_view kDomainTreeEntityNode = ".*";
inline constexpr std::string_view kDomainTreeIncludedRegexes = "/";
inline constexpr std::string_view kDomainTreeExcludedRegexes = "~/";

inline constexpr char kNonIosRulesAndMetadata[] = "non-ios-rules-and-metadata";
inline constexpr char kMetadata[] = "metadata";
inline constexpr char kListChecksums[] = "list-checksums";
inline constexpr char kExceptionRule[] = "exception-rule";
inline constexpr char kIosContentBlockerRules[] = "ios-content-blocker-rules";

class Trigger {
 public:
  enum class LoadContext { kAny = 0, kTopFrame, kChildFrame };

  Trigger(std::string url_filter, bool case_sensitive);
  ~Trigger();

  Trigger& operator=(Trigger&&);
  Trigger(Trigger&&);

  Trigger Clone() const;

  base::DictValue ToDict() const;

  void set_resource_type(ResourceTypes types);
  void set_load_type(std::optional<Party> load_type);
  void set_top_url_filter(std::string url,
                          bool is_exclude,
                          bool case_sensitive);
  void set_top_url_filter(std::vector<std::string> urls,
                          bool is_exclude,
                          bool case_sensitive);
  void set_frame_url_filter(std::vector<std::string> urls,
                            bool is_exclude,
                            bool case_sensitive);

 private:
  Trigger(const Trigger&);

  std::string url_filter_;
  bool case_sensitive_ = false;
  std::optional<ResourceTypes> resource_types_;
  std::optional<Party> load_type_;
  std::vector<std::string> top_url_filter_;
  bool top_url_filter_is_excluding_ = false;
  bool top_url_filter_is_case_sensitive_ = false;
  std::vector<std::string> frame_url_filter_;
  bool frame_url_filter_is_excluding_ = false;
  bool frame_url_filter_is_case_sensitive_ = false;
};

class Action {
 public:
  ~Action();

  Action(Action&&);
  Action& operator=(Action&&);

  Action Clone() const;

  static Action BlockAction();
  static Action IgnorePreviousAction();
  static Action CssHideAction(std::string selector);

  base::DictValue ToDict() const;

 private:
  Action(const char* type);
  Action(const Action&);
  Action& operator=(const Action&);

  const char* type_;
  std::string selector_;
  std::string redirect_url_;
  std::string csp_;
};

base::DictValue MakeRule(const Trigger& trigger, const Action& action);

void AppendFromPattern(std::string_view pattern, std::string& result);
std::string DomainToIfURL(std::string domain,
                          bool subdomains,
                          bool is_domain_regex);
}  // namespace ios_rule_utils
}  // namespace adblock_filter

#endif  // COMPONENTS_AD_BLOCKER_IOS_IOS_RULES_TRIGGER_H_
