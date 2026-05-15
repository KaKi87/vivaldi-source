// Copyright (c) 2020 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_AD_BLOCKER_CORE_PARSER_ADBLOCK_CONTENT_INJECTION_RULE_H_
#define COMPONENTS_AD_BLOCKER_CORE_PARSER_ADBLOCK_CONTENT_INJECTION_RULE_H_

#include <string>
#include <vector>

#include "components/ad_blocker/core/parser/adblock_domain_constraints_tree.h"

namespace adblock_filter {

struct ContentInjectionRuleCore {
 public:
  explicit ContentInjectionRuleCore(bool invert);
  ~ContentInjectionRuleCore();
  ContentInjectionRuleCore(ContentInjectionRuleCore&& other);
  ContentInjectionRuleCore& operator=(ContentInjectionRuleCore&& other);
  bool operator==(const ContentInjectionRuleCore& other) const;

  DomainConstraintsTree domain_constraints;

 private:
  ContentInjectionRuleCore(const ContentInjectionRuleCore& other);
};

struct CosmeticRule {
 public:
  explicit CosmeticRule(ContentInjectionRuleCore core);
  ~CosmeticRule();
  CosmeticRule(CosmeticRule&& other);
  CosmeticRule& operator=(CosmeticRule&& other);
  bool operator==(const CosmeticRule& other) const;

  ContentInjectionRuleCore core;

  std::string selector;
};

using CosmeticRules = std::vector<CosmeticRule>;

struct ScriptletInjectionRule {
 public:
  struct Scriptlet {
    std::string name;
    std::vector<std::string> arguments;

    bool operator==(const Scriptlet& other) const;
  };

  explicit ScriptletInjectionRule(ContentInjectionRuleCore core);
  ~ScriptletInjectionRule();
  ScriptletInjectionRule(ScriptletInjectionRule&& other);
  ScriptletInjectionRule& operator=(ScriptletInjectionRule&& other);
  bool operator==(const ScriptletInjectionRule& other) const;

  ContentInjectionRuleCore core;
  std::vector<Scriptlet> scriptlets;
};

using ScriptletInjectionRules = std::vector<ScriptletInjectionRule>;

// Used for unit tests.
std::ostream& operator<<(std::ostream& os,
                         const ContentInjectionRuleCore& rule);
std::ostream& operator<<(std::ostream& os, const CosmeticRule& rule);
std::ostream& operator<<(std::ostream& os, const ScriptletInjectionRule& rule);
}  // namespace adblock_filter

#endif  // COMPONENTS_AD_BLOCKER_CORE_PARSER_ADBLOCK_CONTENT_INJECTION_RULE_H_
