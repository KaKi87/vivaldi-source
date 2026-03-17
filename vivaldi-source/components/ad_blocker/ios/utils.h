// Copyright (c) 2023 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_AD_BLOCKER_IOS_UTILS_H_
#define COMPONENTS_AD_BLOCKER_IOS_UTILS_H_

#include <string>

#include "base/values.h"

namespace adblock_filter {

int GetIntermediateRepresentationVersionNumber();
int GetOrganizedRulesVersionNumber();

std::string CalculateBufferChecksum(const std::string& data);

struct ContentInjectionArgumentsCompare {
  bool operator()(const base::ListValue* lhs, const base::ListValue* rhs) const;
};
}  // namespace adblock_filter

#endif  // COMPONENTS_AD_BLOCKER_IOS_UTILS_H_
