#include "components/omnibox/vivaldi_calculator_provider.h"

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"

#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/mock_autocomplete_provider_client.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

class VivaldiCalculatorProviderTest : public testing::Test {
 public:
  VivaldiCalculatorProviderTest() = default;

  void SetUp() override {
    client_ = std::make_unique<MockAutocompleteProviderClient>();
    provider_ = new VivaldiCalculatorProvider(client_.get());
  }

  void TearDown() override { provider_ = nullptr; }

 protected:
  void RunTest(const std::u16string& query,
               bool expect_match,
               const std::u16string& expected_fill_value = u"") {
    AutocompleteInput input(query, metrics::OmniboxEventProto::OTHER,
                            TestSchemeClassifier());

    provider_->Start(input, false);

    if (expect_match) {
      ASSERT_FALSE(provider_->matches().empty())
          << "Expected a calculation result, but got none.";
      EXPECT_EQ(provider_->matches()[0].fill_into_edit, expected_fill_value);
    } else {
      EXPECT_TRUE(provider_->matches().empty())
          << "Expected no matches, but the calculator tried to solve it.";
    }
  }

  std::unique_ptr<MockAutocompleteProviderClient> client_;
  scoped_refptr<VivaldiCalculatorProvider> provider_;
};

TEST_F(VivaldiCalculatorProviderTest, QCTests) {
  RunTest(u"5 + 5", true, u"10");
  RunTest(u"100 + 200", true, u"300");
  RunTest(u"204 + 2", true, u"206");

  RunTest(u"204 - 2", true, u"202");

  RunTest(u"204 * 2", true, u"408");

  RunTest(u"204 * 2", true, u"408");

  RunTest(u"204 / 2", true, u"102");
  RunTest(u"204 + 2 + 6", true, u"212");
  RunTest(u"200 * 30 / 5 + 300 - 100", true, u"1400");

  // Decimals (Periods and Commas)
  RunTest(u"1.1 - 0.11", true, u"0.99");
  RunTest(u"1,1 - 0.11", true, u"0.99");

  // Negatives and Parentheses
  RunTest(u"-3 + -5", true, u"-8");
  RunTest(u"(5 + 10) * 2", true, u"30");

  // Superscripts / Powers
  RunTest(u"2⁸", true, u"256");
  RunTest(u"2¹² - (4 - 2)⁸", true, u"3840");

  // Functions and Constants
  RunTest(u"sin(9)", true, u"0.4121184852417566");
  RunTest(u"PI", true, u"3.1415926535898");

  // Floating-point Dust (-0 / 0)
  RunTest(u"0.26-0.15+0.08-0.18+0.08-0.12+0.54-0.54+0.04-0.01", true, u"0");

  // Invalid Formulas (Should return false and stay hidden)
  RunTest(u"10.000,00 * 3", false, u"");
  RunTest(u"1 + 1 42", false, u"");
  RunTest(u"-42 12", false, u"");
}

TEST_F(VivaldiCalculatorProviderTest, OrderOfOperations) {
  RunTest(u"10 + 5 * 2", true, u"20");
  RunTest(u"(10 + 5) * 2", true, u"30");
}

TEST_F(VivaldiCalculatorProviderTest, Decimals) {
  RunTest(u"5.5 + 4.5", true, u"10");
  RunTest(u"-10 + 5", true, u"-5");
  RunTest(u"8.2 / 2", true, u"4.1");
  RunTest(u"8,2 / 2", true, u"4.1");
}

TEST_F(VivaldiCalculatorProviderTest, LeadingAndTrailingDecimals) {
  RunTest(u".5 + .5", true, u"1");
  RunTest(u",5 + ,5", true, u"1");
  RunTest(u"5. + 2.", true, u"7");
  RunTest(u"10, - 2,", true, u"8");
}

TEST_F(VivaldiCalculatorProviderTest, ComplexMixedDecimals) {
  RunTest(u"1.5 * 2.5", true, u"3.75");
  RunTest(u"10,5 / 2", true, u"5.25");
  RunTest(u"2.25 + 3,75", true, u"6");
  RunTest(u"(2.5 + 1,5) * 1.5", true, u"6");
  RunTest(u"10 - (2,5 * 2.0)", true, u"5");
}

TEST_F(VivaldiCalculatorProviderTest, DecimalsWithAdvancedMath) {
  RunTest(u"4 ^ 0.5", true, u"2");
  RunTest(u"4 ^ 1,5", true, u"8");
  RunTest(u"16 ^ 0,25", true, u"2");

  RunTest(u"sqrt(6.25)", true, u"2.5");
  RunTest(u"√6,25", true, u"2.5");
  RunTest(u"√(20,25)", true, u"4.5");
}

TEST_F(VivaldiCalculatorProviderTest, NegativeDecimals) {
  RunTest(u"-1.5 * 2", true, u"-3");
  RunTest(u"10 * -0,5", true, u"-5");
  RunTest(u"-2.5 * -2,5", true, u"6.25");
}

TEST_F(VivaldiCalculatorProviderTest, Exponents) {
  RunTest(u"5^2", true, u"25");
  RunTest(u"2^3", true, u"8");
  RunTest(u"2 * 3^2", true, u"18");
  RunTest(u"(2 * 3)^2", true, u"36");
}

TEST_F(VivaldiCalculatorProviderTest, InvalidQueries) {
  RunTest(u"einn plus einn", false);
  RunTest(u"vivaldi://bookmarks", false);
  RunTest(u"5 / 0", true, u"undefined");
}

TEST_F(VivaldiCalculatorProviderTest, InvalidExpressions) {
  RunTest(u"1.2.3 + 4", false, u"");
  RunTest(u"1,2,3 + 4", false, u"");
  RunTest(u"1.2,3 + 4", false, u"");
  RunTest(u"1..5 + 2", false, u"");
  RunTest(u"1,,5 + 2", false, u"");
}

TEST_F(VivaldiCalculatorProviderTest, ImplicitMultiplication) {
  RunTest(u"2(2+2)", true, u"8");
  RunTest(u"(2)(3)", true, u"6");
  RunTest(u"2(3)^2", true, u"18");
}

TEST_F(VivaldiCalculatorProviderTest, MultiplicationSignVariations) {
  RunTest(u"4 x 5", true, u"20");
  RunTest(u"4 X 5", true, u"20");
  RunTest(u"4 × 5", true, u"20");
  RunTest(u"4x5", true, u"20");
  RunTest(u"4X5", true, u"20");
  RunTest(u"2x(3+2)", true, u"10");
}

TEST_F(VivaldiCalculatorProviderTest, ImplicitMultiplicationWithSquareRoot) {
  RunTest(u"2√9", true, u"6");
  RunTest(u"5√16", true, u"20");
  RunTest(u"3sqrt(25)", true, u"15");
  RunTest(u"2(3)√16", true, u"24");
}

TEST_F(VivaldiCalculatorProviderTest, SquareRoot) {
  RunTest(u"√9", true, u"3");
  RunTest(u"√16", true, u"4");
  RunTest(u"sqrt(25)", true, u"5");
  RunTest(u"sqrt 36", true, u"6");
  RunTest(u"√(9 + 16)", true, u"5");
  RunTest(u"√9 + √16", true, u"7");
  RunTest(u"-√81", true, u"-9");
}

TEST_F(VivaldiCalculatorProviderTest, FlautoMathTest) {
  // Basic Arithmetic
  RunTest(u"100+28", true, u"128");
  RunTest(u"81-11", true, u"70");
  RunTest(u"-1+10", true, u"9");
  RunTest(u"(25+5)", true, u"30");
  RunTest(u"-(103+2)", true, u"-105");
  RunTest(u"1.8 - 0.2", true, u"1.6");

  // Multiplication
  RunTest(u"3*7", true, u"21");
  RunTest(u"-5*6", true, u"-30");
  RunTest(u"1.5*1.5", true, u"2.25");
  // RunTest(u"0.333*3", true, u"0.999");
  RunTest(u"(2+3)*4", true, u"20");
  RunTest(u"(21+7)*(1 + 2)", true, u"84");

  // Division
  RunTest(u"3/4", true, u"0.75");
  RunTest(u"256/8", true, u"32");
  RunTest(u"-1/0.002", true, u"-500");
  RunTest(u"-12/4", true, u"-3");
  RunTest(u"(121 - 21)/5", true, u"20");
  RunTest(u"(81 - 11)/(5+2)", true, u"10");

  // Order of Operations (Mixed)
  RunTest(u"-5 + 32/8", true, u"-1");
  RunTest(u"2 + 3*4/2", true, u"8");
  RunTest(u"(9 - 3)*5/(1 + 2)", true, u"10");

  // Exponents & Roots
  RunTest(u"3^3", true, u"27");
  RunTest(u"(2+7)^2", true, u"81");
  RunTest(u"169^(1/2)", true, u"13");
  RunTest(u"0.05^-1", true, u"20");
  RunTest(u"sqrt(144)", true, u"12");

  // Factorials
  RunTest(u"5!", true, u"120");
  RunTest(u"(2+4)!", true, u"720");

  // Modulo & Ternary
  RunTest(u"256%129", true, u"127");
  RunTest(u"1?(12*2):13", true, u"24");

  // Advanced Math Functions
  RunTest(u"abs(-1)+14", true, u"15");
  RunTest(u"ceil(10.1+1)", true, u"12");
  RunTest(u"floor(11.9*2)", true, u"23");
  RunTest(u"round(19.5)", true, u"20");
  RunTest(u"trunc(0.5-19)", true, u"-18");
  RunTest(u"length(1000)", true, u"4");

  // Trigonometry
  RunTest(u"cos(PI)", true, u"-1");
  RunTest(u"cos(PI/3)", true, u"0.5");
  RunTest(u"sin(3*PI/2)", true, u"-1");
  RunTest(u"tan(3*PI/4)", true, u"-1");

  // Logarithms
  RunTest(u"ln(E^(3+8))", true, u"11");
  RunTest(u"log(E^(3+8))", true, u"11");
  RunTest(u"log(E^(5*2))", true, u"10");
  RunTest(u"log10(10^(3*4))", true, u"12");
  RunTest(u"length(1000)", true, u"4");
}

TEST_F(VivaldiCalculatorProviderTest, TrailingEquals) {
  RunTest(u"5+5=", true, u"10");
  RunTest(u"5 + 5 =", true, u"10");
  RunTest(u"5+5 = ", true, u"10");
  RunTest(u"5 + 5=  ", true, u"10");
  RunTest(u"100 / 2 =", true, u"50");

  RunTest(u"5 = + 5", false);
  RunTest(u"= 5 + 5", false);
  RunTest(u"5 + 5 = =", false);
}
