// Copyright (c) 2023 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/ios/adblock_rules_organizer.h"

#include <map>

#include "base/json/json_string_value_serializer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "components/ad_blocker/core/adblock_domain_constraints_tree.h"
#include "components/ad_blocker/ios/ios_rule_utils.h"
#include "components/ad_blocker/ios/utils.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace adblock_filter {

namespace {
// This is not the maximum amount allowed by webkit. We have been tweaking these
// values to find a sweet spot in terms of performance. WebKit seem to struggle
// processing very large lists instead of many small lists.
constexpr size_t kMaxRules = 15000;

// This restriction isn't imposed by iOS, but since we are going to have a copy
// of all allow rules in every rule list, we better make sure there is
// resaonable space left for other rules.
constexpr size_t kMaxAllowRules = 5000;
constexpr size_t kMaxGenericAllowRules = 500;
constexpr size_t kMaxAllowAndGenericAllowRules =
    kMaxAllowRules + kMaxGenericAllowRules;

const int kMaxSelectorsPerCssRule = 1024;

constexpr std::string_view kBlockUserPassUrl = "^[a-z][a-z+.-]*:\\/\\/[^/]*@";

class BlockListListMaker {
 public:
  BlockListListMaker(base::ListValue allow_rules)
      : allow_rules_(std::move(allow_rules)) {
    next_list_.reserve(kMaxRules);
  }
  ~BlockListListMaker() {
    CHECK(block_lists_.empty() && next_list_.empty())
        << "Result should be read before destruction";
  }

  BlockListListMaker(const BlockListListMaker&) = delete;
  BlockListListMaker& operator=(const BlockListListMaker&) = delete;

  void AddRule(base::DictValue rule) {
    if ((next_list_.size() + allow_rules_.size()) == kMaxRules) {
      AddNextList();
    }
    next_list_.Append(std::move(rule));
  }

  void AddRules(base::ListValue rules) {
    for (auto& rule : rules) {
      if (!rule.is_dict()) {
        continue;
      }
      AddRule(std::move(rule.GetDict()));
    }
  }

  void AddRulePairs(base::ListValue pairs) {
    for (auto& pair : pairs) {
      CHECK(pair.is_list());
      CHECK(pair.GetList().size() <= 2);
      CHECK(pair.GetList().size() >= 1);
      // Make sure we have space for the pair.
      if ((next_list_.size() + allow_rules_.size()) >
          kMaxRules - pair.GetList().size()) {
        AddNextList();
      }
      if (pair.GetList().size() == 2) {
        next_list_.Insert(next_list_.begin(), std::move(pair.GetList()[1]));
        next_list_.Insert(next_list_.begin(), std::move(pair.GetList()[0]));
      }
    }
  }

  base::ListValue GetAndReset() {
    if (!next_list_.empty())
      AddNextList();

    return std::move(block_lists_);
  }

 private:
  void AddNextList() {
    for (const auto& allow_rule : allow_rules_) {
      next_list_.Append(allow_rule.Clone());
    }
    std::string serialized_list;
    JSONStringValueSerializer(&serialized_list).Serialize(next_list_);
    block_lists_.Append(serialized_list);
    next_list_.clear();
    next_list_.reserve(kMaxRules);
  }

  base::ListValue block_lists_;
  base::ListValue next_list_;
  base::ListValue allow_rules_;
};

class ContentInjectionRuleDomainConstraints {
 public:
  struct MergedNode {
    DomainConstraintsTree::Node::NodeType type =
        DomainConstraintsTree::Node::kNone;
    // This map doesn't strictly need ordering, but writing unit tests is much
    // harder without it
    std::map<std::string, MergedNode> subdomains;

    void ExtractConstraints(std::string current_suffixes,
                            std::vector<std::string>* inclusions,
                            std::vector<std::string>& exclusions) const {
      // Inclusions are omitted when processing a geneirc include rule
      switch (type) {
        case DomainConstraintsTree::Node::kIncluded:
          if (inclusions) {
            inclusions->push_back(
                ios_rule_utils::DomainToIfURL(current_suffixes, true, false));
          }
          break;
        case DomainConstraintsTree::Node::kExcluded:
          exclusions.push_back(
              ios_rule_utils::DomainToIfURL(current_suffixes, true, false));
          CHECK(subdomains.empty());
          return;
        case DomainConstraintsTree::Node::kNone:
          break;
      }

      if (!current_suffixes.empty()) {
        current_suffixes.insert(0, ".");
      }

      for (const auto& [label, subdomain_node] : subdomains) {
        subdomain_node.ExtractConstraints(label + current_suffixes, inclusions,
                                          exclusions);
      }
    }
  };

  ContentInjectionRuleDomainConstraints() = default;
  ~ContentInjectionRuleDomainConstraints() = default;
  ContentInjectionRuleDomainConstraints(
      ContentInjectionRuleDomainConstraints&&) = default;
  ContentInjectionRuleDomainConstraints& operator=(
      ContentInjectionRuleDomainConstraints&&) = default;

  void AddContraints(const base::DictValue& constraints) {
    // The caller should ensure this.
    CHECK(root_type_ != DomainConstraintsTree::Node::kExcluded);

    int root_type = constraints.FindInt(ios_rule_utils::kDomainTreeNodeType)
                        .value_or(DomainConstraintsTree::Node::kNone);

    switch (root_type) {
      case DomainConstraintsTree::Node::kExcluded:
      case DomainConstraintsTree::Node::kIncluded:
        root_type_ =
            static_cast<DomainConstraintsTree::Node::NodeType>(root_type);
        break;
      case DomainConstraintsTree::Node::kNone:
        break;
      default:
        return;
    }

    const base::DictValue* hostname_root =
        constraints.FindDict(ios_rule_utils::kDomainTreeHostnameNode);
    if (hostname_root) {
      MergeConstraintTreeNode(*hostname_root, hostnames_root_);
    }

    const base::DictValue* entities_root =
        constraints.FindDict(ios_rule_utils::kDomainTreeEntityNode);
    if (entities_root) {
      MergeConstraintTreeNode(*entities_root, entities_root_);
    }

    const base::ListValue* included_regexes =
        constraints.FindList(ios_rule_utils::kDomainTreeIncludedRegexes);
    if (included_regexes) {
      for (const auto& included_regex : *included_regexes) {
        if (included_regex.is_string() &&
            !excluded_regexes_.contains(included_regex.GetString())) {
          included_regexes_.insert(included_regex.GetString());
        }
      }
    }

    const base::ListValue* excluded_regexes =
        constraints.FindList(ios_rule_utils::kDomainTreeExcludedRegexes);
    if (excluded_regexes) {
      for (const auto& excluded_regex : *excluded_regexes) {
        if (excluded_regex.is_string()) {
          excluded_regexes_.insert(excluded_regex.GetString());
          included_regexes_.erase(excluded_regex.GetString());
        }
      }
    }
  }

  void AddConstraintsList(const base::ListValue& constraints_list) {
    for (auto& constraints : constraints_list) {
      if (root_type_ == DomainConstraintsTree::Node::kExcluded) {
        return;
      }

      if (!constraints.is_dict()) {
        continue;
      }

      AddContraints(constraints.GetDict());
    }
  }

  const MergedNode& hostname_root() const { return hostnames_root_; }
  const MergedNode& entities_root() const { return entities_root_; }
  const absl::flat_hash_set<std::string>& included_regexes() const {
    return included_regexes_;
  }
  const absl::flat_hash_set<std::string>& excluded_regexes() const {
    return excluded_regexes_;
  }

  DomainConstraintsTree::Node::NodeType root_type() const { return root_type_; }

 private:
  void MergeConstraintTreeNode(const base::DictValue& source,
                               MergedNode& destination) {
    int source_type = source.FindInt(ios_rule_utils::kDomainTreeNodeType)
                          .value_or(DomainConstraintsTree::Node::kNone);

    if (destination.type == DomainConstraintsTree::Node::kExcluded) {
      return;
    }

    if (source_type == DomainConstraintsTree::Node::kExcluded) {
      destination.type = DomainConstraintsTree::Node::kExcluded;
      destination.subdomains.clear();
      return;
    }

    if (source_type == DomainConstraintsTree::Node::kIncluded) {
      destination.type = DomainConstraintsTree::Node::kIncluded;
    }

    for (const auto [label, subdomain] : source) {
      if (!subdomain.is_dict()) {
        continue;
      }
      MergeConstraintTreeNode(subdomain.GetDict(),
                              destination.subdomains[label]);
    }
  }

  MergedNode hostnames_root_;
  MergedNode entities_root_;
  absl::flat_hash_set<std::string> included_regexes_;
  absl::flat_hash_set<std::string> excluded_regexes_;

  DomainConstraintsTree::Node::NodeType root_type_ =
      DomainConstraintsTree::Node::kNone;
};

struct MakeCosmeticRulesResult {
  bool is_generic_hide = false;
  std::optional<base::DictValue> rule;
};
MakeCosmeticRulesResult MakeCosmeticRule(
    std::string selector,
    const ContentInjectionRuleDomainConstraints& domain_tree) {
  MakeCosmeticRulesResult result;
  if (domain_tree.root_type() == DomainConstraintsTree::Node::kExcluded) {
    // No rules for this selector
    return result;
  }

  result.is_generic_hide =
      domain_tree.root_type() == DomainConstraintsTree::Node::kIncluded;
  std::vector<std::string> included;
  std::vector<std::string> excluded;
  for (const auto& regexp_constraint : domain_tree.excluded_regexes()) {
    excluded.push_back(
        ios_rule_utils::DomainToIfURL(regexp_constraint, false, false));
  }

  if (!result.is_generic_hide) {
    for (const auto& regexp_constraint : domain_tree.included_regexes()) {
      included.push_back(
          ios_rule_utils::DomainToIfURL(regexp_constraint, false, false));
    }
  }

  domain_tree.hostname_root().ExtractConstraints(
      "", result.is_generic_hide ? nullptr : &included, excluded);
  domain_tree.entities_root().ExtractConstraints(
      std::string(ios_rule_utils::kWildcard),
      result.is_generic_hide ? nullptr : &included, excluded);

  ios_rule_utils::Trigger trigger(ios_rule_utils::kWildcardRegex, false);
  if (result.is_generic_hide) {
    if (excluded.empty()) {
      return result;
    }
    trigger.set_frame_url_filter(excluded, true, true);
  }

  if (!result.is_generic_hide) {
    if (!excluded.empty()) {
      // A specific cosmetic rule with exclusion requires a rules list all of
      // its own. Discard it since cosmetic rules are not providing any
      // privacy or security.
      return result;
    }

    // No include means either the rule was generic, or it was specific, but
    // all the inclusions were overriden by exclusions. We just ruled out both
    // of those.
    CHECK(!included.empty());
    trigger.set_frame_url_filter(included, false, true);
  }

  result.rule = ios_rule_utils::MakeRule(
      trigger, ios_rule_utils::Action::CssHideAction(selector));

  return result;
}

class ScriptletRulesTreeBuilder {
 public:
  ScriptletRulesTreeBuilder() = default;
  ~ScriptletRulesTreeBuilder() = default;

  ScriptletRulesTreeBuilder(ScriptletRulesTreeBuilder&&) = default;
  ScriptletRulesTreeBuilder& operator=(ScriptletRulesTreeBuilder&&) = default;

  void AddRules(const base::ListValue& rules) {
    for (const auto& rule : rules) {
      if (rule.is_dict()) {
        AddRule(rule.GetDict());
      }
    }
  }

  void AddRule(const base::DictValue& rule) {
    const base::DictValue* domain_constraints =
        rule.FindDict(ios_rule_utils::kDomainConstraints);
    CHECK(domain_constraints);
    const base::DictValue* scriptlets =
        rule.FindDict(ios_rule_utils::kScriptletRules);
    CHECK(scriptlets);

    auto constraints = rules_.find(*scriptlets);
    if (constraints == rules_.end()) {
      rules_
          .emplace(scriptlets->Clone(), ContentInjectionRuleDomainConstraints())
          .first->second.AddContraints(*domain_constraints);
    } else {
      constraints->second.AddContraints(*domain_constraints);
    }
  }

  /* Self-destructively builds the resulting tree */
  base::DictValue MakeTree() && {
    base::DictValue result;

    base::ListValue* result_scriptlets =
        result.EnsureList(ios_rule_utils::kScriptletRules);
    base::DictValue* result_domain_tree =
        result.EnsureDict(ios_rule_utils::kDomainConstraints);
    base::DictValue* result_hostnames_root =
        result_domain_tree->EnsureDict(ios_rule_utils::kDomainTreeHostnameNode);
    base::DictValue* result_entities_root =
        result_domain_tree->EnsureDict(ios_rule_utils::kDomainTreeEntityNode);
    base::DictValue* result_included_regexes = result_domain_tree->EnsureDict(
        ios_rule_utils::kDomainTreeIncludedRegexes);
    base::DictValue* result_excluded_regexes = result_domain_tree->EnsureDict(
        ios_rule_utils::kDomainTreeExcludedRegexes);

    while (!rules_.empty()) {
      if (!base::IsValueInRangeForNumericType<int>(result_scriptlets->size())) {
        // Too many rules to allow storing indexes as int. That's an excessive
        // amount. Abort.
        return result;
      }

      auto rule = rules_.extract(rules_.begin());
      if (rule.mapped().root_type() == DomainConstraintsTree::Node::kExcluded) {
        // A generic exclusion was found. Skip the entry.
        continue;
      }

      int index = base::checked_cast<int>(result_scriptlets->size());
      result_scriptlets->Append(std::move(rule.key()));

      MergeDomainConstraintNode(index, rule.mapped().hostname_root(),
                                *result_hostnames_root);

      MergeDomainConstraintNode(index, rule.mapped().entities_root(),
                                *result_entities_root);

      for (const auto& included_regex : rule.mapped().included_regexes()) {
        result_included_regexes->EnsureList(included_regex)->Append(index);
      }

      for (const auto& excluded_regex : rule.mapped().excluded_regexes()) {
        result_excluded_regexes->EnsureList(excluded_regex)->Append(index);
      }
    }

    return result;
  }

 private:
  void MergeDomainConstraintNode(
      int scriptlet_index,
      ContentInjectionRuleDomainConstraints::MergedNode node,
      base::DictValue& into) {
    switch (node.type) {
      case DomainConstraintsTree::Node::kNone:
        break;
      case DomainConstraintsTree::Node::kIncluded:
        into.EnsureList(ios_rule_utils::kIncluded)->Append(scriptlet_index);
        break;
      case DomainConstraintsTree::Node::kExcluded:
        into.EnsureList(ios_rule_utils::kExcluded)->Append(scriptlet_index);
        break;
    }

    for (const auto& [label, subdomain] : node.subdomains) {
      if (label == ios_rule_utils::kDomainTreeNodeType) {
        continue;
      }
      MergeDomainConstraintNode(scriptlet_index, subdomain,
                                *into.EnsureDict(label));
    }
  }

  // Maps each scriptlets+argument pairs to the list of domain constraint tree
  // found for them.
  std::map<base::DictValue, ContentInjectionRuleDomainConstraints> rules_;
};
}  // namespace

CompiledRules::CompiledRules(base::Value rules, std::string checksum)
    : rules_(std::move(rules)), checksum_(std::move(checksum)) {}
CompiledRules::~CompiledRules() = default;

base::Value OrganizeRules(
    std::map<uint32_t, scoped_refptr<CompiledRules>> all_compiled_rules,
    base::Value exception_rule) {
  base::ListValue all_network_allow_rules;
  all_network_allow_rules.reserve(kMaxAllowRules);
  base::ListValue all_network_allow_and_generic_allow_rules;
  all_network_allow_and_generic_allow_rules.reserve(
      kMaxAllowAndGenericAllowRules);
  // This is essentially specifichide allow rules
  base::ListValue all_cosmetic_specific_allow_rules;
  all_cosmetic_specific_allow_rules.reserve(kMaxGenericAllowRules);
  // This is essentially generichide allow rules
  base::ListValue all_cosmetic_generic_allow_rules;
  all_cosmetic_generic_allow_rules.reserve(kMaxGenericAllowRules);

  base::ListValue all_partner_list_allowed_documents;

  ScriptletRulesTreeBuilder merged_scriptlet_rules;

  base::DictValue metadata;

  for (const auto& [id, compiled_rules] : all_compiled_rules) {
    // Record this to ensure we can find out if the organized rules set still
    // matches the original compiled rules lists-
    metadata.EnsureDict(ios_rule_utils::kListChecksums)
        ->Set(base::NumberToString(id), compiled_rules->checksum());

    CHECK(compiled_rules->rules().is_dict());
    const base::DictValue& rules = compiled_rules->rules().GetDict();
    const base::DictValue* network_rules =
        rules.FindDict(ios_rule_utils::kNetworkRules);
    if (network_rules) {
      const base::ListValue* network_allow_rules =
          network_rules->FindList(ios_rule_utils::kAllowRules);
      if (network_allow_rules) {
        for (const auto& rule : *network_allow_rules) {
          all_network_allow_rules.Append(rule.Clone());
          all_network_allow_and_generic_allow_rules.Append(rule.Clone());
        }
      }
      const base::ListValue* network_generic_allow_rules =
          network_rules->FindList(ios_rule_utils::kGenericAllowRules);
      if (network_generic_allow_rules) {
        for (const auto& rule : *network_generic_allow_rules)
          all_network_allow_and_generic_allow_rules.Append(rule.Clone());
      }
    }

    const base::DictValue* cosmetic_rules =
        rules.FindDict(ios_rule_utils::kCosmeticRules);
    if (cosmetic_rules) {
      const base::DictValue* cosmetic_allow_rules =
          cosmetic_rules->FindDict(ios_rule_utils::kAllowRules);
      if (cosmetic_allow_rules) {
        const base::ListValue* cosmetic_specific_allow_rules =
            cosmetic_allow_rules->FindList(ios_rule_utils::kSpecific);
        if (cosmetic_specific_allow_rules) {
          for (const auto& rule : *cosmetic_specific_allow_rules)
            all_cosmetic_specific_allow_rules.Append(rule.Clone());
        }
        const base::ListValue* cosmetic_generic_allow_rules =
            cosmetic_allow_rules->FindList(ios_rule_utils::kGeneric);
        if (cosmetic_generic_allow_rules) {
          for (const auto& rule : *cosmetic_generic_allow_rules)
            all_cosmetic_generic_allow_rules.Append(rule.Clone());
        }
      }
    }

    const base::ListValue* scriptlet_rules =
        rules.FindList(ios_rule_utils::kScriptletRules);
    if (scriptlet_rules) {
      merged_scriptlet_rules.AddRules(*scriptlet_rules);
    }

    const base::ListValue* partner_list_allowed_documents =
        rules.FindList(ios_rule_utils::kPartnerListAllowedDocuments);
    if (partner_list_allowed_documents) {
      for (const auto& partner_list_allowed_document :
           *partner_list_allowed_documents) {
        all_partner_list_allowed_documents.Append(
            partner_list_allowed_document.GetString());
      }
    }
  }

  if (all_network_allow_rules.size() > kMaxAllowRules ||
      all_network_allow_and_generic_allow_rules.size() >
          kMaxAllowAndGenericAllowRules ||
      all_cosmetic_specific_allow_rules.size() > kMaxAllowRules ||
      all_cosmetic_generic_allow_rules.size() > kMaxGenericAllowRules) {
    return base::Value();
  }

  if (exception_rule.is_dict()) {
    all_network_allow_rules.Append(exception_rule.Clone());
    all_network_allow_and_generic_allow_rules.Append(exception_rule.Clone());
    all_cosmetic_specific_allow_rules.Append(exception_rule.Clone());
    all_cosmetic_generic_allow_rules.Append(exception_rule.Clone());
    std::string serialized_exception;
    if (!JSONStringValueSerializer(&serialized_exception)
             .Serialize(exception_rule))
      NOTREACHED();
    metadata.Set(ios_rule_utils::kExceptionRule,
                 CalculateBufferChecksum(serialized_exception));
  }

  // Add a rule to block any url with user credentials. Those are unfortunately
  // passed as-is to the filter, even if the credentials are removed upon making
  // the request, allowing for potential semantic URL attacks on the ad blocker.
  ios_rule_utils::Trigger block_userpass_url_trigger(
      std::string(kBlockUserPassUrl), false);
  base::DictValue block_userpass_url_rule = ios_rule_utils::MakeRule(
      block_userpass_url_trigger, ios_rule_utils::Action::BlockAction());
  all_network_allow_rules.Append(block_userpass_url_rule.Clone());
  all_network_allow_and_generic_allow_rules.Append(
      std::move(block_userpass_url_rule));

  BlockListListMaker network_specific_block_lists_maker(
      std::move(all_network_allow_rules));
  BlockListListMaker network_generic_block_lists_maker(
      std::move(all_network_allow_and_generic_allow_rules));
  BlockListListMaker network_block_important_lists_maker(base::ListValue{});
  // This map doesn't strictly need ordering, but writing unit tests is much
  // harder without it
  std::map<std::string, ContentInjectionRuleDomainConstraints>
      constraints_for_selectors;

  for (const auto& [id, compiled_rules] : all_compiled_rules) {
    CHECK(compiled_rules->rules().is_dict());
    const base::DictValue& rules = compiled_rules->rules().GetDict();
    const base::DictValue* network_rules =
        rules.FindDict(ios_rule_utils::kNetworkRules);
    if (network_rules) {
      const base::ListValue* block_allow_pairs =
          network_rules->FindList(ios_rule_utils::kBlockAllowPairs);
      // The allow rules of block-allow pairs can technically interfere with
      // unrelated block rules, so we place them first to reduce this risk,
      // since allow rules only override rules that came before them. They may
      // still interfere among each other, but avoiding that would require
      // making one list for each pair, which would quickly become expensive.
      // Note that generic rules never result in block-allow pairs.
      if (block_allow_pairs) {
        network_specific_block_lists_maker.AddRulePairs(
            block_allow_pairs->Clone());
      }
      const base::DictValue* block_rules =
          network_rules->FindDict(ios_rule_utils::kBlockRules);
      if (block_rules) {
        const base::ListValue* specific_block_rules =
            block_rules->FindList(ios_rule_utils::kSpecific);
        if (specific_block_rules) {
          network_specific_block_lists_maker.AddRules(
              specific_block_rules->Clone());
        }
        const base::ListValue* generic_block_rules =
            block_rules->FindList(ios_rule_utils::kGeneric);
        if (generic_block_rules) {
          network_generic_block_lists_maker.AddRules(
              generic_block_rules->Clone());
        }
      }
      const base::ListValue* block_important_rules =
          network_rules->FindList(ios_rule_utils::kBlockImportantRules);
      if (block_important_rules) {
        network_block_important_lists_maker.AddRules(
            block_important_rules->Clone());
      }
    }

    const base::DictValue* cosmetic_rules =
        rules.FindDict(ios_rule_utils::kCosmeticRules);
    if (cosmetic_rules) {
      const base::DictValue* cosmetic_rules_selectors =
          cosmetic_rules->FindDict(ios_rule_utils::kSelector);
      if (cosmetic_rules_selectors) {
        for (const auto [selector, constraints_for_selector] :
             *cosmetic_rules_selectors) {
          if (!constraints_for_selector.is_list()) {
            continue;
          }
          constraints_for_selectors[selector].AddConstraintsList(
              constraints_for_selector.GetList());
        }
      }
    }
  }

  base::ListValue ios_content_blocker_rules;
  for (auto* maker :
       {&network_specific_block_lists_maker, &network_generic_block_lists_maker,
        &network_block_important_lists_maker}) {
    base::ListValue lists = maker->GetAndReset();
    for (auto& list : lists) {
      ios_content_blocker_rules.Append(std::move(list));
    }
  }

  std::string always_blocked;
  int always_blocked_count = 0;
  ios_rule_utils::Trigger always_blocked_trigger(ios_rule_utils::kWildcardRegex,
                                                 false);

  BlockListListMaker cosmetic_specific_block_lists_maker(
      all_cosmetic_specific_allow_rules.Clone());
  BlockListListMaker cosmetic_generic_block_list_maker(
      all_cosmetic_generic_allow_rules.Clone());

  for (auto& [selector, constraints] : constraints_for_selectors) {
    MakeCosmeticRulesResult rule = MakeCosmeticRule(selector, constraints);

    if (rule.is_generic_hide) {
      if (rule.rule) {
        cosmetic_generic_block_list_maker.AddRule(std::move(*rule.rule));

      } else {
        if (always_blocked_count == kMaxSelectorsPerCssRule) {
          cosmetic_generic_block_list_maker.AddRule(ios_rule_utils::MakeRule(
              always_blocked_trigger,
              ios_rule_utils::Action::CssHideAction(always_blocked)));
          always_blocked.clear();
          always_blocked_count = 0;
        } else if (always_blocked_count != 0) {
          always_blocked.append(", ");
        }
        always_blocked.append(selector);
        always_blocked_count++;
      }
    } else if (rule.rule) {
      cosmetic_specific_block_lists_maker.AddRule(std::move(*rule.rule));
    }
  }

  if (always_blocked_count != 0) {
    cosmetic_generic_block_list_maker.AddRule(ios_rule_utils::MakeRule(
        always_blocked_trigger,
        ios_rule_utils::Action::CssHideAction(always_blocked)));
  }

  for (auto* maker : {&cosmetic_specific_block_lists_maker,
                      &cosmetic_generic_block_list_maker}) {
    base::ListValue lists = maker->GetAndReset();
    for (auto& list : lists) {
      ios_content_blocker_rules.Append(std::move(list));
    }
  }

  base::DictValue non_ios_rules_and_metadata;
  non_ios_rules_and_metadata.Set(ios_rule_utils::kVersion,
                                 GetOrganizedRulesVersionNumber());
  non_ios_rules_and_metadata.Set(ios_rule_utils::kMetadata,
                                 std::move(metadata));
  non_ios_rules_and_metadata.Set(ios_rule_utils::kScriptletRules,
                                 std::move(merged_scriptlet_rules).MakeTree());
  non_ios_rules_and_metadata.Set(ios_rule_utils::kPartnerListAllowedDocuments,
                                 std::move(all_partner_list_allowed_documents));

  base::DictValue result;
  result.Set(ios_rule_utils::kNonIosRulesAndMetadata,
             std::move(non_ios_rules_and_metadata));
  result.Set(ios_rule_utils::kIosContentBlockerRules,
             std::move(ios_content_blocker_rules));

  return base::Value(std::move(result));
}
}  // namespace adblock_filter
