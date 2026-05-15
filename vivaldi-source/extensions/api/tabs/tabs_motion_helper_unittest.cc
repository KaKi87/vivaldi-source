// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include "extensions/api/tabs/tabs_motion_helper.h"

#include "app/vivaldi_apptools.h"
#include "base/command_line.h"
#include "base/functional/callback_helpers.h"
#include "browser/tab_probe.h"
#include "browser/tab_strip_sanitizer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "components/ext_data/tab_ext_data_impl.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/web_contents_tester.h"
#include "extensions/vivaldi_browser_component_wrapper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vivaldi {

  // ./out/Debug/unit_tests --gtest_filter=TabsMotionHelperTest.*
namespace {
class ParamsFactory {
  public:
  ParamsFactory & SetTarget(const std::string &target) {
    target_ = target;
    return *this;
  }

  ParamsFactory & ResetTweaks(std::optional<std::string> first_tweak = std::nullopt) {
    tweaks_.clear();
    if (first_tweak) {
      tweaks_.push_back(*first_tweak);
    }
    return *this;
  }

  ParamsFactory & AddTweak(const std::string &t) {
    tweaks_.push_back(t);
    return *this;
  }


  ParamsFactory & AddSource(const std::string &src) {
    source_.push_back(src);
    return *this;
  }

  base::DictValue Build() {
    base::DictValue move_props;
    move_props.Set("target", target_);

    if (!source_.empty()) {
      base::ListValue extIds;
      for (const std::string& src : source_) {
        extIds.Append(src);
      }

      move_props.Set("extIds", std::move(extIds));
    }

    if (!tweaks_.empty()) {
      base::ListValue tweaksList;
      for (const std::string &tweak: tweaks_) {
        tweaksList.Append(tweak);
      }
      move_props.Set("tweaks", std::move(tweaksList));
    }

    return move_props;
  }
  private:
  std::string target_;
  std::vector<std::string> source_;
  std::vector<std::string> tweaks_;
};
}


class TabsMotionHelperTest : public BrowserWithTestWindowTest {
 public:
  TabsMotionHelperTest() = default;
  ~TabsMotionHelperTest() override = default;

  void SetUp() override {
    base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
    command_line.AppendSwitch(::switches::kTestType);
    ForceVivaldiRunning(true);
    BrowserWithTestWindowTest::SetUp();
    VivaldiBrowserComponentWrapper::CreateImpl();
  }

  // Helper to create a tab with Vivaldi-specific extension data.
  content::WebContents* AddTabWithExtData(Browser* browser, const std::string& ext_id) {
    std::unique_ptr<content::WebContents> contents =
        content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
    content::WebContents* raw_contents = contents.get();

    // Attach SessionTabHelper to ensure IdForTab(contents) returns a valid ID.
    sessions::SessionTabHelper::CreateForWebContents(raw_contents, base::NullCallback());

    // Initialize and set Vivaldi's TabExtData.
    TabExtData* ext_data = TabExtDataImpl::Create(raw_contents);
    ext_data->Set(TabExtKey::kExtId, ext_id);

    browser->tab_strip_model()->AppendWebContents(std::move(contents), true);
    return raw_contents;
  }

  int FindTabId(TabStripModel *tab_strip, int tab_id) {
    for (int i = 0; i < tab_strip->count(); ++i) {
      content::WebContents* contents = tab_strip->GetWebContentsAt(i);
      if (tab_id == sessions::SessionTabHelper::IdForTab(contents).id())
        return i;
    }
    return -1;
  }

  std::string GetExtIdAt(TabStripModel *tab_strip, int index) {
    auto probe = tab_probe::TabLookup(index, tab_strip);
    CHECK(probe.has_value());
    auto ext_id = tab_probe::GetExtId(*probe);
    CHECK(ext_id.has_value());
    return *ext_id;
  }

  // Helper to set a group ID for a tab.
  void SetGroupId(content::WebContents* contents, const std::string& group_id) {
    TabExtData::Get(contents)->Set(TabExtKey::kGroupId, group_id);
  }

  // Helper to set a group ID for a tab.
  void SetGroupId(int index, const std::string& group_id) {
    TabStripModel* tab_strip = browser()->tab_strip_model();
    content::WebContents* contents = tab_strip->GetWebContentsAt(index);
    CHECK(contents);
    TabExtData::Get(contents)->Set(TabExtKey::kGroupId, group_id);
  }

  void RunMoves(const TabsMotionHelper& helper) {
    TabStripModel* tab_strip = browser()->tab_strip_model();
    for (auto& m : helper.GetMoves()) {
      int source_index = FindTabId(tab_strip, m.id);
      tab_strip->MoveWebContentsAt(source_index, m.insert_index, false);
    }
  }

  TabsMotionHelper::Expected CreateHelper(base::DictValue&& move_properties,
      bool safe=false) {
    namespace mv = extensions::vivaldi::tabs_private::Move;
    base::ListValue args;
    args.Append(std::move(move_properties));
    auto params = mv::Params::Create(args);
    if (!params && safe) {
      TabsMotionHelper::Error error;
      error.error_message = "invalid arguments";
      return base::unexpected(error);
    }
    CHECK(params);
    return TabsMotionHelper::Create(std::move(*params), true);
  }

  std::string DescribeTabStrip() {
    TabStripModel* tab_strip = browser()->tab_strip_model();
    std::vector<std::string> tabs;
    for (int i = 0; i < tab_strip->count(); ++i) {
      content::WebContents* contents = tab_strip->GetWebContentsAt(i);
      TabExtData* ext = TabExtData::Get(contents);
      std::string s = ext->GetExtId() + ";";
      s += ext->GetParentExtId().value_or("") + ";";
      s += ext->GetGroupId().value_or("") + ";";
      s += ext->GetPanelId().value_or("") + ";";

      if (ext->GetWorkspaceId()) {
        s += base::NumberToString(
            static_cast<uint64_t>(ext->GetWorkspaceId().value()));
      } else {
        // ...
      }
      tabs.push_back(s);
    }
    return base::JoinString(tabs, "|");
  }

  void CreateTabs(int count) {
    browser()->tab_strip_model()->CloseAllTabs();
    for (int i = 0; i < count; ++i) {
      AddTabWithExtData(browser(), "t" + base::NumberToString(i));
    }
  }
};

// 1, 2, 3: Verify that TabStripModel, TabExtData, and TabProbe work together.
TEST_F(TabsMotionHelperTest, TabProbeResolvesTabsCorrectly) {
  content::WebContents* contents0 = AddTabWithExtData(browser(), "tab_0");
  content::WebContents* contents1 = AddTabWithExtData(browser(), "tab_1");
  SetGroupId(contents1, "group_A");
  content::WebContents* contents2 = AddTabWithExtData(browser(), "tab_2");
  SetGroupId(contents2, "group_A");

  int id0 = sessions::SessionTabHelper::IdForTab(contents0).id();
  int id1 = sessions::SessionTabHelper::IdForTab(contents1).id();

  // Test ResolveTab by tab_id
  auto probe0 = tab_probe::ResolveTab(id0);
  ASSERT_TRUE(probe0.has_value());
  EXPECT_EQ(probe0->contents, contents0);
  EXPECT_EQ(probe0->index, 0);

  // Test ResolveTabByExtId for a single tab
  auto probe_ext0 = tab_probe::ResolveTabByExtId("tab_0");
  ASSERT_TRUE(probe_ext0.has_value());
  EXPECT_EQ(probe_ext0->tab_id, id0);

  // Test ResolveTabByExtId for a group
  bool is_group = false;
  auto probe_group = tab_probe::ResolveTabByExtId("group_A", &is_group);
  ASSERT_TRUE(probe_group.has_value());
  EXPECT_TRUE(is_group);
  // It usually returns the first tab in the group.
  EXPECT_EQ(probe_group->contents, contents1);

  // Test ResolveTabs (plural)
  std::vector<int> ids = {id0, id1};
  auto probes = tab_probe::ResolveTabs(ids);
  ASSERT_EQ(probes.size(), 2u);
  EXPECT_EQ(probes[0].tab_id, id0);
  EXPECT_EQ(probes[1].tab_id, id1);
}

// ./out/Debug/unit_tests --gtest_filter=TabsMotionHelperTest.*
// out/Debug/gen/vivaldi/extensions/schema/tabs_private.h

// 4: Placeholder for TabsMotionHelper tests.
TEST_F(TabsMotionHelperTest, TabsMotionHelperMoveInBetween) {
  namespace mv = extensions::vivaldi::tabs_private::Move;
  CreateTabs(6);
  base::DictValue move_props;
  move_props.Set("target", 2);
  base::ListValue extIds;
  extIds.Append("t0");
  extIds.Append("t5");
  move_props.Set("extIds", std::move(extIds));
  auto helper_or_error = CreateHelper(std::move(move_props));
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(), "t1;;;;|t2;;;;|t0;;;;|t5;;;;|t3;;;;|t4;;;;");
}

TEST_F(TabsMotionHelperTest, TabsMotionHelperMoveSpecial) {
  namespace mv = extensions::vivaldi::tabs_private::Move;
  CreateTabs(5);
  {
    base::DictValue move_props;
    move_props.Set("target", "left");
    base::ListValue extIds;
    extIds.Append("t3");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_TRUE(helper_or_error.has_value());
    RunMoves(*helper_or_error.value());
    ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t1;;;;|t3;;;;|t2;;;;|t4;;;;");
  }
  {
    base::DictValue move_props;
    move_props.Set("target", "right");
    base::ListValue extIds;
    extIds.Append("t1");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_TRUE(helper_or_error.has_value());
    RunMoves(*helper_or_error.value());
    ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t3;;;;|t1;;;;|t2;;;;|t4;;;;");
  }
  {
    base::DictValue move_props;
    move_props.Set("target", "right");
    base::ListValue extIds;
    extIds.Append("t4");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_FALSE(helper_or_error.has_value());
  }
  {
    base::DictValue move_props;
    move_props.Set("target", "left");
    base::ListValue extIds;
    extIds.Append("t0");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_FALSE(helper_or_error.has_value());
  }

  bool is_group;
  auto probe = tab_probe::ResolveTabByExtId("t1", &is_group);
  ASSERT_TRUE(probe.has_value());
  ASSERT_FALSE(is_group);
  TabExtData::Get(probe->contents)->Set(TabExtKey::kWorkspaceId, double(7));
  ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t3;;;;|t1;;;;7|t2;;;;|t4;;;;");

  {
    base::DictValue move_props;
    move_props.Set("target", "left");
    base::ListValue extIds;
    extIds.Append("t2");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_TRUE(helper_or_error.has_value());
    RunMoves(*helper_or_error.value());
    ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t2;;;;|t3;;;;|t1;;;;7|t4;;;;");
  }

  {
    base::DictValue move_props;
    move_props.Set("target", "right");
    base::ListValue extIds;
    extIds.Append("t3");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_TRUE(helper_or_error.has_value());
    RunMoves(*helper_or_error.value());
    ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t2;;;;|t1;;;;7|t4;;;;|t3;;;;");
  }
}

TEST_F(TabsMotionHelperTest, SanitizeGroupSplit) {
  CreateTabs(8);
  SetGroupId(1, "a");
  SetGroupId(2, "a");
  SetGroupId(4, "a");
  SetGroupId(5, "a");
  SetGroupId(6, "a");
  ::vivaldi::SanitizeGroupSplit(browser()->tab_strip_model());
  ASSERT_EQ(DescribeTabStrip(),
            "t0;;;;|t1;;;;|t2;;;;|t3;;;;|t4;;a;;|t5;;a;;|t6;;a;;|t7;;;;");

  CreateTabs(8);
  SetGroupId(1, "a");
  SetGroupId(2, "a");
  SetGroupId(3, "a");
  SetGroupId(5, "a");
  SetGroupId(6, "a");
  ::vivaldi::SanitizeGroupSplit(browser()->tab_strip_model());
  ASSERT_EQ(DescribeTabStrip(),
            "t0;;;;|t1;;a;;|t2;;a;;|t3;;a;;|t4;;;;|t5;;;;|t6;;;;|t7;;;;");
}

TEST_F(TabsMotionHelperTest, SanitizeGroupst) {
  CreateTabs(7);
  SetGroupId(1, "b");
  SetGroupId(3, "a");
  SetGroupId(4, "a");
  SetGroupId(5, "c");
  ::vivaldi::SanitizeGroups(browser()->tab_strip_model());
  ASSERT_EQ(DescribeTabStrip(),
      "t0;;;;|t1;;;;|t2;;;;|t3;;a;;|t4;;a;;|t5;;;;|t6;;;;");
}

// Move tab above/below another tab.
TEST_F(TabsMotionHelperTest, BelowAbove) {
  CreateTabs(5);
  ParamsFactory factory;
  factory.SetTarget("t1")
    .AddSource("t4")
    .ResetTweaks("above");
  auto helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(),
      "t0;;;;|t4;;;;|t1;;;;|t2;;;;|t3;;;;");
  CreateTabs(5);
  factory.ResetTweaks("below");
  helper_or_error = CreateHelper(factory.Build());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(),
      "t0;;;;|t1;;;;|t4;;;;|t2;;;;|t3;;;;");
  factory.AddTweak("above");
  helper_or_error = CreateHelper(factory.Build(), true);
  ASSERT_FALSE(helper_or_error.has_value());
}

// Move tab into the group.
TEST_F(TabsMotionHelperTest, AutoStack) {
  CreateTabs(6);
  SetGroupId(1, "a");
  SetGroupId(2, "a");
  ParamsFactory factory;
  factory.SetTarget("t0")
    .AddSource("t5")
    .ResetTweaks("below");
  auto helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t5;;;;|t1;;a;;|t2;;a;;|t3;;;;|t4;;;;");
  ASSERT_FALSE(helper_or_error.value()->SuggestGroup());
  factory.SetTarget("t2").ResetTweaks("above");
  helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t1;;a;;|t5;;;;|t2;;a;;|t3;;;;|t4;;;;");
  ASSERT_EQ(helper_or_error.value()->SuggestGroup().value_or("n/a"), "a");
}

// Move tab above/below the stack target.
TEST_F(TabsMotionHelperTest, BelowAboveStack) {
  CreateTabs(7);
  SetGroupId(3, "a");
  SetGroupId(4, "a");
  ParamsFactory factory;
  factory.SetTarget("a")
    .AddSource("t0")
    .AddSource("t6");
  factory.ResetTweaks("above");
  auto helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(), "t1;;;;|t2;;;;|t0;;;;|t6;;;;|t3;;a;;|t4;;a;;|t5;;;;");
  ASSERT_FALSE(helper_or_error.value()->SuggestGroup());
  factory.ResetTweaks("below");
  helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(), "t1;;;;|t2;;;;|t3;;a;;|t4;;a;;|t0;;;;|t6;;;;|t5;;;;");
  ASSERT_FALSE(helper_or_error.value()->SuggestGroup());
}

}  // namespace vivaldi
