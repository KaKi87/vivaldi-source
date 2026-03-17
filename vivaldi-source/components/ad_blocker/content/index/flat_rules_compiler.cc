// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/content/index/flat_rules_compiler.h"

#include <map>

#include "base/files/file_util.h"
#include "components/ad_blocker/content/index/index_utils.h"
#include "components/ad_blocker/core/adblock_request_filter_rule.h"
#include "components/ad_blocker/public/core/adblock_request_filter_rule_types.h"
#include "vivaldi/components/ad_blocker/content/index/flat/adblock_rules_list_generated.h"

namespace adblock_filter {

namespace {
template <typename T>
using FlatOffset = flatbuffers::Offset<T>;

template <typename T>
using FlatVectorOffset = FlatOffset<flatbuffers::Vector<FlatOffset<T>>>;

using FlatStringListOffset = FlatVectorOffset<flatbuffers::String>;

using DomainConstraintsNodeOffset = FlatOffset<flat::DomainConstraintsNode>;
using DomainConstraintsTreeNodes = std::vector<DomainConstraintsNodeOffset>;

struct OffsetVectorCompare {
  bool operator()(const std::vector<FlatStringOffset>& a,
                  const std::vector<FlatStringOffset>& b) const {
    auto compare = [](const FlatStringOffset a_offset,
                      const FlatStringOffset b_offset) {
      DCHECK(!a_offset.IsNull());
      DCHECK(!b_offset.IsNull());
      return a_offset.o < b_offset.o;
    };
    // |lexicographical_compare| is how vector::operator< is implemented.
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                        compare);
  }
};
using FlatStringOffsetMap = std::map<std::vector<FlatStringOffset>,
                                     FlatStringListOffset,
                                     OffsetVectorCompare>;

template <typename T>
FlatStringListOffset SerializeStringList(
    flatbuffers::FlatBufferBuilder* builder,
    const T& container,
    FlatStringOffsetMap* string_offset_map) {
  if (container.empty())
    return FlatStringListOffset();

  std::vector<FlatStringOffset> strings;
  strings.reserve(container.size());
  for (const std::string& str : container)
    strings.push_back(builder->CreateSharedString(str));

  auto precedes = [&builder](FlatStringOffset lhs, FlatStringOffset rhs) {
    return SizePrioritizedStringCompare(
               ToStringPiece(flatbuffers::GetTemporaryPointer(*builder, lhs)),
               ToStringPiece(flatbuffers::GetTemporaryPointer(*builder, rhs))) <
           0;
  };
  if (strings.empty())
    return FlatStringListOffset();
  std::sort(strings.begin(), strings.end(), precedes);

  // Share string lists if we've already serialized an exact duplicate. Note
  // that this can share excluded and included domain lists, and modifier lists.
  DCHECK(string_offset_map);
  auto it = string_offset_map->find(strings);
  if (it == string_offset_map->end()) {
    auto offset = builder->CreateVector(strings);
    (*string_offset_map)[strings] = offset;
    return offset;
  }
  return it->second;
}

flat::DomainConstraintNodeType ToFlatDomainConstraintTreeNodeType(
    const DomainConstraintsTree::Node::NodeType& type) {
  switch (type) {
    case DomainConstraintsTree::Node::kNone:
      return flat::DomainConstraintNodeType_NONE;
    case DomainConstraintsTree::Node::kIncluded:
      return flat::DomainConstraintNodeType_INCLUDED;
    case DomainConstraintsTree::Node::kExcluded:
      return flat::DomainConstraintNodeType_EXCLUDED;
  }
}

void AddNodeToFlatDomainConstraintsTree(
    flatbuffers::FlatBufferBuilder* builder,
    const DomainConstraintsTree::Node& node,
    std::optional<size_t> first_child_node_index,
    DomainConstraintsTreeNodes& nodes) {
  std::vector<flatbuffers::Offset<flatbuffers::String>> subdomains;

  DCHECK(first_child_node_index || node.subdomains().empty());

  for (const auto& subdomain : node.subdomains()) {
    subdomains.push_back(builder->CreateSharedString(subdomain.first));
  }

  auto subdomains_offset = builder->CreateVector(subdomains);

  nodes.push_back(flat::CreateDomainConstraintsNode(
      *builder, first_child_node_index,
      ToFlatDomainConstraintTreeNodeType(node.GetNodeType()),
      subdomains_offset));
}

std::optional<size_t> AddNodeDescendantsToFlatDomainConstraintsTree(
    flatbuffers::FlatBufferBuilder* builder,
    const DomainConstraintsTree::Node& node,
    DomainConstraintsTreeNodes& nodes) {
  if (node.subdomains().empty()) {
    return std::nullopt;
  }

  std::map<const DomainConstraintsTree::Node*, std::optional<size_t>>
      first_child_node_index_for_children;
  for (auto& [label, child] : node.subdomains()) {
    first_child_node_index_for_children.insert(
        {&child,
         AddNodeDescendantsToFlatDomainConstraintsTree(builder, child, nodes)});
  }

  // We add children to nodes below. The index of the first added child will
  // be equal to the size of the vector, before that nodes gets added. Store
  // that index here to return it afterwards, since the size changes with the
  // added nodes.
  size_t first_child_node_index = nodes.size();

  for (auto& [label, child] : node.subdomains()) {
    AddNodeToFlatDomainConstraintsTree(
        builder, child, first_child_node_index_for_children.at(&child), nodes);
  }

  return first_child_node_index;
}

FlatOffset<flat::DomainConstraintsTree> DoSerializeDomainConstraintsTree(
    flatbuffers::FlatBufferBuilder* builder,
    const DomainConstraintsTree& tree,
    FlatStringOffsetMap* string_offset_map) {
  DomainConstraintsTreeNodes nodes;
  std::optional<size_t> first_hostname_child_node_index =
      AddNodeDescendantsToFlatDomainConstraintsTree(builder, tree.hostnames(),
                                                    nodes);

  size_t hostnames_index = nodes.size();
  AddNodeToFlatDomainConstraintsTree(builder, tree.hostnames(),
                                     first_hostname_child_node_index, nodes);

  std::optional<size_t> first_entity_child_node_index =
      AddNodeDescendantsToFlatDomainConstraintsTree(builder, tree.entities(),
                                                    nodes);

  size_t entities_index = nodes.size();
  AddNodeToFlatDomainConstraintsTree(builder, tree.entities(),
                                     first_entity_child_node_index, nodes);
  auto nodes_offset = builder->CreateVector(nodes);
  FlatStringListOffset included_regexes_offset =
      SerializeStringList(builder, tree.included_regexes(), string_offset_map);
  FlatStringListOffset excluded_regexes_offset =
      SerializeStringList(builder, tree.excluded_regexes(), string_offset_map);

  return flat::CreateDomainConstraintsTree(
      *builder, ToFlatDomainConstraintTreeNodeType(tree.GetRootNodeType()),
      hostnames_index, entities_index, nodes_offset, included_regexes_offset,
      excluded_regexes_offset, tree.HasExclusions());
}

FlatOffset<flat::DomainConstraintsTree> SerializeDomainConstraintsTree(
    flatbuffers::FlatBufferBuilder* builder,
    const DomainConstraintsTree& tree,
    FlatStringOffsetMap* string_offset_map) {
  if (tree.GetRootNodeType() == DomainConstraintsTree::Node::kExcluded) {
    // Serializing a generic exclude tree means all rules it contains are
    // overruled by the top-level rule. Serialize an empty tree instead.
    return DoSerializeDomainConstraintsTree(
        builder, DomainConstraintsTree(true), string_offset_map);
  }

  return DoSerializeDomainConstraintsTree(builder, tree, string_offset_map);
}

uint8_t OptionsFromRequestFilterRule(const RequestFilterRule& rule) {
  uint8_t options = 0;
  if (rule.modify_block)
    options |= flat::OptionFlag_MODIFY_BLOCK;
  if (rule.is_case_sensitive)
    options |= flat::OptionFlag_IS_CASE_SENSITIVE;
  return options;
}

uint16_t MethodFromRequestFilterRule(const RequestFilterRule& rule) {
  uint16_t methods = 0;
  for (RequestMethod method : rule.request_methods) {
    methods |= ([](RequestMethod method) {
      switch (method) {
        case RequestMethod::KConnect:
          return flat::Method_CONNECT;
        case RequestMethod::kDelete:
          return flat::Method_DELETE;
        case RequestMethod::kGet:
          return flat::Method_GET;
        case RequestMethod::kHead:
          return flat::Method_HEAD;
        case RequestMethod::kOptions:
          return flat::Method_OPTIONS;
        case RequestMethod::kPatch:
          return flat::Method_PATCH;
        case RequestMethod::kPost:
          return flat::Method_POST;
        case RequestMethod::kPut:
          return flat::Method_PUT;
        case RequestMethod::kOther:
          return flat::Method::Method_OTHER;
      }
    })(method);
  }

  return methods;
}

uint32_t ResourceTypesFromRequestFilterRule(const RequestFilterRule& rule) {
  uint32_t resource_types = 0;
  for (ResourceType type : rule.resource_types) {
    resource_types |= ToFlatResourceType(type);
  }
  for (ResourceType type : rule.explicit_types) {
    resource_types |= ToFlatResourceType(type);
  }

  return resource_types;
}

flat::Decision DecisionFromRequestFilterRule(const RequestFilterRule& rule) {
  switch (rule.decision) {
    case RuleDecision::kModify:
      return flat::Decision_MODIFY;
    case RuleDecision::kPass:
      return flat::Decision_PASS;
    case RuleDecision::kModifyImportant:
      return flat::Decision_MODIFY_IMPORTANT;
  }
}

flat::Modifier ModifierFromRequestFilterModifier(
    const RequestFilterRule& rule) {
  switch (rule.modifier) {
    case ModifierType::kNoModifier:
      return flat::Modifier_NO_MODIFIER;
    case ModifierType::kRedirect:
      return flat::Modifier_REDIRECT;
    case ModifierType::kCsp:
      return flat::Modifier_CSP;
    case ModifierType::kAdQueryTrigger:
      return flat::Modifier_AD_QUERY_TRIGGER;
  }
}

flat::Party PartyFromRequestFilterParty(const RequestFilterRule& rule) {
  if (!rule.party) {
    return flat::Party_ALL;
  }
  switch (*rule.party) {
    case Party::kFirst:
      return flat::Party_FIRST;
    case Party::kThird:
      return flat::Party_THIRD;
    case Party::kStrictFirst:
      return flat::Party_STRICT_FIRST;
    case Party::kStrictThird:
      return flat::Party_STRICT_THIRD;
    case Party::kFirstAndStrictThird:
      return flat::Party_FIRST_AND_STRICT_THIRD;
  }
}

uint8_t ActivationTypesFromRequestFilterRule(const RequestFilterRule& rule) {
  uint8_t activation_types = 0;
  if (rule.activation_types.Has(ActivationType::kWholeDocument))
    activation_types |= flat::ActivationType_DOCUMENT;
  if (rule.activation_types.Has(ActivationType::kSpecificHide))
    activation_types |= flat::ActivationType_SPECIFIC_HIDE;
  if (rule.activation_types.Has(ActivationType::kGenericHide))
    activation_types |= flat::ActivationType_GENERIC_HIDE;
  if (rule.activation_types.Has(ActivationType::kGenericBlock))
    activation_types |= flat::ActivationType_GENERIC_BLOCK;
  if (rule.activation_types.Has(ActivationType::kAttributeAds))
    activation_types |= flat::ActivationType_ATTRIBUTE_ADS;
  return activation_types;
}

flat::PatternType PatternTypeFromRequestFilterRule(
    const RequestFilterRule& rule) {
  switch (rule.pattern_type) {
    case RequestFilterRule::kPlain:
      return flat::PatternType_PLAIN;
    case RequestFilterRule::kWildcarded:
      return flat::PatternType_WILDCARDED;
    case RequestFilterRule::kRegex:
      return flat::PatternType_REGEXP;
  }
}

uint8_t AnchorTypeFromRequestFilterRule(const RequestFilterRule& rule) {
  uint8_t anchor_type = 0;
  if (rule.anchor_type.test(RequestFilterRule::kAnchorStart))
    anchor_type |= flat::AnchorType_START;
  if (rule.anchor_type.test(RequestFilterRule::kAnchorEnd))
    anchor_type |= flat::AnchorType_END;
  if (rule.anchor_type.test(RequestFilterRule::kAnchorHost))
    anchor_type |= flat::AnchorType_HOST;
  return anchor_type;
}

FlatStringOffset StringOffsetFromOptionalString(
    flatbuffers::FlatBufferBuilder* builder,
    const std::optional<std::string>& string) {
  if (!string) {
    return FlatStringOffset();
  }
  return builder->CreateSharedString(*string);
}

void AddRuleToBuffer(
    flatbuffers::FlatBufferBuilder* builder,
    const RequestFilterRule& rule,
    std::vector<FlatOffset<flat::RequestFilterRule>>* rules_offsets,
    std::vector<FlatOffset<flat::RequestFilterRule>>* bad_rules_offsets,
    FlatStringOffsetMap* string_offset_map) {
  FlatOffset<flat::DomainConstraintsTree> domains_constraints_offset =
      SerializeDomainConstraintsTree(builder, rule.from_domain_constraints,
                                     string_offset_map);

  FlatStringOffset pattern_offset = builder->CreateSharedString(rule.pattern);

  FlatStringOffset ngram_search_string_offset =
      StringOffsetFromOptionalString(builder, rule.ngram_search_string);

  FlatStringListOffset ad_domains_and_query_triggers = SerializeStringList(
      builder, rule.ad_domains_and_query_triggers, string_offset_map);

  FlatStringOffset host_offset =
      StringOffsetFromOptionalString(builder, rule.host);
  FlatStringListOffset modifier_value_offset =
      SerializeStringList(builder, rule.modifier_values, string_offset_map);

  FlatStringOffset original_rule_text =
      builder->CreateSharedString(rule.original_rule_text);

  auto rule_offset = flat::CreateRequestFilterRule(
      *builder, DecisionFromRequestFilterRule(rule),
      OptionsFromRequestFilterRule(rule), PartyFromRequestFilterParty(rule),
      MethodFromRequestFilterRule(rule),
      ResourceTypesFromRequestFilterRule(rule),
      ActivationTypesFromRequestFilterRule(rule),
      PatternTypeFromRequestFilterRule(rule),
      AnchorTypeFromRequestFilterRule(rule), host_offset,
      ad_domains_and_query_triggers, domains_constraints_offset,
      ModifierFromRequestFilterModifier(rule), modifier_value_offset,
      pattern_offset, ngram_search_string_offset, original_rule_text);

  if (rule.bad_filter) {
    bad_rules_offsets->push_back(rule_offset);
  } else {
    rules_offsets->push_back(rule_offset);
  }
}

FlatOffset<flat::ContentInjectionRuleCore> AddContentInjectionRuleCoreToBuffer(
    flatbuffers::FlatBufferBuilder* builder,
    const ContentInjectionRuleCore& core,
    FlatStringOffsetMap* string_offset_map) {
  FlatOffset<flat::DomainConstraintsTree> domains_constraints_offset =
      SerializeDomainConstraintsTree(builder, core.domain_constraints,
                                     string_offset_map);
  return flat::CreateContentInjectionRuleCore(*builder,
                                              domains_constraints_offset);
}

void AddRuleToBuffer(flatbuffers::FlatBufferBuilder* builder,
                     const CosmeticRule& rule,
                     std::vector<FlatOffset<flat::CosmeticRule>>* rules_offsets,
                     FlatStringOffsetMap* string_offset_map) {
  FlatStringOffset selector_offset = builder->CreateSharedString(rule.selector);
  FlatOffset<flat::ContentInjectionRuleCore> rule_core_offset =
      AddContentInjectionRuleCoreToBuffer(builder, rule.core,
                                          string_offset_map);
  rules_offsets->push_back(
      flat::CreateCosmeticRule(*builder, rule_core_offset, selector_offset));
}

void AddRuleToBuffer(
    flatbuffers::FlatBufferBuilder* builder,
    const ScriptletInjectionRule& rule,
    std::vector<FlatOffset<flat::ScriptletInjectionRule>>* rules_offsets,
    FlatStringOffsetMap* string_offset_map) {
  std::vector<FlatOffset<flat::Scriptlet>> scriptlets;
  for (const auto& scriptlet : rule.scriptlets) {
    FlatStringOffset scriptlet_name_offset =
        builder->CreateSharedString(scriptlet.name);
    std::vector<FlatStringOffset> argument_offsets;
    for (const auto& argument : scriptlet.arguments) {
      argument_offsets.push_back(builder->CreateSharedString(argument));
    }
    scriptlets.push_back(
        flat::CreateScriptlet(*builder, scriptlet_name_offset,
                              builder->CreateVector(argument_offsets)));
  }
  FlatOffset<flat::ContentInjectionRuleCore> rule_core_offset =
      AddContentInjectionRuleCoreToBuffer(builder, rule.core,
                                          string_offset_map);
  rules_offsets->push_back(flat::CreateScriptletInjectionRule(
      *builder, rule_core_offset, builder->CreateVector(scriptlets)));
}

bool SaveRulesList(const base::FilePath& output_path,
                   base::span<const uint8_t> data,
                   std::string& checksum) {
  if (!base::CreateDirectoryAndGetError(output_path.DirName(), nullptr))
    return false;

  base::File output_file(
      output_path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!output_file.IsValid())
    return false;

  // Write the version header.
  std::string version_header = GetRulesListVersionHeader();
  int version_header_size = static_cast<int>(version_header.size());
  if (output_file.WriteAtCurrentPos(
          version_header.data(), version_header_size) != version_header_size) {
    return false;
  }

  // Write the flatbuffer ruleset.
  if (!base::IsValueInRangeForNumericType<int>(data.size()))
    return false;
  int data_size = static_cast<int>(data.size());
  if (output_file.WriteAtCurrentPos(reinterpret_cast<const char*>(data.data()),
                                    data_size) != data_size) {
    return false;
  }

  checksum = CalculateBufferChecksum(data);

  return true;
}
}  // namespace

bool CompileFlatRules(const ParseResult& parse_result,
                      const RuleSourceSettings& source_settings,
                      const base::FilePath& output_path,
                      std::string& checksum) {
  flatbuffers::FlatBufferBuilder builder;
  std::vector<FlatOffset<flat::RequestFilterRule>> request_filter_rules_offsets;
  std::vector<FlatOffset<flat::RequestFilterRule>>
      bad_request_filter_rules_offsets;
  FlatStringOffsetMap string_offset_map;
  for (const auto& request_filter_rule : parse_result.request_filter_rules) {
    AddRuleToBuffer(&builder, request_filter_rule,
                    &request_filter_rules_offsets,
                    &bad_request_filter_rules_offsets, &string_offset_map);
  }
  std::vector<FlatOffset<flat::CosmeticRule>> cosmetic_rules_offsets;
  for (const auto& cosmetic_rule : parse_result.cosmetic_rules) {
    AddRuleToBuffer(&builder, cosmetic_rule, &cosmetic_rules_offsets,
                    &string_offset_map);
  }

  std::vector<FlatOffset<flat::ScriptletInjectionRule>>
      scriptlet_injection_rules_offsets;
  for (const auto& scriptlet_injection_rule :
       parse_result.scriptlet_injection_rules) {
    AddRuleToBuffer(&builder, scriptlet_injection_rule,
                    &scriptlet_injection_rules_offsets, &string_offset_map);
  }

  FlatOffset<flat::RulesList> root_offset = flat::CreateRulesList(
      builder, builder.CreateVector(request_filter_rules_offsets),
      builder.CreateVector(bad_request_filter_rules_offsets),
      builder.CreateVector(cosmetic_rules_offsets),
      builder.CreateVector(scriptlet_injection_rules_offsets));

  flat::FinishRulesListBuffer(builder, root_offset);

  return SaveRulesList(
      output_path, base::span(builder.GetBufferPointer(), builder.GetSize()),
      checksum);
}
}  // namespace adblock_filter
