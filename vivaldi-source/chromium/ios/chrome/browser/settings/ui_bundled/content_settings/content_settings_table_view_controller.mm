// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/settings/ui_bundled/content_settings/content_settings_table_view_controller.h"

#import "base/apple/foundation_util.h"
#import "base/check_op.h"
#import "base/feature_list.h"
#import "base/memory/raw_ptr.h"
#import "base/metrics/user_metrics.h"
#import "base/metrics/user_metrics_action.h"
#import "components/content_settings/core/browser/host_content_settings_map.h"
#import "components/content_settings/core/common/content_settings.h"
#import "components/content_settings/core/common/content_settings_types.h"
#import "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/mailto_handler/model/mailto_handler_service.h"
#import "ios/chrome/browser/ntp/ui_bundled/new_tab_page_feature.h"
#import "ios/chrome/browser/reader_mode/model/features.h"
#import "ios/chrome/browser/settings/ui_bundled/content_settings/block_popups_table_view_controller.h"
#import "ios/chrome/browser/settings/ui_bundled/settings_navigation_controller.h"
#import "ios/chrome/browser/settings/ui_bundled/settings_table_view_controller_constants.h"
#import "ios/chrome/browser/settings/ui_bundled/utils/content_setting_backed_boolean.h"
#import "ios/chrome/browser/shared/model/prefs/pref_backed_boolean.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/ui/table_view/cells/table_view_detail_icon_item.h"
#import "ios/chrome/browser/shared/ui/table_view/cells/table_view_multi_detail_text_item.h"
#import "ios/chrome/browser/shared/ui/table_view/cells/table_view_switch_item.h"
#import "ios/chrome/browser/shared/ui/table_view/table_view_utils.h"
#import "ios/chrome/browser/web/model/annotations/annotations_util.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/web/common/features.h"
#import "ui/base/l10n/l10n_util.h"
#import "ui/base/l10n/l10n_util_mac.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "components/prefs/ios/pref_observer_bridge.h"
#import "components/prefs/pref_change_registrar.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/reader_mode/reader_mode_controller.h"
#import "ios/ui/settings/pagezoom/vivaldi_pagezoom_settings_coordinator.h"
#import "ios/ui/settings/pagezoom/vivaldi_pagezoom_settings_prefs.h"
#import "ios/ui/settings/reader_mode/vivaldi_reader_mode_prefs.h"
#import "ios/ui/settings/vivaldi_settings_constants.h"
#import "prefs/ios/vivaldi_ios_pref_names.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using vivaldi::IsVivaldiRunning;
using l10n_util::GetNSString;
// End Vivaldi

namespace {

// Is YES when one window has mailTo controller opened.
BOOL openedMailTo = NO;

// Notification name of changes to openedMailTo state.
NSString* const kMailToInstanceChanged = @"MailToInstanceChanged";

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierSettings = kSectionIdentifierEnumZero,

  // Vivaldi
  SectionIdentifierTranslateSettings,
  SectionIdentifierPageZoom,
  // End Vivaldi

  SectionIdentifierDeveloperTools,
  SectionIdentifierReaderMode,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeSettingsBlockPopups = kItemTypeEnumZero,
  ItemTypeSettingsComposeEmail,
  ItemTypeSettingsShowLinkPreview,
  ItemTypeSettingsDefaultSiteMode,
  ItemTypeSettingsDetectAddresses,
  ItemTypeSettingsMiniMapShowNative,
  ItemTypeSettingsDetectUnits,
  ItemTypeSettingsShowReadingModeAvailable,
  ItemTypeSettingsReaderMode,
  ItemTypeSettingsWebInspector,

  // Vivaldi
  ItemTypeSettingsPreferTranslatePanel,
  ItemTypeSettingsPreferTranslatePanelFooter,
  ItemTypeSettingsPageZoom,
  // End Vivaldi

};

}  // namespace

#if defined(VIVALDI_BUILD)
@interface ContentSettingsTableViewController () <BooleanObserver,
                                                  PrefObserverDelegate> {
#else
@interface ContentSettingsTableViewController () <BooleanObserver> {
#endif // End Vivaldi

  // The observable boolean that binds to the "Disable Popups" setting state.
  ContentSettingBackedBoolean* _disablePopupsSetting;

  // Updatable Items
  TableViewDetailIconItem* _blockPopupsDetailItem;
  TableViewDetailIconItem* _composeEmailDetailItem;
  TableViewMultiDetailTextItem* _openedInAnotherWindowItem;
  TableViewDetailIconItem* _defaultSiteMode;
  TableViewDetailIconItem* _webInspectorStateItem;
  TableViewDetailIconItem* _readerModeSectionItem;

  // Vivaldi: Add pref observer bridge
  std::unique_ptr<PrefObserverBridge> _prefObserverBridge;
  // Add pref change registrar
  PrefChangeRegistrar _prefChangeRegistrar;
  // Vivaldi page zoom item
  TableViewDetailIconItem* _vivaldiPageZoomItem;
  // End Vivaldi

}

// PrefBackedBoolean for "Show Link Preview" setting state.
@property(nonatomic, strong, readonly) PrefBackedBoolean* linkPreviewEnabled;

// PrefBackedBoolean for "Detect addresses" setting state.
@property(nonatomic, strong, readonly)
    PrefBackedBoolean* detectAddressesEnabled;

// PrefBackedBoolean for "Detect units" setting state.
@property(nonatomic, strong, readonly) PrefBackedBoolean* detectUnitsEnabled;

// PrefBackedBoolean for "Show when Reading mode is available" setting state.
@property(nonatomic, strong, readonly)
    PrefBackedBoolean* showReadingModeAvailableEnabled;

// The item related to the switch for the "Show Link Preview" setting.
@property(nonatomic, strong) TableViewSwitchItem* linkPreviewItem;

// The item related to the switch for the "Detect Addresses" setting.
@property(nonatomic, strong) TableViewSwitchItem* detectAddressesItem;

// The item related to the switch for the "Detect units" setting.
@property(nonatomic, strong) TableViewSwitchItem* detectUnitsItem;

// The item related to the switch for the "Show when Reading mode is available"
// setting.
@property(nonatomic, strong) TableViewSwitchItem* showReadingModeAvailableItem;

// The item related to the default mode used to load the pages.
@property(nonatomic, strong) TableViewDetailIconItem* defaultModeItem;

// The item related to the switch for the "Web Inspector" setting.
@property(nonatomic, strong) TableViewDetailIconItem* webInspectorItem;

// The item related to the switch for the "Reading Mode" setting.
@property(nonatomic, strong) TableViewDetailIconItem* readerModeItem;

// The setting used to store the default mode.
@property(nonatomic, strong) ContentSettingBackedBoolean* requestDesktopSetting;

// PrefBackedBoolean for Web Inspector setting state.
@property(nonatomic, strong) PrefBackedBoolean* webInspectorEnabled;

// Helpers to create collection view items.
- (id)blockPopupsItem;
- (id)composeEmailItem;

// Vivaldi
// PrefBackedBoolean for "Prefer Translate Panel" setting state.
@property(nonatomic, strong, readonly)
    PrefBackedBoolean* preferTranslatePanelEnabled;

// PrefBackedBoolean for "Reader Mode" setting state.
@property(nonatomic, strong, readonly)
    PrefBackedBoolean* readerModeEnabled;

// The item related to the switch for the "Prefer Translate Panel" setting.
@property(nonatomic, strong) TableViewSwitchItem* preferTranslatePanelItem;

// The item related to the switch for the "Reader Mode" setting.
@property(nonatomic, strong) TableViewSwitchItem* vivaldiReaderModeItem;

// The coordinator showing the view for page zoom setting
@property(nonatomic, strong)
    VivaldiPageZoomSettingsCoordinator* vivaldiPageZoomSettingsCoordinator;
// End Vivaldi

@end

@implementation ContentSettingsTableViewController {
  raw_ptr<HostContentSettingsMap> _settingsMap;
  raw_ptr<MailtoHandlerService> _mailtoHandlerService;
  raw_ptr<PrefService> _prefService;
}

- (instancetype)
    initWithHostContentSettingsMap:(HostContentSettingsMap*)settingsMap
              mailtoHandlerService:(MailtoHandlerService*)mailtoHandlerService
                       prefService:(PrefService*)prefService {
  DCHECK(settingsMap);
  DCHECK(mailtoHandlerService);
  DCHECK(prefService);

  self = [super initWithStyle:ChromeTableViewStyle()];
  if (self) {
    _settingsMap = settingsMap;
    _mailtoHandlerService = mailtoHandlerService;
    _prefService = prefService;
    self.title = l10n_util::GetNSString(IDS_IOS_CONTENT_SETTINGS_TITLE);
    _disablePopupsSetting = [[ContentSettingBackedBoolean alloc]
        initWithHostContentSettingsMap:settingsMap
                             settingID:ContentSettingsType::POPUPS
                              inverted:YES];
    [_disablePopupsSetting setObserver:self];

    _linkPreviewEnabled = [[PrefBackedBoolean alloc]
        initWithPrefService:prefService
                   prefName:prefs::kLinkPreviewEnabled];
    [_linkPreviewEnabled setObserver:self];

    _detectAddressesEnabled = [[PrefBackedBoolean alloc]
        initWithPrefService:prefService
                   prefName:prefs::kDetectAddressesEnabled];
    [_detectAddressesEnabled setObserver:self];

    _detectUnitsEnabled = [[PrefBackedBoolean alloc]
        initWithPrefService:prefService
                   prefName:prefs::kDetectUnitsEnabled];
    [_detectUnitsEnabled setObserver:self];

    if (IsReaderModeAvailable() && IsReaderModeOmniboxEntryPointEnabled() &&
        !IsReaderModeContentSettingsForLinkEnabled()) {
      _showReadingModeAvailableEnabled = [[PrefBackedBoolean alloc]
          initWithPrefService:prefService
                     prefName:prefs::kIosReaderModeShowAvailability];
      [_showReadingModeAvailableEnabled setObserver:self];
    }

    _requestDesktopSetting = [[ContentSettingBackedBoolean alloc]
        initWithHostContentSettingsMap:settingsMap
                             settingID:ContentSettingsType::REQUEST_DESKTOP_SITE
                              inverted:NO];
    [_requestDesktopSetting setObserver:self];

    if (web::features::IsWebInspectorSupportEnabled()) {
      _webInspectorEnabled = [[PrefBackedBoolean alloc]
          initWithPrefService:prefService
                     prefName:prefs::kWebInspectorEnabled];
      [_webInspectorEnabled setObserver:self];
    }

    if (IsVivaldiRunning()) {
      // Prefer Translate Panel
      _preferTranslatePanelEnabled = [[PrefBackedBoolean alloc]
          initWithPrefService:GetApplicationContext()->GetLocalState()
                     prefName:vivaldiprefs::kVivaldiPreferTranslatePanel];
      [_preferTranslatePanelEnabled setObserver:self];
      // Reader Mode
      _readerModeEnabled = [[PrefBackedBoolean alloc]
          initWithPrefService:_prefService
                     prefName:vivaldiprefs::kVivaldiReaderModeEnabled];
      [_readerModeEnabled setObserver:self];
      // Add Vivaldi specific pref observer setup
      _prefObserverBridge.reset(new PrefObserverBridge(self));
      _prefChangeRegistrar.Init(_prefService);
      // Register to observe page zoom level changes
      _prefObserverBridge->ObserveChangesForPreference(
          vivaldiprefs::kVivaldiPageZoomLevel, &_prefChangeRegistrar);
    } // End Vivaldi

  }
  return self;
}

#pragma mark - UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  [self loadModel];
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(mailToControllerChanged)
             name:kMailToInstanceChanged
           object:nil];
  [self checkMailToOwnership];
}

- (void)viewWillDisappear:(BOOL)animated {
  [super viewWillDisappear:animated];
  [self checkMailToOwnership];
  [[NSNotificationCenter defaultCenter] removeObserver:self
                                                  name:kMailToInstanceChanged
                                                object:nil];
}

- (void)didMoveToParentViewController:(UIViewController*)parent {
  [super didMoveToParentViewController:parent];
  if (!parent) {
    [self.presentationDelegate
        contentSettingsTableViewControllerWasRemoved:self];
  }
}

#pragma mark - Public

- (void)disconnect {
  [_disablePopupsSetting stop];
  _disablePopupsSetting.observer = nil;
  _disablePopupsSetting = nil;
  [_requestDesktopSetting stop];
  _requestDesktopSetting.observer = nil;
  _requestDesktopSetting = nil;
  [_linkPreviewEnabled stop];
  _linkPreviewEnabled.observer = nil;
  _linkPreviewEnabled = nil;
  [_detectAddressesEnabled stop];
  _detectAddressesEnabled.observer = nil;
  _detectAddressesEnabled = nil;
  [_detectUnitsEnabled stop];
  _detectUnitsEnabled.observer = nil;
  _detectUnitsEnabled = nil;
  [_showReadingModeAvailableEnabled stop];
  _showReadingModeAvailableEnabled.observer = nil;
  _showReadingModeAvailableEnabled = nil;
  [_webInspectorEnabled stop];
  _webInspectorEnabled.observer = nil;
  _webInspectorEnabled = nil;
  _settingsMap = nullptr;
  _mailtoHandlerService = nullptr;
  _prefService = nullptr;

  if (IsVivaldiRunning()) {
    [self.vivaldiPageZoomSettingsCoordinator stop];
    self.vivaldiPageZoomSettingsCoordinator = nil;
    [_preferTranslatePanelEnabled stop];
    [_preferTranslatePanelEnabled setObserver:nil];
    _preferTranslatePanelEnabled = nil;
    [_readerModeEnabled stop];
    [_readerModeEnabled setObserver:nil];
    _readerModeEnabled = nil;
    _prefChangeRegistrar.RemoveAll();
    _prefObserverBridge.reset();
  } // End Vivaldi
}

#pragma mark - LegacyChromeTableViewController

- (void)loadModel {
  [super loadModel];

  if (!_mailtoHandlerService) {
    return;
  }

  TableViewModel* model = self.tableViewModel;
  [model addSectionWithIdentifier:SectionIdentifierSettings];
  [model addItem:[self blockPopupsItem]
      toSectionWithIdentifier:SectionIdentifierSettings];
  NSString* settingsTitle = _mailtoHandlerService->SettingsTitle();
  // Display email settings only on one window at a time, by checking
  // if this is the current owner.
  _openedInAnotherWindowItem = nil;
  _composeEmailDetailItem = nil;
  if (settingsTitle) {
    if (!openedMailTo) {
      [model addItem:[self composeEmailItem]
          toSectionWithIdentifier:SectionIdentifierSettings];
    } else {
      [model addItem:[self openedInAnotherWindowItem]
          toSectionWithIdentifier:SectionIdentifierSettings];
    }
  }

  [model addItem:[self linkPreviewItem]
      toSectionWithIdentifier:SectionIdentifierSettings];

  self.defaultModeItem = [self defaultSiteMode];
  [model addItem:self.defaultModeItem
      toSectionWithIdentifier:SectionIdentifierSettings];

  if (!IsVivaldiRunning()) {
  [model addItem:[self detectAddressItem]
      toSectionWithIdentifier:SectionIdentifierSettings];
  } // End Vivaldi

  if (base::FeatureList::IsEnabled(web::features::kEnableMeasurements)) {
    [model addItem:[self detectUnitItem]
        toSectionWithIdentifier:SectionIdentifierSettings];
  }

  if (IsVivaldiRunning()) {
    [model addSectionWithIdentifier:SectionIdentifierTranslateSettings];

    [model addItem:[self preferTranslatePanelItem]
        toSectionWithIdentifier:SectionIdentifierTranslateSettings];
    TableViewLinkHeaderFooterItem* footer =
        [[TableViewLinkHeaderFooterItem alloc]
            initWithType:ItemTypeSettingsPreferTranslatePanelFooter];
    footer.forceIndents = YES;
    footer.text =
        l10n_util::GetNSString(IDS_IOS_PANEL_PREFER_TRANSLATE_PANEL_FOOTER);
    [model setFooter:footer
        forSectionWithIdentifier:SectionIdentifierTranslateSettings];
    [model addSectionWithIdentifier:SectionIdentifierPageZoom];
    [model addItem:[self vivaldiPageZoomSettingItem]
        toSectionWithIdentifier:SectionIdentifierPageZoom];
    [model addSectionWithIdentifier:SectionIdentifierReaderMode];
    [model addItem:[self vivaldiReaderModeItem]
        toSectionWithIdentifier:SectionIdentifierReaderMode];
  } else {
  if (!IsReaderModeContentSettingsForLinkEnabled() &&
      self.showReadingModeAvailableEnabled) {
    [model addItem:[self showReadingModeAvailableItem]
        toSectionWithIdentifier:SectionIdentifierSettings];
  }
  } // End Vivaldi

  if (web::features::IsWebInspectorSupportEnabled()) {
    self.webInspectorItem = [self webInspectorStateItem];
    [model addSectionWithIdentifier:SectionIdentifierDeveloperTools];
    [model addItem:self.webInspectorItem
        toSectionWithIdentifier:SectionIdentifierDeveloperTools];
  }

  if (!IsVivaldiRunning()) {
  if (IsReaderModeContentSettingsForLinkEnabled()) {
    // Add a new content setting section for Reading Mode that holds multiple
    // feature options.
    if (IsReaderModeAvailable()) {
      self.readerModeItem = [self readerModeSectionItem];
      [model addSectionWithIdentifier:SectionIdentifierReaderMode];
      [model addItem:self.readerModeItem
          toSectionWithIdentifier:SectionIdentifierReaderMode];
    }
  }
  } // End Vivaldi

}

#pragma mark - SettingsControllerProtocol

- (void)reportDismissalUserAction {
  base::RecordAction(base::UserMetricsAction("MobileContentSettingsClose"));
}

- (void)reportBackUserAction {
  base::RecordAction(base::UserMetricsAction("MobileContentSettingsBack"));
}

#pragma mark - ContentSettingsTableViewController

- (TableViewDetailIconItem*)defaultSiteMode {
  _defaultSiteMode = [[TableViewDetailIconItem alloc]
      initWithType:ItemTypeSettingsDefaultSiteMode];
  _defaultSiteMode.text =
      l10n_util::GetNSString(IDS_IOS_DEFAULT_PAGE_MODE_LABEL);
  _defaultSiteMode.detailText = [self defaultModeDescription];
  _defaultSiteMode.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  _defaultSiteMode.accessibilityIdentifier = kSettingsDefaultSiteModeCellId;
  return _defaultSiteMode;
}

- (TableViewItem*)blockPopupsItem {
  _blockPopupsDetailItem = [[TableViewDetailIconItem alloc]
      initWithType:ItemTypeSettingsBlockPopups];
  NSString* subtitle = [_disablePopupsSetting value]
                           ? l10n_util::GetNSString(IDS_IOS_SETTING_ON)
                           : l10n_util::GetNSString(IDS_IOS_SETTING_OFF);
  _blockPopupsDetailItem.text = l10n_util::GetNSString(IDS_IOS_BLOCK_POPUPS);
  _blockPopupsDetailItem.detailText = subtitle;
  _blockPopupsDetailItem.accessoryType =
      UITableViewCellAccessoryDisclosureIndicator;
  _blockPopupsDetailItem.accessibilityTraits |= UIAccessibilityTraitButton;
  _blockPopupsDetailItem.accessibilityIdentifier = kSettingsBlockPopupsCellId;
  return _blockPopupsDetailItem;
}

- (TableViewItem*)composeEmailItem {
  if (!_mailtoHandlerService) {
    return nil;
  }

  _composeEmailDetailItem = [[TableViewDetailIconItem alloc]
      initWithType:ItemTypeSettingsComposeEmail];
  // Use the handler's preferred title string for the compose email item.
  NSString* settingsTitle = _mailtoHandlerService->SettingsTitle();
  DCHECK([settingsTitle length]);
  // .detailText can display the selected mailto handling app, but the current
  // MailtoHandlerService does not expose this through its API.
  _composeEmailDetailItem.text = settingsTitle;
  _composeEmailDetailItem.accessoryType =
      UITableViewCellAccessoryDisclosureIndicator;
  _composeEmailDetailItem.accessibilityTraits |= UIAccessibilityTraitButton;
  _composeEmailDetailItem.accessibilityIdentifier = kSettingsDefaultAppsCellId;
  return _composeEmailDetailItem;
}

- (TableViewItem*)openedInAnotherWindowItem {
  if (!_mailtoHandlerService) {
    return nil;
  }

  _openedInAnotherWindowItem = [[TableViewMultiDetailTextItem alloc]
      initWithType:ItemTypeSettingsComposeEmail];
  // Use the handler's preferred title string for the compose email item.
  NSString* settingsTitle = _mailtoHandlerService->SettingsTitle();
  DCHECK([settingsTitle length]);
  // .detailText can display the selected mailto handling app, but the current
  // MailtoHandlerService does not expose this through its API.
  _openedInAnotherWindowItem.text = settingsTitle;

  _openedInAnotherWindowItem.trailingDetailText =
      l10n_util::GetNSString(IDS_IOS_SETTING_OPENED_IN_ANOTHER_WINDOW);
  _openedInAnotherWindowItem.accessibilityTraits |=
      UIAccessibilityTraitButton | UIAccessibilityTraitNotEnabled;
  _openedInAnotherWindowItem.accessibilityIdentifier =
      kSettingsDefaultAppsCellId;
  return _openedInAnotherWindowItem;
}

- (TableViewSwitchItem*)linkPreviewItem {
  if (!_linkPreviewItem) {
    _linkPreviewItem = [[TableViewSwitchItem alloc]
        initWithType:ItemTypeSettingsShowLinkPreview];

    _linkPreviewItem.text = l10n_util::GetNSString(IDS_IOS_SHOW_LINK_PREVIEWS);
    _linkPreviewItem.on = [self.linkPreviewEnabled value];
    _linkPreviewItem.target = self;
    _linkPreviewItem.selector = @selector(showLinkPreviewSwitchToggled:);
    _linkPreviewItem.accessibilityIdentifier = kSettingsShowLinkPreviewCellId;
  }
  return _linkPreviewItem;
}

- (TableViewSwitchItem*)showReadingModeAvailableItem {
  if (!_showReadingModeAvailableItem) {
    _showReadingModeAvailableItem = [[TableViewSwitchItem alloc]
        initWithType:ItemTypeSettingsShowReadingModeAvailable];

    _showReadingModeAvailableItem.text =
        l10n_util::GetNSString(IDS_IOS_READING_MODE_SETTING_TITLE);
    _showReadingModeAvailableItem.detailText =
        l10n_util::GetNSString(IDS_IOS_READING_MODE_SETTING_DESCRIPTION);
    _showReadingModeAvailableItem.on =
        [self.showReadingModeAvailableEnabled value];
    _showReadingModeAvailableItem.target = self;
    _showReadingModeAvailableItem.selector =
        @selector(showReadingModeAvailableSwitchToggled:);
    _showReadingModeAvailableItem.accessibilityIdentifier =
        kSettingsShowReadingModeAvailableCellId;
  }
  return _showReadingModeAvailableItem;
}

- (TableViewSwitchItem*)detectAddressItem {
  if (!_detectAddressesItem) {
    _detectAddressesItem = [[TableViewSwitchItem alloc]
        initWithType:ItemTypeSettingsDetectAddresses];

    _detectAddressesItem.text =
        l10n_util::GetNSString(IDS_IOS_DETECT_ADDRESSES_SETTING_TITLE);
    _detectAddressesItem.detailText =
        l10n_util::GetNSString(IDS_IOS_DETECT_ADDRESSES_SETTING_DESCRIPTION);
    _detectAddressesItem.on = [self.detectAddressesEnabled value];
    _detectAddressesItem.target = self;
    _detectAddressesItem.selector = @selector(detectAddressesSwitchToggled:);
    _detectAddressesItem.accessibilityIdentifier =
        kSettingsDetectAddressesCellId;
  }
  return _detectAddressesItem;
}

- (TableViewSwitchItem*)detectUnitItem {
  if (!_detectUnitsItem) {
    _detectUnitsItem =
        [[TableViewSwitchItem alloc] initWithType:ItemTypeSettingsDetectUnits];

    _detectUnitsItem.text =
        l10n_util::GetNSString(IDS_IOS_DETECT_UNITS_SETTING_TITLE);
    _detectUnitsItem.detailText =
        l10n_util::GetNSString(IDS_IOS_DETECT_UNITS_SETTING_DESCRIPTION);
    _detectUnitsItem.on = [self.detectUnitsEnabled value];
    _detectUnitsItem.target = self;
    _detectUnitsItem.selector = @selector(detectUnitsSwitchToggled:);
    _detectUnitsItem.accessibilityIdentifier = kSettingsDetectUnitsCellId;
  }
  return _detectUnitsItem;
}

- (TableViewDetailIconItem*)webInspectorStateItem {
  _webInspectorStateItem = [[TableViewDetailIconItem alloc]
      initWithType:ItemTypeSettingsWebInspector];
  _webInspectorStateItem.text =
      l10n_util::GetNSString(IDS_IOS_WEB_INSPECTOR_LABEL);
  _webInspectorStateItem.detailText = [self webInspectorStateDescription];
  _webInspectorStateItem.accessoryType =
      UITableViewCellAccessoryDisclosureIndicator;
  _webInspectorStateItem.accessibilityIdentifier = kSettingsWebInspectorCellId;
  return _webInspectorStateItem;
}

- (TableViewDetailIconItem*)readerModeSectionItem {
  _readerModeSectionItem =
      [[TableViewDetailIconItem alloc] initWithType:ItemTypeSettingsReaderMode];
  _readerModeSectionItem.text =
      l10n_util::GetNSString(IDS_IOS_READER_MODE_CONTENT_SETTINGS_TITLE);
  _readerModeSectionItem.accessoryType =
      UITableViewCellAccessoryDisclosureIndicator;
  _readerModeSectionItem.accessibilityIdentifier = kSettingsReaderModeCellId;
  return _readerModeSectionItem;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView*)tableView
    didSelectRowAtIndexPath:(NSIndexPath*)indexPath {
  [super tableView:tableView didSelectRowAtIndexPath:indexPath];
  if (!_mailtoHandlerService) {
    return;
  }

  NSInteger itemType = [self.tableViewModel itemTypeForIndexPath:indexPath];
  switch (itemType) {
    case ItemTypeSettingsBlockPopups: {
      BlockPopupsTableViewController* controller =
          [[BlockPopupsTableViewController alloc]
              initWithHostContentSettingsMap:_settingsMap
                                 prefService:_prefService];
      [self configureHandlersForRootViewController:controller];
      [self.navigationController pushViewController:controller animated:YES];
      break;
    }
    case ItemTypeSettingsComposeEmail: {
      if (openedMailTo) {
        break;
      }

      UIViewController* controller =
          _mailtoHandlerService->CreateSettingsController();
      if (controller) {
        [self.navigationController pushViewController:controller animated:YES];
        openedMailTo = YES;
        [[NSNotificationCenter defaultCenter]
            postNotificationName:kMailToInstanceChanged
                          object:nil];
      }
      break;
    }
    case ItemTypeSettingsDefaultSiteMode: {
      [self.presentationDelegate
          contentSettingsTableViewControllerSelectedDefaultPageMode:self];
      break;
    }
    case ItemTypeSettingsWebInspector: {
      [self.presentationDelegate
          contentSettingsTableViewControllerSelectedWebInspector:self];
      break;
    }
    case ItemTypeSettingsReaderMode: {
      [self.presentationDelegate
          contentSettingsTableViewControllerSelectedReaderMode:self];
      break;
    }

    // Vivaldi
    case ItemTypeSettingsPageZoom: {
      [self showVivaldiPageZoomSettings];
      break;
    }
    // End Vivaldi

  }
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

#pragma mark - BooleanObserver

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  if (observableBoolean == _disablePopupsSetting) {
    NSString* subtitle = [_disablePopupsSetting value]
                             ? l10n_util::GetNSString(IDS_IOS_SETTING_ON)
                             : l10n_util::GetNSString(IDS_IOS_SETTING_OFF);
    // Update the item.
    _blockPopupsDetailItem.detailText = subtitle;

    // Update the cell.
    [self reconfigureCellsForItems:@[ _blockPopupsDetailItem ]];
  } else if (observableBoolean == self.linkPreviewEnabled) {
    self.linkPreviewItem.on = [self.linkPreviewEnabled value];
    [self reconfigureCellsForItems:@[ self.linkPreviewItem ]];
  } else if (observableBoolean == self.showReadingModeAvailableEnabled) {
    self.showReadingModeAvailableItem.on =
        [self.showReadingModeAvailableEnabled value];
    [self reconfigureCellsForItems:@[ self.showReadingModeAvailableItem ]];
  } else if (observableBoolean == self.requestDesktopSetting &&
             self.defaultModeItem) {
    self.defaultModeItem.detailText = [self defaultModeDescription];
    [self reconfigureCellsForItems:@[ self.defaultModeItem ]];
  } else if (web::features::IsWebInspectorSupportEnabled() &&
             observableBoolean == self.webInspectorEnabled) {
    self.webInspectorItem.detailText = [self webInspectorStateDescription];
    [self reconfigureCellsForItems:@[ self.webInspectorItem ]];
  } else if (observableBoolean == self.detectAddressesEnabled) {
    self.detectAddressItem.on = [self.detectAddressesEnabled value];
    [self reconfigureCellsForItems:@[ self.detectAddressItem ]];
  } else if (observableBoolean == self.detectUnitsEnabled) {
    self.detectUnitsItem.on = [self.detectUnitsEnabled value];
    [self reconfigureCellsForItems:@[ self.detectUnitsItem ]];
  } else if (IsVivaldiRunning() &&
             observableBoolean == self.preferTranslatePanelEnabled) {
    self.preferTranslatePanelItem.on = [self.preferTranslatePanelEnabled value];
    [self reconfigureCellsForItems:@[ self.preferTranslatePanelItem ]];
  } else if (IsVivaldiRunning() &&
             observableBoolean == self.readerModeEnabled) {
    self.vivaldiReaderModeItem.on = [self.readerModeEnabled value];
    [self reconfigureCellsForItems:@[ self.vivaldiReaderModeItem ]];
    // End Vivaldi
  } else {
    NOTREACHED();
  }
}

#pragma mark - Switch Actions

- (void)showLinkPreviewSwitchToggled:(UISwitch*)sender {
  BOOL newSwitchValue = sender.isOn;
  self.linkPreviewItem.on = newSwitchValue;
  [self.linkPreviewEnabled setValue:newSwitchValue];
}

- (void)showReadingModeAvailableSwitchToggled:(UISwitch*)sender {
  BOOL newSwitchValue = sender.isOn;
  self.showReadingModeAvailableItem.on = newSwitchValue;
  [self.showReadingModeAvailableEnabled setValue:newSwitchValue];
}

- (void)detectAddressesSwitchToggled:(UISwitch*)sender {
  BOOL newSwitchValue = sender.isOn;
  self.detectAddressesItem.on = newSwitchValue;
  [self.detectAddressesEnabled setValue:newSwitchValue];
}

- (void)detectUnitsSwitchToggled:(UISwitch*)sender {
  BOOL newSwitchValue = sender.isOn;
  self.detectUnitsItem.on = newSwitchValue;
  [self.detectUnitsEnabled setValue:newSwitchValue];
}

#pragma mark - Private

// Called to reload data when another window has mailTo settings opened.
- (void)mailToControllerChanged {
  [self reloadData];
}

// Verifies using the navigation stack if this is a return from mailTo settings
// and this instance should reset `openedMailTo`.
- (void)checkMailToOwnership {
  if (!_mailtoHandlerService) {
    return;
  }

  // Since this doesn't know or have access to the mailTo controller code,
  // it detects if the flow is coming back from it, based on the navigation
  // bar stack items.
  NSString* top = self.navigationController.navigationBar.topItem.title;
  NSString* mailToTitle = _mailtoHandlerService->SettingsTitle();
  if ([top isEqualToString:mailToTitle]) {
    openedMailTo = NO;
    [[NSNotificationCenter defaultCenter]
        postNotificationName:kMailToInstanceChanged
                      object:nil];
  }
}

// Returns the string for the default mode.
- (NSString*)defaultModeDescription {
  return self.requestDesktopSetting.value
             ? l10n_util::GetNSString(IDS_IOS_DEFAULT_PAGE_MODE_DESKTOP)
             : l10n_util::GetNSString(IDS_IOS_DEFAULT_PAGE_MODE_MOBILE);
}

// Returns the string description for the current WebInspectorState.
- (NSString*)webInspectorStateDescription {
  return self.webInspectorEnabled.value
             ? l10n_util::GetNSString(IDS_IOS_SETTING_ON)
             : l10n_util::GetNSString(IDS_IOS_SETTING_OFF);
}

- (void)settingsWillBeDismissed {
  // TODO(crbug.com/40272467)
  [self disconnect];
}

#pragma mark - Vivaldi
- (TableViewSwitchItem*)preferTranslatePanelItem {
  if (!_preferTranslatePanelItem) {
    _preferTranslatePanelItem = [[TableViewSwitchItem alloc]
        initWithType:ItemTypeSettingsPreferTranslatePanel];

    _preferTranslatePanelItem.text =
        l10n_util::GetNSString(IDS_IOS_PANEL_PREFER_TRANSLATE_PANEL);
    _preferTranslatePanelItem.on = [self.preferTranslatePanelEnabled value];
    _preferTranslatePanelItem.accessibilityIdentifier =
        kSettingsPreferTranslatePanelCellId;
    _preferTranslatePanelItem.target = self;
    _preferTranslatePanelItem.selector =
        @selector(preferTranslatePanelSwitchToggled:);
  }
  return _preferTranslatePanelItem;
}

- (void)preferTranslatePanelSwitchToggled:(UISwitch*)sender {
  BOOL newSwitchValue = sender.isOn;
  self.preferTranslatePanelItem.on = newSwitchValue;
  [self.preferTranslatePanelEnabled setValue:newSwitchValue];
}

- (TableViewDetailIconItem*)vivaldiPageZoomSettingItem {
  _vivaldiPageZoomItem =
    [[TableViewDetailIconItem alloc] initWithType:ItemTypeSettingsPageZoom];
  _vivaldiPageZoomItem.text =
    l10n_util::GetNSString(IDS_IOS_PAGEZOOM_SETTING_TITLE);
  _vivaldiPageZoomItem.detailText = [self pageZoomDetailText];
  _vivaldiPageZoomItem.accessoryType =
    UITableViewCellAccessoryDisclosureIndicator;
  return _vivaldiPageZoomItem;
}

- (void)showVivaldiPageZoomSettings {
  _vivaldiPageZoomSettingsCoordinator =
  [[VivaldiPageZoomSettingsCoordinator alloc]
    initWithBaseNavigationController:self.navigationController
                             browser:_browser];
  [_vivaldiPageZoomSettingsCoordinator start];
}

- (NSString*)pageZoomDetailText {
  NSInteger zoomLevel = [VivaldiPageZoomSettingPrefs
      getPageZoomLevelWithPrefService:_prefService];
  return [NSString stringWithFormat:@"%ld%%", zoomLevel];
}

- (TableViewSwitchItem*)vivaldiReaderModeItem {
  if (!_vivaldiReaderModeItem) {
    _vivaldiReaderModeItem = [[TableViewSwitchItem alloc]
      initWithType:ItemTypeSettingsReaderMode];
    _vivaldiReaderModeItem.text =
        l10n_util::GetNSString(IDS_IOS_READER_MODE_TITLE);
    _vivaldiReaderModeItem.on = [self.readerModeEnabled value];
    _vivaldiReaderModeItem.accessibilityIdentifier = kSettingsReaderModeCellId;
    _vivaldiReaderModeItem.target = self;
    _vivaldiReaderModeItem.selector = @selector(readerModeSwitchToggled:);
  }
  return _vivaldiReaderModeItem;
}

- (void)readerModeSwitchToggled:(UISwitch*)sender {
  BOOL newSwitchValue = sender.isOn;
  self.vivaldiReaderModeItem.on = newSwitchValue;
  [self.readerModeEnabled setValue:newSwitchValue];

  if (!newSwitchValue) {
    // If reader mode is being disabled globally, turn it off on all active pages
    ReaderModeController::DisableReaderModeGlobally();
  } else {
    // If reader mode is being enabled globally, re-check availability on all pages
    ReaderModeController::UpdateReaderModeAvailabilityGlobally();
  }
}

#pragma mark - PrefObserverDelegate

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  if (IsVivaldiRunning() &&
      preferenceName == vivaldiprefs::kVivaldiPageZoomLevel) {
    _vivaldiPageZoomItem.detailText = [self pageZoomDetailText];
    [self reconfigureCellsForItems:@[ _vivaldiPageZoomItem ]];
    return;
  }
}
// End Vivaldi

@end
