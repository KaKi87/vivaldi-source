// Copyright (c) 2020 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/core/adblock_content_injection_rule.h"

#include <iomanip>
#include <ostream>

namespace adblock_filter {

namespace {
constexpr int kAlignemntPosition = 20;
}  // namespace

ContentInjectionRuleCore::ContentInjectionRuleCore(bool invert)
    : domain_constraints(invert) {}
ContentInjectionRuleCore::~ContentInjectionRuleCore() = default;
ContentInjectionRuleCore::ContentInjectionRuleCore(
    ContentInjectionRuleCore&& other) = default;
ContentInjectionRuleCore& ContentInjectionRuleCore::operator=(
    ContentInjectionRuleCore&& other) = default;
bool ContentInjectionRuleCore::operator==(
    const ContentInjectionRuleCore& other) const = default;

std::ostream& operator<<(std::ostream& os,
                         const ContentInjectionRuleCore& rule) {
  rule.domain_constraints.set_print_indent_(kAlignemntPosition);

  os << "\n"
     << std::setw(kAlignemntPosition)
     << "Domain constraints:" << rule.domain_constraints;

  return os;
}

CosmeticRule::CosmeticRule(ContentInjectionRuleCore core)
    : core(std::move(core)) {}
CosmeticRule::~CosmeticRule() = default;
CosmeticRule::CosmeticRule(CosmeticRule&& other) = default;
CosmeticRule& CosmeticRule::operator=(CosmeticRule&& other) = default;

bool CosmeticRule::operator==(const CosmeticRule& other) const = default;

std::ostream& operator<<(std::ostream& os, const CosmeticRule& rule) {
  return os << "\n"
            << std::setw(kAlignemntPosition) << "Selector:" << rule.selector
            << rule.core;
}

ScriptletInjectionRule::ScriptletInjectionRule(ContentInjectionRuleCore core)
    : core(std::move(core)) {}
ScriptletInjectionRule::~ScriptletInjectionRule() = default;
ScriptletInjectionRule::ScriptletInjectionRule(ScriptletInjectionRule&& other) =
    default;
ScriptletInjectionRule& ScriptletInjectionRule::operator=(
    ScriptletInjectionRule&& other) = default;
bool ScriptletInjectionRule::operator==(
    const ScriptletInjectionRule& other) const = default;

bool ScriptletInjectionRule::Scriptlet::operator==(
    const Scriptlet& other) const = default;

std::ostream& operator<<(std::ostream& os, const ScriptletInjectionRule& rule) {
  os << "\n" << std::setw(kAlignemntPosition) << "Scriptlets:";

  for (const auto& scriptlet : rule.scriptlets) {
    os << "\n"
       << std::setw(kAlignemntPosition + 4) << "Scriptlet:" << scriptlet.name
       << "\n"
       << std::setw(kAlignemntPosition + 4) << "Arguments:";

    for (const auto& argument : scriptlet.arguments) {
      os << argument << " ";
    }
  }
  return os << rule.core;
}

}  // namespace adblock_filter
