// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_CALCULATOR_CALCULATOR_H_
#define COMPONENTS_CALCULATOR_CALCULATOR_H_

#include <optional>
#include <string>

namespace vivaldi::calculator {

bool IsMathQuery(const std::string& query);
std::optional<double> Evaluate(const std::string& expression);

}  // namespace vivaldi::calculator
#endif  // COMPONENTS_CALCULATOR_CALCULATOR_H_
