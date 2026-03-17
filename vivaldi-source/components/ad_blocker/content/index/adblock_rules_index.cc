// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/content/index/adblock_rules_index.h"

#include <utility>
#include <vector>

#include "adblock_rules_index.h"
#include "base/strings/string_split.h"
#include "components/ad_blocker/content/index/adblock_rule_pattern_matcher.h"
#include "components/ad_blocker/content/index/index_utils.h"
#include "components/ad_blocker/content/index/stylesheet_builder.h"
#include "components/ad_blocker/content/utils.h"
#include "components/ad_blocker/core/parse_utils.h"
#include "components/url_pattern_index/closed_hash_map.h"
#include "components/url_pattern_index/ngram_extractor.h"
#include "components/url_pattern_index/uint64_hasher.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/http/http_request_headers.h"
#include "third_party/re2/src/re2/re2.h"
#include "third_party/re2/src/re2/stringpiece.h"
#include "url/origin.h"
#include "vivaldi/components/ad_blocker/content/index/flat/adblock_rules_index_generated.h"
#include "vivaldi/components/ad_blocker/content/index/flat/adblock_rules_list_generated.h"

namespace adblock_filter {
namespace {
constexpr size_t kActivationOriginCacheSize = 500;
constexpr size_t kActivationUrlCacheSize = 100;

constexpr size_t kMatchedRulesOriginCacheSize = 500;
constexpr size_t kMatchedRulesUrlCacheSize = 1000;

// The integer type used to represent N-grams.
using NGram = uint64_t;
// The hasher used for hashing N-grams.
using NGramHasher = url_pattern_index::Uint64ToUint32Hasher;
// The hash table probe sequence used both by UrlPatternIndex and its builder.
using NGramHashTableProber =
    url_pattern_index::DefaultProber<NGram, NGramHasher>;

using FlatNGramIndex =
    flatbuffers::Vector<flatbuffers::Offset<flat::NGramToRules>>;

using FlatRulesByModifierList =
    flatbuffers::Vector<flatbuffers::Offset<flat::PrioritizedRuleList>>;
using FlatRuleIdList = flatbuffers::Vector<flatbuffers::Offset<flat::RuleId>>;

using FlatStringList = flatbuffers::Vector<FlatStringOffset>;

template <class T>
struct ContentInjectionIndexTraversalResult {
  std::set<const T*, ContentInjectionRuleBodyCompare> selected;
  std::set<const T*, ContentInjectionRuleBodyCompare> exceptions;
};

struct RuleAndSource {
  raw_ptr<const flat::RequestFilterRule> rule;
  raw_ptr<const RuleBufferHolder> source_buffer;
};

using ActivationsFound =
    absl::flat_hash_map<flat::ActivationType, RuleAndSource>;

struct FoundModifiersInternal {
  std::map<std::string, RuleAndSource> value_with_decision;
  std::optional<RuleAndSource> pass_all_rule;
  bool found_modify_rules = false;
};

using FoundModifiersByTypeInternal =
    absl::flat_hash_map<flat::Modifier, FoundModifiersInternal>;

RequestFilterRuleStub ToRuleStub(const RuleAndSource& rule_and_source) {
  auto convert_decision = [](flat::Decision decision) {
    switch (decision) {
      case flat::Decision_MODIFY:
        return RuleDecision::kModify;
      case flat::Decision_PASS:
        return RuleDecision::kPass;
      case flat::Decision_MODIFY_IMPORTANT:
        return RuleDecision::kModifyImportant;
      default:
        NOTREACHED();
    }
  };

  RequestFilterRuleStub rule_stub;
  rule_stub.rule_source_id = rule_and_source.source_buffer->source_id();
  const flat::RequestFilterRule* rule = rule_and_source.rule;
  rule_stub.priority = GetRulePriority(*rule);
  rule_stub.decision = convert_decision(rule->decision());
  rule_stub.modify_block = rule->options() & flat::OptionFlag_MODIFY_BLOCK;
  rule_stub.is_attribution_allow_rule =
      rule->ad_domains_and_query_triggers() != nullptr;
  rule_stub.original_rule_text = rule->original_rule_text()
                                     ? rule->original_rule_text()->string_view()
                                     : std::string_view();
  return rule_stub;
}

ActivationResults ToActivationResults(ActivationsFound& activations) {
  auto convert_activation_type = [](flat::ActivationType activation_type) {
    switch (activation_type) {
      case flat::ActivationType_DOCUMENT:
        return ActivationType::kWholeDocument;
      case flat::ActivationType_SPECIFIC_HIDE:
        return ActivationType::kSpecificHide;
      case flat::ActivationType_GENERIC_BLOCK:
        return ActivationType::kGenericBlock;
      case flat::ActivationType_GENERIC_HIDE:
        return ActivationType::kGenericHide;
      case flat::ActivationType_ATTRIBUTE_ADS:
        return ActivationType::kAttributeAds;
      default:
        NOTREACHED();
    }
  };

  ActivationResults activation_results;
  for (const auto& [activation_type, activation] : activations) {
    activation_results.by_type[convert_activation_type(activation_type)]
        .rule_stub = ToRuleStub(activation);
  }

  return activation_results;
}

RulesIndex::FoundModifiers ToPublicFoundModifiers(
    FoundModifiersInternal& found_modifiers) {
  RulesIndex::FoundModifiers result;

  if (found_modifiers.pass_all_rule) {
    result.pass_all_rule = ToRuleStub(*found_modifiers.pass_all_rule);
  }
  result.found_modify_rules = found_modifiers.found_modify_rules;
  for (auto& [value, rule] : found_modifiers.value_with_decision) {
    result.value_with_decision.insert({std::move(value), ToRuleStub(rule)});
  }

  return result;
}

RulesIndex::FoundModifiersByType ToPublicFoundModifiersByType(
    FoundModifiersByTypeInternal& found_modifiers_by_type,
    RulesIndex::FoundModifiersByType& result) {
  auto convert_modifier_type = [](flat::Modifier modifier_type) {
    switch (modifier_type) {
      case flat::Modifier_REDIRECT:
        return ModifierType::kRedirect;
      case flat::Modifier_CSP:
        return ModifierType::kCsp;
      case flat::Modifier_AD_QUERY_TRIGGER:
        return ModifierType::kAdQueryTrigger;
      default:
        NOTREACHED();
    }
  };

  for (auto& [modifier_type, found_modifiers] : found_modifiers_by_type) {
    result[convert_modifier_type(modifier_type)] =
        ToPublicFoundModifiers(found_modifiers);
  }

  return result;
}

void BuildAbpInjectionData(std::string snippets_arguments,
                           std::string scriptlet_name,
                           RulesIndex::InjectionData& injection_data) {
  if (snippets_arguments.empty()) {
    return;
  }
  DCHECK(snippets_arguments.back() == ',');
  // Remove extra comma
  snippets_arguments.pop_back();
  RulesIndex::ScriptletInjection scriptlet_injection;
  scriptlet_injection.first = std::move(scriptlet_name);
  scriptlet_injection.second.push_back(std::move(snippets_arguments));
  injection_data.scriptlet_injections.push_back(std::move(scriptlet_injection));
}

struct ContentInjectionIndexTraversalResults {
  ContentInjectionIndexTraversalResult<flat::CosmeticRule> cosmetic_rules;
  ContentInjectionIndexTraversalResult<flat::ScriptletInjectionRule>
      scriptlet_injection_rules;

  RulesIndex::InjectionData ToInjectionData() {
    RulesIndex::InjectionData injection_data;
    injection_data.stylesheet = BuildStyleSheet(cosmetic_rules.selected);

    std::string abp_snippets_main_arguments;
    std::string abp_snippets_isolated_arguments;
    for (const flat::ScriptletInjectionRule* rule :
         scriptlet_injection_rules.selected) {
      for (const flat::Scriptlet* scriptlet : *rule->scriptlets()) {
        if (scriptlet->name()->string_view() == kAbpSnippetsMainScriptletName) {
          DCHECK(scriptlet->arguments()->size() == 1);
          // The ABP snippet arguments were purposefully left with a trailing
          // comma at the parsing stage. We can just concatenate them here.
          abp_snippets_main_arguments += scriptlet->arguments()->Get(0)->str();
        } else if (scriptlet->name()->string_view() ==
                   kAbpSnippetsIsolatedScriptletName) {
          DCHECK(scriptlet->arguments()->size() == 1);
          abp_snippets_isolated_arguments +=
              scriptlet->arguments()->Get(0)->str();
        } else {
          RulesIndex::ScriptletInjection scriptlet_injection;
          scriptlet_injection.first = scriptlet->name()->str();
          for (auto* argument : *(scriptlet->arguments()))
            scriptlet_injection.second.push_back(argument->str());
          injection_data.scriptlet_injections.push_back(
              std::move(scriptlet_injection));
        }
      }
    }

    BuildAbpInjectionData(std::move(abp_snippets_main_arguments),
                          kAbpSnippetsMainScriptletName, injection_data);
    BuildAbpInjectionData(std::move(abp_snippets_isolated_arguments),
                          kAbpSnippetsIsolatedScriptletName, injection_data);

    return injection_data;
  }
};

std::optional<uint32_t> GetSubdomainNodeIndex(
    std::string_view domain_piece,
    size_t tree_size,
    std::optional<size_t> first_child_node_index,
    const FlatStringList* subdomains) {
  if (subdomains == nullptr || subdomains->size() == 0)
    return std::nullopt;

  CHECK(first_child_node_index);

  // If the `subdomains` list is short, then the simple strategy is usually
  // faster.
  constexpr size_t kSmallSubdomainListSize = 5;
  if (subdomains->size() <= kSmallSubdomainListSize) {
    for (auto subdomain = subdomains->begin(); subdomain != subdomains->end();
         subdomain++) {
      if (subdomain->string_view() == domain_piece) {
        CHECK((subdomain - subdomains->begin()) + *first_child_node_index <
              tree_size);

        return (subdomain - subdomains->begin()) + *first_child_node_index;
      }
    }
    return std::nullopt;
  }

  auto compare = [](const flatbuffers::String* lhs,
                    const std::string_view& rhs) {
    std::string lhs_str = lhs->str();
    return std::lexicographical_compare(lhs_str.begin(), lhs_str.end(),
                                        rhs.begin(), rhs.end());
  };

  const auto& subdomain = std::lower_bound(
      subdomains->begin(), subdomains->end(), domain_piece, compare);
  if (subdomain == subdomains->end())
    return std::nullopt;

  std::string subdomain_str = subdomain->str();
  if (!std::equal(subdomain_str.begin(), subdomain_str.end(),
                  domain_piece.begin(), domain_piece.end()))
    return std::nullopt;

  CHECK((subdomain - subdomains->begin()) + *first_child_node_index <
        tree_size);

  return (subdomain - subdomains->begin()) + *first_child_node_index;
}

bool DoesRulePartyMatch(const flat::RequestFilterRule& rule,
                        const RulesIndex::BaseQuery& query) {
  switch (rule.party()) {
    case flat::Party_ALL:
      return true;
    case flat::Party_FIRST:
      return !query.IsThirdParty();
    case flat::Party_THIRD:
      return query.IsThirdParty();
    case flat::Party_STRICT_FIRST:
      return !query.IsStrictThirdParty();
    case flat::Party_STRICT_THIRD:
      return query.IsStrictThirdParty();
    case flat::Party_FIRST_AND_STRICT_THIRD:
      return !query.IsThirdParty() && query.IsStrictThirdParty();
  }
}

// Returns whether the request resource matches resource flags of the specified
// filtering `rule`.
bool DoesRuleResourceMatch(const flat::RequestFilterRule& rule,
                           flat::ResourceType resource_type) {
  DCHECK(resource_type != flat::ResourceType_NONE);

  return resource_type == flat::ResourceType_ANY ||
         (rule.resource_types() & resource_type);
}

bool DoesRuleMethodMatch(const flat::RequestFilterRule& rule,
                         std::string_view method) {
  if (method == "") {
    return true;
  } else if (method == net::HttpRequestHeaders::kConnectMethod) {
    return rule.methods() & flat::Method_CONNECT;
  } else if (method == net::HttpRequestHeaders::kDeleteMethod) {
    return rule.methods() & flat::Method_DELETE;
  } else if (method == net::HttpRequestHeaders::kGetMethod) {
    return rule.methods() & flat::Method_GET;
  } else if (method == net::HttpRequestHeaders::kHeadMethod) {
    return rule.methods() & flat::Method_HEAD;
  } else if (method == net::HttpRequestHeaders::kOptionsMethod) {
    return rule.methods() & flat::Method_OPTIONS;
  } else if (method == net::HttpRequestHeaders::kPatchMethod) {
    return rule.methods() & flat::Method_PATCH;
  } else if (method == net::HttpRequestHeaders::kPostMethod) {
    return rule.methods() & flat::Method_POST;
  } else if (method == net::HttpRequestHeaders::kPutMethod) {
    return rule.methods() & flat::Method_PUT;
  } else
    return rule.methods() & flat::Method_OTHER;
}

bool DoesUrlMatchRulePattern(const RuleAndSource& rule_and_source,
                             const RulePatternMatcher::UrlInfo& url,
                             bool must_intersect_host) {
  const flat::RequestFilterRule& rule = *rule_and_source.rule;
  if (rule.host() && rule.host()->size()) {
    std::string_view host = url.spec().substr(url.host().begin, url.host().len);
    if (!base::EndsWith(host, rule.host()->str()))
      return false;
    host.remove_suffix(rule.host()->size());
    if (!host.empty() && host.back() != '.')
      return false;
    // This satisfies the condition. We don't need to test it again in pattern
    // matching.
    must_intersect_host = false;
  }

  if (rule.pattern_type() == flat::PatternType_REGEXP) {
    if (must_intersect_host) {
      // Finding out whether a regex matches on the host is tricky, and there
      // are few enough regex rules that we might be able to get away without
      // handling this case.
      return false;
    }
    std::string_view text =
        ((rule.options() & flat::OptionFlag_IS_CASE_SENSITIVE) != 0)
            ? url.spec()
            : url.fold_case_spec();

    return RE2::PartialMatch(
        re2::StringPiece(text.data(), text.size()),
        rule_and_source.source_buffer->GetRegexForPattern(rule.pattern()));
  } else {
    return RulePatternMatcher(rule).MatchesUrl(url, must_intersect_host);
  }
}

// Returns whether the `origin` matches the domain constraints of the `rule`. To
// match a generic constraint, the origin must not match any excluded domain. To
// match a specific constraint (which has some included domains), the origin
// must match one of the included domains.
//
// If `disable_generic_rules` is set, this function will always return false for
// generic rules if they are regular modify rules. Allow rules and important
// rules are always evaluated.
bool DoesOriginMatchFromDomainConstraints(const url::Origin& origin,
                                          const RuleAndSource& rule_and_source,
                                          bool disable_generic_rules) {
  const bool is_generic =
      IsGeneric(*rule_and_source.rule->from_domain_constraints());
  if (disable_generic_rules && is_generic &&
      rule_and_source.rule->decision() == flat::Decision_MODIFY)
    return false;

  // Unique `origin` matches lists of exception domains only.
  if (origin.opaque())
    return is_generic;

  const flat::DomainConstraintsTree* domain_constraints_tree =
      rule_and_source.rule->from_domain_constraints();

  const size_t registry_length =
      net::registry_controlled_domains::GetCanonicalHostRegistryLength(
          origin.host(),
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  const flat::DomainConstraintsNode* hostname_node =
      domain_constraints_tree->nodes()->Get(
          domain_constraints_tree->hostnames_node_index());
  const flat::DomainConstraintsNode* entity_node = nullptr;

  size_t seen_length = 0;
  enum { kIncluded, kExcluded, kNoMatch } match = kNoMatch;

  auto labels = base::SplitStringPiece(
      origin.host(), ".", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SplitResult::SPLIT_WANT_ALL);

  for (std::string_view& label : base::Reversed(labels)) {
    if (hostname_node) {
      std::optional<uint32_t> subdomain_node_index = GetSubdomainNodeIndex(
          label, domain_constraints_tree->nodes()->size(),
          hostname_node->first_child_node_index(), hostname_node->subdomains());
      hostname_node =
          subdomain_node_index
              ? domain_constraints_tree->nodes()->Get(*subdomain_node_index)
              : nullptr;
    }
    if (entity_node) {
      std::optional<uint32_t> subdomain_node_index = GetSubdomainNodeIndex(
          label, domain_constraints_tree->nodes()->size(),
          entity_node->first_child_node_index(), entity_node->subdomains());
      entity_node =
          subdomain_node_index
              ? domain_constraints_tree->nodes()->Get(*subdomain_node_index)
              : nullptr;
    }

    if (!hostname_node && !entity_node && seen_length >= registry_length + 1) {
      // No match
      break;
    }

    if (hostname_node) {
      if (hostname_node->type() == flat::DomainConstraintNodeType_INCLUDED) {
        match = kIncluded;
      } else if (hostname_node->type() ==
                 flat::DomainConstraintNodeType_EXCLUDED) {
        match = kExcluded;
        break;
      }
    }

    if (entity_node) {
      if (entity_node->type() == flat::DomainConstraintNodeType_INCLUDED) {
        match = kIncluded;
      } else if (entity_node->type() ==
                 flat::DomainConstraintNodeType_EXCLUDED) {
        match = kExcluded;
        break;
      }
    }

    seen_length += label.length() + 1;
    if (seen_length == registry_length + 1) {
      entity_node = domain_constraints_tree->nodes()->Get(
          domain_constraints_tree->entities_node_index());
    }
  }

  if (match == kExcluded) {
    return false;
  }

  if (domain_constraints_tree->excluded_regexes()) {
    for (auto* regex : *domain_constraints_tree->excluded_regexes()) {
      if (RE2::PartialMatch(
              re2::StringPiece(origin.host().data(), origin.host().size()),
              rule_and_source.source_buffer->GetRegexForPattern(regex))) {
        return false;
      }
    }
  }

  if (match == kIncluded || (is_generic && match == kNoMatch)) {
    return true;
  }

  if (domain_constraints_tree->included_regexes()) {
    for (auto* regex : *domain_constraints_tree->included_regexes()) {
      if (RE2::PartialMatch(
              re2::StringPiece(origin.host().data(), origin.host().size()),
              rule_and_source.source_buffer->GetRegexForPattern(regex))) {
        return true;
      }
    }
  }

  return false;
}

bool DoesMatchAdAttributionParams(
    const RulesIndex::AdAttributionMatchParams& params,
    std::string_view tracker_url_spec,
    std::string_view ad_domain_and_query_trigger) {
  size_t separator = ad_domain_and_query_trigger.find_first_of('|');
  CHECK(separator != std::string_view::npos);

  if (ad_domain_and_query_trigger.substr(separator + 1) != params.ad_trigger_) {
    return false;
  }

  std::string_view match_domain =
      ad_domain_and_query_trigger.substr(0, separator);

  if (match_domain.back() == '.') {
    match_domain.remove_suffix(1);
  }

  std::string_view ad_click_domain(params.ad_click_domain_);
  if (ad_click_domain.back() == '.') {
    ad_click_domain.remove_suffix(1);
  }

  if (!ad_click_domain.ends_with(match_domain)) {
    return false;
  }

  ad_click_domain.remove_suffix(match_domain.size());
  return ad_click_domain.empty() || ad_click_domain.back() == '.';
}

RuleAndSource GetRequestFilterRuleAndSourceFromId(
    const RulesIndex::RulesBufferMap& rule_buffers,
    const flat::RuleId& rule_id) {
  const RuleBufferHolder& rule_buffer = rule_buffers.at(rule_id.source_id());
  return RuleAndSource{
      rule_buffer.rules_list()->request_filter_rules_list()->Get(
          rule_id.rule_nr()),
      &rule_buffer};
}

const flat::CosmeticRule* GetCosmeticRuleFromId(
    const RulesIndex::RulesBufferMap& rule_buffers,
    const flat::RuleId& rule_id) {
  const RuleBufferHolder& rule_buffer = rule_buffers.at(rule_id.source_id());
  return rule_buffer.rules_list()->cosmetic_rules_list()->Get(
      rule_id.rule_nr());
}

const flat::ScriptletInjectionRule* GetScriptletInjectionRuleFromId(
    const RulesIndex::RulesBufferMap& rule_buffers,
    const flat::RuleId& rule_id) {
  const RuleBufferHolder& rule_buffer = rule_buffers.at(rule_id.source_id());
  return rule_buffer.rules_list()->scriptlet_injection_rules_list()->Get(
      rule_id.rule_nr());
}

bool AddActivationsFromRule(ActivationsFound& activations,
                            RuleAndSource rule_and_source) {
  bool result = false;
  for (uint8_t i = 1; i < flat::ActivationType_ANY; i <<= 1) {
    if ((rule_and_source.rule->activation_types() & i) == 0)
      continue;

    auto& existing = activations[flat::ActivationType(i)];

    if (existing.rule && GetRulePriority(*existing.rule) >
                             GetRulePriority(*rule_and_source.rule)) {
      continue;
    }

    existing = rule_and_source;
    result = true;
  }

  return result;
}

void GetActivationsFromCandidates(
    const FlatRulesByModifierList* candidates,
    const RulesIndex::RulesBufferMap& rule_buffers,
    const RulePatternMatcher::UrlInfo& url,
    const RulesIndex::BaseQuery& query,
    ActivationsFound* activations) {
  // This is used for activations. All rules are expected to be grouped
  // together, regardless of modifier.
  CHECK(candidates->size() == 1);

  for (const flat::RuleId* rule_id : *candidates->Get(0)->rules()) {
    DCHECK_NE(rule_id, nullptr);
    RuleAndSource rule_and_source =
        GetRequestFilterRuleAndSourceFromId(rule_buffers, *rule_id);
    const flat::RequestFilterRule& rule = *rule_and_source.rule;

    ActivationsFound modified_activations = *activations;
    // Avoid expensive tests if the rule wouldn't change anything.
    if (!AddActivationsFromRule(modified_activations, rule_and_source)) {
      continue;
    }

    if (!DoesRulePartyMatch(rule, query)) {
      continue;
    }

    if (!DoesOriginMatchFromDomainConstraints(query.GetOrigin(),
                                              rule_and_source, false)) {
      continue;
    }

    if (!DoesUrlMatchRulePattern(rule_and_source, url, false)) {
      continue;
    }

    std::swap(*activations, modified_activations);
  }

  return;
}

// `sorted_candidates` is sorted by GetRulePriority. This returns the first
// matching rule in `sorted_candidates` or null if no rule matches.
std::optional<RuleAndSource> FindMatchAmongCandidates(
    const FlatRulesByModifierList* sorted_candidates_by_modifier,
    const RulesIndex::RulesBufferMap& rule_buffers,
    const RulePatternMatcher::UrlInfo& url,
    const RulesIndex::RequestQuery& query,
    bool must_intersect_host,
    flat::ResourceType resource_type,
    const std::optional<RulesIndex::AdAttributionMatchParams>&
        ad_attribution_match_params,
    int current_rule_priority) {
  // This is used for request blocking. All rules are expected to be grouped
  // together, regardless of modifier.
  CHECK(sorted_candidates_by_modifier->size() == 1);

  const FlatRuleIdList* sorted_candidates =
      sorted_candidates_by_modifier->Get(0)->rules();

  DCHECK(std::is_sorted(
      sorted_candidates->begin(), sorted_candidates->end(),
      [rule_buffers](const flat::RuleId* lhs, const flat::RuleId* rhs) {
        DCHECK(lhs);
        DCHECK(rhs);
        return GetRulePriority(
                   *GetRequestFilterRuleAndSourceFromId(rule_buffers, *lhs)
                        .rule) >
               GetRulePriority(
                   *GetRequestFilterRuleAndSourceFromId(rule_buffers, *rhs)
                        .rule);
      }));

  for (const flat::RuleId* rule_id : *sorted_candidates) {
    DCHECK_NE(rule_id, nullptr);
    RuleAndSource rule_and_source =
        GetRequestFilterRuleAndSourceFromId(rule_buffers, *rule_id);
    const flat::RequestFilterRule& rule = *rule_and_source.rule;
    if (current_rule_priority >= GetRulePriority(*rule_and_source.rule))
      return std::nullopt;

    if (!DoesRuleResourceMatch(rule, resource_type)) {
      continue;
    }

    if (!DoesRulePartyMatch(rule, query)) {
      continue;
    }

    if (!DoesRuleMethodMatch(rule, query.GetMethod())) {
      continue;
    }

    if (!DoesOriginMatchFromDomainConstraints(
            query.GetOrigin(), rule_and_source,
            query.WantsDisableGenericRules())) {
      continue;
    }

    if (!DoesUrlMatchRulePattern(rule_and_source, url, must_intersect_host)) {
      continue;
    }

    if (rule.ad_domains_and_query_triggers()) {
      if (!ad_attribution_match_params) {
        continue;
      }

      bool query_and_trigger_match = false;
      for (const auto* ad_domain_and_query_trigger :
           *rule.ad_domains_and_query_triggers()) {
        query_and_trigger_match = DoesMatchAdAttributionParams(
            *ad_attribution_match_params, url.fold_case_spec(),
            ad_domain_and_query_trigger->string_view());
        if (query_and_trigger_match) {
          break;
        }
      }

      if (!query_and_trigger_match) {
        continue;
      }
    }

    return rule_and_source;
  }

  return std::nullopt;
}

// `sorted_candidates` is sorted with by GetRulePriority. The modifier value
// of matching rules are stored in `result` based on modifier type. For a
// given modifier value, only the rule with highest priority is stored for a
// given modifier value gets stored. If a pass all rule is encountered, it is
// stored separately and no further tule gets entered for that type.
void FindModifierRulesMatchesCandidates(
    const FlatRulesByModifierList* sorted_candidates_by_modifier,
    const RulesIndex::RulesBufferMap& rule_buffers,
    const RulePatternMatcher::UrlInfo& url,
    const RulesIndex::RequestQuery& query,
    flat::ResourceType resource_type,
    FoundModifiersByTypeInternal& result) {
  for (const flat::PrioritizedRuleList* sorted_candidates_list :
       *sorted_candidates_by_modifier) {
    const FlatRuleIdList* sorted_candidates = sorted_candidates_list->rules();

    DCHECK(std::is_sorted(
        sorted_candidates->begin(), sorted_candidates->end(),
        [rule_buffers](const flat::RuleId* lhs, const flat::RuleId* rhs) {
          DCHECK(lhs);
          DCHECK(rhs);
          return GetRulePriority(
                     *GetRequestFilterRuleAndSourceFromId(rule_buffers, *lhs)
                          .rule) >
                 GetRulePriority(
                     *GetRequestFilterRuleAndSourceFromId(rule_buffers, *rhs)
                          .rule);
        }));

    for (const flat::RuleId* rule_id : *sorted_candidates) {
      CHECK_NE(rule_id, nullptr);
      RuleAndSource rule_and_source =
          GetRequestFilterRuleAndSourceFromId(rule_buffers, *rule_id);
      const flat::RequestFilterRule& rule = *rule_and_source.rule;

      CHECK(rule.modifier() != flat::Modifier_NO_MODIFIER);

      FoundModifiersInternal& found = result[rule.modifier()];
      if (rule.decision() == flat::Decision_MODIFY) {
        found.found_modify_rules = true;
      }
      if (found.pass_all_rule &&
          rule_and_source.rule->decision() <= flat::Decision_PASS) {
        break;
      }

      if (!IsFullModifierPassRule(rule)) {
        for (const auto* modifer_value : *rule.modifier_values()) {
          auto existing = found.value_with_decision.find(modifer_value->str());
          if (existing != found.value_with_decision.end() &&
              GetRulePriority(*existing->second.rule) >
                  GetRulePriority(*rule_and_source.rule)) {
            continue;
          }
        }
      }

      if (!DoesRuleMethodMatch(rule, query.GetMethod())) {
        continue;
      }

      if (!DoesRuleResourceMatch(rule, resource_type)) {
        continue;
      }

      if (!DoesRulePartyMatch(rule, query)) {
        continue;
      }

      if (!DoesOriginMatchFromDomainConstraints(
              query.GetOrigin(), rule_and_source,
              query.WantsDisableGenericRules())) {
        continue;
      }

      if (!DoesUrlMatchRulePattern(rule_and_source, url, false)) {
        continue;
      }

      if (IsFullModifierPassRule(rule)) {
        found.pass_all_rule = rule_and_source;
        std::erase_if(found.value_with_decision,
                      [](const auto& value_with_decision) {
                        return value_with_decision.second.rule->decision() !=
                               flat::Decision_MODIFY_IMPORTANT;
                      });
      } else {
        for (const auto* modifier_value : *rule.modifier_values()) {
          found.value_with_decision[modifier_value->str()] = rule_and_source;
        }
      }
    }
  }
}

void FindMatchingRuleInMap(
    std::string_view url_spec,
    const flat::RulesMap* const rule_map,
    base::FunctionRef<bool(const FlatRulesByModifierList*)> callback) {
  const FlatNGramIndex* hash_table = rule_map->ngram_index();
  const flat::NGramToRules* empty_slot = rule_map->ngram_index_empty_slot();
  DCHECK_NE(hash_table, nullptr);

  NGramHashTableProber prober;

  auto ngrams = url_pattern_index::CreateNGramExtractor<
      RulesIndex::kNGramSize, uint64_t,
      url_pattern_index::NGramCaseExtraction::kCaseSensitive>(
      url_spec, [](char) { return false; });

  for (uint64_t ngram : ngrams) {
    const uint32_t slot_index = prober.FindSlot(
        ngram, hash_table->size(),
        [hash_table, empty_slot](NGram ngram, uint32_t slot_index) {
          const flat::NGramToRules* entry = hash_table->Get(slot_index);
          DCHECK_NE(entry, nullptr);
          return entry == empty_slot || entry->ngram() == ngram;
        });
    DCHECK_LT(slot_index, hash_table->size());

    const flat::NGramToRules* entry = hash_table->Get(slot_index);
    if (entry == empty_slot)
      continue;
    if (callback(entry->rules_by_modifier()))
      return;
  }

  if (rule_map->fallback_rules_by_modifier()->size() != 0) {
    callback(rule_map->fallback_rules_by_modifier());
  }
}

void GetSelectorsForDomain(
    bool skip_cosmetic_rules,
    const RulesIndex::RulesBufferMap& rules_buffers,
    ContentInjectionIndexTraversalResults* results,
    const flatbuffers::Vector<
        flatbuffers::Offset<flat::ContentInjectionRuleForDomain>>&
        rules_for_domain) {
  for (const auto* rule_for_domain : rules_for_domain) {
    switch (rule_for_domain->rule_type()) {
      case flat::ContentInjectionRuleType_COSMETIC: {
        if (skip_cosmetic_rules) {
          continue;
        }
        const flat::CosmeticRule* rule =
            GetCosmeticRuleFromId(rules_buffers, *rule_for_domain->rule_id());
        if (rule_for_domain->allow_for_domain()) {
          results->cosmetic_rules.selected.erase(rule);
          results->cosmetic_rules.exceptions.insert(rule);
        } else if (!results->cosmetic_rules.exceptions.count(rule)) {
          results->cosmetic_rules.selected.insert(rule);
        }
        break;
      }
      case flat::ContentInjectionRuleType_SCRIPTLET_INJECTION: {
        const flat::ScriptletInjectionRule* rule =
            GetScriptletInjectionRuleFromId(rules_buffers,
                                            *rule_for_domain->rule_id());
        if (rule_for_domain->allow_for_domain()) {
          results->scriptlet_injection_rules.selected.erase(rule);
          results->scriptlet_injection_rules.exceptions.insert(rule);
        } else if (!results->scriptlet_injection_rules.exceptions.count(rule)) {
          results->scriptlet_injection_rules.selected.insert(rule);
        }
        break;
      }
    }
  }
  return;
}
}  // namespace

// static
std::unique_ptr<RulesIndex> RulesIndex::CreateInstance(
    RulesBufferMap rules_buffers,
    std::string rules_index_buffer,
    bool* uses_all_buffers) {
  const flat::RulesIndex* const rules_index =
      flat::GetRulesIndex(rules_index_buffer.data());

  *uses_all_buffers = false;

  // Check that the index we got matches the rules for which it was built.
  for (const auto* checksum : *rules_index->sources_checksum()) {
    auto rule_buffer = rules_buffers.find(checksum->id());
    if (rule_buffer == rules_buffers.end())
      return nullptr;
    if (rule_buffer->second.checksum() != checksum->checksum()->str())
      return nullptr;
  }

  *uses_all_buffers =
      rules_index->sources_checksum()->size() == rules_buffers.size();

  return std::make_unique<RulesIndex>(
      std::move(rules_buffers), std::move(rules_index_buffer), rules_index);
}

RulesIndex::RulesIndex(RulesBufferMap rules_buffers,
                       std::string rules_index_buffer,
                       const flat::RulesIndex* const rules_index)
    : rules_buffers_(std::move(rules_buffers)),
      rules_index_buffer_(std::move(rules_index_buffer)),
      rules_index_(rules_index),
      activations_cache_(kActivationOriginCacheSize),
      matched_rules_cache_(kMatchedRulesOriginCacheSize) {}

RulesIndex::~RulesIndex() = default;

const ActivationResults& RulesIndex::FindActivations(
    const BaseQuery& query) const {
  auto origin_hit = activations_cache_.Get(query.GetOrigin());
  if (origin_hit == activations_cache_.end()) {
    origin_hit = activations_cache_.Put(
        query.GetOrigin(), ActivationForURLs(kActivationUrlCacheSize));
  }

  auto url_hit = origin_hit->second.Get(query.GetUrl());
  if (url_hit != origin_hit->second.end()) {
    return url_hit->second;
  }

  ActivationsFound activations;

  CHECK(CanFilterUrl(query.GetUrl(), false));

  RulePatternMatcher::UrlInfo url_info(query.GetUrl());

  auto handle_matches = [this, &activations, &url_info,
                         &query](const FlatRulesByModifierList* rule_list) {
    GetActivationsFromCandidates(rule_list, rules_buffers_, url_info, query,
                                 &activations);
    return false;
  };

  FindMatchingRuleInMap(url_info.fold_case_spec(),
                        rules_index_->activation_rules_map(), handle_matches);

  return origin_hit->second
      .Put(query.GetUrl(), ToActivationResults(activations))
      ->second;
}

const std::optional<RequestFilterRuleStub>&
RulesIndex::FindMatchingBeforeRequestRule(
    const RequestQuery& query,
    bool must_intersect_host,
    ResourceType resource_type,
    std::optional<AdAttributionMatchParams> ad_attribution_match_params) const {
  CHECK(CanFilterUrl(query.GetUrl(),
                     resource_type == ResourceType::kPopup ||
                         resource_type == ResourceType::kPopunder));

  RequestFlagsCacheKey flags_key{
      .disable_generic_rules = query.WantsDisableGenericRules(),
      .method = std::string(query.GetMethod()),
      .resource_type = resource_type,
      .attribution_match_params = std::move(ad_attribution_match_params)};

  auto origin_hit = matched_rules_cache_.Get(query.GetOrigin());
  if (origin_hit == matched_rules_cache_.end()) {
    origin_hit = matched_rules_cache_.Put(
        query.GetOrigin(), MatchedRulesForUrl(kMatchedRulesUrlCacheSize));
  }
  auto url_hit = origin_hit->second.Get(query.GetUrl());
  if (url_hit == origin_hit->second.end()) {
    url_hit = origin_hit->second.Put(query.GetUrl(), MatchedRulesForFlags{});
  }

  auto cache_hit = url_hit->second.find(flags_key);
  if (cache_hit == url_hit->second.end()) {
    cache_hit = url_hit->second
                    .try_emplace(std::move(flags_key), MatchedRulesCacheItem{})
                    .first;
  }

  if (cache_hit->second.blocking_rule) {
    return *cache_hit->second.blocking_rule;
  }

  RulePatternMatcher::UrlInfo url_info(query.GetUrl());

  flat::ResourceType flat_resource_type = ToFlatResourceType(resource_type);

  std::optional<RuleAndSource> rule_and_source = std::nullopt;
  auto handle_matches = [this, &rule_and_source, &url_info, must_intersect_host,
                         &query, flat_resource_type,
                         &cache_hit](const FlatRulesByModifierList* rule_list) {
    std::optional<RuleAndSource> new_rule_and_source = FindMatchAmongCandidates(
        rule_list, rules_buffers_, url_info, query, must_intersect_host,
        flat_resource_type, cache_hit->first.attribution_match_params,
        rule_and_source ? GetRulePriority(*rule_and_source->rule) : -1);
    if (!new_rule_and_source)
      return false;

    if (!rule_and_source)
      rule_and_source = new_rule_and_source;
    else if (GetRulePriority(*new_rule_and_source->rule) >
             GetRulePriority(*rule_and_source->rule))
      rule_and_source = new_rule_and_source;
    return GetRulePriority(*rule_and_source->rule) == GetMaxRulePriority();
  };

  FindMatchingRuleInMap(url_info.fold_case_spec(),
                        rules_index_->before_request_map(), handle_matches);

  std::optional<RequestFilterRuleStub> result;
  if (rule_and_source) {
    result = ToRuleStub(*rule_and_source);
  }

  cache_hit->second.blocking_rule = std::move(result);
  return *cache_hit->second.blocking_rule;
}

const RulesIndex::FoundModifiersByType& RulesIndex::FindMatchingModifierRules(
    ModifierCategory category,
    const RequestQuery& query,
    std::optional<ResourceType> resource_type) const {
  CHECK(CanFilterUrl(query.GetUrl(),
                     resource_type == ResourceType::kPopup ||
                         resource_type == ResourceType::kPopunder));

  RequestFlagsCacheKey flags_key{
      .disable_generic_rules = query.WantsDisableGenericRules(),
      .method = std::string(query.GetMethod()),
      .resource_type = resource_type,
      .attribution_match_params = std::nullopt};

  auto origin_hit = matched_rules_cache_.Get(query.GetOrigin());
  if (origin_hit == matched_rules_cache_.end()) {
    origin_hit = matched_rules_cache_.Put(
        query.GetOrigin(), MatchedRulesForUrl(kMatchedRulesUrlCacheSize));
  }
  auto url_hit = origin_hit->second.Get(query.GetUrl());
  if (url_hit == origin_hit->second.end()) {
    url_hit = origin_hit->second.Put(query.GetUrl(), MatchedRulesForFlags{});
  }

  auto cache_hit = url_hit->second.find(flags_key);
  if (cache_hit == url_hit->second.end()) {
    cache_hit = url_hit->second
                    .try_emplace(std::move(flags_key), MatchedRulesCacheItem{})
                    .first;
  }
  if (cache_hit->second.categories_in_cache.Has(category)) {
    return cache_hit->second.modifiers;
  }

  cache_hit->second.categories_in_cache.Put(category);

  const flat::RulesMap* rule_map = [this, category]() {
    switch (category) {
      case ModifierCategory::kBlockedRequest:
        return rules_index_->blocked_request_modifiers();
      case ModifierCategory::kAllowedRequest:
        return rules_index_->allowed_request_modifiers();
      case ModifierCategory::kHeadersReceived:
        return rules_index_->headers_received_map();
    }
  }();

  FoundModifiersByTypeInternal result;
  RulePatternMatcher::UrlInfo url_info(query.GetUrl());

  flat::ResourceType flat_resource_type =
      resource_type ? ToFlatResourceType(*resource_type)
                    : flat::ResourceType_ANY;

  auto handle_matches = [this, &result, &url_info, &query, flat_resource_type](
                            const FlatRulesByModifierList* rule_list) {
    FindModifierRulesMatchesCandidates(rule_list, rules_buffers_, url_info,
                                       query, flat_resource_type, result);

    return false;
  };

  FindMatchingRuleInMap(url_info.fold_case_spec(), rule_map, handle_matches);

  ToPublicFoundModifiersByType(result, cache_hit->second.modifiers);
  return cache_hit->second.modifiers;
}

std::string RulesIndex::GetDefaultStylesheet() {
  return rules_index_->default_stylesheet()->str();
}

RulesIndex::InjectionData::InjectionData() = default;
RulesIndex::InjectionData::InjectionData(InjectionData&& other) = default;
RulesIndex::InjectionData::~InjectionData() = default;
RulesIndex::InjectionData& RulesIndex::InjectionData::operator=(
    InjectionData&& other) = default;

RulesIndex::InjectionData RulesIndex::GetInjectionDataForOrigin(
    const url::Origin& origin,
    bool disable_specific_cosmetic_rules,
    bool disable_generic_cosmetic_rules) {
  ContentInjectionIndexTraversalResults results;

  const flat::ContentInjectionRulesTreeRoot* tree =
      rules_index_->content_injection_rules_tree();

  for (const auto* rule_for_domain : *tree->rules()) {
    switch (rule_for_domain->rule_type()) {
      case flat::ContentInjectionRuleType_COSMETIC: {
        const flat::CosmeticRule* rule =
            GetCosmeticRuleFromId(rules_buffers_, *rule_for_domain->rule_id());
        if (rule_for_domain->allow_for_domain())
          results.cosmetic_rules.exceptions.insert(rule);
        else if (!disable_generic_cosmetic_rules)
          results.cosmetic_rules.selected.insert(rule);
        break;
      }
      case flat::ContentInjectionRuleType_SCRIPTLET_INJECTION: {
        const flat::ScriptletInjectionRule* rule =
            GetScriptletInjectionRuleFromId(rules_buffers_,
                                            *rule_for_domain->rule_id());
        CHECK(rule_for_domain->allow_for_domain());
        results.scriptlet_injection_rules.exceptions.insert(rule);
        break;
      }
    }
  }

  if (origin.host().empty())
    return results.ToInjectionData();

  // Note that disable_specific_cosmetic hide does not apply to scriptlets in
  // the uBlock implementatio, so we still retrieve those.

  const size_t registry_length =
      net::registry_controlled_domains::GetCanonicalHostRegistryLength(
          origin.host(),
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  const flat::ContentInjectionRulesTreeNode* hostname_node =
      tree->nodes()->Get(tree->hostnames_index());
  const flat::ContentInjectionRulesTreeNode* entity_node = nullptr;
  size_t seen_length = 0;

  auto labels = base::SplitStringPiece(
      origin.host(), ".", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SplitResult::SPLIT_WANT_ALL);

  for (std::string_view& label : base::Reversed(labels)) {
    if (hostname_node) {
      std::optional<uint32_t> subdomain_node_index = GetSubdomainNodeIndex(
          label, tree->nodes()->size(), hostname_node->first_child_node_index(),
          hostname_node->subdomains());
      hostname_node = subdomain_node_index
                          ? tree->nodes()->Get(*subdomain_node_index)
                          : nullptr;
    }
    if (entity_node) {
      std::optional<uint32_t> subdomain_node_index = GetSubdomainNodeIndex(
          label, tree->nodes()->size(), entity_node->first_child_node_index(),
          entity_node->subdomains());
      entity_node = subdomain_node_index
                        ? tree->nodes()->Get(*subdomain_node_index)
                        : nullptr;
    }

    if (hostname_node) {
      GetSelectorsForDomain(disable_specific_cosmetic_rules, rules_buffers_,
                            &results, *hostname_node->rules());
    }

    if (entity_node) {
      GetSelectorsForDomain(disable_specific_cosmetic_rules, rules_buffers_,
                            &results, *entity_node->rules());
    }

    seen_length += label.length() + 1;
    if (seen_length == registry_length + 1) {
      entity_node = tree->nodes()->Get(tree->entities_index());
    }
  }

  for (auto* regex : *tree->regexes()) {
    // Use the rule buffer from the first rule using this regex to cache it.
    // There will in most cases only be one anyway.
    CHECK_GT(regex->rules()->size(), 0ull);
    const RuleBufferHolder& rule_buffer =
        rules_buffers_.at(regex->rules()->Get(0)->rule_id()->source_id());

    if (RE2::PartialMatch(
            re2::StringPiece(origin.host().data(), origin.host().size()),
            rule_buffer.GetRegexForPattern(regex->regex()))) {
      GetSelectorsForDomain(disable_specific_cosmetic_rules, rules_buffers_,
                            &results, *regex->rules());
    }
  }

  return results.ToInjectionData();
}

RulesIndex::BaseQuery::~BaseQuery() = default;
RulesIndex::RequestQuery::~RequestQuery() = default;
}  // namespace adblock_filter
