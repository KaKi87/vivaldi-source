// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/toolbar/legacy/ui_bundled/legacy_toolbar_mediator.h"

#import "base/memory/raw_ptr.h"
#import "components/omnibox/browser/omnibox_pref_names.h"
#import "ios/chrome/browser/ntp/model/new_tab_page_util.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/prefs/pref_backed_boolean.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "ios/chrome/browser/shared/model/utils/first_run_util.h"
#import "ios/chrome/browser/shared/model/web_state_list/active_web_state_observation_forwarder.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list_observer_bridge.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/public/features/system_flags.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/toolbar/legacy/ui_bundled/public/toolbar_omnibox_consumer.h"
#import "ios/web/public/ui/crw_web_view_proxy.h"
#import "ios/web/public/web_state.h"
#import "ios/web/public/web_state_observer_bridge.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/ui/helpers/vivaldi_global_helpers.h"
#import "ios/ui/settings/tabs/vivaldi_tab_setting_prefs.h"
#import "ios/ui/vivaldi_symbols/vivaldi_symbol_names.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using vivaldi::IsVivaldiRunning;
// End Vivaldi

@interface LegacyToolbarMediator () <BooleanObserver,
                                     CRWWebStateObserver,
                                     WebStateListObserving>

/// Type of toolbar containing the omnibox. Unlike
/// `steadyStateOmniboxPosition`, this tracks the omnibox position at all
/// time.
@property(nonatomic, assign) ToolbarType omniboxPosition;
/// Type of the toolbar that contains the omnibox when it's not focused. The
/// animation of focusing/defocusing the omnibox changes depending on this
/// position.
@property(nonatomic, assign) ToolbarType steadyStateOmniboxPosition;

// Vivaldi
@property(nonatomic, assign) BOOL isTabBarEnabled;
// End Vivaldi

@end

@implementation LegacyToolbarMediator {
  /// Bridges C++ WebStateObserver methods to this mediator.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserverBridge;

  /// Forwards observer methods for active WebStates in the WebStateList to
  /// this mediator.
  std::unique_ptr<ActiveWebStateObservationForwarder>
      _activeWebStateObservationForwarder;

  /// Observes web state activation.
  std::unique_ptr<WebStateListObserverBridge> _webStateListObserverBridge;

  raw_ptr<WebStateList> _webStateList;

  /// Pref tracking if bottom omnibox is enabled.
  PrefBackedBoolean* _bottomOmniboxEnabled;
  /// Whether the omnibox is currently focused.
  BOOL _locationBarFocused;
  /// Whether the browser is incognito.
  BOOL _isIncognito;
  /// Whether the last navigated web state is NTP.
  BOOL _isNTP;
  /// Last trait collection of the toolbars.
  UITraitCollection* _toolbarTraitCollection;

  // Vivaldi
  /// Pref tracking if tab bar is enabled.
  PrefBackedBoolean* _tabBarEnabled;
  // End Vivaldi

}

- (instancetype)initWithWebStateList:(WebStateList*)webStateList
                         isIncognito:(BOOL)isIncognito {
  if ((self = [super init])) {
    _webStateList = webStateList;
    _isIncognito = isIncognito;

    _webStateObserverBridge =
        std::make_unique<web::WebStateObserverBridge>(self);
    _activeWebStateObservationForwarder =
        std::make_unique<ActiveWebStateObservationForwarder>(
            webStateList, _webStateObserverBridge.get());
    _webStateListObserverBridge =
        std::make_unique<WebStateListObserverBridge>(self);
    _webStateList->AddObserver(_webStateListObserverBridge.get());

    if (IsBottomOmniboxAvailable()) {
      _bottomOmniboxEnabled = [[PrefBackedBoolean alloc]
          initWithPrefService:GetApplicationContext()->GetLocalState()
                     prefName:omnibox::kIsOmniboxInBottomPosition];
      [_bottomOmniboxEnabled setObserver:self];
      // Initialize to the correct value.
      [self booleanDidChange:_bottomOmniboxEnabled];
    }
  }
  return self;
}

- (void)disconnect {
  _activeWebStateObservationForwarder = nullptr;
  _webStateObserverBridge = nullptr;
  _webStateList->RemoveObserver(_webStateListObserverBridge.get());
  _webStateListObserverBridge = nullptr;

  _webStateList = nullptr;
  [_bottomOmniboxEnabled stop];
  [_bottomOmniboxEnabled setObserver:nil];
  _bottomOmniboxEnabled = nil;

  // Vivaldi
  [self stopObservingTabBarState];
  // End Vivaldi

}

- (void)locationBarFocusChangedTo:(BOOL)focused {
  _locationBarFocused = focused;
  if (IsBottomOmniboxAvailable()) {
    [self updateOmniboxPosition];
  }
}

- (void)toolbarTraitCollectionChangedTo:(UITraitCollection*)traitCollection {
  _toolbarTraitCollection = traitCollection;
  if (IsBottomOmniboxAvailable()) {
    [self updateOmniboxPosition];
  }
}

- (void)setInitialOmniboxPosition {
  [self updateOmniboxPosition];

  if (IsVivaldiRunning()) {
    [self.delegate transitionOmniboxToToolbarType:self.omniboxPosition
                                    tabBarEnabled:[self tabBarEnabled]];
    [self.delegate
      transitionSteadyStateOmniboxToToolbarType:self.steadyStateOmniboxPosition
                                  tabBarEnabled:[self tabBarEnabled]];
  } else {
  [self.delegate transitionOmniboxToToolbarType:self.omniboxPosition];
  [self.delegate transitionSteadyStateOmniboxToToolbarType:
                     self.steadyStateOmniboxPosition];
  } // End Vivaldi

  [self.omniboxConsumer
      steadyStateOmniboxMovedToToolbar:self.steadyStateOmniboxPosition];
}

- (void)setBottomOmniboxOffsetForPopup:(CGFloat)bottomOffset {
  [self.omniboxConsumer setBottomOmniboxOffsetForPopup:bottomOffset];
}


#pragma mark - Setters

- (void)setOmniboxPosition:(ToolbarType)omniboxPosition {
  if (_omniboxPosition != omniboxPosition) {
    _omniboxPosition = omniboxPosition;

    if (IsVivaldiRunning()) {
      [self.delegate transitionOmniboxToToolbarType:omniboxPosition
                                      tabBarEnabled:[self tabBarEnabled]];
    } else {
    [self.delegate transitionOmniboxToToolbarType:omniboxPosition];
    } // End Vivaldi

  }
}

- (void)setSteadyStateOmniboxPosition:(ToolbarType)steadyStateOmniboxPosition {
  if (_steadyStateOmniboxPosition != steadyStateOmniboxPosition) {
    _steadyStateOmniboxPosition = steadyStateOmniboxPosition;

    if (IsVivaldiRunning()) {
      [self.delegate
          transitionSteadyStateOmniboxToToolbarType:steadyStateOmniboxPosition
              tabBarEnabled:[self tabBarEnabled]];
    } else {
    [self.delegate
        transitionSteadyStateOmniboxToToolbarType:steadyStateOmniboxPosition];
    } // End Vivaldi

    [self.omniboxConsumer
        steadyStateOmniboxMovedToToolbar:steadyStateOmniboxPosition];
  }
}

#pragma mark - Boolean Observer

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  if (observableBoolean == _bottomOmniboxEnabled) {
    _preferredOmniboxPosition = _bottomOmniboxEnabled.value
                                    ? ToolbarType::kSecondary
                                    : ToolbarType::kPrimary;
    [self updateOmniboxPosition];
  }

  // Vivaldi
  if (observableBoolean == _tabBarEnabled) {
    [self updateOmniboxPosition];
  }
  // End Vivaldi

}

#pragma mark - CRWWebStateObserver methods.

- (void)webStateWasShown:(web::WebState*)webState {
  [self updateForWebState:webState];
}

- (void)webState:(web::WebState*)webState
    didStartNavigation:(web::NavigationContext*)navigation {
  [self updateForWebState:webState];
}

- (void)webState:(web::WebState*)webState
    didFinishNavigation:(web::NavigationContext*)navigation {
  [self updateForWebState:webState];
}

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  [self updateForWebState:webState];
}

#pragma mark - WebStateListObserving

- (void)didChangeWebStateList:(WebStateList*)webStateList
                       change:(const WebStateListChange&)change
                       status:(const WebStateListStatus&)status {
  if (status.active_web_state_change() && status.new_active_web_state) {
    [self updateForWebState:status.new_active_web_state];
  }
}

#pragma mark - Private

/// Updates the state variables and toolbars with `webState`.
- (void)updateForWebState:(web::WebState*)webState {
  [self.delegate updateToolbar];
  _isNTP = IsVisibleURLNewTabPage(webState);
  if (IsBottomOmniboxAvailable()) {
    [self updateOmniboxPosition];
    [self.omniboxConsumer setIsNTP:_isNTP];
  }
}

/// Computes the toolbar that should contain the unfocused omnibox in the
/// current state.
- (ToolbarType)steadyStateOmniboxPositionInCurrentState {

  if (IsVivaldiRunning()) {
    return _preferredOmniboxPosition;
  } // End Vivaldi

  if (_preferredOmniboxPosition == ToolbarType::kPrimary ||
      !IsSplitToolbarMode(_toolbarTraitCollection)) {
    return ToolbarType::kPrimary;
  }
  if (_isNTP && !_isIncognito) {
    return ToolbarType::kPrimary;
  }
  return _preferredOmniboxPosition;
}

/// Computes the toolbar that should contain the omnibox in the current state.
- (ToolbarType)omniboxPositionInCurrentState {

  if (IsVivaldiRunning()) {
    return [self steadyStateOmniboxPositionInCurrentState];
  } // End Vivaldi

  ToolbarType steadyState = [self steadyStateOmniboxPositionInCurrentState];

  if (!_locationBarFocused) {
    return steadyState;
  }

  return ToolbarType::kPrimary;
}

/// Updates the omnibox position to the correct toolbar.
- (void)updateOmniboxPosition {
  if (!IsBottomOmniboxAvailable()) {
    [self.delegate transitionOmniboxToToolbarType:ToolbarType::kPrimary];
    return;
  }

  [self.omniboxConsumer setPreferredOmniboxPosition:_preferredOmniboxPosition];

  self.omniboxPosition = [self omniboxPositionInCurrentState];
  self.steadyStateOmniboxPosition =
      [self steadyStateOmniboxPositionInCurrentState];

  // Vivaldi
  self.isTabBarEnabled = [self tabBarEnabled];
  // End Vivaldi

}

#pragma mark - Vivaldi
- (void)setOriginalPrefService:(PrefService*)originalPrefService {
  _originalPrefService = originalPrefService;
  if (_originalPrefService) {
    _tabBarEnabled =
        [[PrefBackedBoolean alloc]
            initWithPrefService:_originalPrefService
                prefName:vivaldiprefs::kVivaldiDesktopTabsEnabled];
    [_tabBarEnabled setObserver:self];
  } else {
    [self stopObservingTabBarState];
  }
}

- (void)setIsTabBarEnabled:(BOOL)isTabBarEnabled {
  if (_isTabBarEnabled != isTabBarEnabled) {
    _isTabBarEnabled = isTabBarEnabled;
    [self.delegate transitionOmniboxToToolbarType:self.omniboxPosition
                                    tabBarEnabled:isTabBarEnabled];
  }
}

// Tab bar enabled always on iPad.
- (BOOL)tabBarEnabled {
  return [_tabBarEnabled value] || [VivaldiGlobalHelpers isDeviceTablet];
}

- (void)stopObservingTabBarState {
  [_tabBarEnabled stop];
  [_tabBarEnabled setObserver:nil];
  _tabBarEnabled = nil;
}
// End Vivaldi

@end
