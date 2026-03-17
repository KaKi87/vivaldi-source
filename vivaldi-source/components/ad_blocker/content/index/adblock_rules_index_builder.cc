// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/content/index/adblock_rules_index_builder.h"

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "components/ad_blocker/content/index/adblock_rules_index.h"
#include "components/ad_blocker/content/index/index_utils.h"
#include "components/ad_blocker/content/index/stylesheet_builder.h"
#include "components/url_pattern_index/closed_hash_map.h"
#include "components/url_pattern_index/ngram_extractor.h"
#include "components/url_pattern_index/uint64_hasher.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "vivaldi/components/ad_blocker/content/index/flat/adblock_rules_index_generated.h"
#include "vivaldi/components/ad_blocker/content/index/flat/adblock_rules_list_generated.h"

namespace adblock_filter {

namespace {
// The integer type used to represent N-grams.
using NGram = uint64_t;
// The hasher used for hashing N-grams.
using NGramHasher = url_pattern_index::Uint64ToUint32Hasher;
// The hash table probe sequence used both by UrlPatternIndex and its builder.
using NGramHashTableProber =
    url_pattern_index::DefaultProber<NGram, NGramHasher>;

using SourceChecksumOffset = flatbuffers::Offset<flat::SourceChecksum>;
using RuleIdOffset = flatbuffers::Offset<flat::RuleId>;
using PrioritizedRuleListOffset =
    flatbuffers::Offset<flat::PrioritizedRuleList>;
using RulesMapOffset = flatbuffers::Offset<flat::RulesMap>;
using RulesIndexOffset = flatbuffers::Offset<flat::RulesIndex>;

using MutableRulesList =
    absl::flat_hash_map<flat::Modifier,
                        std::vector<std::pair<RuleIdOffset, int>>>;
using SourceChecksums = std::vector<SourceChecksumOffset>;

using ContentInjectionTreeNodeOffset =
    flatbuffers::Offset<flat::ContentInjectionRulesTreeNode>;
using ContentInjectionRuleTreeNodes =
    std::vector<ContentInjectionTreeNodeOffset>;
using ContentInjectionRuleForDomainOffset =
    flatbuffers::Offset<flat::ContentInjectionRuleForDomain>;
using ContentInjectionRuleForDomainOffsets = flatbuffers::Offset<
    flatbuffers::Vector<ContentInjectionRuleForDomainOffset>>;

using MutableNGramMap = url_pattern_index::
    ClosedHashMap<NGram, MutableRulesList, NGramHashTableProber>;

static_assert(RulesIndex::kNGramSize <= sizeof(NGram),
              "NGram type is too narrow.");

struct IndexBuildData {
  MutableNGramMap map;
  MutableRulesList fallback;
};

struct RuleId {
  RuleId(uint32_t source_id, uint32_t rule_nr)
      : source_id(source_id), rule_nr(rule_nr) {}
  uint32_t source_id;
  uint32_t rule_nr;
};

struct ContentInjectionRuleForDomain {
  ContentInjectionRuleForDomain(RuleId rule_id, bool allow_for_domain)
      : rule_id(rule_id), allow_for_domain(allow_for_domain) {}
  RuleId rule_id;
  bool allow_for_domain;
};

template <class T>
struct RuleType {};

template <>
struct RuleType<flat::CosmeticRule> {
  static constexpr flat::ContentInjectionRuleType value =
      flat::ContentInjectionRuleType_COSMETIC;
};

template <>
struct RuleType<flat::ScriptletInjectionRule> {
  static constexpr flat::ContentInjectionRuleType value =
      flat::ContentInjectionRuleType_SCRIPTLET_INJECTION;
};

struct ContentInjectionRuleTreeNodeContent {
  std::map<const flat::CosmeticRule*,
           ContentInjectionRuleForDomain,
           ContentInjectionRuleBodyCompare>
      rule_from_cosmetic_rule_body;
  std::map<const flat::ScriptletInjectionRule*,
           ContentInjectionRuleForDomain,
           ContentInjectionRuleBodyCompare>
      rule_from_scriptlet_injection_rule_body;

  std::map<const flat::CosmeticRule*,
           ContentInjectionRuleForDomain,
           ContentInjectionRuleBodyCompare>&
  GetMap(const flat::CosmeticRule* rule) {
    return rule_from_cosmetic_rule_body;
  }

  std::map<const flat::ScriptletInjectionRule*,
           ContentInjectionRuleForDomain,
           ContentInjectionRuleBodyCompare>&
  GetMap(const flat::ScriptletInjectionRule* rule) {
    return rule_from_scriptlet_injection_rule_body;
  }
};

struct ContentInjectionRuleTreeNode {
  std::map<std::string, ContentInjectionRuleTreeNode> subdomains;
  ContentInjectionRuleTreeNodeContent content;
};

struct ContentInjectionRuleTreeRoot {
  ContentInjectionRuleTreeNodeContent content;
  ContentInjectionRuleTreeNode hostname_tree;
  ContentInjectionRuleTreeNode entity_tree;
  std::map<std::string, ContentInjectionRuleTreeNodeContent> regexes;
};

std::string GetNGramSearchString(const flat::RequestFilterRule& rule) {
  if (rule.pattern_type() == flat::PatternType_REGEXP)
    return rule.ngram_search_string()->str();
  if ((rule.options() & flat::OptionFlag_IS_CASE_SENSITIVE))
    return rule.ngram_search_string()->str();
  return rule.pattern()->str();
}

void AddRuleToMap(const flat::RequestFilterRule& rule,
                  RuleIdOffset rule_id,
                  bool ignore_modifier,
                  IndexBuildData& build_data) {
  size_t min_list_size = std::numeric_limits<size_t>::max();
  NGram best_ngram = 0;
  std::string pattern = GetNGramSearchString(rule);
  auto ngrams = url_pattern_index::CreateNGramExtractor<
      RulesIndex::kNGramSize, NGram,
      url_pattern_index::NGramCaseExtraction::kCaseSensitive>(
      pattern, [](char c) { return c == '*' || c == '^'; });

  for (uint64_t ngram : ngrams) {
    const MutableRulesList* rules = build_data.map.Get(ngram);
    const size_t list_size = rules ? rules->size() : 0;
    if (list_size < min_list_size) {
      min_list_size = list_size;
      best_ngram = ngram;
      if (list_size == 0)
        break;
    }
  }

  // For activation rules and before request rules, there is no need to take
  // modifiers into account. Group everythin in one list.
  flat::Modifier modifier =
      ignore_modifier ? flat::Modifier_NO_MODIFIER : rule.modifier();

  if (best_ngram) {
    build_data.map[best_ngram][modifier].push_back(
        std::make_pair(rule_id, GetRulePriority(rule)));
  } else {
    build_data.fallback[modifier].push_back(
        std::make_pair(rule_id, GetRulePriority(rule)));
  }
}

RulesMapOffset BuildFlatMap(flatbuffers::FlatBufferBuilder* builder,
                            IndexBuildData& build_data) {
  std::vector<flatbuffers::Offset<flat::NGramToRules>> flat_map(
      build_data.map.table_size());

  flatbuffers::Offset<flat::NGramToRules> empty_slot_offset =
      flat::CreateNGramToRules(*builder);

  auto priority_comparator = [](const std::pair<RuleIdOffset, int>& lhs,
                                const std::pair<RuleIdOffset, int>& rhs) {
    return lhs.second > rhs.second;
  };

  for (size_t i = 0, size = build_data.map.table_size(); i != size; ++i) {
    const uint32_t entry_index = build_data.map.hash_table()[i];
    if (entry_index >= build_data.map.size()) {
      flat_map[i] = empty_slot_offset;
      continue;
    }

    const MutableNGramMap::EntryType& entry =
        build_data.map.entries()[entry_index];

    std::vector<PrioritizedRuleListOffset> rule_list_by_modifier;

    // Retrieve a mutable reference to |entry.second| and sort it in descending
    // order of priority.
    for (auto& [_, rule_list_with_priority] : build_data.map[entry.first]) {
      std::vector<RuleIdOffset> rule_list;
      std::sort(rule_list_with_priority.begin(), rule_list_with_priority.end(),
                priority_comparator);

      for (const auto& rule_with_priority : rule_list_with_priority)
        rule_list.push_back(rule_with_priority.first);
      auto rules_offset = builder->CreateVector(rule_list);
      rule_list_by_modifier.push_back(
          flat::CreatePrioritizedRuleList(*builder, rules_offset));
    }
    auto rule_list_by_modifier_offset =
        builder->CreateVector(rule_list_by_modifier);
    flat_map[i] = flat::CreateNGramToRules(*builder, entry.first,
                                           rule_list_by_modifier_offset);
  }

  auto ngram_index_offset = builder->CreateVector(flat_map);

  std::vector<PrioritizedRuleListOffset> fallback_list_by_modifier;
  for (auto& [_, fallback_list_with_priority] : build_data.fallback) {
    std::sort(fallback_list_with_priority.begin(),
              fallback_list_with_priority.end(), priority_comparator);

    std::vector<RuleIdOffset> fallback_list;
    for (const auto& fallback_with_priority : fallback_list_with_priority)
      fallback_list.push_back(fallback_with_priority.first);

    auto fallback_offset = builder->CreateVector(fallback_list);
    fallback_list_by_modifier.push_back(
        flat::CreatePrioritizedRuleList(*builder, fallback_offset));
  }

  auto fallback_list_by_modifier_offset =
      builder->CreateVector(fallback_list_by_modifier);

  return flat::CreateRulesMap(*builder, RulesIndex::kNGramSize,
                              ngram_index_offset, empty_slot_offset,
                              fallback_list_by_modifier_offset);
}

std::string DoSaveIndex(base::span<const uint8_t> data,
                        const base::FilePath& index_path) {
  // If there is no loaded rule source for this group, the directory may yet
  // have to be created.
  if (!base::CreateDirectoryAndGetError(index_path.DirName(), nullptr))
    return std::string();

  base::File output_file(
      index_path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!output_file.IsValid())
    return std::string();

  // Write the version header.
  std::string version_header = GetIndexVersionHeader();
  int version_header_size = static_cast<int>(version_header.size());
  if (output_file.WriteAtCurrentPos(
          version_header.data(), version_header_size) != version_header_size) {
    return std::string();
  }

  // Write the flatbuffer ruleset.
  if (!base::IsValueInRangeForNumericType<int>(data.size()))
    return std::string();
  int data_size = static_cast<int>(data.size());
  if (output_file.WriteAtCurrentPos(reinterpret_cast<const char*>(data.data()),
                                    data_size) != data_size) {
    return std::string();
  }

  return CalculateBufferChecksum(data);
}

void SaveIndex(std::unique_ptr<flatbuffers::FlatBufferBuilder> index_builder,
               const base::FilePath& index_path,
               IndexSavedCallback index_saved_callback) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(index_saved_callback),
                     DoSaveIndex(base::span(index_builder->GetBufferPointer(),
                                            index_builder->GetSize()),
                                 index_path)));
}

template <class T>
void AddRuleToContentInjectionRulesTreeNode(
    const T* rule,
    const RuleId& rule_id,
    bool already_included,
    const flat::DomainConstraintsNode* rule_domain_constraint_node,
    ContentInjectionRuleTreeNode& node) {
  auto& map = node.content.GetMap(rule);
  // The given rule is already excluded for this domain. This has priority over
  // any subdomain inclusion and implies any subdomain exclusion.
  auto existing_rule = map.find(rule);
  if (existing_rule != map.end()) {
    if (existing_rule->second.allow_for_domain) {
      return;
    }
    already_included = true;
  }

  if (rule_domain_constraint_node->type() ==
          flat::DomainConstraintNodeType_INCLUDED &&
      !already_included) {
    map.insert({rule, ContentInjectionRuleForDomain(rule_id, false)});
  }

  if (rule_domain_constraint_node->type() ==
      flat::DomainConstraintNodeType_EXCLUDED) {
    if (existing_rule != map.end()) {
      map.erase(existing_rule);
    }
    map.insert({rule, ContentInjectionRuleForDomain(rule_id, true)});

    CHECK(rule_domain_constraint_node->subdomains()->empty());
  }

  for (auto subdomain = rule_domain_constraint_node->subdomains()->begin();
       subdomain != rule_domain_constraint_node->subdomains()->end();
       subdomain++) {
    AddRuleToContentInjectionRulesTreeNode(
        rule, rule_id, already_included,
        rule->core()->domain_constraints()->nodes()->Get(
            *rule_domain_constraint_node->first_child_node_index() +
            (subdomain - rule_domain_constraint_node->subdomains()->begin())),
        node.subdomains[subdomain->c_str()]);
  }
}

template <class T>
void AddRuleToContentInjectionRulesIndex(const T* rule,
                                         const RuleId& rule_id,
                                         ContentInjectionRuleTreeRoot& root) {
  flat::ContentInjectionRuleType rule_type = RuleType<T>::value;

  const flat::DomainConstraintsTree* domain_constraints =
      rule->core()->domain_constraints();

  // Generic scriptlet injection rules should have been removed at parsing time.
  CHECK(rule_type != flat::ContentInjectionRuleType_SCRIPTLET_INJECTION ||
        domain_constraints->type() != flat::DomainConstraintNodeType_INCLUDED);

  {
    auto& map = root.content.GetMap(rule);
    auto existing_rule = map.find(rule);
    if (existing_rule != map.end() && existing_rule->second.allow_for_domain) {
      // Generic allow overrides everything
      return;
    }

    if (IsGeneric(*domain_constraints)) {
      bool allow =
          domain_constraints->type() == flat::DomainConstraintNodeType_EXCLUDED;
      // If we have two rules for the same body, the allow rule takes
      // precedence.
      if (existing_rule != map.end() && allow) {
        map.erase(existing_rule);
      }

      map.insert({rule, ContentInjectionRuleForDomain(rule_id, allow)});

      if (allow) {
        // Generic allow overrides everything
        return;
      }
    }

    AddRuleToContentInjectionRulesTreeNode(
        rule, rule_id, existing_rule != map.end(),
        domain_constraints->nodes()->Get(
            domain_constraints->hostnames_node_index()),
        root.hostname_tree);
    AddRuleToContentInjectionRulesTreeNode(
        rule, rule_id, existing_rule != map.end(),
        domain_constraints->nodes()->Get(
            domain_constraints->entities_node_index()),
        root.entity_tree);
  }

  if (domain_constraints->included_regexes()) {
    for (auto regex : *domain_constraints->included_regexes()) {
      auto& map = root.regexes[regex->c_str()].GetMap(rule);
      if (map.contains(rule)) {
        continue;
      }

      map.insert({rule, ContentInjectionRuleForDomain(rule_id, false)});
    }
  }

  if (domain_constraints->excluded_regexes()) {
    for (auto regex : *domain_constraints->excluded_regexes()) {
      auto& map = root.regexes[regex->c_str()].GetMap(rule);
      auto existing_rule = map.find(rule);
      if (existing_rule != map.end()) {
        if (existing_rule->second.allow_for_domain)
          continue;
        map.erase(existing_rule);
      }

      map.insert({rule, ContentInjectionRuleForDomain(rule_id, true)});
    }
  }
}

template <class T>
void AddRuleIdsToList(
    flatbuffers::FlatBufferBuilder* builder,
    const std::map<const T*,
                   ContentInjectionRuleForDomain,
                   ContentInjectionRuleBodyCompare>& ids_map,
    std::vector<flatbuffers::Offset<flat::ContentInjectionRuleForDomain>>&
        rules_for_domain) {
  for (const auto& [_, rule] : ids_map) {
    RuleIdOffset rule_id = flat::CreateRuleId(*builder, rule.rule_id.source_id,
                                              rule.rule_id.rule_nr);
    rules_for_domain.push_back(flat::CreateContentInjectionRuleForDomain(
        *builder, rule_id, RuleType<T>::value, rule.allow_for_domain));
  }
}

ContentInjectionRuleForDomainOffsets BuildFlatNodeContent(
    flatbuffers::FlatBufferBuilder* builder,
    const ContentInjectionRuleTreeNodeContent content) {
  std::vector<ContentInjectionRuleForDomainOffset> rules_for_domain;

  AddRuleIdsToList(builder, content.rule_from_cosmetic_rule_body,
                   rules_for_domain);
  AddRuleIdsToList(builder, content.rule_from_scriptlet_injection_rule_body,
                   rules_for_domain);
  return builder->CreateVector(rules_for_domain);
}

void AddNodeToFlatContentInjectionRuleTree(
    flatbuffers::FlatBufferBuilder* builder,
    const ContentInjectionRuleTreeNode& node,
    std::optional<size_t> first_child_node_index,
    ContentInjectionRuleTreeNodes& nodes) {
  std::vector<flatbuffers::Offset<flatbuffers::String>> subdomains;

  DCHECK(first_child_node_index || node.subdomains.empty());

  for (const auto& subdomain : node.subdomains) {
    subdomains.push_back(builder->CreateSharedString(subdomain.first));
  }

  auto subdomains_offset = builder->CreateVector(subdomains);

  nodes.push_back(flat::CreateContentInjectionRulesTreeNode(
      *builder, BuildFlatNodeContent(builder, node.content),
      first_child_node_index, subdomains_offset));
}

std::optional<size_t> AddNodeDescendantsToFlatContentInjectionRuleTree(
    flatbuffers::FlatBufferBuilder* builder,
    const ContentInjectionRuleTreeNode& node,
    ContentInjectionRuleTreeNodes& nodes) {
  if (node.subdomains.empty()) {
    return std::nullopt;
  }

  std::map<const ContentInjectionRuleTreeNode*, std::optional<size_t>>
      first_child_node_index_for_children;
  for (const auto& [_, child] : node.subdomains) {
    first_child_node_index_for_children.insert(
        {&child, AddNodeDescendantsToFlatContentInjectionRuleTree(
                     builder, child, nodes)});
  }

  size_t first_child_node_index = nodes.size();

  for (const auto& [_, child] : node.subdomains) {
    AddNodeToFlatContentInjectionRuleTree(
        builder, child, first_child_node_index_for_children.at(&child), nodes);
  }

  return first_child_node_index;
}

size_t BuildFlatContentInjectionRuleTree(
    flatbuffers::FlatBufferBuilder* builder,
    const ContentInjectionRuleTreeNode& node,
    ContentInjectionRuleTreeNodes& nodes) {
  std::optional<size_t> first_child_node_index =
      AddNodeDescendantsToFlatContentInjectionRuleTree(builder, node, nodes);
  size_t node_index = nodes.size();
  AddNodeToFlatContentInjectionRuleTree(builder, node, first_child_node_index,
                                        nodes);
  return node_index;
}

// The goal of this comparator is to provide some sort of order as fast as
// possible to make inserting into a map or set fast. We don't care about
// whether the order makes any logical sense. The various parts of the rules
// are compared in an order that loosely aims to check the items that are more
// likely to be different first.
struct RequestFilterRuleCompare {
  static std::weak_ordering CompareDomainConstraintsNode(
      const flat::DomainConstraintsTree* lhs_tree,
      const flat::DomainConstraintsNode* lhs,
      const flat::DomainConstraintsTree* rhs_tree,
      const flat::DomainConstraintsNode* rhs) {
    if (lhs->type() != rhs->type()) {
      return lhs->type() < rhs->type() ? std::weak_ordering::less
                                       : std::weak_ordering::greater;
    }

    if (!lhs->first_child_node_index() && !rhs->first_child_node_index()) {
      return std::weak_ordering::equivalent;
    }

    if (!lhs->first_child_node_index()) {
      return std::weak_ordering::less;
    }

    if (!rhs->first_child_node_index()) {
      return std::weak_ordering::greater;
    }

    if (*lhs->first_child_node_index() != *rhs->first_child_node_index()) {
      return *lhs->first_child_node_index() < *rhs->first_child_node_index()
                 ? std::weak_ordering::less
                 : std::weak_ordering::greater;
    }

    auto subdomains_ordering =
        FastCompareFlatStringVector(lhs->subdomains(), rhs->subdomains());

    if (subdomains_ordering != 0) {
      return subdomains_ordering;
    }

    for (size_t i = 0; i < lhs->subdomains()->size(); i++) {
      auto child_ordering = CompareDomainConstraintsNode(
          lhs_tree, lhs_tree->nodes()->Get(*lhs->first_child_node_index() + i),
          rhs_tree, rhs_tree->nodes()->Get(*rhs->first_child_node_index() + i));

      if (child_ordering != 0) {
        return child_ordering;
      }
    }

    return std::weak_ordering::equivalent;
  }

  static std::weak_ordering CompareDomainConstraintsTree(
      const flat::DomainConstraintsTree* lhs,
      const flat::DomainConstraintsTree* rhs) {
    if (lhs->nodes()->size() < rhs->nodes()->size())
      return std::weak_ordering::less;
    if (lhs->nodes()->size() > rhs->nodes()->size())
      return std::weak_ordering::greater;

    auto included_regexes_ordering = FastCompareFlatStringVector(
        lhs->included_regexes(), rhs->included_regexes());
    if (included_regexes_ordering != 0) {
      return included_regexes_ordering;
    }

    auto excluded_regexes_ordering = FastCompareFlatStringVector(
        lhs->excluded_regexes(), rhs->excluded_regexes());
    if (excluded_regexes_ordering != 0) {
      return excluded_regexes_ordering;
    }

    auto hostnames_ordering = CompareDomainConstraintsNode(
        lhs, lhs->nodes()->Get(lhs->hostnames_node_index()), rhs,
        rhs->nodes()->Get(rhs->hostnames_node_index()));
    if (hostnames_ordering != 0) {
      return hostnames_ordering;
    }

    auto entities_ordering = CompareDomainConstraintsNode(
        lhs, lhs->nodes()->Get(lhs->entities_node_index()), rhs,
        rhs->nodes()->Get(rhs->entities_node_index()));
    if (entities_ordering != 0) {
      return entities_ordering;
    }
    if (lhs->type() != rhs->type()) {
      return lhs->type() < rhs->type() ? std::weak_ordering::less
                                       : std::weak_ordering::greater;
    }

    if (lhs->has_exclusions() == rhs->has_exclusions()) {
      return std::weak_ordering::equivalent;
    }
    return lhs->has_exclusions() ? std::weak_ordering::less
                                 : std::weak_ordering::greater;
  }

  bool operator()(const flat::RequestFilterRule* lhs,
                  const flat::RequestFilterRule* rhs) const {
    return FastCompareFlatString(lhs->pattern(), rhs->pattern()) < 0 ||
           FastCompareFlatString(lhs->host(), rhs->host()) < 0 ||
           lhs->anchor_type() < rhs->anchor_type() ||
           CompareDomainConstraintsTree(lhs->from_domain_constraints(),
                                        rhs->from_domain_constraints()) < 0 ||
           lhs->decision() < rhs->decision() ||
           lhs->options() < rhs->options() || lhs->party() < rhs->party() ||
           lhs->resource_types() < rhs->resource_types() ||
           lhs->activation_types() < rhs->activation_types() ||
           lhs->pattern_type() < rhs->pattern_type() ||
           lhs->modifier() < rhs->modifier() ||
           FastCompareFlatStringVector(lhs->modifier_values(),
                                       rhs->modifier_values()) < 0 ||
           FastCompareFlatStringVector(lhs->ad_domains_and_query_triggers(),
                                       rhs->ad_domains_and_query_triggers()) <
               0;
  }
};
}  // namespace

void BuildAndSaveIndex(
    const std::map<uint32_t, std::unique_ptr<RuleBufferHolder>>& rules_buffers,
    base::SequencedTaskRunner* file_task_runner,
    const base::FilePath& index_path,
    IndexSavedCallback index_saved_callback) {
  SourceChecksums source_checksums;

  IndexBuildData activation_rules;
  IndexBuildData before_request;
  IndexBuildData modify_blocked_request;
  IndexBuildData modify_allowed_request;
  IndexBuildData headers_received;

  std::unique_ptr<flatbuffers::FlatBufferBuilder> builder =
      std::make_unique<flatbuffers::FlatBufferBuilder>();

  std::set<const flat::RequestFilterRule*, RequestFilterRuleCompare> seen_rules;

  // Generic cosmetic block rules that are not cancelled by any other rule on
  // any domain.
  std::map<const flat::CosmeticRule*, RuleId, ContentInjectionRuleBodyCompare>
      default_cosmetic_block_rules;
  // List of all rules with selectors that are potentilly unblocked on some
  // domains, used to build |default_cosmetic_block_rules|.
  std::set<const flat::CosmeticRule*, ContentInjectionRuleBodyCompare>
      cosmetic_allow_selectors;
  ContentInjectionRuleTreeRoot content_injection_rules_tree_root;

  for (const auto& [source_id, rules_buffer] : rules_buffers) {
    source_checksums.push_back(flat::CreateSourceChecksum(
        *builder, source_id, builder->CreateString(rules_buffer->checksum())));
    for (flatbuffers::uoffset_t i = 0;
         i <
         rules_buffer->rules_list()->bad_request_filter_rules_list()->size();
         i++) {
      const flat::RequestFilterRule* rule =
          rules_buffer->rules_list()->bad_request_filter_rules_list()->Get(i);
      // Just inserting the bad rules here will prevent actual rules from being
      // indexed, thanks to the deduplication check below.
      seen_rules.insert(rule);
    }

    for (flatbuffers::uoffset_t i = 0;
         i < rules_buffer->rules_list()->request_filter_rules_list()->size();
         i++) {
      RuleIdOffset rule_id = flat::CreateRuleId(*builder, source_id, i);
      const flat::RequestFilterRule* rule =
          rules_buffer->rules_list()->request_filter_rules_list()->Get(i);

      if (seen_rules.contains(rule)) {
        continue;
      }

      seen_rules.insert(rule);

      DCHECK((rule->modifier() != flat::Modifier_NO_MODIFIER) ||
             rule->activation_types() != 0 ||
             (rule->options() & flat::OptionFlag_MODIFY_BLOCK));

      if (rule->activation_types() != 0)
        AddRuleToMap(*rule, rule_id, true, activation_rules);

      if (rule->options() & flat::OptionFlag_MODIFY_BLOCK) {
        CHECK(rule->resource_types() != 0);
        AddRuleToMap(*rule, rule_id, true, before_request);
      }

      switch (rule->modifier()) {
        case flat::Modifier_NO_MODIFIER:
          break;

        case flat::Modifier_CSP:
          AddRuleToMap(*rule, rule_id, false, headers_received);
          break;

        case flat::Modifier_REDIRECT:
          AddRuleToMap(*rule, rule_id, false, modify_blocked_request);
          break;

        case flat::Modifier_AD_QUERY_TRIGGER:
          AddRuleToMap(*rule, rule_id, false, modify_allowed_request);
      }
    }

    for (flatbuffers::uoffset_t i = 0;
         i < rules_buffer->rules_list()->cosmetic_rules_list()->size(); i++) {
      RuleId rule_id(source_id, i);
      const auto* rule =
          rules_buffer->rules_list()->cosmetic_rules_list()->Get(i);

      const flat::DomainConstraintsTree* domain_constraints =
          rule->core()->domain_constraints();

      // Pure generic block rules that are not matched by any exclusion are
      // placed in the default list
      if (!domain_constraints->has_exclusions() &&
          domain_constraints->type() ==
              flat::DomainConstraintNodeType_INCLUDED &&
          !cosmetic_allow_selectors.count(rule)) {
        default_cosmetic_block_rules.insert({rule, rule_id});
        continue;
      }
      AddRuleToContentInjectionRulesIndex(rule, rule_id,
                                          content_injection_rules_tree_root);

      // If a rule was earlier placed in the default list, but an allow rule now
      // matches it, remove it from the list and add it to the tree. We do this
      // after adding the allow rule because currently, generic allow rules only
      // trigger pruning of matching block rules that are added after
      // themselves.
      if (domain_constraints->has_exclusions()) {
        auto matching_block = default_cosmetic_block_rules.find(rule);
        if (matching_block != default_cosmetic_block_rules.end()) {
          AddRuleToContentInjectionRulesIndex(
              matching_block->first, matching_block->second,
              content_injection_rules_tree_root);
          default_cosmetic_block_rules.erase(matching_block);
        }
        cosmetic_allow_selectors.insert(rule);
      }
    }

    for (flatbuffers::uoffset_t i = 0;
         i <
         rules_buffer->rules_list()->scriptlet_injection_rules_list()->size();
         i++) {
      RuleId rule_id(source_id, i);
      const auto* rule =
          rules_buffer->rules_list()->scriptlet_injection_rules_list()->Get(i);
      AddRuleToContentInjectionRulesIndex(rule, rule_id,
                                          content_injection_rules_tree_root);
    }
  }

  auto source_checksums_offset =
      builder->CreateVectorOfSortedTables(&source_checksums);
  RulesMapOffset activation_rules_map_offset =
      BuildFlatMap(builder.get(), activation_rules);
  RulesMapOffset before_request_map_offset =
      BuildFlatMap(builder.get(), before_request);
  RulesMapOffset modify_blocked_request_map_offset =
      BuildFlatMap(builder.get(), modify_blocked_request);
  RulesMapOffset modify_allowed_request_map_offset =
      BuildFlatMap(builder.get(), modify_allowed_request);
  RulesMapOffset headers_received_map_offset =
      BuildFlatMap(builder.get(), headers_received);

  auto default_stylesheet_offset =
      builder->CreateString(BuildStyleSheet(default_cosmetic_block_rules));

  ContentInjectionRuleTreeNodes flat_content_injection_rules_tree_nodes;
  size_t hostnames_index = BuildFlatContentInjectionRuleTree(
      builder.get(), content_injection_rules_tree_root.hostname_tree,
      flat_content_injection_rules_tree_nodes);
  size_t entities_index = BuildFlatContentInjectionRuleTree(
      builder.get(), content_injection_rules_tree_root.entity_tree,
      flat_content_injection_rules_tree_nodes);
  CHECK(flat_content_injection_rules_tree_nodes.size() > 0);
  auto root_content_offset = BuildFlatNodeContent(
      builder.get(), content_injection_rules_tree_root.content);
  auto flat_content_injection_rule_nodes_offset =
      builder->CreateVector(flat_content_injection_rules_tree_nodes);
  std::vector<flatbuffers::Offset<flat::ContentInjectionRuleRegex>>
      flat_content_injection_rule_regexes;
  for (const auto& [regex, content] :
       content_injection_rules_tree_root.regexes) {
    flat_content_injection_rule_regexes.push_back(
        flat::CreateContentInjectionRuleRegex(
            *builder, builder->CreateSharedString(regex),
            BuildFlatNodeContent(builder.get(), content)));
  }

  auto flat_content_injection_rules_tree =
      flat::CreateContentInjectionRulesTreeRoot(
          *builder, hostnames_index, entities_index, root_content_offset,
          flat_content_injection_rule_nodes_offset,
          builder->CreateVector(flat_content_injection_rule_regexes));

  auto rule_index_offset = flat::CreateRulesIndex(
      *builder, source_checksums_offset, activation_rules_map_offset,
      before_request_map_offset, modify_blocked_request_map_offset,
      modify_allowed_request_map_offset, headers_received_map_offset,
      default_stylesheet_offset, flat_content_injection_rules_tree);

  flat::FinishRulesIndexBuffer(*builder, rule_index_offset);

  file_task_runner->PostTask(
      FROM_HERE, base::BindOnce(&SaveIndex, std::move(builder), index_path,
                                std::move(index_saved_callback)));
}
}  // namespace adblock_filter
