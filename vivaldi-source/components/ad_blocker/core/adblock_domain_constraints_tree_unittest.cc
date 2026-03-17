// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/core/adblock_domain_constraints_tree.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace adblock_filter {

TEST(AdBlockDomainConstraintsTreeTest, EmptyTree) {
  DomainConstraintsTree tree(false);
  EXPECT_TRUE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.entities().subdomains().empty());
  EXPECT_TRUE(tree.included_regexes().empty());
  EXPECT_TRUE(tree.excluded_regexes().empty());

  DomainConstraintsTree invert_tree(true);
  EXPECT_TRUE(invert_tree.IsGeneric());
  EXPECT_EQ(invert_tree.GetRootNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(invert_tree.hostnames().GetNodeType(),
            DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(invert_tree.hostnames().subdomains().empty());
  EXPECT_EQ(invert_tree.entities().GetNodeType(),
            DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(invert_tree.entities().subdomains().empty());
  EXPECT_TRUE(invert_tree.included_regexes().empty());
  EXPECT_TRUE(invert_tree.excluded_regexes().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, WithSpecificHostnames) {
  DomainConstraintsTree tree(false);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = false,
            .constraint = "example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = false,
            .constraint = "sub.example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "exclude.example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "sub.exclude.example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = false,
            .constraint = "include.exclude.example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = false,
            .constraint = "example2.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "sub.example2.com"});

  EXPECT_FALSE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kNone);

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(tree.hostnames().subdomains().size(), 1ULL);
  ASSERT_TRUE(tree.hostnames().subdomains().contains("com"));

  const DomainConstraintsTree::Node& com_node =
      tree.hostnames().subdomains().at("com");
  EXPECT_EQ(com_node.GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(com_node.subdomains().size(), 2ULL);
  ASSERT_TRUE(com_node.subdomains().contains("example"));
  ASSERT_TRUE(com_node.subdomains().contains("example2"));

  const DomainConstraintsTree::Node& example_com_node =
      com_node.subdomains().at("example");
  EXPECT_EQ(example_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(example_com_node.subdomains().size(), 1ULL);
  ASSERT_TRUE(example_com_node.subdomains().contains("exclude"));

  const DomainConstraintsTree::Node& exclude_example_com_node =
      example_com_node.subdomains().at("exclude");
  EXPECT_EQ(exclude_example_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(exclude_example_com_node.subdomains().size(), 0ULL);

  const DomainConstraintsTree::Node& example2_com_node =
      com_node.subdomains().at("example2");
  EXPECT_EQ(example2_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(example2_com_node.subdomains().size(), 1ULL);
  ASSERT_TRUE(example2_com_node.subdomains().contains("sub"));

  const DomainConstraintsTree::Node& sub_example2_com_node =
      example2_com_node.subdomains().at("sub");
  EXPECT_EQ(sub_example2_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(sub_example2_com_node.subdomains().size(), 0ULL);

  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.entities().subdomains().empty());
  EXPECT_TRUE(tree.included_regexes().empty());
  EXPECT_TRUE(tree.excluded_regexes().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, WithGenericHostnames) {
  DomainConstraintsTree tree(false);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "sub.example.com"});

  EXPECT_TRUE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kIncluded);

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(tree.hostnames().subdomains().size(), 1ULL);
  ASSERT_TRUE(tree.hostnames().subdomains().contains("com"));

  const DomainConstraintsTree::Node& com_node =
      tree.hostnames().subdomains().at("com");
  EXPECT_EQ(com_node.GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(com_node.subdomains().size(), 1ULL);
  ASSERT_TRUE(com_node.subdomains().contains("example"));

  const DomainConstraintsTree::Node& example_com_node =
      com_node.subdomains().at("example");
  EXPECT_EQ(example_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(example_com_node.subdomains().size(), 0ULL);

  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.entities().subdomains().empty());
  EXPECT_TRUE(tree.included_regexes().empty());
  EXPECT_TRUE(tree.excluded_regexes().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, InvertedWithHostnames) {
  DomainConstraintsTree tree(true);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = false,
            .constraint = "example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = false,
            .constraint = "sub.example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "exclude.example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "sub.exclude.example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = false,
            .constraint = "include.exclude.example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "example2.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = false,
            .constraint = "sub.example2.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "sub2.example2.com"});

  EXPECT_FALSE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kNone);

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(tree.hostnames().subdomains().size(), 1ULL);
  ASSERT_TRUE(tree.hostnames().subdomains().contains("com"));

  const DomainConstraintsTree::Node& com_node =
      tree.hostnames().subdomains().at("com");
  EXPECT_EQ(com_node.GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(com_node.subdomains().size(), 2ULL);
  ASSERT_TRUE(com_node.subdomains().contains("example"));
  ASSERT_TRUE(com_node.subdomains().contains("example2"));

  const DomainConstraintsTree::Node& example_com_node =
      com_node.subdomains().at("example");
  EXPECT_EQ(example_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(example_com_node.subdomains().size(), 0ULL);

  const DomainConstraintsTree::Node& example2_com_node =
      com_node.subdomains().at("example2");
  EXPECT_EQ(example2_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(example2_com_node.subdomains().size(), 1ULL);
  ASSERT_TRUE(example2_com_node.subdomains().contains("sub"));

  const DomainConstraintsTree::Node& sub_example2_com_node =
      example2_com_node.subdomains().at("sub");
  EXPECT_EQ(sub_example2_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(sub_example2_com_node.subdomains().size(), 0ULL);

  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.entities().subdomains().empty());
  EXPECT_TRUE(tree.included_regexes().empty());
  EXPECT_TRUE(tree.excluded_regexes().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, InvertedWithGenericHostnames) {
  DomainConstraintsTree tree(true);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "example.com"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kHostname,
            .excluded = true,
            .constraint = "sub.example.com"});

  EXPECT_TRUE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kExcluded);

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(tree.hostnames().subdomains().size(), 1ULL);
  ASSERT_TRUE(tree.hostnames().subdomains().contains("com"));

  const DomainConstraintsTree::Node& com_node =
      tree.hostnames().subdomains().at("com");
  EXPECT_EQ(com_node.GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(com_node.subdomains().size(), 1ULL);
  ASSERT_TRUE(com_node.subdomains().contains("example"));

  const DomainConstraintsTree::Node& example_com_node =
      com_node.subdomains().at("example");
  EXPECT_EQ(example_com_node.GetNodeType(),
            DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(example_com_node.subdomains().size(), 0ULL);
}

TEST(AdBlockDomainConstraintsTreeTest, WithSpecificEntities) {
  DomainConstraintsTree tree(false);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = false,
            .constraint = "example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = false,
            .constraint = "sub.example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "exclude.example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "sub.exclude.example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = false,
            .constraint = "include.exclude.example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = false,
            .constraint = "example2"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "sub.example2"});

  EXPECT_FALSE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kNone);

  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(tree.entities().subdomains().size(), 2ULL);
  ASSERT_TRUE(tree.entities().subdomains().contains("example"));
  ASSERT_TRUE(tree.entities().subdomains().contains("example2"));

  const DomainConstraintsTree::Node& example_node =
      tree.entities().subdomains().at("example");
  EXPECT_EQ(example_node.GetNodeType(), DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(example_node.subdomains().size(), 1ULL);
  ASSERT_TRUE(example_node.subdomains().contains("exclude"));

  const DomainConstraintsTree::Node& exclude_example_node =
      example_node.subdomains().at("exclude");
  EXPECT_EQ(exclude_example_node.GetNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(exclude_example_node.subdomains().size(), 0ULL);

  const DomainConstraintsTree::Node& example2_node =
      tree.entities().subdomains().at("example2");
  EXPECT_EQ(example2_node.GetNodeType(),
            DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(example2_node.subdomains().size(), 1ULL);
  ASSERT_TRUE(example2_node.subdomains().contains("sub"));

  const DomainConstraintsTree::Node& sub_example2_node =
      example2_node.subdomains().at("sub");
  EXPECT_EQ(sub_example2_node.GetNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(sub_example2_node.subdomains().size(), 0ULL);

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_TRUE(tree.included_regexes().empty());
  EXPECT_TRUE(tree.excluded_regexes().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, WithGenericEntities) {
  DomainConstraintsTree tree(false);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "sub.example"});

  EXPECT_TRUE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kIncluded);

  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(tree.entities().subdomains().size(), 1ULL);
  ASSERT_TRUE(tree.entities().subdomains().contains("example"));

  const DomainConstraintsTree::Node& example_node =
      tree.entities().subdomains().at("example");
  EXPECT_EQ(example_node.GetNodeType(), DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(example_node.subdomains().size(), 0ULL);

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_TRUE(tree.included_regexes().empty());
  EXPECT_TRUE(tree.excluded_regexes().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, InvertedWithEntities) {
  DomainConstraintsTree tree(true);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = false,
            .constraint = "example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = false,
            .constraint = "sub.example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "exclude.example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "sub.exclude.example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = false,
            .constraint = "include.exclude.example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "example2"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = false,
            .constraint = "sub.example2"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "sub2.example2"});

  EXPECT_FALSE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kNone);

  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(tree.entities().subdomains().size(), 2ULL);
  ASSERT_TRUE(tree.entities().subdomains().contains("example"));
  ASSERT_TRUE(tree.entities().subdomains().contains("example2"));

  const DomainConstraintsTree::Node& example_node =
      tree.entities().subdomains().at("example");
  EXPECT_EQ(example_node.GetNodeType(), DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(example_node.subdomains().size(), 0ULL);

  const DomainConstraintsTree::Node& example2_node =
      tree.entities().subdomains().at("example2");
  EXPECT_EQ(example2_node.GetNodeType(),
            DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(example2_node.subdomains().size(), 1ULL);
  ASSERT_TRUE(example2_node.subdomains().contains("sub"));

  const DomainConstraintsTree::Node& sub_example2_node =
      example2_node.subdomains().at("sub");
  EXPECT_EQ(sub_example2_node.GetNodeType(),
            DomainConstraintsTree::Node::kExcluded);
  EXPECT_EQ(sub_example2_node.subdomains().size(), 0ULL);

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_TRUE(tree.included_regexes().empty());
  EXPECT_TRUE(tree.excluded_regexes().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, InvertedWithGenericEntities) {
  DomainConstraintsTree tree(true);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kEntity,
            .excluded = true,
            .constraint = "sub.example"});

  EXPECT_TRUE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kExcluded);

  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_EQ(tree.entities().subdomains().size(), 1ULL);
  ASSERT_TRUE(tree.entities().subdomains().contains("example"));

  const DomainConstraintsTree::Node& example_node =
      tree.entities().subdomains().at("example");
  EXPECT_EQ(example_node.GetNodeType(), DomainConstraintsTree::Node::kIncluded);
  EXPECT_EQ(example_node.subdomains().size(), 0ULL);

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_TRUE(tree.included_regexes().empty());
  EXPECT_TRUE(tree.excluded_regexes().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, WithRegexes) {
  DomainConstraintsTree tree(false);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kRegex,
            .excluded = false,
            .constraint = "^example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kRegex,
            .excluded = true,
            .constraint = ".example2"});

  EXPECT_FALSE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kNone);

  EXPECT_EQ(tree.included_regexes().size(), 1ULL);
  EXPECT_TRUE(tree.included_regexes().contains("^example"));
  EXPECT_EQ(tree.excluded_regexes().size(), 1ULL);
  EXPECT_TRUE(tree.excluded_regexes().contains(".example2"));

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.entities().subdomains().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, WithGenericRegexes) {
  DomainConstraintsTree tree(false);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kRegex,
            .excluded = true,
            .constraint = ".example2"});

  EXPECT_TRUE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kIncluded);

  EXPECT_EQ(tree.excluded_regexes().size(), 1ULL);
  EXPECT_TRUE(tree.excluded_regexes().contains(".example2"));

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.entities().subdomains().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, InvertedWithRegexes) {
  DomainConstraintsTree tree(true);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kRegex,
            .excluded = false,
            .constraint = "^example"});
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kRegex,
            .excluded = true,
            .constraint = ".example2"});

  EXPECT_FALSE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kNone);

  EXPECT_EQ(tree.excluded_regexes().size(), 1ULL);
  EXPECT_TRUE(tree.excluded_regexes().contains("^example"));
  EXPECT_EQ(tree.included_regexes().size(), 1ULL);
  EXPECT_TRUE(tree.included_regexes().contains(".example2"));

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.entities().subdomains().empty());
}

TEST(AdBlockDomainConstraintsTreeTest, InvertedWithGenericRegexes) {
  DomainConstraintsTree tree(true);
  tree.Add({.kind = DomainConstraintsTree::NormalizedConstraint::kRegex,
            .excluded = true,
            .constraint = ".example2"});

  EXPECT_TRUE(tree.IsGeneric());
  EXPECT_EQ(tree.GetRootNodeType(), DomainConstraintsTree::Node::kExcluded);

  EXPECT_EQ(tree.included_regexes().size(), 1ULL);
  EXPECT_TRUE(tree.included_regexes().contains(".example2"));

  EXPECT_EQ(tree.hostnames().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.hostnames().subdomains().empty());
  EXPECT_EQ(tree.entities().GetNodeType(), DomainConstraintsTree::Node::kNone);
  EXPECT_TRUE(tree.entities().subdomains().empty());
}
}  // namespace adblock_filter
