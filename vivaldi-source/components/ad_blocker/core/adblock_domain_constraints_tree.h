// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_AD_BLOCKER_CORE_ADBLOCK_DOMAIN_CONSTRAINTS_TREE_H_
#define COMPONENTS_AD_BLOCKER_CORE_ADBLOCK_DOMAIN_CONSTRAINTS_TREE_H_

#include <string>
#include <string_view>
#include <vector>

#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace adblock_filter {
class DomainConstraintsTree {
 public:
  struct NormalizedConstraint {
    enum { kHostname, kEntity, kRegex } kind;
    bool excluded;
    std::string constraint;
  };

  class Node {
   public:
    enum NodeType { kNone = 0, kIncluded, kExcluded };
    Node();
    virtual ~Node();

    Node(Node&&);
    Node& operator=(Node&&);

    bool operator==(const Node&) const;

    virtual void AddSubDomains(std::vector<std::string_view> labels,
                               bool excluded);

    const std::map<std::string, Node>& subdomains() const {
      return subdomains_;
    }
    virtual NodeType GetNodeType() const;

    void PrintDescendants(std::ostream&, int indent, int level) const;

   private:
    std::map<std::string, Node> subdomains_;
    NodeType node_type_ = kNone;
  };

  static std::optional<DomainConstraintsTree> Create(
      bool invert,
      std::string_view constraints,
      char separator,
      bool allow_entity,
      bool allow_regex);

  DomainConstraintsTree(bool invert);
  ~DomainConstraintsTree();

  DomainConstraintsTree(DomainConstraintsTree&&);
  DomainConstraintsTree& operator=(DomainConstraintsTree&&);

  bool operator==(const DomainConstraintsTree&) const;

  void Add(const NormalizedConstraint& constraint);

  bool IsGeneric() const;

  bool HasExclusions() const;

  Node::NodeType GetRootNodeType() const;
  const Node& entities() const { return entities_; }
  const Node& hostnames() const { return hostnames_; }

  const absl::flat_hash_set<std::string>& included_regexes() const {
    return included_regexes_;
  }
  const absl::flat_hash_set<std::string>& excluded_regexes() const {
    return excluded_regexes_;
  }

  // Makes the next call (and only the next call) to Print use an indent.
  void set_print_indent_(int print_indent) const {
    print_indent_ = print_indent;
  }
  void Print(std::ostream&) const;

 private:
  bool inverted_ = false;
  bool has_exclusions_ = false;
  mutable int print_indent_ = 0;
  Node::NodeType root_type_;
  Node entities_;
  Node hostnames_;
  absl::flat_hash_set<std::string> included_regexes_;
  absl::flat_hash_set<std::string> excluded_regexes_;
};

std::ostream& operator<<(std::ostream&, const DomainConstraintsTree&);
}  // namespace adblock_filter

#endif  // COMPONENTS_AD_BLOCKER_CORE_ADBLOCK_DOMAIN_CONSTRAINTS_TREE_H_
