// Copyright 2022 Vivaldi Technologies. All rights reserved.

#import "ios/ui/ntp/vivaldi_speed_dial_view_controller.h"

#import <UIKit/UIKit.h>

#import "components/direct_match/direct_match_service.h"
#import "ios/chrome/browser/favicon/model/ios_chrome_favicon_loader_factory.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/direct_match/direct_match_service_factory.h"
#import "ios/ui/bookmarks_editor/vivaldi_bookmarks_constants.h"
#import "ios/ui/helpers/vivaldi_global_helpers.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"
#import "ios/ui/ntp/bottom_toolbar/bottom_toolbar_swift.h"
#import "ios/ui/ntp/bottom_toolbar/vivaldi_ntp_bottom_toolbar_consumer.h"
#import "ios/ui/ntp/navigation_bar/vivaldi_custom_navigation_bar_swift.h"
#import "ios/ui/ntp/top_toolbar/top_toolbar_swift.h"
#import "ios/ui/ntp/vivaldi_ntp_constants.h"
#import "ios/ui/ntp/vivaldi_speed_dial_base_controller.h"
#import "ios/ui/ntp/vivaldi_speed_dial_constants.h"
#import "ios/ui/ntp/vivaldi_speed_dial_container_view.h"
#import "ios/ui/ntp/vivaldi_speed_dial_home_mediator.h"
#import "ios/ui/settings/start_page/quick_settings/vivaldi_start_page_quick_settings_coordinator.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_prefs_helper.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_prefs.h"
#import "ui/base/l10n/l10n_util_mac.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

namespace {
// Height for bottom toolbar view.
CGFloat bottomToolbarHeight = 70.f;
// Animation start delay for bottom toolbar
CGFloat animationStartDelay = 0.3;
// Top toolbar navigation bar view height.
CGFloat navigationBarHeight = 44.f;
// Notification Identifier For Background Wallpaper
NSString* vivaldiWallpaperUpdate = @"VivaldiBackgroundWallpaperUpdate";

BOOL ShouldUseModernNavigationBar() {
  if (@available(iOS 26.0, *)) {
    return YES;
  }
  return NO;
}
}

@interface VivaldiSpeedDialViewController ()<VivaldiSpeedDialContainerDelegate,
                                                         SpeedDialHomeConsumer,
                                              VivaldiNTPBottomToolbarConsumer> {
direct_match::DirectMatchService* _directMatchService;
// Start page settings coordinator.
VivaldiStartPageQuickSettingsCoordinator* _startPageSettingsCoordinator;
}
// The view that holds the speed dial folder children
@property(weak,nonatomic) VivaldiSpeedDialContainerView* speedDialContainerView;
// Bottom toolbar view provider
@property(nonatomic, strong)
    VivaldiBottomToolbarViewProvider* bottomToolbarProvider;
// Bottom toolbar view
@property(nonatomic, strong) UIViewController* bottomToolbarView;
// The background Image for Speed Dial
@property(nonatomic, strong) UIImageView* backgroundImageView;
// Custom navigation bar view.
@property(nonatomic, strong) VivaldiCustomNavigationBarView* navigationBarView;
// Top constraint for the content container.
@property(nonatomic, strong) NSLayoutConstraint* contentTopConstraint;
// Bookmark Model that holds the bookmark data
@property(assign,nonatomic) BookmarkModel* bookmarks;
// FaviconLoader is a keyed service that uses LargeIconService to retrieve
// favicon images.
@property(assign,nonatomic) FaviconLoader* faviconLoader;
// The user's profile model used.
@property(assign,nonatomic) ProfileIOS* profile;
// Current browser
@property(assign,nonatomic) Browser* browser;
// The mediator that provides data for this view controller.
@property(strong,nonatomic) VivaldiSpeedDialHomeMediator* mediator;
// Array to hold the children
@property(strong,nonatomic) NSMutableArray *speedDialChildItems;
// Speed dial folder which is currently presented
@property(strong,nonatomic) VivaldiSpeedDialItem* currentItem;
// Parent of Speed dial folder which is currently presented.
@property(strong,nonatomic) VivaldiSpeedDialItem* parentItem;

@end


@implementation VivaldiSpeedDialViewController

@synthesize speedDialContainerView = _speedDialContainerView;
@synthesize bookmarks = _bookmarks;
@synthesize browser = _browser;
@synthesize profile = _profile;
@synthesize mediator = _mediator;
@synthesize currentItem = _currentItem;
@synthesize parentItem = _parentItem;
@synthesize faviconLoader = _faviconLoader;
@synthesize speedDialChildItems = _speedDialChildItems;

#pragma mark - INITIALIZERS
+ (instancetype)initWithItem:(VivaldiSpeedDialItem*)item
                      parent:(VivaldiSpeedDialItem*)parent
                   bookmarks:(BookmarkModel*)bookmarks
                     browser:(Browser*)browser
               faviconLoader:(FaviconLoader*)faviconLoader {
  DCHECK(bookmarks);
  DCHECK(bookmarks->loaded());
  VivaldiSpeedDialViewController* controller =
    [[VivaldiSpeedDialViewController alloc] initWithBookmarks:bookmarks
                                                      browser:browser];
  controller.faviconLoader = faviconLoader;
  controller.currentItem = item;
  controller.parentItem = parent;
  controller.browser = browser;
  controller.title = item.title;
  return controller;
}

- (instancetype)initWithBookmarks:(BookmarkModel*)bookmarks
                          browser:(Browser*)browser {
  self = [super init];
  if (self) {
    _bookmarks = bookmarks;
    _profile = browser->GetProfile()->GetOriginalProfile();
    _directMatchService =
        direct_match::DirectMatchServiceFactory::GetForProfile(_profile);
    [VivaldiStartPagePrefs setPrefService:_profile->GetPrefs()];
  }
  return self;
}

- (void)dealloc {
  [self removeObservers];
}

#pragma mark - VIEW CONTROLLER LIFECYCLE
- (void)viewDidLoad {
  [super viewDidLoad];
  [self setUpUI];
  [self setupWallpaperUpdateNotifications];
  [self fadeInBottomToolbarView];
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self setupSpeedDialBackground];
  [self startObservingDeviceOrientationChange];
  [self loadSpeedDialViews];
  if (ShouldUseModernNavigationBar()) {
    [self.navigationController setNavigationBarHidden:YES animated:animated];
    [self refreshNavigationBarDisplay];
  } else {
    [self.navigationController setNavigationBarHidden:NO animated:animated];
  }
}

- (void)viewDidAppear:(BOOL)animated {
  [super viewDidAppear:animated];

  // Check if provider reference is still there. If no reference exits this
  // method is getting called when view appears after child view controller
  // is popped. In that case setup toolbar again and fade in.
  if (!self.bottomToolbarProvider) {
    [self setUpBottomToolbarView];
    [self.bottomToolbarProvider handleToolbarVisibilityWithProgress:0
                                                           animated:YES];
  }

  if (ShouldUseModernNavigationBar()) {
    [self refreshNavigationBarDisplay];
  }
}

- (void)viewDidDisappear:(BOOL)animated {
  [super viewDidDisappear:animated];
  [self removeObservers];
}

#pragma mark - PRIVATE
#pragma mark - SET UP UI COMPONENETS
/// Set up all views
- (void)setUpUI {
  self.view.backgroundColor =
    [UIColor colorNamed:vNTPSpeedDialContainerbackgroundColor];
  if (ShouldUseModernNavigationBar()) {
    [self setUpNavigationBar];
  }
  [self setupSpeedDialView];
  [self setUpBottomToolbarView];
}

- (void)setUpNavigationBar {
  VivaldiCustomNavigationBarView* navigationBarView =
      [VivaldiCustomNavigationBarView new];
  _navigationBarView = navigationBarView;
  navigationBarView.backgroundColor = [UIColor colorNamed:vNTPBackgroundColor];

  __weak __typeof(self) weakSelf = self;
  navigationBarView.onBackButtonTapped = ^{
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf.navigationController) {
      return;
    }
    [strongSelf.navigationBarView dismissMenuIfNeeded];
    [strongSelf.navigationController popViewControllerAnimated:YES];
  };
  navigationBarView.onBreadcrumbSelected = ^(NSInteger index) {
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf.navigationController) {
      return;
    }
    NSArray<UIViewController*>* controllers =
        strongSelf.navigationController.viewControllers;
    if (index < 0 || index >= (NSInteger)controllers.count - 1) {
      return;
    }
    UIViewController* target = controllers[index];
    [strongSelf.navigationBarView dismissMenuIfNeeded];
    [strongSelf.navigationController popToViewController:target animated:YES];
  };

  [self.view addSubview:navigationBarView];
  [navigationBarView anchorTop:self.view.safeTopAnchor
                       leading:self.view.leadingAnchor
                        bottom:nil
                      trailing:self.view.trailingAnchor
                          size:CGSizeMake(0, navigationBarHeight)];

  [self refreshNavigationBarDisplay];
}

-(void)setupSpeedDialBackground {
  [self.backgroundImageView removeFromSuperview];

  UIImageView* backgroundImageView =
      [[UIImageView alloc] initWithImage:[self getWallpaperImage]];
  backgroundImageView.contentMode = UIViewContentModeScaleAspectFill;
  backgroundImageView.clipsToBounds = YES;
  self.backgroundImageView = backgroundImageView;
  [self.view insertSubview:backgroundImageView atIndex:0];
  [self.backgroundImageView fillSuperview];
}

/// Set up the speed dial view
- (void)setupSpeedDialView {
  // The container view to hold the speed dial view
  UIView* bodyContainerView = [UIView new];
  bodyContainerView.backgroundColor = UIColor.clearColor;
  [self.view addSubview:bodyContainerView];

  if (ShouldUseModernNavigationBar()) {
    [bodyContainerView anchorTop:nil
                         leading:self.view.safeLeftAnchor
                          bottom:self.view.safeBottomAnchor
                        trailing:self.view.safeRightAnchor];
    NSLayoutConstraint* topConstraint = [bodyContainerView.topAnchor
        constraintEqualToAnchor:self.navigationBarView.bottomAnchor];
    self.contentTopConstraint = topConstraint;
    topConstraint.active = YES;
  } else {
    self.contentTopConstraint = nil;
    [bodyContainerView fillSuperviewToSafeAreaInset];
  }

  VivaldiSpeedDialContainerView* speedDialContainerView =
      [VivaldiSpeedDialContainerView new];
  _speedDialContainerView = speedDialContainerView;
  speedDialContainerView.delegate = self;
  [bodyContainerView addSubview:speedDialContainerView];
  [speedDialContainerView fillSuperview];
}

- (void)refreshNavigationBarDisplay {
  if (!ShouldUseModernNavigationBar()) {
    return;
  }

  if (!self.navigationBarView) {
    return;
  }

  NSString* title = self.currentItem ? self.currentItem.title : self.title;
  NSArray<UIViewController*>* controllers =
      self.navigationController.viewControllers ?: @[];
  NSUInteger controllerCount = controllers.count;
  BOOL showsBackButton = controllerCount > 1;

  NSMutableArray<NavigationBarBreadcrumbItem*>* breadcrumbItems =
      [NSMutableArray array];
  NSString* backButtonTitle =
      l10n_util::GetNSString(IDS_VIVALDI_IOS_NAVIGATION_BACK_BUTTON_TITLE);

  if (controllerCount > 1) {
    for (NSUInteger index = 0; index < controllerCount - 1; ++index) {
      UIViewController* controller = controllers[index];
      NSString* crumbTitle = [self navigationTitleForController:controller];
      if (!crumbTitle.length) {
        continue;
      }
      NavigationBarBreadcrumbItem* item =
          [[NavigationBarBreadcrumbItem alloc] initWithTitle:crumbTitle
                                                       index:(NSInteger)index];
      [breadcrumbItems addObject:item];
    }

    // Use the last breadcrumb item as back button title
    NavigationBarBreadcrumbItem* lastItem = breadcrumbItems.lastObject;
    if (lastItem.title.length > 0) {
      backButtonTitle = lastItem.title;
    }
  }

  [self.navigationBarView configureWithTitle:title ?: @""
                             showsBackButton:showsBackButton
                             backButtonTitle:backButtonTitle];
  [self.navigationBarView updateBreadcrumbMenuWithItems:breadcrumbItems];

  if (!showsBackButton) {
    [self.navigationBarView dismissMenuIfNeeded];
  }
}

- (NSString*)navigationTitleForController:(UIViewController*)controller {
  if (!controller) {
    return @"";
  }
  if ([controller isKindOfClass:[VivaldiSpeedDialViewController class]]) {
    VivaldiSpeedDialViewController* speedDialController =
        (VivaldiSpeedDialViewController*)controller;
    if (speedDialController.currentItem &&
        speedDialController.currentItem.title.length) {
      return speedDialController.currentItem.title;
    }
    return controller.title;
  }
  if ([controller isKindOfClass:[VivaldiSpeedDialBaseController class]]) {
    return controller.title;
  }
  return controller.title;
}

- (void)setUpBottomToolbarView {
  [self.bottomToolbarView.view removeFromSuperview];

  VivaldiBottomToolbarViewProvider *toolbarProvider =
      [[VivaldiBottomToolbarViewProvider alloc] init];
  self.bottomToolbarProvider = toolbarProvider;
  toolbarProvider.consumer = self;

  self.bottomToolbarView = [toolbarProvider makeViewController];
  self.bottomToolbarView.view.backgroundColor = [UIColor clearColor];

  [self addChildViewController:self.bottomToolbarView];
  [self.view addSubview:self.bottomToolbarView.view];
  [self.bottomToolbarView didMoveToParentViewController:self];

  [self.view addSubview:self.bottomToolbarView.view];
  [self.bottomToolbarView.view anchorTop:nil
                                leading:self.view.leadingAnchor
                                 bottom:self.view.bottomAnchor
                               trailing:self.view.trailingAnchor
                                   size:CGSizeMake(0, bottomToolbarHeight)];

  // Hide the toolbar initially.
  [self.bottomToolbarProvider handleToolbarVisibilityWithProgress:1];

  // Trigger update for initial state.
  [self updateBottomToolbarStateIfNeeded];
}

#pragma mark - PRIVATE

/// Create and start mediator to load the speed dial folder items
- (void)loadSpeedDialViews {
  BOOL loadable = self.bookmarks->loaded() &&
                  self.currentItem;
  if (!loadable)
    return;
  self.mediator = [[VivaldiSpeedDialHomeMediator alloc]
                      initWithProfile:self.profile
                        bookmarkModel:self.bookmarks];
  self.mediator.consumer = self;
  [self.mediator computeSpeedDialChildItems:self.currentItem];
}

/// Set up observer to notify whenever device orientation is changed.
- (void)startObservingDeviceOrientationChange {
  [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
  [[NSNotificationCenter defaultCenter]
     addObserver:self
        selector:@selector(handleDeviceOrientationChange:)
            name:UIDeviceOrientationDidChangeNotification
          object:[UIDevice currentDevice]];
}

/// Device orientation change handler
- (void)handleDeviceOrientationChange:(NSNotification*)note {
  [self.speedDialContainerView reloadLayoutWithStyle:[self currentLayoutStyle]
                                        layoutColumn:[self currentLayoutColumn]];
}

/// Refresh the UI when data source is updated.
- (void)refreshUI {
  [self loadSpeedDialViews];
}

-(void)setupWallpaperUpdateNotifications {
  [[NSNotificationCenter defaultCenter] addObserver:self
                                        selector:@selector(updateWallpaper)
                                        name:vivaldiWallpaperUpdate
                                        object:nil];
}

- (void)updateWallpaper {
  __weak __typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf) {
      strongSelf.backgroundImageView.image = [strongSelf getWallpaperImage];
      [strongSelf updateBottomToolbarStateIfNeeded];
    }
  });
}

- (UIImage *)getWallpaperImage {
  // Loading the image name from preferences
  NSString *wallpaper = [self selectedDefaultWallpaper];

  // setting it to nil if string is empty
  // so that we don't get warning : Invalid asset name supplied
  if ([wallpaper isEqualToString:@""]) {
    wallpaper = nil;
  }
  // Create and set the background image
  UIImage *wallpaperImage = wallpaper ? [UIImage imageNamed:wallpaper] : nil;
  // Check if the wallpaper name is nil and get custom wallpaper
  if (wallpaper == nil) {
    wallpaperImage = [self selectedCustomWallpaper];
  }
  return wallpaperImage;
}

- (void)fadeInBottomToolbarView {
  __weak VivaldiSpeedDialViewController* weakSelf = self;
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW,
                    (int64_t)(animationStartDelay * NSEC_PER_SEC)),
      dispatch_get_main_queue(), ^{
        VivaldiSpeedDialViewController* strongSelf = weakSelf;
        if (!strongSelf)
          return;
        [strongSelf.bottomToolbarProvider
            handleToolbarVisibilityWithProgress:0 animated:YES];
      });
}

- (void)updateBottomToolbarStateIfNeeded {
  // Update the customize and add button from prefs
  [self.bottomToolbarProvider
      setCustomizeButtonVisible:[self showStartPageCustomizeButton]];

  [self.bottomToolbarProvider
      setAddButtonVisible:[self showAddButton]];

  // Update background shading for the toolbar
  if ([self getWallpaperImage]) {
    BOOL shouldUseDarkText =
        [VivaldiGlobalHelpers
            shouldUseDarkTextForImage:[self getWallpaperImage]];
    [self.bottomToolbarProvider setHasBackground:YES];
    // If the background demands dark text over it, it means the background is
    // brighter colors and we need to use bright shadow for the toolbar.
    [self.bottomToolbarProvider
        setUseLightBackgroundGradient:shouldUseDarkText];
  } else {
    [self.bottomToolbarProvider setHasBackground:NO];
    [self.bottomToolbarProvider setUseLightBackgroundGradient:NO];
  }
}

/// Remove all observers set up.
- (void)removeObservers {
  [[NSNotificationCenter defaultCenter]
     removeObserver:self
               name:UIDeviceOrientationDidChangeNotification
             object:nil];
  [[NSNotificationCenter defaultCenter] removeObserver:self
                                                  name:vivaldiWallpaperUpdate
                                                object:nil];
  self.mediator.consumer = nil;
  [self.mediator disconnect];

  _bottomToolbarProvider.consumer = nil;
  _bottomToolbarProvider = nil;

  _startPageSettingsCoordinator = nullptr;
}

/// Returns preloaded wallpaper name
- (NSString*)selectedDefaultWallpaper {
  return [VivaldiStartPagePrefsHelper getWallpaperName];
}

/// Returns custom wallpaper name
- (UIImage*)selectedCustomWallpaper {
  // It doesn't require size traits, image contentMode is aspect fill
  UIImage *wallpaper =
    UIDeviceOrientationIsLandscape([UIDevice currentDevice].orientation)
      ? [VivaldiStartPagePrefsHelper getLandscapeWallpaper] :
          [VivaldiStartPagePrefsHelper getPortraitWallpaper];
  return wallpaper;
}

/// Returns whether show speed dials is enabled
- (BOOL)showSpeedDials {
  return [VivaldiStartPagePrefsHelper showSpeedDials];
}

/// Returns whether show start page customize button is enabled
- (BOOL)showStartPageCustomizeButton {
  return [VivaldiStartPagePrefsHelper showStartPageCustomizeButton];
}

/// Returns true when show Add button is enabled
/// `showSpeedDials` enabled.
- (BOOL)showAddButton {
  return [VivaldiStartPagePrefsHelper showAddButton] && [self showSpeedDials];
}

/// Returns current layout style for start page
- (VivaldiStartPageLayoutStyle)currentLayoutStyle {
  return [VivaldiStartPagePrefsHelper getStartPageLayoutStyle];
}

/// Returns current layout column for start page
- (VivaldiStartPageLayoutColumn)currentLayoutColumn {
  return [VivaldiStartPagePrefsHelper getStartPageSpeedDialMaximumColumns];
}

#pragma mark - SPEED DIAL HOME CONSUMER

- (void)bookmarkModelLoaded {
  // No op.
}

- (void)topSitesModelDidLoad {
  // No op.
}

- (void)refreshNode:(const bookmarks::BookmarkNode*)bookmarkNode {
  NSNumber *bookmarkNodeId = @(bookmarkNode->id());
  NSDictionary *userInfo = @{vSpeedDialIdentifierKey:bookmarkNodeId};
  [[NSNotificationCenter defaultCenter]
       postNotificationName:vSpeedDialPropertyDidChange
                     object:nil
                   userInfo:userInfo];
}

- (void)refreshMenuItems:(NSArray*)items
               SDFolders:(NSArray*)SDFolders {
  // No op here since there's no menu in this view.
}

- (void)refreshMenuItems:(NSArray<VivaldiNTPTopToolbarItem*>*)items {
  // No op.
}

- (void)selectToolbarItemWithIndex:(NSInteger)index {
  // No op.
}

- (void)refreshChildItems:(NSArray<VivaldiSpeedDialItem*>*)items
                   parent:(VivaldiNTPTopToolbarItem*)parent {

  if (!self.currentItem || !self.faviconLoader)
    return;

  if (self.currentItem.idValue != parent.primaryId) {
    return;
  }

  BrowserActionFactory* actionFactory =
      [[BrowserActionFactory alloc] initWithBrowser:_browser
              scenario:kMenuScenarioHistogramBookmarkEntry];
  [self.speedDialContainerView configureActionFactory:actionFactory];
  [self.speedDialContainerView configureWith:items
                                      parent:self.currentItem
                               faviconLoader:self.faviconLoader
                          directMatchService:_directMatchService
                                 layoutStyle:[self currentLayoutStyle]
                                layoutColumn:[self currentLayoutColumn]
                                showAddGroup:NO
                           frequentlyVisited:NO
                           topSitesAvailable:NO
                            topToolbarHidden:NO
                           verticalSizeClass:
                                self.view.traitCollection.verticalSizeClass
                                   wallpaper:[self getWallpaperImage]];
}

- (void)setFrequentlyVisitedPagesEnabled:(BOOL)enabled {
  // No op. Handled in base view controller.
}

- (void)setSpeedDialsEnabled:(BOOL)enabled {
  // If speed dials set to disabled dismiss the customize sheet and
  // take user to homepage as the folder is invalid in that state.
  if (!enabled) {
    [self.navigationController dismissViewControllerAnimated:YES completion:nil];
    [self.navigationController popToRootViewControllerAnimated:YES];
    [self removeObservers];
  }
}

- (void)setShowCustomizeStartPageButtonEnabled:(BOOL)enabled {
  [self.bottomToolbarProvider setCustomizeButtonVisible:enabled];
}

- (void)setShowAddButtonEnabled:(BOOL)enabled {
  [self.bottomToolbarProvider setAddButtonVisible:enabled];
}

- (void)reloadLayout {
  [self.speedDialContainerView reloadLayoutWithStyle:[self currentLayoutStyle]
                                        layoutColumn:[self currentLayoutColumn]];
}

#pragma mark - VivaldiNTPBottomToolbarViewConsumer

- (void)didTapAddButton {
  if ([self currentItem]) {
    [self didSelectAddNewSpeedDial:NO
                            parent:[self currentItem]];
  }
}

- (void)didTapAddFolderButton {
  if ([self currentItem]) {
    [self didSelectAddNewSpeedDial:YES
                            parent:[self currentItem]];
  }
}

- (void)didTapCustomizeButton {
  _startPageSettingsCoordinator =
      [[VivaldiStartPageQuickSettingsCoordinator alloc]
            initWithBaseNavigationController:self.navigationController
                                     browser:_browser];
  [_startPageSettingsCoordinator start];
}

#pragma mark - VivaldiSpeedDialContainerDelegate

- (void)collectionViewHasScrollableContent:(BOOL)hasScrollableContents
                                    parent:(VivaldiSpeedDialItem*)parent {
  if (!self.parentItem || !parent)
    return;

  if (self.parentItem.idValue == parent.idValue) {
    [self.bottomToolbarProvider
        setToolbarShouldShowShadow:hasScrollableContents];
  }
}

- (void)didSelectItem:(VivaldiSpeedDialItem*)item
               parent:(VivaldiSpeedDialItem*)parent {
  // Navigate to the folder if the selected speed dial is a folder.
  if (item.isFolder) {
    // Hide toolbar when user navigates to a folder.
    [self.bottomToolbarProvider handleToolbarVisibilityWithProgress:1];

    VivaldiSpeedDialViewController *controller =
      [VivaldiSpeedDialViewController initWithItem:item
                                            parent:parent
                                         bookmarks:self.bookmarks
                                           browser:self.browser
                                     faviconLoader:self.faviconLoader];
    controller.delegate = self;
    [self.navigationController pushViewController:controller
                                         animated:YES];
  } else {
    // Pass it to delegate to open the URL.
    if (self.delegate)
      [self.delegate didSelectItem:item parent:parent];
  }
}

- (void)didSelectEditItem:(VivaldiSpeedDialItem*)item
                   parent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didSelectEditItem:item parent:parent];
}

- (void)didSelectMoveItem:(VivaldiSpeedDialItem*)item
                   parent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didSelectMoveItem:item parent:parent];
}

- (void)didMoveItemByDragging:(VivaldiSpeedDialItem*)item
                       parent:(VivaldiSpeedDialItem*)parent
                   toPosition:(NSInteger)position {
  if (self.delegate)
    [self.delegate didMoveItemByDragging:item
                                  parent:parent
                              toPosition:position];
}

- (void)didSelectDeleteItem:(VivaldiSpeedDialItem*)item
                     parent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didSelectDeleteItem:item parent:parent];
}

- (void)didRefreshThumbnailForItem:(VivaldiSpeedDialItem*)item
                            parent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didRefreshThumbnailForItem:item parent:parent];
}

- (void)didSelectAddNewSpeedDial:(BOOL)isFolder
                          parent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didSelectAddNewSpeedDial:isFolder parent:parent];
}

- (void)didSelectAddNewGroupForParent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didSelectAddNewGroupForParent:parent];
}

- (void)didSelectItemToOpenInNewTab:(VivaldiSpeedDialItem*)item
                             parent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didSelectItemToOpenInNewTab:item parent:parent];
}

- (void)didSelectItemToOpenInBackgroundTab:(VivaldiSpeedDialItem*)item
                                    parent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didSelectItemToOpenInBackgroundTab:item parent:parent];
}

- (void)didSelectItemToOpenInPrivateTab:(VivaldiSpeedDialItem*)item
                                 parent:(VivaldiSpeedDialItem*)parent {
  if (self.delegate)
    [self.delegate didSelectItemToOpenInPrivateTab:item parent:parent];
}

- (void)didSelectItemToShare:(VivaldiSpeedDialItem*)item
                      parent:(VivaldiSpeedDialItem*)parent
                    fromView:(UIView*)view {
  if (self.delegate)
    [self.delegate didSelectItemToShare:item parent:parent fromView:view];
}

- (void)didTapOnCollectionViewEmptyArea {
  if (self.delegate)
    [self.delegate didTapOnCollectionViewEmptyArea];
}

@end
