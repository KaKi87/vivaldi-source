// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include "extensions/api/tabs_private/tabs_motion_helper.h"

#include "app/vivaldi_apptools.h"
#include "base/command_line.h"
#include "base/functional/callback_helpers.h"
#include "browser/tab_probe.h"
#include "browser/tab_strip_sanitizer.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "components/ext_data/tab_ext_data_impl.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/web_contents_tester.h"
#include "extensions/vivaldi_browser_component_wrapper.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "browser/tab_positioning.h"
#include "components/ext_data/tab_positioning_params.h"
#include "extensions/api/tabs_private/tests/unit_test_snapshots.h"

// ./out/Debug/unit_tests --gtest_filter=TabsMotionHelperTest.*
namespace tpos = ::vivaldi::tab_positioning;
namespace vivaldi {

namespace {

constexpr tpos::TabPlacingStrategy kStrategies[] = {
    tpos::TabPlacingStrategy::kDirectRightOfCurrent,
    tpos::TabPlacingStrategy::kRightOfCurrent,
    tpos::TabPlacingStrategy::kAlwaysLast,
    tpos::TabPlacingStrategy::kOpenInTabstackWithRelated};

std::string DescribeStrategy(tpos::TabPlacingStrategy s) {
  switch (s) {
    case tpos::TabPlacingStrategy::kRightOfCurrent:
      return "RightOfCurrent";
    case tpos::TabPlacingStrategy::kDirectRightOfCurrent:
      return "DirectRightOfCurrent";
    case tpos::TabPlacingStrategy::kAlwaysLast:
      return "AlwaysLast";
    case tpos::TabPlacingStrategy::kOpenInTabstackWithRelated:
      return "OpenInTabstackWithRelated";
    default:
      NOTREACHED();
  }
}

std::string DescribeStackMode(tpos::TabstackMode s) {
  switch (s) {
    case tpos::TabstackMode::kOff:
      return "Off";
    case tpos::TabstackMode::kDotted:
      return "Dotted";
    case tpos::TabstackMode::kSubstrip:
      return "Substrip";
    case tpos::TabstackMode::kAccordion:
      return "Accordion";
    case tpos::TabstackMode::kUnknown:
      return "Unknown";
  }
}

std::string DescribeInvokedBy(TabInvokedBy invoked_by) {
  switch (invoked_by) {
    case TabInvokedBy::kNone:
      return "None";
    case TabInvokedBy::kMainStrip:
      return "MainStrip";
    case TabInvokedBy::kSubStrip:
      return "SubStrip";
    case TabInvokedBy::kKeyboard:
      return "Keyboard";
    case TabInvokedBy::kAccordion:
      return "Accordion";
    case TabInvokedBy::kTabBarButton:
      return "TabBarButton";
    case TabInvokedBy::kChromiumExtension:
      return "ChromiumExtension";
    case TabInvokedBy::kEmailUi:
      return "EmailUi";
    case TabInvokedBy::kHtml:
      return "Html";
    case TabInvokedBy::kBackground:
      return "Background";
    case TabInvokedBy::kBookmarks:
      return "Bookmarks";
    case TabInvokedBy::kSpeedDial:
      return "SpeedDial";
    case TabInvokedBy::kCommand:
      return "Command";
    case TabInvokedBy::kEmailLinkBackground:
      return "EmailLinkBackground";
    case TabInvokedBy::kEmailLink:
      return "EmailLink";
    case TabInvokedBy::kPanelLinkBackground:
      return "PanelLinkBackground";
    case TabInvokedBy::kPanelLink:
      return "PanelLink";
    case TabInvokedBy::kVivaldiUi:
      return "vivaldiUi";
    case TabInvokedBy::kDownload:
      return "Download";
  }
}

void SetCommand(content::WebContents* contents, const std::string& command) {
  if (command.empty())
    return;
  TabExtData* ext = TabExtData::Get(contents);
  TabPositioningParams positional_params = ext->GetPositioningParams();
  positional_params.invoked_by_extra_arg = "{ \"type\": \"" + command + "\" }";
  positional_params.invoked_by = TabInvokedBy::kCommand;
  ext->SetPositioningParams(positional_params);
}

class ParamsFactory {
 public:
  ParamsFactory& SetTarget(const std::string& target) {
    target_tab_ = target;
    target_index_ = std::nullopt;
    return *this;
  }

  ParamsFactory& SetTarget(int index) {
    target_tab_ = std::nullopt;
    target_index_ = index;
    return *this;
  }

  ParamsFactory& ResetTweaks(
      std::optional<std::string> first_tweak = std::nullopt) {
    tweaks_.clear();
    if (first_tweak) {
      tweaks_.push_back(*first_tweak);
    }
    return *this;
  }

  ParamsFactory& AddTweak(const std::string& t) {
    tweaks_.push_back(t);
    return *this;
  }

  ParamsFactory& SetWorkspaceId(double workspace_id) {
    workspace_id_ = workspace_id;
    return *this;
  }

  ParamsFactory& AddSource(const std::string& src) {
    source_.push_back(src);
    return *this;
  }

  base::DictValue Build() {
    base::DictValue move_props;
    if (target_tab_) {
      CHECK(!target_index_);
      move_props.Set("target", *target_tab_);
    }

    if (target_index_) {
      CHECK(!target_tab_);
      move_props.Set("target", *target_index_);
    }

    if (workspace_id_) {
      move_props.Set("workspaceId", *workspace_id_);
    }

    if (!source_.empty()) {
      base::ListValue extIds;
      for (const std::string& src : source_) {
        extIds.Append(src);
      }

      move_props.Set("extIds", std::move(extIds));
    }

    if (!tweaks_.empty()) {
      base::ListValue tweaksList;
      for (const std::string& tweak : tweaks_) {
        tweaksList.Append(tweak);
      }
      move_props.Set("tweaks", std::move(tweaksList));
    }

    return move_props;
  }

 private:
  std::optional<std::string> target_tab_;
  std::optional<int> target_index_;
  std::vector<std::string> source_;
  std::vector<std::string> tweaks_;
  std::optional<double> workspace_id_;
};

class SnapshotSampler {
 public:
  void Snap(int active,
            std::string prefix,
            tpos::TabPlacingStrategy strategy,
            int index) {
    Snap(base::NumberToString(active) + ";" + prefix + ";" +
                    DescribeStrategy(strategy) + ";" +
                    base::NumberToString(index));
  }

  void Snap(const std::string &s) {
    CHECK(samples.find(s) == samples.end())
        << "The sample is not unique: " << s;
    samples.insert(s);
    if (expected.find(s) == expected.end()) {
      errors.push_back(s);
    } else {
      expected.erase(s);
    }
  }

  void Dump() const {
    LOG(INFO) << "Double-check before you replace an invalid snapshot!!!";
    LOG(INFO) << "VALID SNAPSHOT START";
    for (auto sample : samples) {
      LOG(INFO) << "\"" << sample << "\"" << ",";
    }
    LOG(INFO) << "VALID SNAPSHOT END";
  }

  void Check() {
    if (!errors.empty() || !expected.empty()) {
      for (auto err : errors) {
        ADD_FAILURE() << "Invalid sample: " << err;
      }
      for (auto err : expected) {
        ADD_FAILURE() << "Missing sample: " << err;
      }
      Dump();
    }
  }

  std::set<std::string> expected;
  std::set<std::string> samples;
  std::vector<std::string> errors;
};

}  // namespace

class TabsMotionHelperTest : public BrowserWithTestWindowTest {
 public:
  TabsMotionHelperTest() = default;
  ~TabsMotionHelperTest() override = default;

  raw_ptr<BrowserWindow> window2_;
  std::unique_ptr<Browser> browser2_;

  Browser* browser2() {
    CHECK(browser2_.get());
    return browser2_.get();
  }

  void SetUp() override {
    base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
    command_line.AppendSwitch(::switches::kTestType);
    ForceVivaldiRunning(true);
    BrowserWithTestWindowTest::SetUp();
    VivaldiBrowserComponentWrapper::CreateImpl();

    auto window2 = CreateBrowserWindow();
    window2_ = window2.get();
    browser2_ = CreateBrowser(profile(), Browser::TYPE_NORMAL, false,
                              window2.release());
  }

  void TearDown() {
    window2_ = nullptr;
    if (browser2_) {
      browser2_->tab_strip_model()->CloseAllTabs();
      browser2_->GetFeatures().TearDownPreBrowserWindowDestruction();
      browser2_.reset();
    }
    BrowserWithTestWindowTest::TearDown();
  }

  int GetWindowIdOfTab(const std::string& extId) {
    auto probe = tab_probe::ResolveTabByExtId(extId);
    CHECK(probe);
    return ::extensions::ExtensionTabUtil::GetWindowIdOfTab(probe->contents);
  }

  // Helper to create a tab with Vivaldi-specific extension data.
  content::WebContents* AddTabWithExtData(Browser* browser,
                                          const std::string& ext_id) {
    std::unique_ptr<content::WebContents> contents =
        content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
    content::WebContents* raw_contents = contents.get();

    // Attach SessionTabHelper to ensure IdForTab(contents) returns a valid ID.
    sessions::SessionTabHelper::CreateForWebContents(raw_contents,
                                                     base::NullCallback());

    // Initialize and set Vivaldi's TabExtData.
    TabExtData* ext_data = TabExtDataImpl::Create(raw_contents);
    ext_data->Set(TabExtKey::kExtId, ext_id);

    browser->tab_strip_model()->AppendWebContents(std::move(contents), true);
    return raw_contents;
  }

  int FindTabId(TabStripModel* tab_strip, int tab_id) {
    for (int i = 0; i < tab_strip->count(); ++i) {
      content::WebContents* contents = tab_strip->GetWebContentsAt(i);
      if (tab_id == sessions::SessionTabHelper::IdForTab(contents).id())
        return i;
    }
    return -1;
  }

  std::string GetExtIdAt(TabStripModel* tab_strip, int index) {
    auto probe = tab_probe::TabLookup(index, tab_strip);
    CHECK(probe.has_value());
    auto ext_id = tab_probe::GetExtId(*probe);
    CHECK(ext_id.has_value());
    return *ext_id;
  }

  // Helper to set a group ID for a tab.
  void SetGroupId(content::WebContents* contents, const std::string& group_id) {
    TabExtData::Get(contents)->SetForTesting(
        TabExtKey::kGroupId_, base::Value(std::string(group_id)));
  }

  // Helper to set a group ID for a tab.
  void SetGroupId(int index, const std::string& group_id) {
    TabStripModel* tab_strip = browser()->tab_strip_model();
    content::WebContents* contents = tab_strip->GetWebContentsAt(index);
    CHECK(contents);
    TabExtData::Get(contents)->SetForTesting(
        TabExtKey::kGroupId_, base::Value(std::string(group_id)));
  }

  void RunMoves(const TabsMotionHelper& helper) {
    TabStripModel* tab_strip = browser()->tab_strip_model();
    for (auto& m : helper.GetMoves()) {
      int source_index = FindTabId(tab_strip, m.id);
      tab_strip->MoveWebContentsAt(source_index, m.insert_index, false);
    }
  }

  TabsMotionHelper::Expected CreateHelper(base::DictValue&& move_properties,
                                          bool safe = false) {
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

  std::string DescribeTabStrip(int browser_index = 0) {
    Browser* browser_ptr = nullptr;
    if (browser_index == 0) {
      browser_ptr = browser();
    } else if (browser_index == 1) {
      browser_ptr = browser2();
    }

    CHECK(browser_ptr);
    TabStripModel* tab_strip = browser_ptr->tab_strip_model();
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

  void CreateTabs2(int count) {
    browser2()->tab_strip_model()->CloseAllTabs();
    for (int i = 0; i < count; ++i) {
      AddTabWithExtData(browser2(), "s" + base::NumberToString(i));
    }
  }

  struct SnapshotContent {
    bool reparent_id = true;
  };

  // Move every tab everywhere, all the combinations.
  void
  CheckMoveSnapshotScenario(
      const SnapshotContent& content,
      std::set<std::string> expected,
      std::function<void(ParamsFactory*)> setup_factory_fn) {
    CreateTabs(10);
    SetGroupId(1, "a");
    SetGroupId(2, "a");
    SetGroupId(3, "b");
    SetGroupId(4, "b");
    SetGroupId(6, "c");
    SetGroupId(7, "c");

    // t0 [t1 t2] [t3 t4] t5 [t6 t7] t8 t9
    SnapshotSampler sampler;
    sampler.expected =
        expected;  // vivaldi::test_snapshots::MoveSnapshotBasic();

    for (int source_index = 0; source_index < 10; ++source_index) {
      for (int target_index = 0; target_index < 10; ++target_index) {
        std::string target_tab = "t" + base::NumberToString(target_index);
        std::string source_tab = "t" + base::NumberToString(source_index);
        ParamsFactory factory;
        factory.SetTarget(target_tab).AddSource(source_tab);
        setup_factory_fn(&factory);
        auto helper_or_error = CreateHelper(factory.Build());
        std::string snapshot = source_tab + ">" + target_tab;
        if (helper_or_error.has_value()) {
          auto& helper = helper_or_error.value();
          ASSERT_EQ(helper->GetMoves().size(), 1);
          auto& m = helper->GetMoves()[0];
          snapshot += ";" + base::NumberToString(m.insert_index);
          snapshot += helper->SuggestGroup().value_or("-");
          if (content.reparent_id) {
            snapshot += ";" + helper->GetReparentId().value_or("-");
          }
        } else {
          snapshot += ";E";
        }
        sampler.Snap(snapshot);
      }
    }
    sampler.Check();
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

// VB-128777 [Tabs][Commands] "Move Active Tabs Forward" (or Backward) do not
// move past stacks
TEST_F(TabsMotionHelperTest, TabsMotionHelperShiftOverGroup) {
  namespace mv = extensions::vivaldi::tabs_private::Move;
  CreateTabs(5);
  SetGroupId(2, "a");
  SetGroupId(3, "a");

  {
    base::DictValue move_props;
    move_props.Set("target", "right");
    base::ListValue extIds;
    extIds.Append("t1");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_TRUE(helper_or_error.has_value());
    RunMoves(*helper_or_error.value());
    ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t2;;a;;|t3;;a;;|t1;;;;|t4;;;;");
  }

  {
    base::DictValue move_props;
    move_props.Set("target", "right");
    base::ListValue extIds;
    extIds.Append("t3");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_FALSE(helper_or_error.has_value());
  }

  {
    base::DictValue move_props;
    move_props.Set("target", "left");
    base::ListValue extIds;
    extIds.Append("t2");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_FALSE(helper_or_error.has_value());
  }

  {
    base::DictValue move_props;
    move_props.Set("target", "right");
    base::ListValue extIds;
    extIds.Append("t2");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_TRUE(helper_or_error.has_value());
    ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t2;;a;;|t3;;a;;|t1;;;;|t4;;;;");
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
  factory.SetTarget("t1").AddSource("t4").ResetTweaks("above");
  auto helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t4;;;;|t1;;;;|t2;;;;|t3;;;;");
  CreateTabs(5);
  factory.ResetTweaks("below");
  helper_or_error = CreateHelper(factory.Build());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t1;;;;|t4;;;;|t2;;;;|t3;;;;");
  factory.AddTweak("above");
  helper_or_error = CreateHelper(factory.Build(), true);
  ASSERT_FALSE(helper_or_error.has_value());
}

// Move to last/first
TEST_F(TabsMotionHelperTest, MoveToEdge) {
#if 0  // needs: ondrej/tab-motion-helper-refactoring
  {
    CreateTabs(5);
    ParamsFactory factory;
    factory.SetTarget("last").AddSource("t1");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    ASSERT_EQ(helper->GetTargetType(), TabsMotionHelper::TargetType::kLast);
    ASSERT_TRUE(helper->IsBelow());
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t2;;;;|t3;;;;|t4;;;;|t1;;;;");
  }
#endif
  {
    CreateTabs(5);
    ParamsFactory factory;
    factory.SetTarget("first").AddSource("t1");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    RunMoves(*helper_or_error.value());
    ASSERT_EQ(DescribeTabStrip(), "t1;;;;|t0;;;;|t2;;;;|t3;;;;|t4;;;;");
  }
}

// Move tab to index
TEST_F(TabsMotionHelperTest, MoveToIndex) {
  CreateTabs(5);
  ParamsFactory factory;
  factory.SetTarget(2).AddSource("t4");
  auto helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(), "t0;;;;|t1;;;;|t4;;;;|t2;;;;|t3;;;;");
}

// Move tab into the group.
TEST_F(TabsMotionHelperTest, AutoStack) {
  CreateTabs(6);
  SetGroupId(1, "a");
  SetGroupId(2, "a");
  ParamsFactory factory;
  factory.SetTarget("t0").AddSource("t5").ResetTweaks("below");
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
  factory.SetTarget("a").AddSource("t0").AddSource("t6");
  factory.ResetTweaks("above");
  auto helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(),
            "t1;;;;|t2;;;;|t0;;;;|t6;;;;|t3;;a;;|t4;;a;;|t5;;;;");
  ASSERT_FALSE(helper_or_error.value()->SuggestGroup());
  factory.ResetTweaks("below");
  helper_or_error = CreateHelper(factory.Build());
  ASSERT_TRUE(helper_or_error.has_value());
  RunMoves(*helper_or_error.value());
  ASSERT_EQ(DescribeTabStrip(),
            "t1;;;;|t2;;;;|t3;;a;;|t4;;a;;|t0;;;;|t6;;;;|t5;;;;");
  ASSERT_FALSE(helper_or_error.value()->SuggestGroup());
}

// The target window is decided by the target tab.
TEST_F(TabsMotionHelperTest, MultiWindowTabTarget) {
  CreateTabs(3);
  CreateTabs2(3);
  auto probe0 = tab_probe::ResolveTabByExtId("s1");
  auto probe1 = tab_probe::ResolveTabByExtId("t1");
  ASSERT_TRUE(probe0.has_value());
  ASSERT_TRUE(probe1.has_value());
  ParamsFactory factory;
  factory.AddSource("t1");

  {
    factory.SetTarget("s1");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    const TabsMotionHelper& helper = *helper_or_error.value();
    int target_window_id = helper.GetWindowId();
    ASSERT_EQ(target_window_id, GetWindowIdOfTab("s1"));
  }

  {
    factory.SetTarget("t1");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    const TabsMotionHelper& helper = *helper_or_error.value();
    int target_window_id = helper.GetWindowId();
    ASSERT_EQ(target_window_id, GetWindowIdOfTab("t1"));
  }

  // Ensure we have 2 windows (the test makes sense)
  ASSERT_NE(GetWindowIdOfTab("s1"), GetWindowIdOfTab("t1"));
}

// Target is an index + workspace. The tab must move to the window with the
// workspace.
TEST_F(TabsMotionHelperTest, WorkspaceTarget) {
  CreateTabs(4);
  CreateTabs2(4);
  auto probe0 = tab_probe::ResolveTabByExtId("s2");
  auto probe1 = tab_probe::ResolveTabByExtId("t2");
  ASSERT_TRUE(probe0.has_value());
  ASSERT_TRUE(probe1.has_value());
  TabExtData* ext = TabExtData::Get(probe0->contents);
  ext->Set(TabExtKey::kWorkspaceId, double(7));
  ext = TabExtData::Get(probe1->contents);
  ext->Set(TabExtKey::kWorkspaceId, double(8));

  ParamsFactory factory;
  factory.SetTarget(1).AddSource("s0").AddSource("t0");

  {
    factory.SetWorkspaceId(7);
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    ASSERT_EQ(helper_or_error.value()->GetWindowId(), GetWindowIdOfTab("s2"));
  }

  {
    factory.SetWorkspaceId(8);
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    ASSERT_EQ(helper_or_error.value()->GetWindowId(), GetWindowIdOfTab("t2"));
  }

  ASSERT_NE(GetWindowIdOfTab("s2"), GetWindowIdOfTab("t2"));
}

TEST_F(TabsMotionHelperTest, NewTabAllStrategiesExtApp) {
  {
    using tpos::TabPlacingStrategy;
    using tpos::TabSource;
    using tpos::TabstackMode;
    CreateTabs(8);
    SetGroupId(4, "b");
    SetGroupId(5, "b");
    SetGroupId(6, "b");
    auto* tab_strip = browser()->tab_strip_model();
    tpos::TabBarState state;
    state.source = TabSource::kExternalApp;
    std::optional<tpos::TabPosition> pos;
    int count = 0;
    for (int open_in_current_tab_stack = 0; open_in_current_tab_stack <= 1;
         open_in_current_tab_stack++) {
      state.open_in_current_tab_stack = open_in_current_tab_stack;
      for (int substrip_locked = 0; substrip_locked <= 1; ++substrip_locked) {
        state.is_substrip_locked = substrip_locked;
        for (auto stacking :
             {TabstackMode::kOff, TabstackMode::kDotted,
              TabstackMode::kSubstrip, TabstackMode::kAccordion}) {
          state.tab_stack_mode = stacking;
          tab_strip->ActivateTabAt(1);
          for (auto strategy :
               {TabPlacingStrategy::kRightOfCurrent,
                TabPlacingStrategy::kDirectRightOfCurrent,
                TabPlacingStrategy::kAlwaysLast,
                TabPlacingStrategy::kOpenInTabstackWithRelated}) {
            state.placement_strategy = strategy;
            pos = DetermineInsertionIndexFromState(
                tab_strip, tab_strip->GetWebContentsAt(1), TabSource::kGeneral,
                state);
            count++;
            ASSERT_TRUE(pos.has_value());
            ASSERT_EQ(pos->pinned, false);
            ASSERT_EQ(pos->index,
                      state.placement_strategy ==
                              TabPlacingStrategy::kDirectRightOfCurrent
                          ? 2
                          : 8);
          }

          tab_strip->ActivateTabAt(4);
          for (auto strategy :
               {TabPlacingStrategy::kRightOfCurrent,
                TabPlacingStrategy::kDirectRightOfCurrent,
                TabPlacingStrategy::kAlwaysLast,
                TabPlacingStrategy::kOpenInTabstackWithRelated}) {
            state.placement_strategy = strategy;
            count++;
            pos = DetermineInsertionIndexFromState(
                tab_strip, tab_strip->GetWebContentsAt(1), TabSource::kGeneral,
                state);
            ASSERT_TRUE(pos.has_value());
            ASSERT_EQ(pos->pinned, false);

            int exception = 7;
            if (stacking == TabstackMode::kOff)
              exception = 5;

            ASSERT_EQ(pos->index,
                      state.placement_strategy ==
                              TabPlacingStrategy::kDirectRightOfCurrent
                          ? exception
                          : 8);
          }
        }
      }
    }
    // Check the test did some work to avoid stupid typos.
    ASSERT_GT(count, 100);
  }
}

TEST_F(TabsMotionHelperTest, MoveToEdgeOfGroup) {
  {
    CreateTabs(7);
    SetGroupId(2, "a");
    SetGroupId(3, "a");
    SetGroupId(4, "a");
    SetGroupId(5, "a");
    ParamsFactory factory;
    factory.SetTarget("last").AddSource("t3");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t0;;;;|t1;;;;|t2;;a;;|t4;;a;;|t5;;a;;|t3;;a;;|t6;;;;");
  }

  {
    CreateTabs(7);
    SetGroupId(2, "a");
    SetGroupId(3, "a");
    SetGroupId(4, "a");
    SetGroupId(5, "a");
    ParamsFactory factory;
    factory.SetTarget("first").AddSource("t3");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t0;;;;|t1;;;;|t3;;a;;|t2;;a;;|t4;;a;;|t5;;a;;|t6;;;;");
  }
}

TEST_F(TabsMotionHelperTest, MoveToGroupExtId) {
  {
    CreateTabs(7);
    SetGroupId(2, "a");
    SetGroupId(3, "a");
    SetGroupId(4, "a");
    ParamsFactory factory;
    factory.SetTarget("a").AddSource("t0");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t1;;;;|t0;;;;|t2;;a;;|t3;;a;;|t4;;a;;|t5;;;;|t6;;;;");
  }

  {
    CreateTabs(7);
    SetGroupId(2, "a");
    SetGroupId(3, "a");
    SetGroupId(4, "a");
    ParamsFactory factory;
    factory.SetTarget("a").AddSource("t0").AddTweak("below");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t1;;;;|t2;;a;;|t3;;a;;|t4;;a;;|t0;;;;|t5;;;;|t6;;;;");
  }

  {
    CreateTabs(7);
    SetGroupId(2, "a");
    SetGroupId(3, "a");
    SetGroupId(4, "a");
    ParamsFactory factory;
    factory.SetTarget("t4").AddSource("t0").AddTweak("below");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t1;;;;|t2;;a;;|t3;;a;;|t4;;a;;|t0;;;;|t5;;;;|t6;;;;");
  }
}

TEST_F(TabsMotionHelperTest, MoveStackToStack) {
  {
    CreateTabs(8);
    SetGroupId(2, "a");
    SetGroupId(3, "a");
    SetGroupId(5, "b");
    SetGroupId(6, "b");
    ParamsFactory factory;
    factory
        // Put content of group a into middle of group b.
        .SetTarget("t5")
        .AddSource("a")
        .AddTweak("below");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t0;;;;|t1;;;;|t4;;;;|t5;;b;;|t2;;a;;|t3;;a;;|t6;;b;;|t7;;;;");
  }

  {
    CreateTabs(8);
    SetGroupId(5, "b");
    SetGroupId(6, "b");
    ParamsFactory factory;
    factory
        // Put content of group a into middle of group b.
        .SetTarget("t5")
        .AddSource("t2")
        .AddSource("t3")
        .AddTweak("below");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t0;;;;|t1;;;;|t4;;;;|t5;;b;;|t2;;;;|t3;;;;|t6;;b;;|t7;;;;");
  }

  {
    CreateTabs(8);
    SetGroupId(5, "b");
    SetGroupId(6, "b");
    ParamsFactory factory;
    factory
        // Put content of group a into middle of group b.
        .SetTarget(5)
        .AddSource("t2")
        .AddSource("t3")
        .AddTweak("below");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t0;;;;|t1;;;;|t4;;;;|t5;;b;;|t2;;;;|t3;;;;|t6;;b;;|t7;;;;");
    ASSERT_EQ(helper->SuggestGroup().value_or("ggg"), "b");
  }
}

TEST_F(TabsMotionHelperTest, BaseMoveOn) {
  {
    CreateTabs(4);
    ParamsFactory factory;
    factory.SetTarget("t3").AddSource("t1").AddTweak("on");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(helper->GetReparentId().value_or("-"), "t3");
  }

  {
    CreateTabs(4);
    ParamsFactory factory;
    factory.SetTarget("t3").AddSource("t1").AddTweak("below");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(helper->GetReparentId().value_or("-"), "-");
  }
}

TEST_F(TabsMotionHelperTest, MoveStripDown) {
  {
    CreateTabs(8);
    SetGroupId(2, "a");
    SetGroupId(3, "a");
    SetGroupId(5, "b");
    SetGroupId(6, "b");
    ParamsFactory factory;
    factory.SetTarget("t5").AddSource("a").AddTweak("below").AddTweak(
        "strip-down");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t0;;;;|t1;;;;|t4;;;;|t5;;b;;|t2;;a;;|t3;;a;;|t6;;b;;|t7;;;;");
    ASSERT_EQ(helper->SuggestGroup().value_or("-"), "b");
  }
  {
    CreateTabs(8);
    SetGroupId(5, "b");
    SetGroupId(6, "b");
    ParamsFactory factory;
    factory.SetTarget("t5").AddSource("t1").AddTweak("above").AddTweak(
        "strip-down");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(helper->SuggestGroup().value_or("-"), "b");
  }
  {
    CreateTabs(8);
    SetGroupId(5, "b");
    SetGroupId(6, "b");
    ParamsFactory factory;
    factory.SetTarget("t6").AddSource("t1").AddTweak("below").AddTweak(
        "strip-down");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(helper->SuggestGroup().value_or("-"), "b");
  }
}

TEST_F(TabsMotionHelperTest, MoveOnGroup) {
  {
    CreateTabs(8);
    SetGroupId(5, "b");
    SetGroupId(6, "b");
    ParamsFactory factory;
    factory.SetTarget("b").AddSource("t1").AddTweak("on");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
              "t0;;;;|t2;;;;|t3;;;;|t4;;;;|t1;;;;|t5;;b;;|t6;;b;;|t7;;;;");
    ASSERT_EQ(helper->SuggestGroup().value_or("-"), "b");
  }
}
// Test by snapshot. The main purpose of this test is to avoid regressions.
//
// If the snapshot does not match, it logs an expected
// snapshot which you can cut-paste into the code to update sampler.expected.
//
// The snapshot contains tabs placed by jscommands:
//  * "About vivaldi" (and the tabs invoked from Help > ... menu)
//  * VB-126235 the tab placed by the mouse gesture
//  * and ordinary tab placed by chrome.tabs.create()
TEST_F(TabsMotionHelperTest, NewTabByCommandPosition) {
  using tpos::TabBarState;
  using tpos::TabPlacingStrategy;
  using tpos::TabSource;
  using tpos::TabstackMode;
  constexpr int last_index = 10;
  CreateTabs(last_index);
  SetGroupId(4, "b");
  SetGroupId(5, "b");
  SetGroupId(6, "b");
  auto* tab_strip = browser()->tab_strip_model();
  TabBarState state;
  state.source = TabSource::kGeneral;
  std::optional<tpos::TabPosition> pos;
  state.tab_stack_mode = TabstackMode::kSubstrip;

  SnapshotSampler sampler;
  sampler.expected = vivaldi::test_snapshots::NewTabByCommandPosition();

  for (std::string command : {"INFO_PAGE_TAG", "COMMAND_NEW_TAB_LINK", ""}) {
    for (int active_tab : {5, 2}) {
      tab_strip->ActivateTabAt(active_tab);
      for (auto strategy : kStrategies) {
        state.placement_strategy = strategy;
        std::unique_ptr<content::WebContents> new_contents =
            content::WebContentsTester::CreateTestWebContents(profile(),
                                                              nullptr);
        TabExtDataImpl::Create(new_contents.get());
        SetCommand(new_contents.get(), command);
        pos = DetermineInsertionIndexFromState(tab_strip, new_contents.get(),
                                               TabSource::kGeneral, state);
        sampler.Snap(active_tab, command, strategy, pos->index);
      }
    }
  }
  sampler.Check();
}

// VB-129239 [regression] "New top level tab" command doesn't open new tab as
// last tab
TEST_F(TabsMotionHelperTest, NewTopLevelTab) {
  using tpos::TabBarState;
  using tpos::TabPlacingStrategy;
  using tpos::TabSource;
  using tpos::TabstackMode;
  constexpr int last_index = 10;
  CreateTabs(last_index);
  SetGroupId(4, "b");
  SetGroupId(5, "b");
  SetGroupId(6, "b");
  auto* tab_strip = browser()->tab_strip_model();
  TabBarState state;
  state.source = TabSource::kGeneral;
  std::optional<tpos::TabPosition> pos;
  state.tab_stack_mode = TabstackMode::kSubstrip;

  SnapshotSampler sampler;

  sampler.expected = vivaldi::test_snapshots::NewTopLevelTab();
  const std::string command = "COMMAND_NEW_TAB_OUTSIDE_GROUP";

  for (int active_tab : {5, 2}) {
    tab_strip->ActivateTabAt(active_tab);
    for (auto strategy : kStrategies) {
      state.placement_strategy = strategy;
      std::unique_ptr<content::WebContents> new_contents =
          content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
      TabExtDataImpl::Create(new_contents.get());
      SetCommand(new_contents.get(), command);
      pos = DetermineInsertionIndexFromState(tab_strip, new_contents.get(),
                                             TabSource::kGeneral, state);
      sampler.Snap(active_tab, command, strategy, pos->index);
    }
  }
  sampler.Check();
}

// Ensures TabInvokedBy::kEmailLink, TabInvokedBy::kEmailLinkBackground,
// TabInvokedBy::kHtml, and TabInvokedBy::kBackground behavior won't change
// unnoticed.
TEST_F(TabsMotionHelperTest, InvokedByLinkClickSnapshot) {
  using tpos::TabBarState;
  using tpos::TabPlacingStrategy;
  using tpos::TabSource;
  using tpos::TabstackMode;
  constexpr int last_index = 10;
  CreateTabs(last_index);
  SetGroupId(4, "b");
  SetGroupId(5, "b");
  SetGroupId(6, "b");
  auto* tab_strip = browser()->tab_strip_model();
  tab_strip->SetTabPinned(0, true);
  TabBarState state;
  state.source = TabSource::kGeneral;
  std::optional<tpos::TabPosition> pos;
  state.tab_stack_mode = TabstackMode::kSubstrip;

  SnapshotSampler sampler;

  sampler.expected = vivaldi::test_snapshots::InvokedByLinkClickSnapshot();

  for (int active_tab : {0, 5, 2}) {
    tab_strip->ActivateTabAt(active_tab);
    for (auto invoked_by : {
             TabInvokedBy::kEmailLink,
             TabInvokedBy::kEmailLinkBackground,
             TabInvokedBy::kHtml,
             TabInvokedBy::kBackground,
         }) {
      for (auto strategy : kStrategies) {
        state.placement_strategy = strategy;
        std::unique_ptr<content::WebContents> new_contents =
            content::WebContentsTester::CreateTestWebContents(profile(),
                                                              nullptr);
        auto* ext = TabExtDataImpl::Create(new_contents.get());
        TabPositioningParams positioning_params;
        positioning_params.invoked_by = invoked_by;
        ext->SetPositioningParams(positioning_params);

        pos = DetermineInsertionIndexFromState(tab_strip, new_contents.get(),
                                               TabSource::kGeneral, state);
        sampler.Snap(active_tab, DescribeInvokedBy(invoked_by), strategy,
                     pos->index);
      }
    }
  }
  sampler.Check();
}

// The tab was created by clicking an email link; VB-126815
TEST_F(TabsMotionHelperTest, NewTabEmailLink) {
  using tpos::TabBarState;
  using tpos::TabPlacingStrategy;
  using tpos::TabSource;
  using tpos::TabstackMode;
  constexpr int last_index = 10;
  CreateTabs(last_index);

  const int last_in_group = 6;
  SetGroupId(last_in_group - 2, "b");
  SetGroupId(last_in_group - 1, "b");
  SetGroupId(last_in_group, "b");

  auto* tab_strip = browser()->tab_strip_model();
  tab_strip->SetTabPinned(0, true);
  tab_strip->SetTabPinned(1, true);
  TabBarState state;
  state.source = TabSource::kGeneral;
  std::optional<tpos::TabPosition> pos;
  state.tab_stack_mode = TabstackMode::kSubstrip;

  for (int active_tab = 0; active_tab < last_index; ++active_tab) {
    tab_strip->ActivateTabAt(active_tab);
    for (auto invoked_by :
         {TabInvokedBy::kEmailLink, TabInvokedBy::kEmailLinkBackground}) {
      for (auto strategy : kStrategies) {
        state.placement_strategy = strategy;
        std::unique_ptr<content::WebContents> new_contents =
            content::WebContentsTester::CreateTestWebContents(profile(),
                                                              nullptr);
        auto* ext = TabExtDataImpl::Create(new_contents.get());

        TabPositioningParams positioning_params;
        positioning_params.invoked_by = invoked_by;
        ext->SetPositioningParams(positioning_params);
        pos = DetermineInsertionIndexFromState(tab_strip, new_contents.get(),
                                               TabSource::kGeneral, state);
        ASSERT_TRUE(pos.has_value());
        // This is described in VB-126815
        if (strategy == tpos::TabPlacingStrategy::kDirectRightOfCurrent) {
          // Exception: kDirectRightOfCurrent works as defined
          // Exception of exception: if the active tab is within a group, the
          // new tab is created after this group.
          if (active_tab == 4 || active_tab == 5 || active_tab == 6) {
            ASSERT_EQ(pos->index, last_in_group + 1);
          } else {
            ASSERT_EQ(pos->index, active_tab + 1);
          }
        } else {
          // Always last.
          ASSERT_EQ(pos->index, last_index);
        }
      }
    }
  }
}

// VB-129622 [Tabs][Speed dial] Opening from the speed dial in new tabs are not
// opening at the end of the tab strip
TEST_F(TabsMotionHelperTest, TapFromSpeedDialPosition) {
  using tpos::TabBarState;
  using tpos::TabPlacingStrategy;
  using tpos::TabSource;
  using tpos::TabstackMode;
  constexpr int last_index = 10;
  CreateTabs(last_index);
  SetGroupId(4, "b");
  SetGroupId(5, "b");
  SetGroupId(6, "b");
  auto* tab_strip = browser()->tab_strip_model();
  tab_strip->SetTabPinned(0, true);
  TabBarState state;
  state.source = TabSource::kGeneral;
  std::optional<tpos::TabPosition> pos;
  state.tab_stack_mode = TabstackMode::kSubstrip;

  SnapshotSampler sampler;
  sampler.expected = vivaldi::test_snapshots::TapFromSpeedDialPosition();

  for (int active_tab : {0, 2, 5}) {
    tab_strip->ActivateTabAt(active_tab);
    for (auto strategy : kStrategies) {
      state.placement_strategy = strategy;
      std::unique_ptr<content::WebContents> new_contents =
          content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
      auto* ext = TabExtDataImpl::Create(new_contents.get());
      TabPositioningParams positioning_params;
      positioning_params.invoked_by = TabInvokedBy::kSpeedDial;
      ext->SetPositioningParams(positioning_params);
      pos = DetermineInsertionIndexFromState(tab_strip, new_contents.get(),
                                             TabSource::kGeneral, state);
      sampler.Snap(active_tab, "", strategy, pos->index);
    }
  }
  sampler.Check();
}

// VB-129590 [Tabs][Web Panels] Tabs opened from web panels are considered as
// related to the focussed tab
TEST_F(TabsMotionHelperTest, PanelLinkAndConfigTabPosition) {
  using tpos::TabBarState;
  using tpos::TabPlacingStrategy;
  using tpos::TabSource;
  using tpos::TabstackMode;
  constexpr int last_index = 10;
  CreateTabs(last_index);
  SetGroupId(4, "b");
  SetGroupId(5, "b");
  SetGroupId(6, "b");
  auto* tab_strip = browser()->tab_strip_model();
  tab_strip->SetTabPinned(0, true);
  TabBarState state;
  state.source = TabSource::kGeneral;
  std::optional<tpos::TabPosition> pos;
  state.tab_stack_mode = TabstackMode::kSubstrip;

  SnapshotSampler sampler;
  sampler.expected = vivaldi::test_snapshots::PanelLinkAndConfigTabPosition();
  for (int active_tab : {0, 2, 5}) {
    tab_strip->ActivateTabAt(active_tab);
    for (auto invoked_by :
         {TabInvokedBy::kPanelLinkBackground, TabInvokedBy::kPanelLink,
          TabInvokedBy::kVivaldiUi}) {
      for (auto strategy : kStrategies) {
        state.placement_strategy = strategy;
        std::unique_ptr<content::WebContents> new_contents =
            content::WebContentsTester::CreateTestWebContents(profile(),
                                                              nullptr);
        auto* ext = TabExtDataImpl::Create(new_contents.get());
        TabPositioningParams positioning_params;
        positioning_params.invoked_by = invoked_by;
        ext->SetPositioningParams(positioning_params);

        pos = DetermineInsertionIndexFromState(tab_strip, new_contents.get(),
                                               TabSource::kGeneral, state);
        sampler.Snap(active_tab, DescribeInvokedBy(invoked_by), strategy,
                     pos->index);
      }
    }
  }
  sampler.Check();
}

TEST_F(TabsMotionHelperTest, NewTabAllStrategies) {
  {
    using tpos::TabBarState;
    using tpos::TabPlacingStrategy;
    using tpos::TabSource;
    using tpos::TabstackMode;
    CreateTabs(8);
    SetGroupId(4, "b");
    SetGroupId(5, "b");
    SetGroupId(6, "b");

    // Tab strip created: *t0 t1 t2 t3 [ t4 t5 t6 ] t7
    // t0 - pinned
    // t4, t5, t6 - in stack with groupId="b"

    auto* tab_strip = browser()->tab_strip_model();
    TabBarState state;
    state.source = TabSource::kGeneral;
    std::optional<tpos::TabPosition> pos;
    tab_strip->SetTabPinned(0, true);
    SnapshotSampler sampler;
    sampler.expected = vivaldi::test_snapshots::NewTabAllStrategies();
    int checked_states_count = 0;
    for (bool open_in_stack : {true, false}) {
      state.open_in_current_tab_stack = open_in_stack;
      for (bool substrip_locked : {true, false}) {
        state.is_substrip_locked = substrip_locked;
        for (auto stacking :
             {TabstackMode::kOff, TabstackMode::kDotted,
              TabstackMode::kSubstrip, TabstackMode::kAccordion}) {
          state.tab_stack_mode = stacking;
          for (int index : {0, 1, 4, 5, 6}) {
            tab_strip->ActivateTabAt(index);
            for (auto strategy :
                 {TabPlacingStrategy::kRightOfCurrent,
                  TabPlacingStrategy::kDirectRightOfCurrent,
                  TabPlacingStrategy::kAlwaysLast,
                  TabPlacingStrategy::kOpenInTabstackWithRelated}) {
              state.placement_strategy = strategy;
              std::unique_ptr<content::WebContents> new_contents =
                  content::WebContentsTester::CreateTestWebContents(profile(),
                                                                    nullptr);
              TabExtDataImpl::Create(new_contents.get());
              pos = DetermineInsertionIndexFromState(
                  tab_strip, new_contents.get(), TabSource::kGeneral, state);
              ASSERT_TRUE(pos.has_value());
              checked_states_count++;
              std::string prefix = state.open_in_current_tab_stack ? "O" : "X";
              if (substrip_locked) {
                prefix += "L";
              }
              prefix += ";" + DescribeStackMode(stacking);
              sampler.Snap(index, prefix, strategy, pos->index);
            }
          }
        }
      }
    }
    // Ensure we did many checks.
    ASSERT_GT(checked_states_count, 100);

    sampler.Check();
  }
}

TEST_F(TabsMotionHelperTest, MoveTabOutOfGroupGroup) {
  namespace mv = extensions::vivaldi::tabs_private::Move;
  CreateTabs(5);
  SetGroupId(2, "a");
  SetGroupId(3, "a");

  {
    base::DictValue move_props;
    move_props.Set("target", "right");
    base::ListValue extIds;
    extIds.Append("t3");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_FALSE(helper_or_error.has_value());
  }

  {
    base::DictValue move_props;
    move_props.Set("target", "left");
    base::ListValue extIds;
    extIds.Append("t2");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_FALSE(helper_or_error.has_value());
  }

  {
    base::DictValue move_props;
    move_props.Set("target", "left");
    base::ListValue extIds;
    extIds.Append("t3");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_TRUE(helper_or_error.has_value());
  }

  {
    base::DictValue move_props;
    move_props.Set("target", "right");
    base::ListValue extIds;
    extIds.Append("t2");
    move_props.Set("extIds", std::move(extIds));
    auto helper_or_error = CreateHelper(std::move(move_props));
    ASSERT_TRUE(helper_or_error.has_value());
  }
}

TEST_F(TabsMotionHelperTest, MoveToEdgeOfWorkspace) {
  {
    CreateTabs(5);
    ParamsFactory factory;
    factory.SetTarget("last").AddSource("t2");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
        "t0;;;;|t1;;;;|t3;;;;|t4;;;;|t2;;;;");
  }
  {
    CreateTabs(5);
    ParamsFactory factory;
    factory.SetTarget("first").AddSource("t2");
    auto helper_or_error = CreateHelper(factory.Build());
    ASSERT_TRUE(helper_or_error.has_value());
    auto& helper = helper_or_error.value();
    RunMoves(*helper);
    ASSERT_EQ(DescribeTabStrip(),
        "t2;;;;|t0;;;;|t1;;;;|t3;;;;|t4;;;;");
  }
}

TEST_F(TabsMotionHelperTest, MoveSnapshotBasic) {
  SnapshotContent content;
  content.reparent_id = false;
  CheckMoveSnapshotScenario(content,
                            vivaldi::test_snapshots::MoveSnapshotBasic(),
                            [](ParamsFactory* factory) {});
}

// Move every tab everywhere, all the combinations
TEST_F(TabsMotionHelperTest, MoveSnapshotOn) {
  SnapshotContent content;
  CheckMoveSnapshotScenario(
      content, vivaldi::test_snapshots::MoveSnapshotOn(),
      [](ParamsFactory* factory) { factory->AddTweak("on"); });
}

}  // namespace vivaldi
