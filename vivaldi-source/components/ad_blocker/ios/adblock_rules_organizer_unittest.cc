#include "components/ad_blocker/ios/adblock_rules_organizer.h"

#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
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

std::string FormatRules(std::initializer_list<std::string> rules) {
  base::ListValue list;
  for (auto& rule : rules) {
    JSONStringValueDeserializer deserializer(rule);
    int error_code;
    std::string error;
    std::unique_ptr<base::Value> value =
        deserializer.Deserialize(&error_code, &error);
    if (!value) {
      LOG(FATAL) << error_code << ":" << error;
    }

    list.Append(std::move(*value));
  }
  std::string result;
  JSONStringValueSerializer serializer(&result);
  CHECK(serializer.Serialize(list));
  base::Value output(std::move(result));
  JSONStringValueSerializer serializer2(&result);
  serializer2.set_pretty_print(true);
  CHECK(serializer2.Serialize(output));
  return result;
}

std::string PrettyPrint(const base::Value& value) {
  std::string output;
  JSONStringValueSerializer serializer(&output);
  serializer.set_pretty_print(true);
  CHECK(serializer.Serialize(value));

  return output;
}

scoped_refptr<CompiledRules> BuildCompiledRules(std::string rules,
                                                std::string checksum) {
  JSONStringValueDeserializer deserializer(rules);

  int error_code;
  std::string error;
  std::unique_ptr<base::Value> value =
      deserializer.Deserialize(&error_code, &error);
  if (!value) {
    LOG(FATAL) << error_code << ":" << error;
  }

  return new CompiledRules(std::move(*value), checksum);
}
}  // namespace

TEST(AdBlockRulesOrganizerTest, Empty) {
  base::Value result = OrganizeRules({}, {});
  std::string expected(R"===({
    "ios-content-blocker-rules": [],
    "non-ios-rules-and-metadata": {
      "metadata": {},
      "partner-list-allowed-documents": [],
      "scriptlets": {
        "domain-constraints": {
          ".": {},
          ".*": {},
          "/": {},
          "~/": {}
        },
        "scriptlets": []
      },
      "version": 3
    }
  })===");

  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(PrettyPrint(result), expected);
}

TEST(AdBlockRulesOrganizerTest, NetworkRules) {
  std::map<uint32_t, scoped_refptr<CompiledRules>> rules;

  // For this test, we use dummy "rules", which are just a dictionary containing
  // some identifier. Since network rules are expected to be copied over as-is,
  // without checking their content, this is sufficient to check that everything
  // behaves as expected.
  rules.insert({1U, BuildCompiledRules(R"===({
    "network": {
      "block": {
        "specific": [
          {
            "test_id": 100
          },
          {
            "test_id": 101
          }
        ],
        "generic": [
          {
            "test_id": 200
          },
          {
            "test_id": 201
          }
        ]
      },
      "allow" : [
        {
          "test_id": 300
        },
        {
          "test_id": 301
        }
      ],
      "generic-allow" : [
        {
          "test_id": 400
        },
        {
          "test_id": 401
        }
      ]
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===",
                                       "dummy1")});

  rules.insert({2U, BuildCompiledRules(R"===({
    "network": {
      "block": {
        "specific": [
          {
            "test_id": 105
          },
          {
            "test_id": 106
          }
        ]
      },
      "block-allow-pairs": [
        [
          {
            "test_id": 501
          },
          {
            "test_id": 502
          }
        ],
        [
          {
            "test_id": 503
          },
          {
            "test_id": 504
          }
        ]
      ],
      "allow" : [
        {
          "test_id": 305
        },
        {
          "test_id": 306
        }
      ]
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===",
                                       "dummy2")});

  rules.insert({3U, BuildCompiledRules(R"===({
    "network": {
      "block": {
        "generic": [
          {
            "test_id": 205
          },
          {
            "test_id": 206
          }
        ]
      },
      "block-allow-pairs": [
        [
          {
            "test_id": 505
          },
          {
            "test_id": 506
          }
        ]
      ],
      "generic-allow" : [
        {
          "test_id": 405
        }
      ]
    },
    "cosmetic": {},
    "scriptlets": [],
    "version": 3
  })===",
                                       "dummy3")});

  base::Value result = OrganizeRules({rules}, {});
  std::string expected(R"===({
    "ios-content-blocker-rules": [
      "[{\"test_id\":505},{\"test_id\":506},{\"test_id\":503},{\"test_id\":504},{\"test_id\":501},{\"test_id\":502},{\"test_id\":100},{\"test_id\":101},{\"test_id\":105},{\"test_id\":106},{\"test_id\":300},{\"test_id\":301},{\"test_id\":305},{\"test_id\":306},{\"action\":{\"type\":\"block\"},\"trigger\":{\"url-filter\":\"^[a-z][a-z+.-]*:\\\\/\\\\/[^/]*@\"}}]",
      "[{\"test_id\":200},{\"test_id\":201},{\"test_id\":205},{\"test_id\":206},{\"test_id\":300},{\"test_id\":301},{\"test_id\":400},{\"test_id\":401},{\"test_id\":305},{\"test_id\":306},{\"test_id\":405},{\"action\":{\"type\":\"block\"},\"trigger\":{\"url-filter\":\"^[a-z][a-z+.-]*:\\\\/\\\\/[^/]*@\"}}]"
    ],
    "non-ios-rules-and-metadata": {
      "metadata": {
        "list-checksums": {
          "1": "dummy1",
          "2": "dummy2",
          "3": "dummy3"
        }
      },
      "partner-list-allowed-documents": [],
      "scriptlets": {
        "domain-constraints": {
          ".": {},
          ".*": {},
          "/": {},
          "~/": {}
        },
        "scriptlets": []
      },
      "version": 3
    }
  })===");

  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(PrettyPrint(result), expected);
}

TEST(AdBlockRulesOrganizerTest, CosmeticRules) {
  std::map<uint32_t, scoped_refptr<CompiledRules>> rules;
  rules.insert({1U, BuildCompiledRules(R"===({
    "network": {},
    "cosmetic": {
      "selector": {
        ".ad1": [ {
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
                ".type": 1
              }
            }
          },
          ".*": {
            "example": {
              ".type": 1
            }
          }
        } ],
        ".ad3": [ {
          ".": {
            "com": {
              "example": {
                ".type": 1,
                "skip": {
                  ".type": 2
                }
              }
            }
          },
          "/": {
            "foo\\..*\\.bar\\.com": {
              ".type": 1
            }
          }
        }, {
          ".": {
            "com": {
              "evil": {
                ".type": 1
              }
            }
          }
        } ],
        ".ad4": [ {
          ".*": {
            "example": {
              ".type": 1
            }
          }
        } ],
        ".ad5": [ {
          ".type": 1,
          ".*": {
            "something": {
              ".type": 2
            }
          }
        } ],
        ".ad6": [ {
          ".type": 1,
          ".*": {
            "something": {
              "foo": {
                ".type": 2
              }
            }
          }
        }, {
          ".*": {
            "something": {
              ".type": 1
            }
          }
        } ],
        ".ad7": [ {
          ".*": {
            "example": {
              ".type": 1
            }
          }
        } ]
      },
      "allow": {
        "specific": [
          {
            "test_rule_id": 101
          },
          {
            "test_rule_id": 102
          }
        ],
        "generic": [
          {
            "test_rule_id": 201
          }
        ]
      }
    },
    "scriptlets": [],
    "version": 3
  })===",
                                       "dummy1")});

  rules.insert({2U, BuildCompiledRules(R"===({
    "network": {},
    "cosmetic": {
      "selector": {
        ".ad1": [ {
          ".": {
            "com": {
              "evil": {
                ".type": 1
              }
            }
          }
        } ],
        ".ad2": [ {
          ".*": {
            "example": {
              "skip": {
                ".type": 2
              }
            }
          }
        } ],
        ".ad4": [ {
          ".type": 2
        } ],
        ".ad5": [ {
          ".": {
            "com": {
              "example": {
                ".type": 1
              }
            }
          }
        } ],
        ".ad6": [ {
          ".type": 1,
          ".*": {
            "example": {
              ".type": 2
            }
          }
        } ],
        ".ad7": [ {
          ".type": 1
        } ],
        ".ad8": [ {
          ".type": 1
        } ]
      },
      "allow": {
        "specific": [
          {
            "test_rule_id": 105
          }
        ],
        "generic": [
          {
            "test_rule_id": 205
          },
          {
            "test_rule_id": 206
          }
        ]
      }
    },
    "scriptlets": [],
    "version": 3
  })===",
                                       "dummy2")});

  base::Value result = OrganizeRules({rules}, {});
  std::string expected_specific_rules = FormatRules({R"===({
                "trigger": {
                  "url-filter": ".*",
                  "if-frame-url": [
                    "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*evil\\.com[:/]",
                    "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*example\\.com[:/]"
                  ],
                  "frame-url-filter-is-case-sensitive": true
                },
                "action": {
                  "type": "css-display-none",
                  "selector": ".ad1"
                }
              })===",
                                                     R"===({
                "test_rule_id": 101
              })===",
                                                     R"===({
                "test_rule_id": 102
              })===",
                                                     R"===({
                "test_rule_id": 105
              })==="});
  std::string expected_generic_rules = FormatRules({R"===({
                "trigger": {
                  "url-filter": ".*",
                  "unless-frame-url": [
                    "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*something\\..*[:/]"
                  ],
                  "frame-url-filter-is-case-sensitive": true
                },
                "action": {
                  "type": "css-display-none",
                  "selector": ".ad5"
                }
              })===",
                                                    R"===({
                "trigger": {
                  "url-filter": ".*",
                  "unless-frame-url": [
                    "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*example\\..*[:/]",
                    "^[a-z][a-z+.-]*:\\/\\/([^/]*\\.)*foo\\.something\\..*[:/]"
                  ],
                  "frame-url-filter-is-case-sensitive": true
                },
                "action": {
                  "type": "css-display-none",
                  "selector": ".ad6"
                }
              })===",
                                                    R"===({
                "trigger": {
                  "url-filter": ".*"
                },
                "action": {
                  "type": "css-display-none",
                  "selector": ".ad7, .ad8"
                }
              })===",
                                                    R"===({
                "test_rule_id": 201
              })===",
                                                    R"===({
                "test_rule_id": 205
              })===",
                                                    R"===({
                "test_rule_id": 206
              })==="});

  std::string expected(R"===({
    "ios-content-blocker-rules": [
  )===");
  expected.append(expected_specific_rules);
  expected.append(",");
  expected.append(expected_generic_rules);
  expected.append(R"===(
    ],
    "non-ios-rules-and-metadata": {
      "metadata": {
        "list-checksums": {
          "1": "dummy1",
          "2": "dummy2"
        }
      },
      "partner-list-allowed-documents": [],
      "scriptlets": {
        "domain-constraints": {
          ".": {},
          ".*": {},
          "/": {},
          "~/": {}
        },
        "scriptlets": []
      },
      "version": 3
    }
  })===");

  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(PrettyPrint(result), expected);
}

TEST(AdBlockRulesOrganizerTest, SnippetRules) {
  std::map<uint32_t, scoped_refptr<CompiledRules>> rules;

  rules.insert({1U, BuildCompiledRules(R"===({
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
        },
        ".*": {
          "foo": {
            ".type": 1
          },
          "bar": {
            "baz": {
              ".type": 1
            }
          }
        }
      },
      "scriptlets": {
        "s1": [ "a1", "a2" ],
        "s2": [ "a3" ]
      }
    },
    {
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
        "s3": [ "a4", "a5" ]
      }
    },
    {
      "domain-constraints": {
        ".*": {
          "bar": {
            ".type": 1
          }
        }
      },
      "scriptlets": {
        "s4": [ "a10", "a11", "a12" ],
        "s5": [ "a13" ]
      }
    },
    {
      "domain-constraints": {
        ".": {
          "com": {
            "foobar" : {
              ".type": 1
            }
          }
        }
      },
      "scriptlets": {
        "s7": [ "a15" ]
      }
    } ],
    "version": 3
  })===",
                                       "dummy1")});

  rules.insert({2U, BuildCompiledRules(R"===({
    "network": {},
    "cosmetic": {},
    "scriptlets": [ {
      "domain-constraints": {
        ".*": {
          "foo": {
            ".type": 2
          },
          "bar": {
            ".type": 2
          }
        }
      },
      "scriptlets": {
        "s1": [ "a1", "a2" ],
        "s2": [ "a3" ]
      }
    },
    {
      "domain-constraints": {
        ".": {
          "com": {
            "example": {
              "skip": {
                ".type": 2
              }
            }
          }
        }
      },
      "scriptlets": {
        "s3": [ "a4", "a5" ]
      }
    },
    {
      "domain-constraints": {
        ".type": 2
      },
      "scriptlets": {
        "s4": [ "a10", "a11", "a12" ],
        "s5": [ "a13" ]
      }
    } ],
    "version": 3
  })===",
                                       "dummy2")});

  base::Value result = OrganizeRules(rules, {});
  std::string expected(R"===({
    "ios-content-blocker-rules": [],
    "non-ios-rules-and-metadata": {
      "metadata": {
        "list-checksums": {
          "1": "dummy1",
          "2": "dummy2"
        }
      },
      "partner-list-allowed-documents": [],
      "scriptlets": {
        "domain-constraints": {
          ".": {
            "com": {
              "example": {
                ".included": [ 0, 1 ],
                "skip": {
                  ".excluded": [ 1 ]
                }
              },
              "foobar": {
                ".included": [ 2 ]
              }
            }
          },
          ".*": {
            "foo": {
              ".excluded": [ 0 ]
            },
            "bar": {
              ".excluded": [ 0 ]
            }
          },
          "/": {},
          "~/": {}
        },
        "scriptlets": [
          {
            "s1": [ "a1", "a2" ],
            "s2": [ "a3" ]
          }, {
            "s3": [ "a4", "a5" ]
          },
          {
            "s7": [ "a15" ]
          }
        ]
      },
      "version": 3
    }
  })===");

  ASSERT_TRUE(FormatJSON(expected));
  EXPECT_EQ(PrettyPrint(result), expected);
}
}  // namespace adblock_filter
