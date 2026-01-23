// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#import "ios/panel/panel_view_controller.h"

#import <stdint.h>

#import "base/strings/utf_string_conversions.h"
#import "ios/chrome/browser/history/ui_bundled/history_coordinator.h"
#import "ios/chrome/browser/history/ui_bundled/history_table_view_controller.h"
#import "ios/chrome/browser/shared/ui/table_view/table_view_navigation_controller.h"
#import "ios/chrome/browser/url_loading/model/url_loading_params.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/panel/panel_constants.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"
#import "ios/ui/notes/note_home_view_controller.h"
#import "ios/ui/notes/note_navigation_controller.h"
#import "ios/ui/vivaldi_overflow_menu/vivaldi_oveflow_menu_constants.h"
#import "ui/base/l10n/l10n_util_mac.h"
#import "ui/base/l10n/l10n_util.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

using l10n_util::GetNSString;

namespace {
  constexpr CGFloat kSegmentedControlHeight = 40.0;
  constexpr CGFloat kSegmentedControlCornerRadius = 20.0;
}

@interface PanelViewController (){
    UISegmentedControl* segmentedControl;
    NoteNavigationController* noteNavigationController;
    UINavigationController* historyController;
    UINavigationController* bookmarkController;
    UINavigationController* readinglistController;
    UINavigationController* translateController;
    UIView* pageSwitcherBackgroundView;
    NSLayoutConstraint* positionConstraint;
}

@property(nonatomic, strong) UIPageViewController* pageController;

- (void)segmentTapped:(UISegmentedControl*)sender;

@end

@implementation PanelViewController

- (instancetype)init {
    return [super initWithNibName:nil bundle:nil];
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  if ([self.pageController.viewControllers count] == 0) {
    return;
  }
  UINavigationController* navController =
      self.pageController.viewControllers[0];
  if (!navController)
    return;
  int position = navController.navigationBar.frame.size.height;
  if (![self isIPad]) {
    if (@available(iOS 26, *))
      position += panel_top_padding;
  }
  // Adjust topAnchor if neccessary
  if (position != 0.0) {
    positionConstraint.active = NO;
    positionConstraint.constant = position;
    positionConstraint.active = YES;
  }
}

- (void)viewDidLoad {
  [super viewDidLoad];

  self.pageController = [[UIPageViewController alloc]
      initWithTransitionStyle:UIPageViewControllerTransitionStyleScroll
        navigationOrientation:UIPageViewControllerNavigationOrientationVertical
                    options:nil];
  self.pageController.delegate = self;
  [self addChildViewController:self.pageController];
  [self.view addSubview:self.pageController.view];
  [self.pageController didMoveToParentViewController:self];

  UIView* topView = [[UIView alloc] init];
  pageSwitcherBackgroundView = topView;
  topView.frame = CGRectMake(0, 0, 0, panel_top_view_height);
  topView.backgroundColor = [UIColor colorNamed:kGroupedPrimaryBackgroundColor];
  topView.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:topView];

  NSLayoutConstraint* centerHorizontallyConstraint2 =
      [NSLayoutConstraint
                    constraintWithItem: topView
                    attribute:NSLayoutAttributeCenterX
                    relatedBy:NSLayoutRelationEqual
                    toItem:self.view
                    attribute:NSLayoutAttributeCenterX
                    multiplier:1.0
                    constant:0];
  [topView.leadingAnchor constraintEqualToAnchor:
      self.view.leadingAnchor constant:0].active = YES;
  [self.view addConstraint:centerHorizontallyConstraint2];
  positionConstraint = [topView.topAnchor constraintEqualToAnchor:
                        self.view.topAnchor constant:panel_search_view_height];
  [topView.heightAnchor constraintEqualToConstant:panel_search_view_height]
        .active = YES;
  positionConstraint.active = YES;
  // Create and add segmentedControl
  segmentedControl = [[UISegmentedControl alloc]
                      initWithItems:@[
                [UIImage imageNamed:vPanelBookmarks],
                [UIImage imageNamed:vPanelReadingList],
                [UIImage imageNamed:vPanelHistory],
                [UIImage imageNamed:vPanelNotes],
                [UIImage imageNamed:vPanelTranslate]]];
  [segmentedControl addTarget:self action:@selector(segmentTapped:)
                 forControlEvents:UIControlEventValueChanged];
  self.segmentControl = segmentedControl;
  segmentedControl.translatesAutoresizingMaskIntoConstraints = NO;
  [topView addSubview:segmentedControl];
  NSLayoutConstraint* centerHorizontallyConstraint =
    [NSLayoutConstraint
                  constraintWithItem: segmentedControl
                  attribute:NSLayoutAttributeCenterX
                  relatedBy:NSLayoutRelationEqual
                  toItem:topView
                  attribute:NSLayoutAttributeCenterX
                  multiplier:1.0
                  constant:0];
  [segmentedControl.topAnchor constraintEqualToAnchor:
    topView.topAnchor constant:panel_top_padding].active = YES;
  [segmentedControl.leadingAnchor constraintEqualToAnchor:
    self.view.safeLeftAnchor constant:panel_horizontal_padding].active = YES;
  [topView addConstraint:centerHorizontallyConstraint];

  if (@available(iOS 26.0, *)) {
    topView.backgroundColor = [UIColor clearColor];
    [segmentedControl.heightAnchor
      constraintEqualToConstant:kSegmentedControlHeight].active = YES;
    UIGlassEffect* glassEffect =
      [UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
    glassEffect.tintColor = [UIColor clearColor];
    UIVisualEffectView* segmentedGlassEffectView =
      [[UIVisualEffectView alloc] initWithEffect:glassEffect];
    segmentedGlassEffectView.translatesAutoresizingMaskIntoConstraints = NO;
    [topView insertSubview:segmentedGlassEffectView
              belowSubview:segmentedControl];
    [segmentedGlassEffectView.centerXAnchor constraintEqualToAnchor:
      segmentedControl.centerXAnchor].active = YES;
    [segmentedGlassEffectView.centerYAnchor constraintEqualToAnchor:
      segmentedControl.centerYAnchor].active = YES;
    [segmentedGlassEffectView.widthAnchor constraintEqualToAnchor:
      segmentedControl.widthAnchor].active = YES;
    [segmentedGlassEffectView.heightAnchor constraintEqualToAnchor:
      segmentedControl.heightAnchor].active = YES;
    segmentedGlassEffectView.contentView.clipsToBounds = YES;
    segmentedControl.layer.cornerRadius = kSegmentedControlCornerRadius;
    segmentedControl.clipsToBounds = YES;
    segmentedControl.selectedSegmentTintColor =
      [UIColor secondarySystemFillColor];
    segmentedGlassEffectView.layer.cornerRadius = kSegmentedControlCornerRadius;
    segmentedGlassEffectView.layer.masksToBounds = YES;

    NSDictionary* selectedAttrs = @{
        NSForegroundColorAttributeName: [UIColor colorNamed: kBlueColor] };
    [segmentedControl
        setTitleTextAttributes:selectedAttrs forState:UIControlStateSelected];
  }
  [self setIndexForControl:BookmarksPage];
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self applyTransparecyOn:self.navigationController];
}

/*
  Setup controllers
 */
- (void)setupControllers:(NoteNavigationController*)nvc
      withBookmarkController:(UINavigationController*)bvc
          andReadinglistController:(UINavigationController*)rvc
              andHistoryController:(UINavigationController*)hc
                  andTranslateController:(UINavigationController*)tc {
  noteNavigationController = nvc;
  bookmarkController = bvc;
  readinglistController = rvc;
  historyController = hc;
  translateController = tc;
  nvc.navigationBar.prefersLargeTitles = NO;
  bvc.navigationBar.prefersLargeTitles = NO;
  rvc.navigationBar.prefersLargeTitles = NO;
  hc.navigationBar.prefersLargeTitles = NO;
  tc.navigationBar.prefersLargeTitles = NO;

  [self applyTransparecyOn:nvc];
  [self applyTransparecyOn:bvc];
  [self applyTransparecyOn:rvc];
  [self applyTransparecyOn:hc];
  [self applyTransparecyOn:tc];
}

- (void)setIndexForControl:(int)index {
    UIViewController* uv = nil;
    switch (index) {
      case PanelPage::BookmarksPage:
          uv = bookmarkController;
            break;
      case PanelPage::ReadinglistPage:
          uv = readinglistController;
            break;
      case PanelPage::NotesPage:
          uv = noteNavigationController;
            break;
      case PanelPage::HistoryPage:
          uv = historyController;
            break;
      case PanelPage::TranslatePage:
          uv = translateController;
            break;
    }
    [self.pageController setViewControllers:@[uv]
                direction:UIPageViewControllerNavigationDirectionForward
                         animated:NO
                        completion:nil];
}

- (void)segmentTapped:(UISegmentedControl*)sender {
  if (!sender || sender.selectedSegmentIndex < 0
      || sender.selectedSegmentIndex > 4)
      return;
  UINavigationController* navController = nil;
  navController = self.pageController.viewControllers.firstObject;
  if (navController) {
    UIViewController* vc = [navController visibleViewController];
      if ([vc isKindOfClass:[UISearchController class]]) {
          [navController dismissViewControllerAnimated:YES completion:^{
              ((UISearchController*)vc).active = NO;
              int index = sender.selectedSegmentIndex;
              [self setIndexForControl:index];
          }];
          return;
      }
  }

  int index = sender.selectedSegmentIndex;
  [self setIndexForControl:index];
}

- (void)panelDismissed {
  [self.pageController.view removeFromSuperview];
  [self.pageController removeFromParentViewController];
  self.pageController = nil;
  bookmarkController = nil;
  readinglistController = nil;
  noteNavigationController = nil;
  historyController = nil;
  translateController = nil;
}

- (void)applyTransparecyOn:(UINavigationController*)controller {
  if (!controller) {
    return;
  }
  if (@available(iOS 26, *)) {
    UINavigationBarAppearance* appearance =
        [[UINavigationBarAppearance alloc] init];
    [appearance configureWithTransparentBackground];
    appearance.backgroundColor = [UIColor clearColor];
    appearance.backgroundEffect = nil;
    appearance.shadowColor = [UIColor clearColor];
    controller.navigationBar.standardAppearance = appearance;
    controller.navigationBar.scrollEdgeAppearance = appearance;
    controller.navigationBar.compactAppearance = appearance;
    controller.navigationBar.compactScrollEdgeAppearance = appearance;
    controller.navigationBar.translucent = YES;
    controller.navigationBar.barTintColor = [UIColor clearColor];
    controller.navigationBar.backgroundColor = [UIColor clearColor];
  }
}

- (BOOL)isIPad {
  return UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad;
}

@end
