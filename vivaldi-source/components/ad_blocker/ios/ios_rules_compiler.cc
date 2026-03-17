// Copyright (c) 2023 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/ios/ios_rules_compiler.h"

#include <bitset>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/strings/string_util.h"
#include "base/system/sys_info.h"
#include "base/values.h"
#include "components/ad_blocker/core/adblock_content_injection_rule.h"
#include "components/ad_blocker/core/adblock_domain_constraints_tree.h"
#include "components/ad_blocker/core/adblock_request_filter_rule.h"
#include "components/ad_blocker/ios/ios_rule_utils.h"
#include "components/ad_blocker/ios/utils.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace adblock_filter {

namespace {

std::optional<std::string> GetRegexFromRule(const RequestFilterRule& rule) {
  std::string_view pattern(rule.pattern);

  // Unicode not supported by iOS content blocker
  if (!base::IsStringASCII(pattern))
    return std::nullopt;

  if (pattern.empty())
    return ios_rule_utils::kWildcardRegex;

  if (rule.pattern_type == RequestFilterRule::kRegex) {
    bool escaped = false;

    if (pattern.front() == '^') {
      pattern.remove_prefix(1);
    }

    for (const auto c : pattern) {
      switch (c) {
        case '\\':
          escaped = !escaped;
          break;
        case '{':
        case '|':
        case '^':
          if (!escaped)
            return std::nullopt;
          break;
        default:
          // character classes, word boundaries and backreferences unsupported
          if ((base::IsAsciiAlpha(c) || base::IsAsciiDigit(c)) && escaped)
            return std::nullopt;
      }
    }

    return rule.pattern;
  }
  std::string result = "";

  bool start_anchored = rule.anchor_type.test(RequestFilterRule::kAnchorStart);
  bool host_anchored = rule.anchor_type.test(RequestFilterRule::kAnchorHost);
  if (rule.host && !start_anchored && !host_anchored) {
    std::string_view host(*rule.host);
    bool pattern_matches_host = false;
    size_t first_slash = pattern.find_first_of("/^");
    size_t pattern_host_size = first_slash;
    bool has_first_slash = true;
    if (first_slash == std::string_view::npos) {
      pattern_host_size = pattern.size();
      has_first_slash = false;
    }
    if (host.size() < pattern_host_size &&
        pattern.substr(pattern_host_size - host.size(), host.size()) == host &&
        pattern[pattern_host_size - host.size() - 1] == '.') {
      GURL validation_url(std::string("https://") +
                          std::string(pattern.substr(0, pattern_host_size)));
      if (validation_url.is_valid() && validation_url.has_host() &&
          !validation_url.has_query() && !validation_url.has_ref() &&
          !validation_url.has_username() && !validation_url.has_password()) {
        host_anchored = true;
        pattern_matches_host = true;
      }
    }
    if (host.size() >= pattern_host_size) {
      if (has_first_slash &&
          host.substr(host.size() - pattern_host_size, pattern_host_size) ==
              pattern.substr(0, pattern_host_size)) {
        pattern.remove_prefix(pattern_host_size);
        pattern_matches_host = true;
      } else if (!has_first_slash &&
                 host.find(pattern) != std::string_view::npos) {
        pattern_matches_host = true;
      }
    }

    if (!host_anchored) {
      result.append(ios_rule_utils::kSchemeRegex);
      result.append(ios_rule_utils::kSubdomainRegex);
      ios_rule_utils::AppendFromPattern(*rule.host, result);
    }

    if (!has_first_slash && pattern_matches_host) {
      if (host_anchored) {
        result.append(ios_rule_utils::kSchemeRegex);
        result.append(ios_rule_utils::kSubdomainRegex);
        ios_rule_utils::AppendFromPattern(pattern, result);
      }
      result.append(ios_rule_utils::kDelimRegex);
      return result;
    }

    if (!pattern_matches_host) {
      result.append(ios_rule_utils::kDelimRegex);
      result.append(ios_rule_utils::kWildcardRegex);
    }
  }

  if (start_anchored) {
    result.append(ios_rule_utils::kRegexBegin);
  } else if (host_anchored) {
    result.append(ios_rule_utils::kSchemeRegex);
    result.append(ios_rule_utils::kSubdomainRegex);
  }

  bool ends_with_delim = pattern.back() == ios_rule_utils::kDelim;
  if (ends_with_delim) {
    pattern.remove_suffix(1);
  }

  ios_rule_utils::AppendFromPattern(pattern, result);

  if (rule.anchor_type.test(RequestFilterRule::kAnchorEnd)) {
    if (ends_with_delim) {
      result.append(ios_rule_utils::kDelimRegex);
      result.append(ios_rule_utils::kRegexOptional);
    }
    result.append(ios_rule_utils::kRegexEnd);
  } else if (ends_with_delim) {
    result.append(ios_rule_utils::kLastDelimRegex);
  }

  return result;
}

base::ListValue* GetTarget(base::DictValue& compiled_rules,
                           RuleDecision decision,
                           bool is_generic) {
  switch (decision) {
    case RuleDecision::kModify:
      return compiled_rules.EnsureDict(ios_rule_utils::kBlockRules)
          ->EnsureList(is_generic ? ios_rule_utils::kGeneric
                                  : ios_rule_utils::kSpecific);
    case RuleDecision::kPass:
      return compiled_rules.EnsureList(ios_rule_utils::kAllowRules);
    case RuleDecision::kModifyImportant:
      return compiled_rules.EnsureList(ios_rule_utils::kBlockImportantRules);
  }
}

void ExtractConstraints(bool had_inclusions,
                        const DomainConstraintsTree::Node& node,
                        std::string current_suffixes,
                        std::vector<std::string>& inclusions,
                        std::vector<std::string>* exclusions) {
  // Exclusions may be omitted for situations where they can't be handled by
  // WebKit
  switch (node.GetNodeType()) {
    case DomainConstraintsTree::Node::kIncluded:
      inclusions.push_back(
          ios_rule_utils::DomainToIfURL(current_suffixes, true, false));
      if (!exclusions) {
        // Inclusion nodes cannot have inclusion descendents
        return;
      }
      had_inclusions = true;
      break;
    case DomainConstraintsTree::Node::kExcluded:
      if (exclusions && had_inclusions) {
        exclusions->push_back(
            ios_rule_utils::DomainToIfURL(current_suffixes, true, false));
      }
      CHECK(node.subdomains().empty());
      return;
    case DomainConstraintsTree::Node::kNone:
      break;
  }

  if (!current_suffixes.empty()) {
    current_suffixes.insert(0, ".");
  }

  for (const auto& [label, subdomain_node] : node.subdomains()) {
    ExtractConstraints(had_inclusions, subdomain_node, label + current_suffixes,
                       inclusions, exclusions);
  }
}

void CompilePlainRequestFilter(const RequestFilterRule& rule,
                               base::DictValue& compiled_rules,
                               ios_rule_utils::Trigger trigger) {
  if (rule.modifier != ModifierType::kNoModifier ||
      rule.request_methods != RequestMethods::All()) {
    // TODO(julien): Implement
    return;
  }

  // iOS cannot handle exception on specific request allow rules.
  bool handle_exceptions = rule.decision == RuleDecision::kModify ||
                           rule.from_domain_constraints.IsGeneric();
  std::vector<std::string> included;
  std::vector<std::string> excluded;
  ExtractConstraints(rule.from_domain_constraints.IsGeneric(),
                     rule.from_domain_constraints.hostnames(), "", included,
                     handle_exceptions ? &excluded : nullptr);
  ExtractConstraints(rule.from_domain_constraints.IsGeneric(),
                     rule.from_domain_constraints.entities(),
                     std::string(ios_rule_utils::kWildcard), included,
                     handle_exceptions ? &excluded : nullptr);
  for (const auto& regexp_constraint :
       rule.from_domain_constraints.included_regexes()) {
    included.push_back(
        ios_rule_utils::DomainToIfURL(regexp_constraint, false, true));
  }
  if (handle_exceptions) {
    for (const auto& regexp_constraint :
         rule.from_domain_constraints.excluded_regexes()) {
      excluded.push_back(
          ios_rule_utils::DomainToIfURL(regexp_constraint, false, true));
    }
  }

  bool needs_block_allow_pair = !included.empty() && !excluded.empty();

  if (!included.empty()) {
    trigger.set_top_url_filter(std::move(included), false, true);
  } else {
    if (!rule.from_domain_constraints.IsGeneric()) {
      // No-op rule
      return;
    }
    trigger.set_top_url_filter(std::move(excluded), true, true);
  }

  if (!needs_block_allow_pair) {
    base::ListValue* target =
        GetTarget(compiled_rules, rule.decision,
                  rule.from_domain_constraints.IsGeneric());
    CHECK(target);

    ios_rule_utils::Action action =
        (rule.decision == RuleDecision::kPass)
            ? ios_rule_utils::Action::IgnorePreviousAction()
            : ios_rule_utils::Action::BlockAction();

    target->Append(ios_rule_utils::MakeRule(trigger, action));

    return;
  }

  base::ListValue* target =
      compiled_rules.EnsureList(ios_rule_utils::kBlockAllowPairs);

  CHECK_NE(rule.decision, RuleDecision::kPass);
  ios_rule_utils::Trigger trigger2 = trigger.Clone();
  trigger2.set_top_url_filter(std::move(excluded), false, true);
  base::ListValue pair;
  pair.Append(
      ios_rule_utils::MakeRule(trigger, ios_rule_utils::Action::BlockAction()));
  pair.Append(ios_rule_utils::MakeRule(
      trigger2, ios_rule_utils::Action::IgnorePreviousAction()));
  target->Append(std::move(pair));
}

void CompileRequestFilterRule(bool allow_strict_blocking,
                              const RequestFilterRule& rule,
                              const RuleSourceSettings& source_settings,
                              base::DictValue& compiled_request_filter_rules,
                              base::DictValue& compiled_cosmetic_filter_rules,
                              base::ListValue& partner_list_allowed_documents) {
  if (!rule.ad_domains_and_query_triggers.empty()) {
    // No possibility to support this on iOS.
    return;
  }

  if (rule.bad_filter) {
    // It might be possible to implement this, but it might be expensive and
    // won't work for all rules.
    return;
  }

  std::optional<std::string> url_filter = GetRegexFromRule(rule);
  if (!url_filter)
    return;
  RegularResourceTypes resource_types = rule.resource_types;
  ExplicitResourceTypes explicit_types = rule.explicit_types;
  ActivationTypes activations = rule.activation_types;

  if (!resource_types.empty() || (explicit_types.Has(ResourceType::kDocument) &&
                                  rule.decision != RuleDecision::kPass)) {
    ios_rule_utils::Trigger trigger(*url_filter, rule.is_case_sensitive);
    trigger.set_load_type(rule.party);

    ResourceTypes ios_resource_types;
    for (auto resource_type : resource_types) {
      // Unsupported on iOS
      if (resource_type == ResourceType::kWebTransport ||
          resource_type == ResourceType::kWebBundle ||
          resource_type == ResourceType::kWebRTC) {
        continue;
      }

      ios_resource_types.Put(resource_type);
    }

    if (explicit_types.Has(ResourceType::kDocument) &&
        rule.decision != RuleDecision::kPass && allow_strict_blocking) {
      ios_resource_types.Put(ResourceType::kDocument);
    }

    // If no resource type remains after removing the unsupported ones, we don't
    // have a rule
    if (!ios_resource_types.empty()) {
      trigger.set_resource_type(ios_resource_types);
      CompilePlainRequestFilter(rule, compiled_request_filter_rules,
                                std::move(trigger));
    }
  }

  if (rule.decision == RuleDecision::kPass && !activations.empty()) {
    ios_rule_utils::Trigger trigger(ios_rule_utils::kWildcardRegex, false);
    trigger.set_load_type(rule.party);
    trigger.set_top_url_filter(*url_filter, false, rule.is_case_sensitive);
    if (activations.Has(ActivationType::kWholeDocument)) {
      if (source_settings.allow_attribution_tracker_rules && rule.host) {
        partner_list_allowed_documents.Append(*rule.host);
      }
      compiled_request_filter_rules.EnsureList(ios_rule_utils::kAllowRules)
          ->Append(ios_rule_utils::MakeRule(
              trigger, ios_rule_utils::Action::IgnorePreviousAction()));
    }

    if (activations.Has(ActivationType::kGenericBlock)) {
      compiled_request_filter_rules
          .EnsureList(ios_rule_utils::kGenericAllowRules)
          ->Append(ios_rule_utils::MakeRule(
              trigger, ios_rule_utils::Action::IgnorePreviousAction()));
    }

    if (activations.Has(ActivationType::kSpecificHide) ||
        activations.Has(ActivationType::kWholeDocument)) {
      compiled_cosmetic_filter_rules.EnsureDict(ios_rule_utils::kAllowRules)
          ->EnsureList(ios_rule_utils::kSpecific)
          ->Append(ios_rule_utils::MakeRule(
              trigger, ios_rule_utils::Action::IgnorePreviousAction()));
    }

    if (activations.Has(ActivationType::kGenericHide) ||
        activations.Has(ActivationType::kWholeDocument)) {
      compiled_cosmetic_filter_rules.EnsureDict(ios_rule_utils::kAllowRules)
          ->EnsureList(ios_rule_utils::kGeneric)
          ->Append(ios_rule_utils::MakeRule(
              trigger, ios_rule_utils::Action::IgnorePreviousAction()));
    }
  }
}

base::DictValue CompileDomainConstraintsNode(
    const DomainConstraintsTree::Node& node) {
  base::DictValue compiled;
  // Don't write the node type if it's none (the most common).
  if (node.GetNodeType() != DomainConstraintsTree::Node::kNone) {
    compiled.Set(ios_rule_utils::kDomainTreeNodeType, node.GetNodeType());
  }

  for (const auto& [label, subdomain] : node.subdomains()) {
    compiled.Set(label, CompileDomainConstraintsNode(subdomain));
  }

  return compiled;
}

base::ListValue CompileDomainConstraintsRegexes(
    const absl::flat_hash_set<std::string>& regexes) {
  base::ListValue compiled;

  for (const auto& regex : regexes) {
    compiled.Append(regex);
  }

  return compiled;
}

base::DictValue CompileDomainConstraintsTree(
    const DomainConstraintsTree& tree) {
  base::DictValue compiled;
  // Don't write the node type if it's none (the most common).
  if (tree.GetRootNodeType() != DomainConstraintsTree::Node::kNone) {
    compiled.Set(ios_rule_utils::kDomainTreeNodeType, tree.GetRootNodeType());
  }

  if (tree.GetRootNodeType() == DomainConstraintsTree::Node::kExcluded) {
    // Serializing a generic exclude tree means all rules it contains are
    // overruled by the top-level rule. Serialize an empty tree instead.
    return compiled;
  }

  if (!tree.hostnames().subdomains().empty()) {
    compiled.Set(ios_rule_utils::kDomainTreeHostnameNode,
                 CompileDomainConstraintsNode(tree.hostnames()));
  }
  if (!tree.entities().subdomains().empty()) {
    compiled.Set(ios_rule_utils::kDomainTreeEntityNode,
                 CompileDomainConstraintsNode(tree.entities()));
  }

  if (!tree.included_regexes().empty()) {
    compiled.Set(ios_rule_utils::kDomainTreeIncludedRegexes,
                 CompileDomainConstraintsRegexes(tree.included_regexes()));
  }
  if (!tree.excluded_regexes().empty()) {
    compiled.Set(ios_rule_utils::kDomainTreeExcludedRegexes,
                 CompileDomainConstraintsRegexes(tree.excluded_regexes()));
  }

  return compiled;
}

base::DictValue CompileScriptletInjectionRule(
    const ScriptletInjectionRule& rule) {
  base::DictValue compiled;
  CHECK(rule.core.domain_constraints.GetRootNodeType() !=
        DomainConstraintsTree::Node::kIncluded);
  base::DictValue scriptlets;
  for (const auto& scriptlet : rule.scriptlets) {
    base::ListValue arguments;
    for (const auto& argument : scriptlet.arguments) {
      arguments.Append(argument);
    }
    scriptlets.Set(scriptlet.name, std::move(arguments));
  }

  compiled.Set(ios_rule_utils::kScriptletRules, std::move(scriptlets));
  compiled.Set(ios_rule_utils::kDomainConstraints,
               CompileDomainConstraintsTree(rule.core.domain_constraints));

  return compiled;
}
}  // namespace

std::string CompileIosRulesToString(bool allow_strict_blocking,
                                    const ParseResult& parse_result,
                                    const RuleSourceSettings& source_settings,
                                    bool pretty_print) {
  base::DictValue compiled_request_filter_rules;
  base::DictValue compiled_cosmetic_filter_rules;
  base::ListValue compiled_scriptlet_injection_rules;
  base::ListValue partner_list_allowed_documents;
  for (const auto& request_filter_rule : parse_result.request_filter_rules) {
    CompileRequestFilterRule(allow_strict_blocking, request_filter_rule,
                             source_settings, compiled_request_filter_rules,
                             compiled_cosmetic_filter_rules,
                             partner_list_allowed_documents);
  }
  /*  for (const auto& cosmetic_rule : parse_result.cosmetic_rules) {
      compiled_cosmetic_filter_rules.EnsureDict(ios_rule_utils::kSelector)
          ->EnsureList(cosmetic_rule.selector)
          ->Append(CompileDomainConstraintsTree(
              cosmetic_rule.core.domain_constraints));
    }*/
  for (const auto& scriptlet_injection_rule :
       parse_result.scriptlet_injection_rules) {
    compiled_scriptlet_injection_rules.Append(
        CompileScriptletInjectionRule(scriptlet_injection_rule));
  }
  base::DictValue result;
  result.Set(ios_rule_utils::kVersion,
             GetIntermediateRepresentationVersionNumber());
  result.Set(ios_rule_utils::kNetworkRules,
             std::move(compiled_request_filter_rules));
  result.Set(ios_rule_utils::kCosmeticRules,
             std::move(compiled_cosmetic_filter_rules));
  result.Set(ios_rule_utils::kScriptletRules,
             std::move(compiled_scriptlet_injection_rules));
  if (!partner_list_allowed_documents.empty()) {
    result.Set(ios_rule_utils::kPartnerListAllowedDocuments,
               std::move(partner_list_allowed_documents));
  }
  std::string output;
  JSONStringValueSerializer serializer(&output);
  serializer.set_pretty_print(pretty_print);
  serializer.Serialize(base::Value(std::move(result)));
  return output;
}

bool CompileIosRules(bool allow_strict_blocking,
                     const ParseResult& parse_result,
                     const RuleSourceSettings& source_settings,
                     const base::FilePath& output_path,
                     std::string& checksum) {
  if (!base::CreateDirectory(output_path.DirName()))
    return false;
  std::string ios_rules = CompileIosRulesToString(
      allow_strict_blocking, parse_result, source_settings, false);
  checksum = CalculateBufferChecksum(ios_rules);
  return base::WriteFile(output_path, ios_rules);
}

base::Value CompileExceptionsRule(const std::set<std::string>& exceptions,
                                  bool process_list) {
  std::vector<std::string> if_urls;
  std::transform(exceptions.cbegin(), exceptions.cend(),
                 std::back_inserter(if_urls), [](std::string domain) {
                   return ios_rule_utils::DomainToIfURL(domain, true, false);
                 });

  ios_rule_utils::Trigger trigger(ios_rule_utils::kWildcardRegex, false);
  trigger.set_top_url_filter(if_urls, process_list, true);

  return base::Value(ios_rule_utils::MakeRule(
      trigger, ios_rule_utils::Action::IgnorePreviousAction()));
}
}  // namespace adblock_filter
