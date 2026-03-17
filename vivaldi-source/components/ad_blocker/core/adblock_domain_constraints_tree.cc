// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/core/adblock_domain_constraints_tree.h"

#include <iomanip>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace adblock_filter {

namespace {
std::ostream& operator<<(std::ostream& os,
                         DomainConstraintsTree::Node::NodeType type) {
  switch (type) {
    case DomainConstraintsTree::Node::kNone:
      return os << ' ';
    case DomainConstraintsTree::Node::kExcluded:
      return os << 'X';
    case DomainConstraintsTree::Node::kIncluded:
      return os << 'I';
  }
}
}  // namespace

DomainConstraintsTree::Node::Node() = default;
DomainConstraintsTree::Node::~Node() = default;
DomainConstraintsTree::Node::Node(Node&&) = default;
DomainConstraintsTree::Node& DomainConstraintsTree::Node::operator=(Node&&) =
    default;

DomainConstraintsTree::Node::NodeType DomainConstraintsTree::Node::GetNodeType()
    const {
  return node_type_;
}

void DomainConstraintsTree::Node::AddSubDomains(
    std::vector<std::string_view> labels,
    bool excluded) {
  if (node_type_ == kExcluded) {
    // uBlock considers any match on an excluded subdomain to take precedence,
    // even if an included deeper subdomain is specified.
    return;
  }

  if (labels.empty()) {
    node_type_ = excluded ? kExcluded : kIncluded;
    if (excluded) {
      // uBlock considers any match on an excluded subdomain to take precedence,
      // even if an included deeper subdomain is specified.
      subdomains_.clear();
    }
    return;
  }

  if (node_type_ == kIncluded) {
    if (!excluded) {
      // No need to propagate further. Either the existing inclusion will cover
      // the subdomain, or an exclusion further down the tree will prevent it
      // from being added anyway (see above)
      return;
    }
  }

  std::string subomain(labels.back());
  CHECK(!subomain.empty());  // Normalization should prevent this being empty.
  labels.pop_back();
  subdomains_[subomain].AddSubDomains(std::move(labels), excluded);
}

void DomainConstraintsTree::Node::PrintDescendants(std::ostream& os,
                                                   int indent,
                                                   int level) const {
  for (const auto& [label, subdomain] : subdomains_) {
    os << std::setw(indent) << "" << subdomain.node_type_ << " ";
    for (int i = 0; i < level; i++) {
      os << "|   ";
    }
    os << "+-- " << label << "\n";
    subdomain.PrintDescendants(os, indent, level + 1);
  }
}

bool DomainConstraintsTree::Node::operator==(
    const DomainConstraintsTree::Node&) const = default;

DomainConstraintsTree::DomainConstraintsTree(bool invert)
    : inverted_(invert),
      root_type_(inverted_ ? DomainConstraintsTree::Node::kExcluded
                           : DomainConstraintsTree::Node::kIncluded) {}

DomainConstraintsTree::~DomainConstraintsTree() = default;
DomainConstraintsTree::DomainConstraintsTree(DomainConstraintsTree&&) = default;
DomainConstraintsTree& DomainConstraintsTree::operator=(
    DomainConstraintsTree&&) = default;

bool DomainConstraintsTree::operator==(const DomainConstraintsTree&) const =
    default;

void DomainConstraintsTree::Add(const NormalizedConstraint& constraint) {
  bool excluded = inverted_ ? !constraint.excluded : constraint.excluded;
  if (excluded) {
    has_exclusions_ = true;
  }

  // These are basically tests for generic rules.
  if (root_type_ == Node::kIncluded && !excluded) {
    root_type_ = Node::kNone;
  }

  if (root_type_ == Node::kExcluded && excluded) {
    root_type_ = Node::kNone;
  }

  Node* destination = nullptr;
  switch (constraint.kind) {
    case NormalizedConstraint::kRegex:
      if (excluded) {
        included_regexes_.erase(constraint.constraint);
        excluded_regexes_.insert(constraint.constraint);
      } else if (!excluded_regexes_.contains(constraint.constraint)) {
        included_regexes_.insert(constraint.constraint);
      }
      return;
    case NormalizedConstraint::kEntity:
      destination = &entities_;
      break;
    case NormalizedConstraint::kHostname:
      destination = &hostnames_;
      break;
  }

  CHECK(destination);

  std::vector<std::string_view> labels = base::SplitStringPiece(
      constraint.constraint, ".", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SplitResult::SPLIT_WANT_ALL);
  destination->AddSubDomains(labels, excluded);
}

DomainConstraintsTree::Node::NodeType DomainConstraintsTree::GetRootNodeType()
    const {
  return root_type_;
}

bool DomainConstraintsTree::IsGeneric() const {
  return root_type_ != Node::kNone;
}

bool DomainConstraintsTree::HasExclusions() const {
  return has_exclusions_ || inverted_;
}

void DomainConstraintsTree::Print(std::ostream& os) const {
  os << root_type_ << " Root" << (inverted_ ? "(inverted)" : "(normal)") << "\n"
     << std::setw(print_indent_) << "" << hostnames_.GetNodeType()
     << " +-- .\n";
  hostnames_.PrintDescendants(os, print_indent_, 1);
  os << std::setw(print_indent_) << "" << entities_.GetNodeType()
     << " +-- .*\n";
  entities_.PrintDescendants(os, print_indent_, 1);
  os << std::setw(print_indent_) << "" << "  +-- /\n";
  for (const auto& regex : included_regexes_) {
    os << std::setw(print_indent_) << "" << "I |  +--" << regex << "\n";
  }
  os << std::setw(print_indent_) << "" << "  +-- ~/\n";
  for (const auto& regex : excluded_regexes_) {
    os << std::setw(print_indent_) << "" << "X |  +--" << regex << "\n";
  }

  // Reset indent when we're done
  print_indent_ = 0;
}

std::ostream& operator<<(std::ostream& os, const DomainConstraintsTree& tree) {
  tree.Print(os);
  return os;
}
}  // namespace adblock_filter
