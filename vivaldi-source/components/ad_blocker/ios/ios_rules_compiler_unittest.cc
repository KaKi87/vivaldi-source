#include "components/ad_blocker/ios/ios_rules_compiler.h"

#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "components/ad_blocker/core/adblock_rule_parser.h"
#include "components/ad_blocker/core/parse_result.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock_filter {
namespace {
bool FormatJSON(std::string& json) {
  JSONStringValueDeserializer deserializer(json);
  std::string output;
  JSONStringValueSerializer serializer(&output);
  serializer.set_pretty_print(true);
  int error_code;
  std::string error;
  std::unique_ptr<base::Value> value =
      deserializer.Deserialize(&error_code, &error);
  if (!value) {
    LOG(ERROR) << error_code << ":" << error;
    return false;
  }
  serializer.Serialize(*value);
  json.swap(output);
  return true;
}
}  // namespace

TEST(AdBlockIosRuleCompilerTest, SimpleRule) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RequestFilterRule rule;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("example");
  rule_parser.Parse("blåbærsyltetøy");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "example",
              "resource-type": [
                "style-sheet",
                "image",
                "media",
                "script",
                "fetch",
                "child-document",
                "font",
                "media",
                "websocket",
                "ping",
                "other"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, RuleWithResource) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("something$script");
  rule_parser.Parse("something$image,match-case");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "something",
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "something",
              "url-filter-is-case-sensitive": true,
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, SubdocumentRule) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("something$subdocument");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "something",
              "resource-type": [
                "child-document"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, RuleWithParty) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("something_from_self$script,~third-party");
  rule_parser.Parse("something_from_others$script,third-party");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "something_from_self",
              "load-type": [
                "first-party"
              ],
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "something_from_others",
              "load-type": [
                "third-party"
              ],
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, AllowRuleWithResource) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("@@something$script");
  std::string expected(R"===({
    "network": {
      "allow": [
        {
          "trigger": {
            "url-filter": "something",
            "resource-type": [
              "script"
            ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        }
      ]
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, AnchoredRules) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("|https://example.com/$script");
  rule_parser.Parse("||google.com/$script");
  rule_parser.Parse("ad.js|$script");

  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "^https:\\/\\/example\\.com\\/",
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*google\\.com\\/",
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "ad\\.js$",
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, WildcardsAndSpecialCharsRules) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("part1*part2?part(3)$ping");
  rule_parser.Parse("example.com^bad^$websocket");
  rule_parser.Parse("google.com^|$media");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "part1.*part2\\?part\\(3\\)",
              "resource-type": [
                "ping"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "example\\.com[^a-zA-Z0-9_.%-]bad([^a-zA-Z0-9_.%-].*)?$",
              "resource-type": [
                "websocket"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "google\\.com[^a-zA-Z0-9_.%-]?$",
              "resource-type": [
                "media"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, RulesWithHost) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("advert$host=example.com,image");
  rule_parser.Parse("foo.com/bar$host=foo.com,image");
  rule_parser.Parse("google.com/something$host=evil.google.com,image");
  rule_parser.Parse("ads.example.com/something$host=example.com,image");
  rule_parser.Parse("baz$host=baz.com,image");
  rule_parser.Parse("xxx.elg.no$host=elg.no,image");
  rule_parser.Parse("ulv.no.zzz$host=ulv.no,image");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*example\\.com[^a-zA-Z0-9_.%-].*advert",
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*foo\\.com\\/bar",
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*evil\\.google\\.com\\/something",
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*ads\\.example\\.com\\/something",
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*baz\\.com[^a-zA-Z0-9_.%-]",
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*xxx\\.elg\\.no[^a-zA-Z0-9_.%-]",
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*ulv\\.no[^a-zA-Z0-9_.%-].*ulv\\.no\\.zzz",
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, RegexRules) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("/ad(vert)?[0-9]$/$image");
  rule_parser.Parse("/ba{1-3}d/$image");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "ad(vert)?[0-9]$",
              "resource-type": [
                "image"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, DocumentRulesWithoutStrictBlocking) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("evil$document");
  rule_parser.Parse("dangerous$script,document");
  rule_parser.Parse("@@good$image,document");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "dangerous",
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      },
      "allow" : [
        {
          "trigger": {
            "url-filter": "good",

            "resource-type": [
              "image"
            ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        },
        {
          "trigger": {
            "url-filter": ".*",
            "if-top-url": [ "good" ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        }
      ]
    },
    "cosmetic": {
      "allow": {
        "generic" : [ {
          "trigger": {
            "url-filter": ".*",
            "if-top-url": [ "good" ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        } ],
        "specific" : [ {
          "trigger": {
            "url-filter": ".*",
            "if-top-url": [ "good" ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        } ]
      }
    },
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, DocumentRulesWithStrictBlocking) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("evil$document");
  rule_parser.Parse("dangerous$script,document");
  rule_parser.Parse("@@good$image,document");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "evil",
              "resource-type": [
                "top-document"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "dangerous",
              "resource-type": [
                "script",
                "top-document"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      },
      "allow" : [
        {
          "trigger": {
            "url-filter": "good",

            "resource-type": [
              "image"
            ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        },
        {
          "trigger": {
            "url-filter": ".*",
            "if-top-url": [ "good" ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        }
      ]
    },
    "cosmetic": {
      "allow": {
        "generic" : [ {
          "trigger": {
            "url-filter": ".*",
            "if-top-url": [ "good" ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        } ],
        "specific" : [ {
          "trigger": {
            "url-filter": ".*",
            "if-top-url": [ "good" ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        } ]
      }
    },
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(true, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, OtherActivations) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("@@no.generic.blocks$genericblock");
  rule_parser.Parse("@@no.generic.hide$generichide,match-case");
  rule_parser.Parse("@@no.element.hide$elemhide");
  rule_parser.Parse("@@no.specific.hide$specifichide");
  std::string expected(R"===({
    "network": {
      "generic-allow" : [
        {
          "trigger": {
            "url-filter": ".*",
            "if-top-url": [ "no\\.generic\\.blocks" ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        }
      ]
    },
    "cosmetic": {
      "allow" : {
        "specific": [
          {
            "trigger": {
              "url-filter": ".*",
              "if-top-url": [ "no\\.element\\.hide" ]
            },
            "action": {
              "type": "ignore-previous-rules"
            }
          },
          {
            "trigger": {
              "url-filter": ".*",
              "if-top-url": [ "no\\.specific\\.hide" ]
            },
            "action": {
              "type": "ignore-previous-rules"
            }
          }
        ],
        "generic" : [
          {
            "trigger": {
              "url-filter": ".*",
              "if-top-url": [ "no\\.generic\\.hide" ],
              "top-url-filter-is-case-sensitive": true
            },
            "action": {
              "type": "ignore-previous-rules"
            }
          },
          {
            "trigger": {
              "url-filter": ".*",
              "if-top-url": [ "no\\.element\\.hide" ]
            },
            "action": {
              "type": "ignore-previous-rules"
            }
          }
        ]
      }
    },
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, RulesWithIncludedDomains) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("danger$domain=evil.com|bad.*,script");
  rule_parser.Parse("@@allowed$domain=nice.com,script");
  std::string expected(R"===({
    "network": {
      "block": {
        "specific": [
          {
            "trigger": {
              "url-filter": "danger",
              "if-top-url": [
                "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*evil\\.com[:/]",
                "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*bad\\..*[:/]"
              ],
              "top-url-filter-is-case-sensitive": true,
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      },
      "allow": [
        {
          "trigger": {
            "url-filter": "allowed",
            "if-top-url": [
              "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*nice\\.com[:/]"
            ],
            "top-url-filter-is-case-sensitive": true,
            "resource-type": [
              "script"
            ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        }
      ]
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, RulesWithExcludedDomains) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("danger$domain=~nice.com,script");
  rule_parser.Parse("@@allowed$domain=~evil.com|~/foot?.co(.uk|m)/,script");
  std::string expected(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "trigger": {
              "url-filter": "danger",
              "unless-top-url": [
                "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*nice\\.com[:/]"
              ],
              "top-url-filter-is-case-sensitive": true,
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      },
      "allow": [
        {
          "trigger": {
            "url-filter": "allowed",
            "unless-top-url": [
              "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*evil\\.com[:/]",
              "^[a-z][a-z+.-]*:\\/\\/foot?.co(.uk|m)[:/]"
            ],
            "top-url-filter-is-case-sensitive": true,
            "resource-type": [
              "script"
            ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        }
      ]
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, BlockRuleWithInclusionsAndExclusions) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse(
      "something$domain=evil.com|~nice.com|~good.evil.com|except.good.evil.com|"
      "bad.evil.com|danger.com|~safe.danger.com|~allowed.safe.danger.com,"
      "script");
  std::string expected(R"===({
    "network": {
      "block-allow-pairs": [
        [
          {
            "trigger": {
              "url-filter": "something",
              "if-top-url": [
                "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*danger\\.com[:/]",
                "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*evil\\.com[:/]"
              ],
              "top-url-filter-is-case-sensitive": true,
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          },
          {
            "trigger": {
              "url-filter": "something",
              "if-top-url": [
                "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*safe\\.danger\\.com[:/]",
                "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*good\\.evil\\.com[:/]"
              ],
              "top-url-filter-is-case-sensitive": true,
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "ignore-previous-rules"
            }
          }
        ]
      ]
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, BlockRuleWithInclusionsCancelledByExclusions) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("something$domain=example.com|~example.com");
  std::string expected(R"===({
    "network": {},
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, BlockRuleWithSuperfluousExclusions) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("something$domain=evil.com|~nice.com,script");
  std::string expected(R"===({
    "network": {
      "block": {
        "specific": [
          {
            "trigger": {
              "url-filter": "something",
              "if-top-url": [
                "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*evil\\.com[:/]"
              ],
              "top-url-filter-is-case-sensitive": true,
              "resource-type": [
                "script"
              ]
            },
            "action": {
              "type": "block"
            }
          }
        ]
      }
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, AllowRuleWithInclusionsAndExclusions) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse(
      "@@something$domain=nice.com|~bad.nice.com|except.bad.nice.com|foo."
      "except.bad.nice.com|good.com|~evil.com|,script");
  std::string expected(R"===({
    "network": {
      "allow": [
        {
          "trigger": {
            "url-filter": "something",
            "if-top-url": [
              "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*good\\.com[:/]",
              "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*nice\\.com[:/]"
             ],
            "top-url-filter-is-case-sensitive": true,
            "resource-type": [
              "script"
            ]
          },
          "action": {
            "type": "ignore-previous-rules"
          }
        }
      ]
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, GenericCosmeticHideRule) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("##.adfoo");
  rule_parser.Parse("##.adbar");
  std::string expected(R"===({
    "network": {},
    "cosmetic": {
      "selector": {
        ".adbar" : [ {
          ".type": 1
        } ],
        ".adfoo" : [ {
          ".type": 1
        } ]
      }
    },
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, SpecificCosmeticHideRule) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("example.com##.ad");
  rule_parser.Parse("example.*,~skip.example.com##.ad2");
  std::string expected(R"===({
    "network": {},
    "cosmetic": {
      "selector": {
        ".ad": [ {
          ".": {
            "com": {
              "example": {
                ".type": 1
              }
            }
          }
        } ],
        ".ad2": [ {
          ".": {
            "com": {
              "example": {
                "skip": {
                  ".type": 2
                }
              }
            }
          },
          ".*": {
            "example": {
              ".type": 1
            }
          }
        } ]
      }
    },
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, CosmeticAllowRule) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {});
  rule_parser.Parse("example.com#@#.show");
  rule_parser.Parse("#@#.nice");
  std::string expected(R"===({
    "network": {},
    "cosmetic": {
      "selector": {
        ".show": [ {
          "." : {
            "com": {
              "example" : {
                ".type": 2
              }
            }
          }
        } ],
        ".nice": [ {
          ".type": 2
        } ]
      }
    },
    "scriptlets": [],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}

TEST(AdBlockIosRuleCompilerTest, AbpSnippetRule) {
  RuleSourceSettings settings;
  ParseResult parse_result;
  RuleParser rule_parser(&parse_result, {.allow_abp_snippets = true});
  rule_parser.Parse(
      "example.com#$#log 'Some log'; abort-on-property-write atob");
  std::string expected(R"===({
    "network": {},
    "cosmetic": {},
    "scriptlets": [ {
      "domain-constraints": {
        ".": {
          "com": {
            "example" : {
              ".type": 1
            }
          }
        }
      },
      "scriptlets": {
        "abp-isolated.js": [ "[\"log\",\"Some log\"]," ],
        "abp-main.js": [ "[\"abort-on-property-write\",\"atob\"]," ]
      }
    } ],
    "version": 3
  })===");
  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(CompileIosRulesToString(false, parse_result, settings, true),
            expected);
}
}  // namespace adblock_filter
