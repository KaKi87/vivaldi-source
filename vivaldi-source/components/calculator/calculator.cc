// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include "calculator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace vivaldi::calculator {

namespace {

void SkipSpaces(const std::string& expr, size_t& pos);
std::optional<double> ParseTernary(const std::string& expr, size_t& pos);
std::optional<double> ParseExpression(const std::string& expr, size_t& pos);
std::optional<double> ParseTerm(const std::string& expr, size_t& pos);
std::optional<double> ParseExponent(const std::string& expr, size_t& pos);
std::optional<double> ParseFactor(const std::string& expr, size_t& pos);
std::string NormalizeMathQuery(std::string query);

void SkipSpaces(const std::string& expr, size_t& pos) {
  while (pos < expr.length() &&
         std::isspace(static_cast<unsigned char>(expr[pos]))) {
    pos++;
  }
}

std::optional<double> ParseTernary(const std::string& expr, size_t& pos) {
  std::optional<double> condition = ParseExpression(expr, pos);
  if (!condition)
    return std::nullopt;

  SkipSpaces(expr, pos);
  if (pos < expr.length() && expr[pos] == '?') {
    pos++;
    std::optional<double> true_branch = ParseTernary(expr, pos);
    if (!true_branch)
      return std::nullopt;

    SkipSpaces(expr, pos);
    if (pos < expr.length() && expr[pos] == ':') {
      pos++;
      std::optional<double> false_branch = ParseTernary(expr, pos);
      if (!false_branch)
        return std::nullopt;

      return (condition.value() != 0.0) ? true_branch.value()
                                        : false_branch.value();
    } else {
      return std::nullopt;
    }
  }
  return condition;
}

std::optional<double> ParseFactor(const std::string& expr, size_t& pos) {
  SkipSpaces(expr, pos);
  if (pos >= expr.length())
    return std::nullopt;

  double sign = 1.0;
  if (expr[pos] == '-') {
    sign = -1.0;
    pos++;
  } else if (expr[pos] == '+') {
    pos++;
  }

  SkipSpaces(expr, pos);

  double parsed_val = 0.0;
  bool has_val = false;

  std::string funcs[] = {"sqrt",  "abs",    "ceil", "floor", "round",
                         "trunc", "length", "cos",  "sin",   "tan",
                         "log10", "log",    "ln"};
  bool matched_func = false;

  for (const auto& func : funcs) {
    if (pos + func.length() <= expr.length() &&
        expr.substr(pos, func.length()) == func) {
      pos += func.length();
      std::optional<double> inner = ParseFactor(expr, pos);
      if (!inner)
        return std::nullopt;

      double val = inner.value();

      if (func == "sqrt") {
        if (val < 0)
          return std::numeric_limits<double>::quiet_NaN();
        parsed_val = std::sqrt(val);
      } else if (func == "abs")
        parsed_val = std::abs(val);
      else if (func == "ceil")
        parsed_val = std::ceil(val);
      else if (func == "floor")
        parsed_val = std::floor(val);
      else if (func == "round")
        parsed_val = std::round(val);
      else if (func == "trunc")
        parsed_val = std::trunc(val);
      else if (func == "length")
        parsed_val = static_cast<double>(
            std::to_string(static_cast<long long>(std::abs(val))).length());
      else if (func == "cos")
        parsed_val = std::cos(val);
      else if (func == "sin")
        parsed_val = std::sin(val);
      else if (func == "tan")
        parsed_val = std::tan(val);
      else if (func == "log10")
        parsed_val = std::log10(val);
      else if (func == "log" || func == "ln")
        parsed_val = std::log(val);

      has_val = true;
      matched_func = true;
      break;
    }
  }

  if (!matched_func) {
    if (pos + 2 <= expr.length() && expr.substr(pos, 2) == "pi") {
      parsed_val = 3.14159265358979323846;
      pos += 2;
      has_val = true;
    } else if (pos + 1 <= expr.length() && expr[pos] == 'e') {
      parsed_val = 2.71828182845904523536;
      pos += 1;
      has_val = true;
    } else if (pos < expr.length() && expr[pos] == '(') {
      pos++;
      std::optional<double> inner = ParseTernary(expr, pos);
      if (!inner)
        return std::nullopt;
      SkipSpaces(expr, pos);
      if (pos < expr.length() && expr[pos] == ')') {
        pos++;
        parsed_val = inner.value();
        has_val = true;
      } else {
        return std::nullopt;
      }
    } else {
      size_t startPos = pos;
      while (pos < expr.length() && (std::isdigit(expr[pos]) ||
                                     expr[pos] == '.' || expr[pos] == ',')) {
        pos++;
      }
      if (startPos == pos)
        return std::nullopt;

      std::string clean_number = expr.substr(startPos, pos - startPos);
      std::replace(clean_number.begin(), clean_number.end(), ',', '.');
      if (!clean_number.empty()) {
        if (clean_number.front() == '.')
          clean_number.insert(0, "0");
        if (clean_number.back() == '.')
          clean_number.push_back('0');
      }

      if (!base::StringToDouble(clean_number, &parsed_val))
        return std::nullopt;
      has_val = true;
    }
  }

  if (!has_val)
    return std::nullopt;

  // Postfix factorial (e.g., 5!)
  while (pos < expr.length()) {
    SkipSpaces(expr, pos);
    if (pos < expr.length() && expr[pos] == '!') {
      pos++;
      parsed_val = std::tgamma(parsed_val + 1.0);
    } else {
      break;
    }
  }

  return sign * parsed_val;
}

std::optional<double> ParseExponent(const std::string& expr, size_t& pos) {
  std::optional<double> result = ParseFactor(expr, pos);
  if (!result)
    return std::nullopt;

  while (pos < expr.length()) {
    SkipSpaces(expr, pos);
    if (pos >= expr.length() || expr[pos] != '^')
      break;
    pos++;
    std::optional<double> next = ParseFactor(expr, pos);
    if (!next)
      return std::nullopt;
    result = std::pow(result.value(), next.value());
  }
  return result;
}

std::optional<double> ParseTerm(const std::string& expr, size_t& pos) {
  std::optional<double> result = ParseExponent(expr, pos);
  if (!result)
    return std::nullopt;

  while (pos < expr.length()) {
    SkipSpaces(expr, pos);
    if (pos >= expr.length())
      break;

    char op = expr[pos];
    if (op == '*' || op == '/' || op == '%') {
      pos++;
      std::optional<double> next = ParseExponent(expr, pos);
      if (!next)
        return std::nullopt;

      if (op == '*')
        result = result.value() * next.value();
      else if (op == '/') {
        if (next.value() == 0)
          result = std::numeric_limits<double>::quiet_NaN();
        else
          result = result.value() / next.value();
      } else if (op == '%') {
        if (next.value() == 0)
          result = std::numeric_limits<double>::quiet_NaN();
        else
          result = std::fmod(result.value(), next.value());
      }
    } else if (op == '(' || std::isalpha(op)) {
      std::optional<double> next = ParseExponent(expr, pos);
      if (!next)
        return std::nullopt;
      result = result.value() * next.value();
    } else {
      break;
    }
  }
  return result;
}

std::optional<double> ParseExpression(const std::string& expr, size_t& pos) {
  std::optional<double> result = ParseTerm(expr, pos);
  if (!result)
    return std::nullopt;

  while (pos < expr.length()) {
    SkipSpaces(expr, pos);
    if (pos >= expr.length())
      break;
    char op = expr[pos];
    if (op != '+' && op != '-')
      break;
    pos++;
    std::optional<double> next = ParseTerm(expr, pos);
    if (!next)
      return std::nullopt;

    if (op == '+')
      result = result.value() + next.value();
    else
      result = result.value() - next.value();
  }
  return result;
}

std::string NormalizeMathQuery(std::string query) {
  query = base::ToLowerASCII(query);
  base::ReplaceSubstringsAfterOffset(&query, 0, "x", "*");
  base::ReplaceSubstringsAfterOffset(&query, 0, "×", "*");
  base::ReplaceSubstringsAfterOffset(&query, 0, "÷", "/");
  base::ReplaceSubstringsAfterOffset(&query, 0, "−", "-");
  base::ReplaceSubstringsAfterOffset(&query, 0, "√", "sqrt");

  // Map superscripts to standard digits, adding a temporary "_S_" marker
  // Example: "2¹²" becomes "2^1_S_^2_S_"
  base::ReplaceSubstringsAfterOffset(&query, 0, "⁰", "^0_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "¹", "^1_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "²", "^2_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "³", "^3_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "⁴", "^4_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "⁵", "^5_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "⁶", "^6_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "⁷", "^7_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "⁸", "^8_S_");
  base::ReplaceSubstringsAfterOffset(&query, 0, "⁹", "^9_S_");

  // Merge contiguous superscripts by deleting the boundary between them
  // Example: "2^1_S_^2_S_" becomes "2^12_S_"
  base::ReplaceSubstringsAfterOffset(&query, 0, "_S_^", "");

  // Clean up the leftover markers at the end of the exponent block
  // Example: "2^12_S_" becomes "2^12"
  base::ReplaceSubstringsAfterOffset(&query, 0, "_S_", "");

  return query;
}

}  // namespace

bool IsMathQuery(const std::string& query) {
  std::string normalized = NormalizeMathQuery(query);
  bool has_operand = false;
  bool has_operator = false;

  for (char c : normalized) {
    if (std::isdigit(c)) {
      has_operand = true;
    } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^' ||
               c == '(' || c == ')' || c == '%' || c == '!' || c == '?' ||
               c == ':') {
      has_operator = true;
    } else if (std::isalpha(c)) {
      has_operator = true;
      has_operand = true;
    }
  }

  return has_operand && has_operator;
}

std::optional<double> Evaluate(const std::string& expression) {
  std::string normalized = NormalizeMathQuery(expression);
  size_t pos = 0;

  std::optional<double> result = ParseTernary(normalized, pos);
  if (!result)
    return std::nullopt;

  SkipSpaces(normalized, pos);

  // Allow and consume a trailing '=' character
  if (pos < normalized.length() && normalized[pos] == '=') {
    pos++;
    SkipSpaces(normalized, pos);
  }

  if (pos < normalized.length()) {
    return std::nullopt;
  }

  double rounded_value = result.value();
  if (std::isfinite(rounded_value)) {
    double rounded = std::round(rounded_value * 1e13) / 1e13;
    if (std::abs(rounded_value - rounded) < 1e-14) {
      rounded_value = rounded;
    }
  }

  // Forcing negative zero (-0.0) to positive 0.0
  if (rounded_value == 0.0) {
    rounded_value = 0.0;
  }

  return rounded_value;
}
}  // namespace vivaldi::calculator
