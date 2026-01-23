// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/history/ui_bundled/history_coordinator.h"

#import "ios/chrome/browser/history/ui_bundled/base_history_coordinator+subclassing.h"
#import "ios/chrome/browser/history/ui_bundled/history_table_view_controller.h"
#import "ios/chrome/browser/menu/ui_bundled/menu_histograms.h"
#import "ios/chrome/browser/shared/coordinator/alert/action_sheet_coordinator.h"

// Vivaldi
#include "app/vivaldi_apptools.h"
#include "ios/panel/panel_interaction_controller.h"

@interface HistoryCoordinator () {
  // ViewController being managed by this Coordinator.
  TableViewNavigationController* _historyNavigationController;
  HistoryTableViewController* _viewController;
}
@end

@implementation HistoryCoordinator

- (void)start {
  // Initialize and configure HistoryTableViewController.
  _viewController = [[HistoryTableViewController alloc] init];
  _viewController.searchTerms = self.searchTerms;
  _viewController.delegate = self;

  // Configure and present HistoryNavigationController.
  _historyNavigationController =
      [[TableViewNavigationController alloc] initWithTable:_viewController];
  _historyNavigationController.toolbarHidden = NO;

  if (vivaldi::IsVivaldiRunning()) {
    UINavigationBarAppearance* transparentAppearance =
        [[UINavigationBarAppearance alloc] init];
    [transparentAppearance configureWithTransparentBackground];
    self.historyNavigationController.navigationBar.standardAppearance =
      transparentAppearance;
    self.historyNavigationController.navigationBar.compactAppearance =
      transparentAppearance;
    self.historyNavigationController.navigationBar.scrollEdgeAppearance =
      transparentAppearance;
  } // End Vivaldi

  [_historyNavigationController
      setModalPresentationStyle:UIModalPresentationFormSheet];
  _historyNavigationController.presentationController.delegate =
      _viewController;

  [super start];

  // Vivaldi
  if (!vivaldi::IsVivaldiRunning())
  [self.baseViewController presentViewController:_historyNavigationController
                                        animated:YES
                                      completion:nil];
   // End Vivaldi

}

// This method should always execute the `completionHandler`.
- (void)dismissWithCompletion:(ProceduralBlock)completionHandler {
  if (_historyNavigationController) {
      [self dismissHistoryNavigationWithCompletion:completionHandler];
  } else if (completionHandler) {
    completionHandler();
  }
  [super dismissWithCompletion:completionHandler];
}

- (void)dismissHistoryNavigationWithCompletion:(ProceduralBlock)completion {
  if (vivaldi::IsVivaldiRunning()) {
    [self.historyNavigationController dismissViewControllerAnimated:YES
                                                         completion:completion];
    [self.panelDelegate panelDismissed];
    self.panelDelegate = nil;
  } else // End Vivaldi
  [_historyNavigationController dismissViewControllerAnimated:YES
                                                   completion:completion];
  _historyNavigationController = nil;
  _viewController.historyService = nullptr;
}

#pragma mark - Setters & Getters

- (BaseHistoryViewController*)viewController {
  return _viewController;
}

- (MenuScenarioHistogram)scenario {
  return kMenuScenarioHistogramHistoryEntry;
}

@end
