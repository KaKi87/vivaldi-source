// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/toolbar/coordinator/main_toolbar_coordinator.h"

#import "base/apple/foundation_util.h"
#import "base/memory/raw_ptr.h"
#import "components/omnibox/browser/omnibox_pref_names.h"
#import "components/omnibox/common/omnibox_features.h"
#import "components/prefs/pref_service.h"
#import "ios/chrome/app/profile/profile_state.h"
#import "ios/chrome/browser/banner_promo/model/default_browser_banner_promo_app_agent.h"
#import "ios/chrome/browser/bubble/model/tab_based_iph_browser_agent.h"
#import "ios/chrome/browser/fullscreen/model/fullscreen_browser_agent.h"
#import "ios/chrome/browser/fullscreen/model/fullscreen_browser_agent_observer_bridge.h"
#import "ios/chrome/browser/fullscreen/ui_bundled/fullscreen_controller.h"
#import "ios/chrome/browser/fullscreen/ui_bundled/fullscreen_ui_updater.h"
#import "ios/chrome/browser/intelligence/bwg/model/gemini_browser_agent.h"
#import "ios/chrome/browser/intelligence/bwg/model/gemini_service.h"
#import "ios/chrome/browser/intelligence/bwg/model/gemini_service_factory.h"
#import "ios/chrome/browser/intelligence/features/features.h"
#import "ios/chrome/browser/location_bar/ui_bundled/location_bar_coordinator.h"
#import "ios/chrome/browser/menu/ui_bundled/browser_action_factory.h"
#import "ios/chrome/browser/ntp/model/new_tab_page_util.h"
#import "ios/chrome/browser/orchestrator/ui_bundled/omnibox_focus_orchestrator.h"
#import "ios/chrome/browser/overlays/model/public/overlay_presentation_context.h"
#import "ios/chrome/browser/prerender/model/prerender_browser_agent.h"
#import "ios/chrome/browser/shared/coordinator/layout_guide/layout_guide_util.h"
#import "ios/chrome/browser/shared/coordinator/scene/scene_state.h"
#import "ios/chrome/browser/shared/coordinator/scene/state/layout_state.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/browser/browser_provider.h"
#import "ios/chrome/browser/shared/model/browser/browser_provider_interface.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/model/url/url_util.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#import "ios/chrome/browser/shared/public/commands/activity_service_commands.h"
#import "ios/chrome/browser/shared/public/commands/browser_coordinator_commands.h"
#import "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#import "ios/chrome/browser/shared/public/commands/contextual_panel_entrypoint_commands.h"
#import "ios/chrome/browser/shared/public/commands/custom_leading_view_type.h"
#import "ios/chrome/browser/shared/public/commands/find_in_page_commands.h"
#import "ios/chrome/browser/shared/public/commands/fullscreen_commands.h"
#import "ios/chrome/browser/shared/public/commands/gemini_commands.h"
#import "ios/chrome/browser/shared/public/commands/guided_tour_commands.h"
#import "ios/chrome/browser/shared/public/commands/help_commands.h"
#import "ios/chrome/browser/shared/public/commands/location_bar_badge_commands.h"
#import "ios/chrome/browser/shared/public/commands/new_tab_page_commands.h"
#import "ios/chrome/browser/shared/public/commands/page_action_menu_entry_point_commands.h"
#import "ios/chrome/browser/shared/public/commands/popup_menu_commands.h"
#import "ios/chrome/browser/shared/public/commands/reader_mode_chip_commands.h"
#import "ios/chrome/browser/shared/public/commands/scene_commands.h"
#import "ios/chrome/browser/shared/public/commands/settings_commands.h"
#import "ios/chrome/browser/shared/public/commands/text_zoom_commands.h"
#import "ios/chrome/browser/shared/public/commands/toolbar_commands.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/ui/util/layout_guide_names.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/shared/ui/util/util_swift.h"
#import "ios/chrome/browser/signin/model/authentication_service.h"
#import "ios/chrome/browser/signin/model/authentication_service_factory.h"
#import "ios/chrome/browser/toolbar/coordinator/main_toolbar_mediator.h"
#import "ios/chrome/browser/toolbar/coordinator/toolbar_mediator.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/adaptive_toolbar_view_controller.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/legacy_toolbar_mediator.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/primary_toolbar_coordinator.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/primary_toolbar_view_controller_delegate.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_constants.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_omnibox_consumer.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_type.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_utils.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/secondary_toolbar_coordinator.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/toolbar_coordinatee.h"
#import "ios/chrome/browser/toolbar/tab_group/coordinator/tab_group_indicator_coordinator.h"
#import "ios/chrome/browser/toolbar/tab_group/ui/tab_group_indicator_constants.h"
#import "ios/chrome/browser/toolbar/ui/buttons/toolbar_button_factory.h"
#import "ios/chrome/browser/toolbar/ui/toolbar_constants.h"
#import "ios/chrome/browser/toolbar/ui/toolbar_utils.h"
#import "ios/chrome/browser/toolbar/ui/toolbar_view_controller.h"
#import "ios/chrome/browser/web/model/web_navigation_browser_agent.h"
#import "ios/chrome/common/ui/util/constraints_ui_util.h"
#import "ios/chrome/common/ui/util/ui_util.h"
#import "ios/components/webui/web_ui_url_constants.h"
#import "ios/web/public/web_state.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/chrome/browser/browser_view/ui_bundled/browser_view_controller.h"
#import "ios/chrome/browser/location_bar/ui_bundled/location_bar_steady_view_consumer.h"
#import "ios/chrome/browser/shared/coordinator/layout_guide/layout_guide_util.h"
#import "ios/chrome/browser/shared/ui/util/layout_guide_names.h"
#import "ios/chrome/browser/shared/ui/util/util_swift.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/tab_switcher/tab_strip/ui/swift_constants_for_objective_c.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/primary_toolbar_view.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/primary_toolbar_view_controller.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/secondary_toolbar_view.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/secondary_toolbar_view_controller.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/buttons/legacy_toolbar_button.h"
#import "ios/chrome/browser/toolbar/ui/buttons/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/vivaldi_tools_menu_guide_helper.h"
#import "ios/ui/ad_tracker_blocker/manager/vivaldi_atb_manager.h"
#import "ios/ui/helpers/vivaldi_global_helpers.h"
#import "ios/ui/toolbar/vivaldi_toolbar_constants.h"

using vivaldi::IsVivaldiRunning;
// End Vivaldi

namespace layout_state {
class MainToolbarCoordinatorPassKeyFactory {
 public:
  static base::PassKey<MainToolbarCoordinatorPassKeyFactory> CreateKey() {
    return base::PassKey<MainToolbarCoordinatorPassKeyFactory>();
  }
};
}  // namespace layout_state

namespace {
// Extra vertical spacing when the banner promo is active on split mode.
constexpr CGFloat kBannerPromoVerticalSpacing = 8;

// Helper function to return the domain passkey used to mutate the layout state.
inline LayoutStateToolbarPassKey PassKey() {
  return layout_state::MainToolbarCoordinatorPassKeyFactory::CreateKey();
}
}  // namespace

@interface MainToolbarCoordinator () <ContextualPanelEntrypointCommands,
                                      FullscreenBrowserAgentObserving,
                                      GuidedTourCommands,
                                      LayoutStateObserver,
                                      LocationBarBadgeCommands,
                                      PageActionMenuEntryPointCommands,
                                      PrimaryToolbarViewControllerDelegate,
                                      ReaderModeChipCommands,
                                      ToolbarCommands,

                                      // Vivaldi
                                      LocationBarSteadyViewConsumer,
                                      VivaldiATBConsumer,
                                      // End Vivaldi

                                      ToolbarMediatorDelegate>

/// Whether this coordinator has been started.
@property(nonatomic, assign) BOOL started;
/// Coordinator for the location bar containing the omnibox.
@property(nonatomic, strong) LocationBarCoordinator* locationBarCoordinator;
/// Coordinator for the primary toolbar at the top of the screen.
@property(nonatomic, strong)
    PrimaryToolbarCoordinator* primaryToolbarCoordinator;
/// Coordinator for the secondary toolbar at the bottom of the screen.
@property(nonatomic, strong)
    SecondaryToolbarCoordinator* secondaryToolbarCoordinator;

/// Mediator observing WebStateList for toolbars.
@property(nonatomic, strong) LegacyToolbarMediator* legacyToolbarMediator;
/// Orchestrator for the omnibox focus animation.
@property(nonatomic, strong) OmniboxFocusOrchestrator* orchestrator;
/// Whether the omnibox is currently focused.
@property(nonatomic, assign) BOOL locationBarFocused;

// Vivaldi
@property(nonatomic, strong) LayoutGuideCenter* layoutGuideCenter;

// Important!!!: (prio@vivaldi.com) - Note to self or someone visiting this part
// Chrome moves only omnibox to the bottom, but for us we have buttons
// (shield, menu, panels) alongside omnibox to move to bottom when bottom
// omnibox is enabled, which are part of primary toolbar.
// Therefore, we create a separate view with this
// coordinator to use that as top primary toolbar. And, use chrome's whole
// primary toolbar as bottom omnibox with buttons, and not just the omnibox part.
@property(nonatomic, strong)
    PrimaryToolbarCoordinator* vivaldiTopToolbarCoordinator;
// Manager to keep track of adblocker shield state for the web state.
@property(nonatomic, strong) VivaldiATBManager* adblockManager;
// End Vivaldi

@end

@implementation MainToolbarCoordinator {
  // The mediator for this coordinator.
  MainToolbarMediator* _mainToolbarMediator;
  // The layout state for the scene.
  __weak LayoutState* _layoutState;
  /// Type of toolbar containing the omnibox. Unlike
  /// `_steadyStateOmniboxPosition`, this tracks the omnibox position at all
  /// time.
  ToolbarType _omniboxPosition;
  /// Type of the toolbar that contains the omnibox when it's not focused. The
  /// animation of focusing/defocusing the omnibox changes depending on this
  /// position.
  ToolbarType _steadyStateOmniboxPosition;
  //// Indicates whether the focus came from a tap on the NTP's fakebox.
  BOOL _focusedFromFakebox;
  /// Indicates whether the fakebox was pinned on last signal to focus from
  /// the fakebox.
  BOOL _fakeboxPinned;
  /// Command handler for showing the IPH.
  id<HelpCommands> _helpHandler;
  /// Top toolbar mediator.
  ToolbarMediator* _topToolbarMediator;
  /// Top toolbar view controller.
  ToolbarViewController* _topToolbarViewController;
  /// Fullscreen UI Updater for the top toolbar.
  std::unique_ptr<FullscreenUIUpdater> _topToolbarFullscreenUIUpdater;
  /// Top location bar coordinator.
  LocationBarCoordinator* _topLocationBarCoordinator;
  /// Coordinator for the tab group indicator.
  TabGroupIndicatorCoordinator* _tabGroupIndicatorCoordinator;
  /// Bottom toolbar mediator.
  ToolbarMediator* _bottomToolbarMediator;
  /// Bottom toolbar view controller.
  ToolbarViewController* _bottomToolbarViewController;
  /// Observer for fullscreen layout calculations.
  std::unique_ptr<FullscreenBrowserAgentObserverBridge> _fullscreenObserver;
  std::unique_ptr<FullscreenUIUpdater> _bottomToolbarFullscreenUIUpdater;
  /// Bottom location bar coordinator.
  LocationBarCoordinator* _bottomLocationBarCoordinator;

  // Vivaldi
  // Pref backed boolean to track tab bar style state.
  BOOL _tabBarEnabled;
  // End Vivaldi

}

- (instancetype)initWithBrowser:(Browser*)browser {
  CHECK(browser);
  self = [super initWithBaseViewController:nil browser:browser];
  if (self) {
    // Initialize both coordinators here as they might be referenced before
    // `start`.
    _primaryToolbarCoordinator =
        [[PrimaryToolbarCoordinator alloc] initWithBrowser:browser];
    _secondaryToolbarCoordinator =
        [[SecondaryToolbarCoordinator alloc] initWithBrowser:browser];

    // Vivaldi
    _vivaldiTopToolbarCoordinator =
        [[PrimaryToolbarCoordinator alloc] initWithBrowser:browser];
    // End Vivaldi

    [self.browser->GetCommandDispatcher()
        startDispatchingToTarget:self
                     forProtocol:@protocol(ToolbarCommands)];

    _helpHandler =
        HandlerForProtocol(browser->GetCommandDispatcher(), HelpCommands);
  }
  return self;
}

- (void)start {
  if (self.started) {
    return;
  }
  // Set a default position, overridden by `setInitialOmniboxPosition` below.
  _omniboxPosition = ToolbarType::kPrimary;

  Browser* browser = self.browser;
  _layoutState = browser->GetSceneState().layoutState;
  [browser->GetCommandDispatcher()
      startDispatchingToTarget:self
                   forProtocol:@protocol(FakeboxFocuser)];

  if (IsBestOfAppGuidedTourEnabled() && !IsChromeNextIaEnabled()) {
    [self.browser->GetCommandDispatcher()
        startDispatchingToTarget:self
                     forProtocol:@protocol(GuidedTourCommands)];
  }

  self.legacyToolbarMediator = [[LegacyToolbarMediator alloc]
      initWithWebStateList:browser->GetWebStateList()
               isIncognito:browser->GetProfile()->IsOffTheRecord()];
  self.legacyToolbarMediator.delegate = self;

  _mainToolbarMediator = [[MainToolbarMediator alloc]
      initWithPrefService:GetApplicationContext()->GetLocalState()
              layoutState:_layoutState];
  [browser->GetCommandDispatcher()
      startDispatchingToTarget:self
                   forProtocol:@protocol(ReaderModeChipCommands)];
  BOOL isToolbarAtBottom = [self isToolbarPositionBottom];

  [_layoutState addObserver:self];

  if (IsChromeNextIaEnabled()) {
    _topLocationBarCoordinator =
        [self createLocationBarCoordinatorActive:!isToolbarAtBottom
                                     topPosition:YES];
    _topToolbarMediator = [self createToolbarMediatorTopPosition:YES];
    _topToolbarViewController = [self
        createToolbarViewControllerForMediator:_topToolbarMediator
                                   locationBar:_topLocationBarCoordinator
                                                   .locationBarViewController
                                   topPosition:YES];
    _tabGroupIndicatorCoordinator = [[TabGroupIndicatorCoordinator alloc]
        initWithBaseViewController:self.baseViewController
                           browser:browser];
    _tabGroupIndicatorCoordinator.toolbarHeightDelegate =
        self.toolbarHeightDelegate;
    [_tabGroupIndicatorCoordinator start];
    [_topToolbarMediator
        setUICurrentlySupportsPromo:!_tabGroupIndicatorCoordinator.viewVisible];
    [_topToolbarViewController
        setTabGroupIndicatorView:_tabGroupIndicatorCoordinator.view];

    if (!IsFullscreenRefactoringEnabled()) {
      _topToolbarFullscreenUIUpdater = std::make_unique<FullscreenUIUpdater>(
          FullscreenController::FromBrowser(browser),
          _topToolbarViewController);
    } else {
      _fullscreenObserver =
          std::make_unique<FullscreenBrowserAgentObserverBridge>(
              self, FullscreenBrowserAgent::FromBrowser(browser));
    }

    _bottomLocationBarCoordinator =
        [self createLocationBarCoordinatorActive:isToolbarAtBottom
                                     topPosition:NO];
    _bottomToolbarMediator = [self createToolbarMediatorTopPosition:NO];
    _bottomToolbarViewController = [self
        createToolbarViewControllerForMediator:_bottomToolbarMediator
                                   locationBar:_bottomLocationBarCoordinator
                                                   .locationBarViewController
                                   topPosition:NO];
    if (!IsFullscreenRefactoringEnabled()) {
      _bottomToolbarFullscreenUIUpdater = std::make_unique<FullscreenUIUpdater>(
          FullscreenController::FromBrowser(browser),
          _bottomToolbarViewController);
    }

    LayoutGuideCenter* layoutGuideCenter = LayoutGuideCenterForBrowser(browser);
    [layoutGuideCenter referenceView:_topToolbarViewController.view
                           underName:kPrimaryToolbarGuide];
    [layoutGuideCenter
        referenceView:_topLocationBarCoordinator.locationBarViewController.view
            underName:kTopOmniboxGuide];
    [layoutGuideCenter referenceView:_bottomToolbarViewController.view
                           underName:kSecondaryToolbarGuide];

    [self.browser->GetCommandDispatcher()
        startDispatchingToTarget:self
                     forProtocol:@protocol(ContextualPanelEntrypointCommands)];
    [self.browser->GetCommandDispatcher()
        startDispatchingToTarget:self
                     forProtocol:@protocol(LocationBarBadgeCommands)];
    if (IsPageActionMenuEnabled()) {
      [browser->GetCommandDispatcher()
          startDispatchingToTarget:self
                       forProtocol:@protocol(PageActionMenuEntryPointCommands)];
    }
    [self updateLayoutForToolbarPosition:_layoutState.toolbarPosition];
    self.started = YES;
    return;
  }

  self.locationBarCoordinator =
      [[LocationBarCoordinator alloc] initWithBrowser:browser];
  self.locationBarCoordinator.delegate = self.omniboxFocusDelegate;
  self.locationBarCoordinator.popupPresenterDelegate =
      self.popupPresenterDelegate;
  [self.locationBarCoordinator start];

  self.primaryToolbarCoordinator.viewControllerDelegate = self;
  self.primaryToolbarCoordinator.toolbarHeightDelegate =
      self.toolbarHeightDelegate;
  [self.primaryToolbarCoordinator start];
  self.secondaryToolbarCoordinator.toolbarHeightDelegate =
      self.toolbarHeightDelegate;
  [self.secondaryToolbarCoordinator start];

  // Vivaldi
  PrefService* originalPrefs =
      browser->GetProfile()->GetOriginalProfile()->GetPrefs();
  self.legacyToolbarMediator.originalPrefService = originalPrefs;

  self.locationBarCoordinator.steadyViewConsumer = self;
  self.legacyToolbarMediator.omniboxConsumer =
      self.locationBarCoordinator.toolbarOmniboxConsumer;

  self.vivaldiTopToolbarCoordinator.viewControllerDelegate = self;
  [self.vivaldiTopToolbarCoordinator start];

  LayoutGuideCenter* layoutGuideCenter =
      LayoutGuideCenterForBrowser(self.browser);
  _layoutGuideCenter = layoutGuideCenter;
  [_layoutGuideCenter
        referenceView:self.secondaryToolbarCoordinator.viewController.view
            underName:vivaldiBottomOmniboxGuide
        forcesSynchronousLayoutUpdates:YES];
  [self initialiseAdblockManager];
  // End Vivaldi

  if (!IsChromeNextIaEnabled()) {
    self.orchestrator = [[OmniboxFocusOrchestrator alloc] init];
    [self updateOrchestratorAnimatee];
  }

  if (IsBottomOmniboxAvailable()) {
    [self.legacyToolbarMediator setInitialOmniboxPosition];
  } else {
    [self.primaryToolbarCoordinator
        setLocationBarViewController:self.locationBarCoordinator
                                         .locationBarViewController];
  }

  // Force the initial layout setup to ensure the view hierarchy is constructed
  // and the location bar view is loaded before setting up the command
  // dispatchers.
  [self updateLayoutForToolbarPosition:_layoutState.toolbarPosition];

  if (IsPageActionMenuEnabled()) {
    [self.locationBarCoordinator setPageActionMenuEntryPointDispatcher];
  }

  [self updateToolbarsLayout];

  self.started = YES;
}

- (void)stop {
  if (!self.started) {
    return;
  }

  if (IsChromeNextIaEnabled()) {
    [_topToolbarMediator disconnect];
    _topToolbarMediator = nil;
    [_topLocationBarCoordinator stop];
    _topLocationBarCoordinator = nil;
    _topToolbarViewController = nil;
    _fullscreenObserver = nullptr;
    _topToolbarFullscreenUIUpdater = nullptr;

    [_tabGroupIndicatorCoordinator stop];

    [_bottomToolbarMediator disconnect];
    _bottomToolbarMediator = nil;
    [_bottomLocationBarCoordinator stop];
    _bottomLocationBarCoordinator = nil;
    _bottomToolbarViewController = nil;
    _bottomToolbarFullscreenUIUpdater = nullptr;
  }

  self.orchestrator.editViewAnimatee = nil;
  self.orchestrator.locationBarAnimatee = nil;
  self.orchestrator = nil;

  [self.primaryToolbarCoordinator stop];
  self.primaryToolbarCoordinator.viewControllerDelegate = nil;
  self.primaryToolbarCoordinator = nil;

  [self.secondaryToolbarCoordinator stop];
  self.secondaryToolbarCoordinator = nil;

  [self.locationBarCoordinator stop];
  self.locationBarCoordinator.popupPresenterDelegate = nil;
  self.locationBarCoordinator = nil;

  [self.legacyToolbarMediator disconnect];
  self.legacyToolbarMediator.omniboxConsumer = nil;
  self.legacyToolbarMediator.delegate = nil;
  self.legacyToolbarMediator = nil;

  [_mainToolbarMediator disconnect];
  _mainToolbarMediator = nil;

  [_layoutState removeObserver:self];

  // Vivaldi
  [self.vivaldiTopToolbarCoordinator stop];
  self.vivaldiTopToolbarCoordinator.viewControllerDelegate = nil;
  self.vivaldiTopToolbarCoordinator = nil;
  self.locationBarCoordinator.steadyViewConsumer = nil;
  if (self.adblockManager) {
    self.adblockManager.consumer = nil;
    [self.adblockManager disconnect];
  }
  // End Vivaldi

  [self.browser->GetCommandDispatcher() stopDispatchingToTarget:self];
  self.started = NO;
}

#pragma mark - Public

- (UIViewController*)baseViewController {
  return self.primaryToolbarCoordinator.baseViewController;
}

- (void)setBaseViewController:(UIViewController*)baseViewController {
  self.primaryToolbarCoordinator.baseViewController = baseViewController;
  self.secondaryToolbarCoordinator.baseViewController = baseViewController;
}

- (UIViewController*)primaryToolbarViewController {

  if (IsVivaldiRunning()) {
    return self.vivaldiTopToolbarCoordinator.viewController;
  } // End Vivaldi

  if (IsChromeNextIaEnabled()) {
    return _topToolbarViewController;
  }
  return self.primaryToolbarCoordinator.viewController;
}

- (UIViewController*)secondaryToolbarViewController {
  if (IsChromeNextIaEnabled()) {
    return _bottomToolbarViewController;
  }
  return self.secondaryToolbarCoordinator.viewController;
}

- (UIView*)shareButton {
  return self.primaryToolbarCoordinator.shareButton;
}

// Public and in `ToolbarMediatorDelegate`.
- (void)updateToolbar {
  web::WebState* webState =
      self.browser->GetWebStateList()->GetActiveWebState();
  if (!webState) {
    return;
  }

  // Please note, this notion of isLoading is slightly different from WebState's
  // IsLoading().
  BOOL isToolbarLoading =
      webState->IsLoading() &&
      !webState->GetLastCommittedURL().SchemeIs(kChromeUIScheme);

  if (self.isLoadingPrerenderer && isToolbarLoading) {
    for (id<ToolbarCoordinatee> coordinator in self.coordinators) {
      [coordinator showPrerenderingAnimation];
    }
  }

  if (IsVivaldiRunning()) {
    [self updateVivaldiToolsMenuButtonGuides];
  } // End Vivaldi

  id<FindInPageCommands> findInPageCommandsHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), FindInPageCommands);
  [findInPageCommandsHandler showFindUIIfActive];

  id<TextZoomCommands> textZoomCommandsHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), TextZoomCommands);
  [textZoomCommandsHandler showTextZoomUIIfActive];

  BOOL isNTP = IsVisibleURLNewTabPage(webState);
  BOOL isOffTheRecord = self.isOffTheRecord;
  BOOL canShowTabStrip = CanShowTabStrip(self.traitEnvironment);

  if (!IsChromeNextIaEnabled()) {
    // Hide the toolbar when displaying content suggestions without the tab
    // strip, without the focused omnibox, only when in split toolbar mode.
    BOOL hideToolbar = isNTP && !isOffTheRecord && ![self inEditState] &&
                       !canShowTabStrip &&
                       IsSplitToolbarMode(self.traitEnvironment);


  if (IsVivaldiRunning()) {
    self.primaryToolbarViewController.view.hidden = NO;
  } else { // Vivaldi
  self.primaryToolbarViewController.view.hidden = hideToolbar;
  } // End Vivaldi

  }
}

- (void)updateToolbarPositionForActiveBrowser {
  if (IsChromeNextIaEnabled()) {
    return;
  }
  [self.legacyToolbarMediator setInitialOmniboxPosition];
}

- (BOOL)isLoadingPrerenderer {
  if (!_started) {
    return NO;
  }

  PrerenderBrowserAgent* prerenderBrowserAgent =
      PrerenderBrowserAgent::FromBrowser(self.browser);
  return prerenderBrowserAgent && prerenderBrowserAgent->IsInsertingPrerender();
}

- (void)setToolbarHeightDelegate:(id<ToolbarHeightDelegate>)delegate {
  _toolbarHeightDelegate = delegate;
  _topToolbarViewController.toolbarHeightDelegate = delegate;
  _bottomToolbarViewController.toolbarHeightDelegate = delegate;
}

#pragma mark Omnibox and LocationBar

- (void)transitionToLocationBarFocusedState:(BOOL)focused
                                 completion:(ProceduralBlock)completion {
  CHECK(!IsChromeNextIaEnabled());
  // Disable infobarBanner overlays when focusing the omnibox as they overlap
  // with primary toolbar.
  OverlayPresentationContext* infobarBannerContext =
      OverlayPresentationContext::FromBrowser(self.browser,
                                              OverlayModality::kInfobarBanner);
  if (infobarBannerContext) {
    infobarBannerContext->SetUIDisabled(focused);
  }

  if (self.traitEnvironment.traitCollection.verticalSizeClass ==
      UIUserInterfaceSizeClassUnspecified) {
    return;
  }
  [self.legacyToolbarMediator locationBarFocusChangedTo:focused];

  BOOL animateTransition =
      (_steadyStateOmniboxPosition == ToolbarType::kPrimary);

  BOOL toolbarExpanded = focused && !CanShowTabStrip(self.traitEnvironment);

  // Vivaldi
  [self updateToolbarBackgroundColorWithOmniboxFocus:focused];
  // End Vivaldi

  [self.orchestrator transitionToStateOmniboxFocused:focused
                                     toolbarExpanded:toolbarExpanded
                                             trigger:[self omniboxFocusTrigger]
                                            animated:animateTransition
                                          completion:completion];

  [self.primaryToolbarCoordinator.viewController setLocationBarFocused:focused];
  [self.secondaryToolbarCoordinator.viewController
      setLocationBarFocused:focused];
  self.locationBarFocused = focused;
}

- (BOOL)isOmniboxFirstResponder {
  CHECK(!IsChromeNextIaEnabled());
  return [self.locationBarCoordinator isOmniboxFirstResponder];
}

- (BOOL)showingOmniboxPopup {
  CHECK(!IsChromeNextIaEnabled());
  return [self.locationBarCoordinator showingOmniboxPopup];
}

- (void)setBottomOmniboxOffsetForPopup:(CGFloat)bottomOffset {
  [self.legacyToolbarMediator setBottomOmniboxOffsetForPopup:bottomOffset];
}

#pragma mark ToolbarHeightProviding

- (CGFloat)collapsedPrimaryToolbarHeight {
  if (IsChromeNextIaEnabled()) {
    if ([self isToolbarHidden:_topToolbarViewController.view]) {
      // TODO(crbug.com/40279063): Find out why primary toolbar height cannot be
      // zero. This is a temporary fix for the pdf bug.
      return IsFullscreenRefactoringEnabled() ? 0.0 : 1.0;
    }
    if ([self isToolbarPositionBottom]) {
      // TODO(crbug.com/40279063): Find out why primary toolbar height cannot be
      // zero. This is a temporary fix for the pdf bug.
      return IsFullscreenRefactoringEnabled() ? 0 : 1;
    }
    if (CanShowTabStrip(self.traitEnvironment)) {
      return kTopToolbarIPadHeightFullscreen;
    }
    if (!IsSplitToolbarMode(self.traitEnvironment)) {
      return kToolbarHeightFullscreen;
    }
    return kTopToolbarIPhonePortraitHeightFullscreen;
  }
  if (_omniboxPosition == ToolbarType::kSecondary) {
    // TODO(crbug.com/40279063): Find out why primary toolbar height cannot be
    // zero. This is a temporary fix for the pdf bug.

    if (IsVivaldiRunning()) {
      return 0.0;
    } // End Vivaldi

    return IsFullscreenRefactoringEnabled() ? 0.0 : 1.0;
  }

  return ToolbarCollapsedHeight(
      self.traitEnvironment.traitCollection.preferredContentSizeCategory);
}

- (CGFloat)expandedPrimaryToolbarHeight {
  if (IsChromeNextIaEnabled()) {
    if ([self isToolbarHidden:_topToolbarViewController.view]) {
      // TODO(crbug.com/40279063): Find out why primary toolbar height cannot be
      // zero. This is a temporary fix for the pdf bug.
      return IsFullscreenRefactoringEnabled() ? 0.0 : 1.0;
    }
    BOOL isOmniboxInBottomPosition = [self isToolbarPositionBottom];
    CGFloat height = 0;
    if (_tabGroupIndicatorCoordinator.viewVisible) {
      height += kTabGroupIndicatorHeight;
      if (isOmniboxInBottomPosition) {
        height -= kTopToolbarUnsplitMargin;
      }
    }
    if (_topToolbarViewController.bannerPromoVisible) {
      height += kToolbarPromoBannerHeight;
      if (IsSplitToolbarMode(_topToolbarViewController) &&
          !isOmniboxInBottomPosition) {
        height += kBannerPromoVerticalSpacing;
      }
    }
    if (isOmniboxInBottomPosition) {
      // TODO(crbug.com/40279063): Find out why primary toolbar height cannot be
      // zero. This is a temporary fix for the pdf bug.
      if (IsFullscreenRefactoringEnabled()) {
        return height;
      } else {
        return height > 0 ? height : 1;
      }
    }
    if (ShouldHaveFullHeightTopToolbar(self.traitEnvironment)) {
      return height + kToolbarHeight;
    }
    return height + kTopToolbarIPhonePortraitHeight;
  }

  if (IsVivaldiRunning()) {
    CGFloat height =
        self.primaryToolbarViewController.view.intrinsicContentSize.height;
    if ((!IsSplitToolbarMode(self.traitEnvironment) || _tabBarEnabled) &&
      _omniboxPosition == ToolbarType::kPrimary) {
      // When the adaptive toolbar is unsplit or the tab strip is visible for
      // top omnibox, add a margin.
      height += kTopToolbarUnsplitMargin;
      return height;
    } else {
      return height;
    }
  } // End Vivaldi

  CGFloat height =
      self.primaryToolbarViewController.view.intrinsicContentSize.height;
  if (!IsSplitToolbarMode(self.traitEnvironment) ||
      CanShowTabStrip(self.traitEnvironment)) {
    // When the adaptive toolbar is unsplit or the tab strip is visible, add a
    // margin.
    height += kTopToolbarUnsplitMargin;
  }
  return height;
}

- (CGFloat)collapsedSecondaryToolbarHeight {
  if (IsChromeNextIaEnabled()) {
    if ([self isToolbarHidden:_bottomToolbarViewController.view]) {
      return 0.0;
    }
    if ([self isToolbarPositionBottom]) {
      if (IsAppBarHiddenInFullscreen() &&
          _layoutState.appBarPosition == AppBarPosition::kBottom) {
        CGFloat safeAreaBottom = 0.0;
        if (self.browser->GetSceneState().window) {
          safeAreaBottom =
              self.browser->GetSceneState().window.safeAreaInsets.bottom;
        }
        return ToolbarCollapsedHeight(self.traitEnvironment.traitCollection
                                          .preferredContentSizeCategory) +
               safeAreaBottom;
      }
      return kToolbarHeightFullscreen;
    }
    return 0.0;
  }
  if (_omniboxPosition == ToolbarType::kSecondary) {
    return ToolbarCollapsedHeight(
        self.traitEnvironment.traitCollection.preferredContentSizeCategory);
  }
  return 0.0;
}

- (CGFloat)expandedSecondaryToolbarHeight {
  if (IsChromeNextIaEnabled()) {
    if ([self isToolbarHidden:_bottomToolbarViewController.view]) {
      return 0.0;
    }
    if ([self isToolbarPositionBottom]) {
      return kToolbarHeight;
    }
    return 0.0;
  }
  if (!IsSplitToolbarMode(self.traitEnvironment)) {

    // Important(prio@vivaldi.com) - This enables the bottom omnibox area for
    // iPads and iPhone landscape.
    if (_omniboxPosition == ToolbarType::kSecondary) {
      CGFloat height =
          self.secondaryToolbarViewController.view.intrinsicContentSize.height;
      if (_tabBarEnabled || [VivaldiGlobalHelpers isDeviceTablet]) {
        CHECK(IsBottomOmniboxAvailable());
        height += ToolbarExpandedHeight(
            self.traitEnvironment.traitCollection.preferredContentSizeCategory);
        // When tab strip is visible, add a margin. -1 is to make the location
        // bar optically centered as there is a separator on top of the
        // secondary toolbar which creates visual offset otherwise.
        height += kTopToolbarUnsplitMargin - 1;
      } else {
        height += vBottomAdaptiveLocationBarTopMargin;
      }
      return height;
    }
    // End Vivaldi

    return 0.0;
  }
  CGFloat height =
      self.secondaryToolbarViewController.view.intrinsicContentSize.height;
  if (_omniboxPosition == ToolbarType::kSecondary) {
    height += ToolbarExpandedHeight(
        self.traitEnvironment.traitCollection.preferredContentSizeCategory);

    if (IsVivaldiRunning() && _tabBarEnabled) {
      // When tab strip is visible, add a margin. -1 is to make the location
      // bar optically centered as there is a separator on top of the secondary
      // toolbar which creates visual offset otherwise.
      height += kTopToolbarUnsplitMargin - 1;
    } // End Vivaldi

  }
  return height;
}

#pragma mark - FakeboxFocuser

- (void)focusOmniboxFromFakebox:(BOOL)fromFakebox
                            pinned:(BOOL)pinned
    fakeboxButtonsSnapshotProvider:
        (id<FakeboxButtonsSnapshotProvider>)provider {
  CHECK(!IsChromeNextIaEnabled());
  _focusedFromFakebox = fromFakebox;
  _fakeboxPinned = pinned;
  [self.locationBarCoordinator setFakeboxButtonsSnapshotProvider:provider];
  [self.locationBarCoordinator focusOmniboxFromFakebox];
}

- (void)onFakeboxBlur {
  CHECK(!IsChromeNextIaEnabled());

  if (IsVivaldiRunning()) {
    self.primaryToolbarViewController.view.hidden = NO;
  } else {
  // Hide the toolbar if the NTP is currently displayed.
  web::WebState* webState =
      self.browser->GetWebStateList()->GetActiveWebState();
  if (webState && IsVisibleURLNewTabPage(webState)) {
    self.primaryToolbarViewController.view.hidden =
        IsSplitToolbarMode(self.traitEnvironment) &&
        !CanShowTabStrip(self.traitEnvironment);
  }
  } // End Vivaldi

}

- (void)onFakeboxAnimationComplete {
  CHECK(!IsChromeNextIaEnabled());
  self.primaryToolbarViewController.view.hidden = NO;
}

#pragma mark - NewTabPageControllerDelegate

- (void)setScrollProgressForTabletOmnibox:(CGFloat)progress {
  if (IsChromeNextIaEnabled()) {
    [_topToolbarViewController setNTPScrollProgress:progress];
    [_bottomToolbarViewController setNTPScrollProgress:progress];
    return;
  }

  for (id<NewTabPageControllerDelegate> coordinator in self.coordinators) {
    [coordinator setScrollProgressForTabletOmnibox:progress];
  }
}

- (UIResponder<UITextInput>*)fakeboxScribbleForwardingTarget {
  return self.locationBarCoordinator.omniboxScribbleForwardingTarget;
}


#pragma mark - OmniboxStateProvider

- (BOOL)isOmniboxFocused {
  CHECK(!IsChromeNextIaEnabled());
  return [self.locationBarCoordinator isOmniboxFocused];
}

#pragma mark - PopupMenuUIUpdating

- (void)updateUIForOverflowMenuIPHDisplayed {
  if (IsChromeNextIaEnabled()) {
    [_topToolbarViewController updateUIForOverflowMenuIPHDisplayed];
    [_bottomToolbarViewController updateUIForOverflowMenuIPHDisplayed];
  }
  for (id<ToolbarCoordinatee> coordinator in self.coordinators) {
    [coordinator.popupMenuUIUpdater updateUIForOverflowMenuIPHDisplayed];
  }
}

- (void)updateUIForIPHDismissed {
  if (IsChromeNextIaEnabled()) {
    [_topToolbarViewController updateUIForIPHDismissed];
    [_bottomToolbarViewController updateUIForIPHDismissed];
  }
  for (id<ToolbarCoordinatee> coordinator in self.coordinators) {
    [coordinator.popupMenuUIUpdater updateUIForIPHDismissed];
  }
}

- (void)setOverflowMenuBlueDot:(BOOL)hasBlueDot {
  if (IsChromeNextIaEnabled()) {
    [_topToolbarViewController setOverflowMenuBlueDot:hasBlueDot];
    [_bottomToolbarViewController setOverflowMenuBlueDot:hasBlueDot];
    [HandlerForProtocol(self.browser->GetCommandDispatcher(),
                        NewTabPageCommands) setNTPBlueDotVisible:hasBlueDot];
  }
  for (id<ToolbarCoordinatee> coordinator in self.coordinators) {
    [coordinator.popupMenuUIUpdater setOverflowMenuBlueDot:hasBlueDot];
  }
}

#pragma mark - PrimaryToolbarViewControllerDelegate

- (void)viewControllerTraitCollectionDidChange:
    (UITraitCollection*)previousTraitCollection {
  CHECK(!IsChromeNextIaEnabled());
  if (!_started) {
    return;
  }
  [self updateToolbarsLayout];
}

- (void)close {
  CHECK(!IsChromeNextIaEnabled());
  if (self.locationBarFocused) {
    id<SceneCommands> sceneHandler =
        HandlerForProtocol(self.browser->GetCommandDispatcher(), SceneCommands);
    [sceneHandler dismissModalDialogsWithCompletion:nil];
  }
}

- (void)locationBarExpandedInViewController:
    (PrimaryToolbarViewController*)viewController {
  CHECK(!IsChromeNextIaEnabled());
  // Do nothing.
}
- (void)locationBarContractedInViewController:
    (PrimaryToolbarViewController*)viewController {
  CHECK(!IsChromeNextIaEnabled());
  // Do nothing.
}

- (void)viewController:(PrimaryToolbarViewController*)viewController
    tabGroupIndicatorVisibilityUpdated:(BOOL)visible {
  CHECK(!IsChromeNextIaEnabled());
  // Do nothing.
}

#pragma mark - SideSwipeToolbarInteracting

- (BOOL)isInsideToolbar:(CGPoint)point {
  if (IsChromeNextIaEnabled()) {
    return
        [self isPoint:point insideViewController:_topToolbarViewController] ||
        [self isPoint:point insideViewController:_bottomToolbarViewController];
  }
  for (id<ToolbarCoordinatee> coordinator in self.coordinators) {

    if (IsVivaldiRunning()) {
      // Important(prio@vivaldi.com) - When tab bar is enabled we have to deduct
      // tab bar height and bottom safe area height from the toolbar view.
      // Otherwise, swiping on tab bar conflicts with the omnibox swipe gesture.
      CGRect toolbarFrame =
          CGRectInset([coordinator viewController].view.bounds, -1, -1);
      if (_omniboxPosition == ToolbarType::kSecondary && _tabBarEnabled &&
          coordinator == self.secondaryToolbarCoordinator) {
        CGFloat tabStripHeight = TabStripCollectionViewConstants.height;
        BrowserViewController* browserViewController =
            base::apple::ObjCCast<BrowserViewController>(
                self.baseViewController);
        if (browserViewController) {
          tabStripHeight = [browserViewController tabStripHeight];
        }
        toolbarFrame.size.height = toolbarFrame.size.height - tabStripHeight -
                                   VivaldiGlobalHelpers.safeAreaInsets.bottom;
      }

      CGPoint pointInToolbarCoordinates =
          [[coordinator viewController].view convertPoint:point fromView:nil];
      if (CGRectContainsPoint(toolbarFrame, pointInToolbarCoordinates)) {
        return YES;
      }

      continue;
    }  // End Vivaldi

    if ([self isPoint:point
            insideViewController:[coordinator viewController]]) {
      return YES;
    }
  }
  return NO;
}

#pragma mark - SideSwipeToolbarSnapshotProviding

- (UIImage*)toolbarSideSwipeSnapshotForWebState:(web::WebState*)webState
                                withToolbarType:(ToolbarType)toolbarType {
  if (IsChromeNextIaEnabled()) {
    ToolbarViewController* toolbar;
    ToolbarMediator* mediator;
    switch (toolbarType) {
      case ToolbarType::kPrimary:
        toolbar = _topToolbarViewController;
        mediator = _topToolbarMediator;
        break;
      case ToolbarType::kSecondary:
        toolbar = _bottomToolbarViewController;
        mediator = _bottomToolbarMediator;
        break;
    }

    [mediator updateConsumerWithWebState:webState animated:NO];

    UIView* toolbarView = toolbar.view;
    // The toolbar must be in the view hierarchy to be snapshotted.
    if (!toolbarView.window) {
      return nil;
    }
    UIImage* toolbarSnapshot = CaptureViewWithOption(
        toolbarView, toolbarView.window.screen.scale, kClientSideRendering);

    [mediator updateConsumerWithWebState:self.browser->GetWebStateList()
                                             ->GetActiveWebState()
                                animated:NO];

    return toolbarSnapshot;
  }

  AdaptiveToolbarCoordinator* adaptiveToolbarCoordinator =
      [self coordinatorWithToolbarType:toolbarType];

  [adaptiveToolbarCoordinator updateToolbarForSideSwipeSnapshot:webState];

  UIView* toolbarView = adaptiveToolbarCoordinator.viewController.view;
  // The toolbar must be in the view hierarchy to be snapshotted.
  if (!toolbarView.window) {
    return nil;
  }
  UIImage* toolbarSnapshot = CaptureViewWithOption(
      toolbarView, toolbarView.traitCollection.displayScale,
      kClientSideRendering);

  [adaptiveToolbarCoordinator resetToolbarAfterSideSwipeSnapshot];

  return toolbarSnapshot;
}

#pragma mark SideSwipeToolbarSnapshotProviding Private

/// Returns the coordinator coresponding to `toolbarType`.
- (AdaptiveToolbarCoordinator*)coordinatorWithToolbarType:
    (ToolbarType)toolbarType {
  CHECK(!IsChromeNextIaEnabled());
  switch (toolbarType) {
    case ToolbarType::kPrimary:

      if (IsVivaldiRunning())
        return self.vivaldiTopToolbarCoordinator; // End Vivaldi

      return self.primaryToolbarCoordinator;
    case ToolbarType::kSecondary:
      return self.secondaryToolbarCoordinator;
  }
}

/// Prepares location bar for a side swipe snapshot with`webState`.
- (void)updateLocationBarForSideSwipeSnapshot:(web::WebState*)webState {
  CHECK(!IsChromeNextIaEnabled());
  // Hide LocationBarView when taking a snapshot on a web state that is not the
  // active one, as the URL is not updated.
  if (webState != self.browser->GetWebStateList()->GetActiveWebState()) {
    [self.locationBarCoordinator.locationBarViewController.view setHidden:YES];
  }
}

/// Resets location bar after a side swipe snapshot.
- (void)resetLocationBarAfterSideSwipeSnapshot {
  CHECK(!IsChromeNextIaEnabled());
  [self.locationBarCoordinator.locationBarViewController.view setHidden:NO];
}

#pragma mark - GuidedTourCommands

- (void)highlightViewInStep:(GuidedTourStep)step {
  CHECK(!IsChromeNextIaEnabled());
  for (id<GuidedTourCommands> coordinator in self.coordinators) {
    [coordinator highlightViewInStep:step];
  }
}

- (void)stepCompleted:(GuidedTourStep)step {
  CHECK(!IsChromeNextIaEnabled());
  for (id<GuidedTourCommands> coordinator in self.coordinators) {
    [coordinator stepCompleted:step];
  }
}

#pragma mark - ComposeboxAnimationBase

- (void)setEntrypointViewHidden:(BOOL)hidden {
  if (IsChromeNextIaEnabled()) {
    [_topToolbarViewController setLocationBarHidden:hidden];
    [_bottomToolbarViewController setLocationBarHidden:hidden];
    return;
  }
  AdaptiveToolbarCoordinator* adaptiveToolbarCoordinator =
      [self coordinatorWithToolbarType:_omniboxPosition];
  adaptiveToolbarCoordinator.viewController.locationBarContainer.hidden =
      hidden;
}

- (UIView*)entrypointViewVisualCopy {
  if (IsChromeNextIaEnabled()) {
    if ([self isToolbarPositionBottom] || [self isNTP]) {
      return nil;
    }

    UIView* entrypointCopy =
        [_topToolbarViewController locationBarContainerCopy];
    UIView* locationBarSteadyViewVisualCopy =
        _topLocationBarCoordinator.locationBarSteadyViewVisualCopy;
    [entrypointCopy addSubview:locationBarSteadyViewVisualCopy];
    locationBarSteadyViewVisualCopy.translatesAutoresizingMaskIntoConstraints =
        NO;

    AddSameConstraints(entrypointCopy, locationBarSteadyViewVisualCopy);
    return entrypointCopy;
  }

  if (_omniboxPosition == ToolbarType::kSecondary || [self isNTP]) {
    return nil;
  }

  AdaptiveToolbarCoordinator* adaptiveToolbarCoordinator =
      [self coordinatorWithToolbarType:_omniboxPosition];
  UIView* locationBarContainer =
      adaptiveToolbarCoordinator.viewController.locationBarContainer;

  UIView* entrypointCopy = [[UIView alloc] init];
  entrypointCopy.frame =
      [locationBarContainer convertRect:locationBarContainer.bounds toView:nil];
  entrypointCopy.layer.cornerRadius = locationBarContainer.layer.cornerRadius;
  entrypointCopy.backgroundColor = locationBarContainer.backgroundColor;
  UIView* locationBarSteadyViewVisualCopy =
      self.locationBarCoordinator.locationBarSteadyViewVisualCopy;
  [entrypointCopy addSubview:locationBarSteadyViewVisualCopy];
  locationBarSteadyViewVisualCopy.translatesAutoresizingMaskIntoConstraints =
      NO;

  [NSLayoutConstraint activateConstraints:@[
    [locationBarSteadyViewVisualCopy.centerXAnchor
        constraintEqualToAnchor:entrypointCopy.centerXAnchor],
    [locationBarSteadyViewVisualCopy.centerYAnchor
        constraintEqualToAnchor:entrypointCopy.centerYAnchor],
    [locationBarSteadyViewVisualCopy.widthAnchor
        constraintEqualToAnchor:entrypointCopy.widthAnchor],
    [locationBarSteadyViewVisualCopy.heightAnchor
        constraintEqualToAnchor:entrypointCopy.heightAnchor],
  ]];

  return entrypointCopy;
}

#pragma mark - LocationBarBadgeCommands

- (void)updateBadgeConfig:(LocationBarBadgeConfiguration*)config {
  CHECK(IsChromeNextIaEnabled());
  [_topLocationBarCoordinator updateBadgeConfig:config];
  [_bottomLocationBarCoordinator updateBadgeConfig:config];
}

- (void)updateColorForIPH {
  CHECK(IsChromeNextIaEnabled());
  [_topLocationBarCoordinator updateColorForIPH];
  [_bottomLocationBarCoordinator updateColorForIPH];
}

- (void)markDisplayedBadgeAsUnread:(BOOL)read {
  CHECK(IsChromeNextIaEnabled());
  [_topLocationBarCoordinator markDisplayedBadgeAsUnread:read];
  [_bottomLocationBarCoordinator markDisplayedBadgeAsUnread:read];
}

- (void)setBadgeCustomLeadingViewType:(CustomLeadingViewType)type {
  CHECK(IsChromeNextIaEnabled());
  [_topLocationBarCoordinator setBadgeCustomLeadingViewType:type];
  [_bottomLocationBarCoordinator setBadgeCustomLeadingViewType:type];
}

#pragma mark - ReaderModeChipCommands

- (void)showReaderModeChip {
  if (IsChromeNextIaEnabled()) {
    [_topLocationBarCoordinator.readerModeChipHandler showReaderModeChip];
    [_bottomLocationBarCoordinator.readerModeChipHandler showReaderModeChip];
  } else {
    [self.locationBarCoordinator.readerModeChipHandler showReaderModeChip];
  }
}

- (void)hideReaderModeChip {
  if (IsChromeNextIaEnabled()) {
    [_topLocationBarCoordinator.readerModeChipHandler hideReaderModeChip];
    [_bottomLocationBarCoordinator.readerModeChipHandler hideReaderModeChip];
  } else {
    [self.locationBarCoordinator.readerModeChipHandler hideReaderModeChip];
  }
}

#pragma mark - ContextualPanelEntrypointCommands

- (void)notifyContextualPanelEntrypointIPHDismissed {
  CHECK(IsChromeNextIaEnabled());
  [_topLocationBarCoordinator notifyContextualPanelEntrypointIPHDismissed];
  [_bottomLocationBarCoordinator notifyContextualPanelEntrypointIPHDismissed];
}

- (void)cancelContextualPanelEntrypointLoudMoment {
  CHECK(IsChromeNextIaEnabled());
  [_topLocationBarCoordinator cancelContextualPanelEntrypointLoudMoment];
  [_bottomLocationBarCoordinator cancelContextualPanelEntrypointLoudMoment];
}

#pragma mark - PageActionMenuEntryPointCommands

- (void)toggleEntryPointHighlight:(BOOL)highlight {
  CHECK(IsChromeNextIaEnabled());
  CHECK(IsPageActionMenuEnabled());
  [_topLocationBarCoordinator
      togglePageActionMenuEntryPointHighlight:highlight];
  [_bottomLocationBarCoordinator
      togglePageActionMenuEntryPointHighlight:highlight];
}

#pragma mark - ToolbarCommands

- (void)indicateLensOverlayVisible:(BOOL)lensOverlayVisible {
  if (IsChromeNextIaEnabled()) {
    [_topLocationBarCoordinator setLensOverlayVisible:lensOverlayVisible];
    [_bottomLocationBarCoordinator setLensOverlayVisible:lensOverlayVisible];
    return;
  } else {
    [self.locationBarCoordinator setLensOverlayVisible:lensOverlayVisible];
  }

  for (id<ToolbarCommands> coordinator in self.coordinators) {
    [coordinator indicateLensOverlayVisible:lensOverlayVisible];
  }
}

- (void)focusLocationBarForVoiceOver {
  if (IsChromeNextIaEnabled()) {
    if (_topToolbarViewController.hasOmnibox) {
      [_topLocationBarCoordinator focusOmniboxForVoiceOver];
    } else {
      [_bottomLocationBarCoordinator focusOmniboxForVoiceOver];
    }
  } else {
    id<OmniboxCommands> omniboxHandler = HandlerForProtocol(
        self.browser->GetCommandDispatcher(), OmniboxCommands);
    [omniboxHandler focusOmniboxForVoiceOver];
  }
}



#pragma mark - ToolbarMediatorDelegate

- (void)transitionOmniboxToToolbarType:(ToolbarType)toolbarType {
  if (IsChromeNextIaEnabled()) {
    return;
  }

  // Only the visible coordinator (normal vs. incognito) is allowed to update
  // the shared LayoutState.
  Browser* activeBrowser = self.browser->GetSceneState()
                               .browserProviderInterface
                               .currentBrowserProvider.browser;
  if (activeBrowser && self.browser != activeBrowser) {
    return;
  }

  ToolbarPosition position = (toolbarType == ToolbarType::kSecondary)
                                 ? ToolbarPosition::kBottom
                                 : ToolbarPosition::kTop;
  // When Chrome Next is disabled, the active toolbar position changes
  // dynamically during focus/NTP transitions (managed by
  // LegacyToolbarMediator). Update the LayoutState to keep it in sync.
  [self updateLayoutStateToolbarPosition:position];
}

- (void)transitionSteadyStateOmniboxToToolbarType:(ToolbarType)toolbarType {
  _steadyStateOmniboxPosition = toolbarType;
}

- (CGFloat)keyboardAttachedBottomOmniboxHeight {

  if (IsVivaldiRunning()) {
    if (!self.locationBarFocused ||
        _omniboxPosition != ToolbarType::kSecondary) {
      return 0;
    }

    return self.locationBarCoordinator.locationBarViewController.view.frame.size
               .height +
           2 * kBottomAdaptiveLocationBarTopMargin;
  }  // End Vivaldi

  if (IsChromeNextIaEnabled()) {
    if (_layoutState.appBarPosition == AppBarPosition::kBottom) {
      return kKeyboardAttachedOmniboxBottomPadding;
    } else {
      return kKeyboardAttachedOmniboxBottomPaddingLandscape;
    }
  }
  return 0;
}

#pragma mark - FullscreenBrowserAgentObserving

- (void)fullscreenWillUpdateObscuredInsetRange:(FullscreenBrowserAgent*)agent {
  agent->AddObscuredInsetRange(UIRectEdgeTop,
                               [self collapsedPrimaryToolbarHeight],
                               [self expandedPrimaryToolbarHeight]);
  agent->AddObscuredInsetRange(UIRectEdgeBottom,
                               [self collapsedSecondaryToolbarHeight],
                               [self expandedSecondaryToolbarHeight]);
}

- (void)fullscreenWillUpdateState:(FullscreenBrowserAgent*)agent {
  CGFloat topMin = [self collapsedPrimaryToolbarHeight];
  CGFloat topMax = [self expandedPrimaryToolbarHeight];
  CGFloat topInset = topMin + (topMax - topMin) * agent->top_progress();
  agent->AddObscuredInset(UIRectEdgeTop, topInset);
  [_topToolbarViewController updateForFullscreenProgress:agent->top_progress()];

  CGFloat bottomMin = [self collapsedSecondaryToolbarHeight];
  CGFloat bottomMax = [self expandedSecondaryToolbarHeight];
  CGFloat bottomInset =
      bottomMin + (bottomMax - bottomMin) * agent->bottom_progress();
  agent->AddObscuredInset(UIRectEdgeBottom, bottomInset);
  [_bottomToolbarViewController
      updateForFullscreenProgress:agent->bottom_progress()];
}

#pragma mark - LayoutStateObserver

- (void)layoutState:(LayoutState*)layoutState
    didChangeToolbarPosition:(ToolbarPosition)toolbarPosition {
  [self updateLayoutForToolbarPosition:toolbarPosition];
}

#pragma mark - Private

/// Whether the omnibox is currently in edit state.
- (BOOL)inEditState {
  CHECK(!IsChromeNextIaEnabled());
  return [self isOmniboxFirstResponder] || [self showingOmniboxPopup];
}

/// Returns primary and secondary coordinator in a array. Helper to call method
/// on both coordinators.
- (NSArray<id<ToolbarCoordinatee>>*)coordinators {

  if (IsVivaldiRunning()) {
    return @[
      self.vivaldiTopToolbarCoordinator,
      self.secondaryToolbarCoordinator
    ];
  } // End Vivaldi

  return @[ self.primaryToolbarCoordinator, self.secondaryToolbarCoordinator ];
}

/// Returns the trait environment of the toolbars.
- (id<UITraitEnvironment>)traitEnvironment {
  return self.primaryToolbarViewController;
}

/// Updates toolbars layout whith current omnibox focus state and trait
/// collection.
- (void)updateToolbarsLayout {
  CHECK(!IsChromeNextIaEnabled());
  [self.legacyToolbarMediator
      toolbarTraitCollectionChangedTo:self.traitEnvironment.traitCollection];
  BOOL omniboxFocused = [self inEditState];
  [self.orchestrator
      transitionToStateOmniboxFocused:omniboxFocused
                      toolbarExpanded:omniboxFocused &&
                                      !CanShowTabStrip(self.traitEnvironment)
                              trigger:[self omniboxFocusTrigger]
                             animated:NO
                           completion:nil];
}

/// Returns the appropriate `OmniboxFocusTrigger` depending on whether this is
/// an incognito browser, the NTP is displayed, and whether the fakebox was
/// pinned if it was selected.
- (OmniboxFocusTrigger)omniboxFocusTrigger {
  web::WebState* webState =
      self.browser->GetWebStateList()->GetActiveWebState();
  if (!webState) {
    return OmniboxFocusTrigger::kOther;
  }
  if (!IsVisibleURLNewTabPage(webState)) {
    return OmniboxFocusTrigger::kOther;
  }

  // (De)focusing on NTP.

  if (self.isOffTheRecord || !IsSplitToolbarMode(self.traitEnvironment)) {
    return _focusedFromFakebox ? OmniboxFocusTrigger::kUnpinnedFakebox
                               : OmniboxFocusTrigger::kNTPOmnibox;
  }

  return _fakeboxPinned ? OmniboxFocusTrigger::kPinnedFakebox
                        : OmniboxFocusTrigger::kUnpinnedFakebox;
}

- (void)updateOrchestratorAnimatee {
  CHECK(!IsChromeNextIaEnabled());
  id<ToolbarAnimatee> updatedToolbarAnimatee =
      _omniboxPosition == ToolbarType::kPrimary
          ? self.primaryToolbarCoordinator.toolbarAnimatee
          : self.secondaryToolbarCoordinator.toolbarAnimatee;
  BOOL willChangeToolbarAnimatee =
      updatedToolbarAnimatee != self.orchestrator.toolbarAnimatee;

  // If a change occurs, clear any previous animation effects to prevent the
  // toolbar from remaining expanded
  if (willChangeToolbarAnimatee) {
    [self.orchestrator
        transitionToStateOmniboxFocused:NO
                        toolbarExpanded:NO
                                trigger:OmniboxFocusTrigger::kOther
                               animated:NO
                             completion:nil];
  }

  self.orchestrator.toolbarAnimatee = updatedToolbarAnimatee;
  self.orchestrator.locationBarAnimatee =
      [self.locationBarCoordinator locationBarAnimatee];
  self.orchestrator.editViewAnimatee =
      [self.locationBarCoordinator editViewAnimatee];
}

- (BOOL)isNTP {
  web::WebState* webState =
      self.browser->GetWebStateList()->GetActiveWebState();
  if (!webState) {
    return NO;
  }
  return IsVisibleURLNewTabPage(webState);
}

// Creates a new toolbar view controller, for the associated `mediator`.
- (ToolbarViewController*)
    createToolbarViewControllerForMediator:(ToolbarMediator*)mediator
                               locationBar:(UIViewController*)locationBar
                               topPosition:(BOOL)topPosition {
  CHECK(IsChromeNextIaEnabled());

  BOOL incognito = self.profile->IsOffTheRecord();

  Browser* browser = self.browser;
  CommandDispatcher* dispatcher = browser->GetCommandDispatcher();

  ToolbarViewController* toolbarViewController =
      [[ToolbarViewController alloc] initInIncognito:incognito
                                         topPosition:topPosition];
  toolbarViewController.layoutGuideCenter =
      LayoutGuideCenterForBrowser(browser);
  toolbarViewController.layoutState = _layoutState;
  ToolbarButtonFactory* toolbarButtonFactory =
      [[ToolbarButtonFactory alloc] initWithIncognito:incognito];
  if (!incognito) {
    toolbarButtonFactory.geminiHandler =
        HandlerForProtocol(browser->GetCommandDispatcher(), GeminiCommands);
  }
  toolbarViewController.buttonFactory = toolbarButtonFactory;
  toolbarViewController.mutator = mediator;
  toolbarViewController.browserCoordinatorHandler =
      HandlerForProtocol(dispatcher, BrowserCoordinatorCommands);
  toolbarViewController.popupMenuHandler =
      HandlerForProtocol(dispatcher, PopupMenuCommands);
  toolbarViewController.activityServiceHandler =
      HandlerForProtocol(dispatcher, ActivityServiceCommands);
  toolbarViewController.sceneHandler =
      HandlerForProtocol(dispatcher, SceneCommands);
  toolbarViewController.toolbarHeightDelegate = self.toolbarHeightDelegate;
  toolbarViewController.locationBarViewController = locationBar;
  toolbarViewController.bannerPromoDelegate = mediator;

  if (incognito) {
    toolbarViewController.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
  }

  mediator.consumer = toolbarViewController;

  return toolbarViewController;
}

// Creates a new location bar coordinator.
- (LocationBarCoordinator*)createLocationBarCoordinatorActive:(BOOL)active
                                                  topPosition:
                                                      (BOOL)topPosition {
  LocationBarCoordinator* coordinator =
      [[LocationBarCoordinator alloc] initWithBrowser:self.browser];
  [coordinator start];
  [coordinator setTopPosition:topPosition];
  [coordinator setLocationBarActive:active];

  return coordinator;
}

// Creates a new toolbar mediator.
- (ToolbarMediator*)createToolbarMediatorTopPosition:(BOOL)topPosition {
  CHECK(IsChromeNextIaEnabled());

  Browser* browser = self.browser;
  BrowserActionFactory* actionFactory = [[BrowserActionFactory alloc]
      initWithBrowser:browser
             scenario:kMenuScenarioHistogramToolbarMenu];

  BOOL isIncognito = self.profile->IsOffTheRecord();
  DefaultBrowserBannerPromoAppAgent* agent = nil;
  if (topPosition && !isIncognito) {
    agent = [DefaultBrowserBannerPromoAppAgent
        agentFromApp:browser->GetSceneState().profileState.appState];
  }

  ProfileIOS* profile = self.profile;
  AuthenticationService* authService =
      AuthenticationServiceFactory::GetForProfile(profile);
  GeminiService* geminiService = GeminiServiceFactory::GetForProfile(profile);
  GeminiBrowserAgent* geminiBrowserAgent =
      GeminiBrowserAgent::FromBrowser(browser);

  ToolbarMediator* toolbarMediator = [[ToolbarMediator alloc]
                 initWithIncognito:isIncognito
                      webStateList:browser->GetWebStateList()
                     actionFactory:actionFactory
                       prefService:profile->GetPrefs()
              fullscreenController:FullscreenController::FromBrowser(browser)
            fullscreenBrowserAgent:FullscreenBrowserAgent::FromBrowser(browser)
                       topPosition:topPosition
      defaultBrowserBannerAppAgent:agent
             authenticationService:authService
                     geminiService:geminiService
                geminiBrowserAgent:geminiBrowserAgent];
  toolbarMediator.navigationBrowserAgent =
      WebNavigationBrowserAgent::FromBrowser(browser);
  toolbarMediator.tabBasedIPHAgent =
      TabBasedIPHBrowserAgent::FromBrowser(browser);
  if (IsFullscreenRefactoringEnabled()) {
    toolbarMediator.fullscreenCommands =
        HandlerForProtocol(browser->GetCommandDispatcher(), FullscreenCommands);
  }
  toolbarMediator.settingsHandler =
      HandlerForProtocol(browser->GetCommandDispatcher(), SettingsCommands);
  toolbarMediator.geminiHandler =
      HandlerForProtocol(browser->GetCommandDispatcher(), GeminiCommands);
  toolbarMediator.baseViewController = self.baseViewController;
  toolbarMediator.sceneHandler =
      HandlerForProtocol(browser->GetCommandDispatcher(), SceneCommands);

  return toolbarMediator;
}

// Returns whether the toolbar position is currently at the bottom of the
// screen.
- (BOOL)isToolbarPositionBottom {
  return _layoutState.toolbarPosition == ToolbarPosition::kBottom;
}

// Returns whether `point` in window coordinates is inside the frame of
// `viewController`'s view.
- (BOOL)isPoint:(CGPoint)point
    insideViewController:(UIViewController*)viewController {
  // The toolbar bounds are inset by 1 because CGRectContainsPoint does
  // include points on the max X and Y edges, which will happen frequently
  // with edge swipes from the right side.
  CGRect toolbarBounds = CGRectInset(viewController.view.bounds, -1, -1);
  CGPoint pointInToolbarCoordinates = [viewController.view convertPoint:point
                                                               fromView:nil];
  return CGRectContainsPoint(toolbarBounds, pointInToolbarCoordinates);
}

// Updates the LayoutState's toolbarPosition property.
- (void)updateLayoutStateToolbarPosition:(ToolbarPosition)position {
  CHECK(!IsChromeNextIaEnabled());
  [_layoutState setToolbarPosition:position passKey:PassKey()];
}

// Updates the visual layout and child coordinators to match the given position.
- (void)updateLayoutForToolbarPosition:(ToolbarPosition)toolbarPosition {
  BOOL isToolbarAtBottom = toolbarPosition == ToolbarPosition::kBottom;
  _omniboxPosition =
      isToolbarAtBottom ? ToolbarType::kSecondary : ToolbarType::kPrimary;

  if (!IsChromeNextIaEnabled()) {
    [self updateOrchestratorAnimatee];
  }

  if (IsChromeNextIaEnabled()) {
    [_topLocationBarCoordinator setLocationBarActive:!isToolbarAtBottom];
    [_bottomLocationBarCoordinator setLocationBarActive:isToolbarAtBottom];
  } else if (IsVivaldiRunning()) {
    [self vivaldiTransitionOmniboxToToolbarType:_omniboxPosition];
  } else { // End Vivaldi
    if (isToolbarAtBottom) {
      [self.secondaryToolbarCoordinator
          setLocationBarViewController:self.locationBarCoordinator
                                           .locationBarViewController];
      [self.primaryToolbarCoordinator setLocationBarViewController:nil];
    } else {
      [self.primaryToolbarCoordinator
          setLocationBarViewController:self.locationBarCoordinator
                                           .locationBarViewController];
      [self.secondaryToolbarCoordinator setLocationBarViewController:nil];
    }
  }

  [self.toolbarHeightDelegate toolbarsHeightChanged];
}

// Helper for public methods returning the toolbar height. Returns whether the
// toolbar is hidden. This check requires the active WebState to be the NTP to
// ensure that during NTP-to-webpage transitions, the full toolbar height is
// returned immediately when the navigation starts. Otherwise, checking only
// `toolbarView.isHidden` would return a height of 0.0 to the caller during the
// loading phase, causing the webpage to layout with incorrect content insets.
- (BOOL)isToolbarHidden:(UIView*)toolbarView {
  CHECK(IsChromeNextIaEnabled());
  if (!toolbarView.isHidden) {
    return NO;
  }

  web::WebState* webState =
      self.browser->GetWebStateList()->GetActiveWebState();
  if (!webState) {
    // No web state to have a toolbar for.
    return YES;
  }

  // If the active page is the NTP, the toolbar is legitimately hidden.
  if (IsVisibleURLNewTabPage(webState)) {
    return YES;
  }

  // If the toolbar is hidden, but in the middle of transitioning from the NTP
  // to a non-NTP page, treat it as NOT hidden so the full height is returned
  // immediately.
  BOOL transitioningFromNTP =
      webState->IsLoading() && IsUrlNtp(webState->GetLastCommittedURL());
  if (transitioningFromNTP) {
    return NO;
  }

  return YES;
}

#pragma mark - VIVALDI

- (void)resetToolbarAfterSideSwipeSnapshot {
  [self.locationBarCoordinator.locationBarViewController.view setHidden:NO];
}

- (PrimaryToolbarView*)primaryToolbarView {
  if (_omniboxPosition == ToolbarType::kPrimary) {
    PrimaryToolbarView* primaryView =
      (PrimaryToolbarView*)[self.primaryToolbarViewController view];
    return primaryView;
  } else {
    PrimaryToolbarView* primaryView =
      (PrimaryToolbarView*)[self.primaryToolbarCoordinator.viewController view];
    return primaryView;
  }
}

- (SecondaryToolbarView*)secondaryToolbarView {
  SecondaryToolbarView* secondaryView =
    (SecondaryToolbarView*)
      [self.secondaryToolbarCoordinator.viewController view];
  return secondaryView;
}

// Returns the toolbar button stack view for Secondary Toolbar. This is hidden
// when bottom omnibox and tab bar both enabled.
- (UIStackView*)secondaryToolbarButtonStackView {
  SecondaryToolbarViewController* viewController =
      (SecondaryToolbarViewController*)
          self.secondaryToolbarCoordinator.viewController;
  return viewController.toolbarButtonStackView;
}

// Sets the correct toolbar after settings is changed.
- (void)vivaldiTransitionOmniboxToToolbarType:(ToolbarType)toolbarType {
  [self updateProgressBarVisibilityWithToolbarType:toolbarType];
  switch (toolbarType) {
    case ToolbarType::kPrimary:
      [self.vivaldiTopToolbarCoordinator
          setLocationBarViewController:
              self.locationBarCoordinator.locationBarViewController];
      [self.secondaryToolbarCoordinator setLocationBarViewController:nil];
      [self.primaryToolbarCoordinator
          setLocationBarViewController:nil];

      if (self.vivaldiTopToolbarCoordinator.viewController) {
        self.orchestrator.toolbarAnimatee =
            self.vivaldiTopToolbarCoordinator.toolbarAnimatee;
      }
      break;
    case ToolbarType::kSecondary:
      [self.primaryToolbarCoordinator
          setLocationBarViewController:
              self.locationBarCoordinator.locationBarViewController];
      [self.secondaryToolbarCoordinator
          setLocationBarViewController
              :self.primaryToolbarCoordinator.viewController];
      [self.vivaldiTopToolbarCoordinator setLocationBarViewController:nil];

      if (self.primaryToolbarCoordinator.viewController) {
        self.orchestrator.toolbarAnimatee =
            self.primaryToolbarCoordinator.toolbarAnimatee;
      }
      break;
  }
  [self updateVivaldiToolsMenuButtonGuides];
}

// Maintain the visibility of progress bar for the toolbar. Each toolbar has
// a progress bar. Make sure only one progress bar is visible, and based on the
// ToolbarType.
- (void)updateProgressBarVisibilityWithToolbarType:(ToolbarType)toolbarType {
  BOOL bottomOmniboxEnabled = toolbarType == ToolbarType::kSecondary;
  [self.primaryToolbarCoordinator
      setLocationBarShouldShowProgressBar:!bottomOmniboxEnabled];
  [self.vivaldiTopToolbarCoordinator
      setLocationBarShouldShowProgressBar:!bottomOmniboxEnabled];
  [self.secondaryToolbarCoordinator
      setLocationBarShouldShowProgressBar:bottomOmniboxEnabled];
}

- (void)updateVivaldiToolsMenuButtonGuides {
  VivaldiUpdateToolsMenuButtonGuides(
      self.layoutGuideCenter, _omniboxPosition,
      self.primaryToolbarCoordinator.viewController,
      self.vivaldiTopToolbarCoordinator.viewController);
}

#pragma mark - ToolbarMediatorDelegate (Vivaldi)

- (void)transitionOmniboxToToolbarType:(ToolbarType)toolbarType
                         tabBarEnabled:(BOOL)tabBarEnabled {
  _omniboxPosition = toolbarType;
  _tabBarEnabled = tabBarEnabled;

  ToolbarPosition previousToolbarPosition = _layoutState.toolbarPosition;
  [self transitionOmniboxToToolbarType:toolbarType];
  if (previousToolbarPosition == _layoutState.toolbarPosition) {
    [self vivaldiTransitionOmniboxToToolbarType:toolbarType];
  }

  [self updateToolbarWithBottomOmniboxEnabled:
      toolbarType == ToolbarType::kSecondary
                                tabBarEnabled:tabBarEnabled];
  self.secondaryToolbarButtonStackView.hidden =
      (tabBarEnabled && toolbarType == ToolbarType::kSecondary) ||
          !IsSplitToolbarMode(self.traitEnvironment);
  [self updateToolbarsLayout];
  [self updateVivaldiToolsMenuButtonGuides];

  AdaptiveToolbarCoordinator* adaptiveToolbarCoordinator =
      [self coordinatorWithToolbarType:toolbarType];
  if (![self activeWebState])
    return;
  [adaptiveToolbarCoordinator updateConsumerForWebState:[self activeWebState]];
}

- (void)transitionSteadyStateOmniboxToToolbarType:(ToolbarType)toolbarType
                                    tabBarEnabled:(BOOL)tabBarEnabled {
  _steadyStateOmniboxPosition = toolbarType;
}

- (void)updateToolbarWithBottomOmniboxEnabled:(BOOL)bottomOmniboxEnabled
                                tabBarEnabled:(BOOL)tabBarEnabled {
  PrimaryToolbarView* primaryView = [self primaryToolbarView];
  if (primaryView) {
    primaryView.bottomOmniboxEnabled = bottomOmniboxEnabled;
    primaryView.tabBarEnabled = tabBarEnabled;
    [primaryView redrawToolbarButtons];

    // Add guide to omnibox view to track omnibox position in the subviews.
    [self.layoutGuideCenter
        referenceView:primaryView.locationBarContainer
            underName:kTopOmniboxGuide
        forcesSynchronousLayoutUpdates:YES];
  }

  SecondaryToolbarView* secondaryView = [self secondaryToolbarView];
  if (secondaryView) {
    secondaryView.bottomOmniboxEnabled = bottomOmniboxEnabled;
    secondaryView.tabBarEnabled = tabBarEnabled;
  }

  [self updateVivaldiToolsMenuButtonGuides];
}

/// Triggers updating the toolbar accent color with omnibox focus state
/// when tab bar is disabled. When omnibox is focused custom or dynamic accent
/// color is replaced by default background color to match the color of omnibox
/// search results  view color.
- (void)updateToolbarBackgroundColorWithOmniboxFocus:(BOOL)focused {
  if (_tabBarEnabled)
    return;

  AdaptiveToolbarCoordinator* adaptiveToolbarCoordinator =
      [self coordinatorWithToolbarType:_omniboxPosition];
  [adaptiveToolbarCoordinator.viewController setIsOmniboxFocused:focused];

  AdaptiveToolbarViewController* primaryVC;
  if (_omniboxPosition == ToolbarType::kSecondary) {
    primaryVC = (PrimaryToolbarViewController*)
        [self.primaryToolbarCoordinator viewController];
  }
  if (primaryVC) {
    [primaryVC setIsOmniboxFocused:focused];
  }
}

#pragma mark - Adblocker manager

- (void)initialiseAdblockManager {
  if (!self.browser)
    return;
  self.adblockManager =
      [[VivaldiATBManager alloc] initWithBrowser:self.browser];
  self.adblockManager.consumer = self;
  [self updateVivaldiShieldState];
}

- (void)updateVivaldiShieldState {
  [self.locationBarCoordinator
      setTrackerBlockerSettingForActiveWebState:
          [self atbSettingsForActiveWebState]];
}

- (ATBSettingType)atbSettingsForActiveWebState {
  web::WebState* webState = [self activeWebState];
  if (!webState)
    return [self globalATBSetting];

  BOOL isNTP = IsVisibleURLNewTabPage(webState);
  // Return Global Adblocker Settings for New Tab Page.
  if (isNTP)
    return [self globalATBSetting];

  // Find the settings for last committed URL of the active WebState.
  NSString* lastCommittedURLString =
      base::SysUTF8ToNSString(webState->GetLastCommittedURL().spec());
  NSURL* lastCommittedURL = [NSURL URLWithString:lastCommittedURLString];
  NSString* host = [lastCommittedURL host];
  if (!host)
    return [self globalATBSetting];
  return [self.adblockManager blockingSettingForDomain:host];
}

- (ATBSettingType)globalATBSetting {
  return [self.adblockManager globalBlockingSetting];
}

- (web::WebState*)activeWebState {
  return self.browser->GetWebStateList()->GetActiveWebState();
}

#pragma mark: - VivaldiATBConsumer
- (void)didRefreshSettingOptions:(NSArray*)options {
  if (options.count > 0)
    [self updateVivaldiShieldState];
}

- (void)didRefreshExceptionsList:(NSArray*)exceptions {
  [self updateVivaldiShieldState];
}

- (void)ruleServiceStateDidLoad {
  [self updateVivaldiShieldState];
}

#pragma mark - LocationBarSteadyViewConsumer

- (void)updateLocationText:(NSString*)text clipTail:(BOOL)clipTail {
  // No op.
}

- (void)updateLocationText:(NSString*)text
                    domain:(NSString*)domain
                  showFull:(BOOL)showFull
                  clipTail:(BOOL)clipTail {
  [self.steadyViewConsumer updateLocationText:text
                                       domain:domain
                                     showFull:showFull
                                     clipTail:clipTail];
  [self updateVivaldiShieldState];
}

- (void)updateLocationIcon:(UIImage*)icon
        securityStatusText:(NSString*)statusText {
  [self.steadyViewConsumer updateLocationIcon:icon
                           securityStatusText:statusText];
}

- (void)updateAfterNavigatingToNTP {
  [self.steadyViewConsumer updateAfterNavigatingToNTP];
}

- (void)updateLocationShareable:(BOOL)shareable {
  [self.steadyViewConsumer updateLocationShareable:shareable];
}

- (void)attemptShowingLensOverlayIPH {
  // No op.
}

- (void)recordLensOverlayAvailability {
  // No op.
}

- (void)updateAIHubNewBadgeVisibility {
  // No op.
}
// End Vivaldi

@end
