// Copyright 2025 Vivaldi Technologies. All rights reserved.

#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_coordinator.h"

#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/ui/helpers/helpers_swift.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_mediator.h"
#import "ios/ui/settings/search_engine/editor/vivaldi_search_engine_editor_swift.h"
#import "ui/base/l10n/l10n_util_mac.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"

namespace {
NSString* const kCustomDetentIdentifier = @"searchEngineCustomDetent";
const CGFloat minimumDetentHeight = 280.0;
}

@interface VivaldiSearchEngineEditorCoordinator () <
    VivaldiHostingControllerPresentationDelegate> {
  UINavigationController* _navigationController;
}

// SwiftUI view provider for the search engine editor interface
@property(nonatomic, strong)
    VivaldiSearchEngineEditorViewProvider* viewProvider;

// Main view controller hosting the SwiftUI interface
@property(nonatomic, strong) UIViewController* controller;

// Mediator handling business logic and data management
@property(nonatomic, strong) VivaldiSearchEngineEditorMediator* mediator;

// Whether we're adding or editing.
@property(nonatomic, assign)
    VivaldiSearchEngineEditorEntryReason entryReason;

// Whether the user has made unsaved changes
@property(nonatomic, assign) BOOL hasActiveChanges;

// Whether the edited form data is valid and can be saved
@property(nonatomic, assign) BOOL isFormDataValid;

@property(nonatomic, assign) VivaldiSearchEngineEditorEntryPoint entryPoint;
@property(nonatomic, assign) BOOL allowsCancel;
@property(nonatomic, strong) VivaldiSearchEngineEditorItem* item;

@end

@implementation VivaldiSearchEngineEditorCoordinator

@synthesize baseNavigationController = _baseNavigationController;

#pragma mark - Initialization

- (instancetype)
    initWithBaseViewController:(UIViewController*)viewController
                       browser:(Browser*)browser
                    entryPoint:(VivaldiSearchEngineEditorEntryPoint)entryPoint
                   entryReason:(VivaldiSearchEngineEditorEntryReason)entryReason
                          item:(VivaldiSearchEngineEditorItem*)item
                  allowsCancel:(BOOL)allowsCancel {
  self = [super initWithBaseViewController:viewController browser:browser];
  if (self) {
    _entryPoint = entryPoint;
    _entryReason = entryReason;
    _item = item;
    _hasActiveChanges = NO;
    _allowsCancel = allowsCancel;
  }
  return self;
}

- (instancetype)initWithBaseNavigationController:
                    (UINavigationController*)navigationController
                                         browser:(Browser*)browser
                                      entryPoint:
                                          (VivaldiSearchEngineEditorEntryPoint)
                                              entryPoint
                                     entryReason:
                                         (VivaldiSearchEngineEditorEntryReason)
                                             entryReason
                                            item:
                                                (VivaldiSearchEngineEditorItem*)
                                                    item
                                     allowsCancel:(BOOL)allowsCancel {
  self = [self initWithBaseViewController:navigationController
                                   browser:browser
                                entryPoint:entryPoint
                               entryReason:entryReason
                                      item:item
                               allowsCancel:allowsCancel];
  if (self) {
    _baseNavigationController = navigationController;
  }
  return self;
}

#pragma mark - ChromeCoordinator

- (void)start {
  [self setupViewProvider];
  [self setupViewController];
  [self setupMediator];
  [self setupNavigationItems];
  [self presentViewController];
}

- (void)stop {
  [super stop];
  [self cleanup];
}

#pragma mark - Private Setup Methods

- (void)setupViewProvider {
  self.viewProvider = [[VivaldiSearchEngineEditorViewProvider alloc] init];

  __weak __typeof(self) weakSelf = self;

  // Observe item state changes to show save button
  [self.viewProvider
      observeItemStateChangeEvent:^(VivaldiSearchEngineEditorItem* item) {
        [weakSelf handleItemStateChange:item];
      }];

  // Observe backend changes to dismiss editor
  [self.viewProvider observeSearchEngineBackendDidChangeEvent:^{
    [weakSelf handleBackendChange];
  }];
}

- (void)setupViewController {
  self.controller = [self.viewProvider
      makeViewControllerWithPresentationDelegate:self
                                      entryPoint:self.entryPoint];

  NSString* title = nil;
  if (self.entryPoint ==
      VivaldiSearchEngineEditorEntryPointContextMenu) {
    title = l10n_util::GetNSString(
        IDS_VIVALDI_CONTEXT_MENU_ADD_AS_ENGINE_TITLE);
  } else {
    title = (self.entryReason ==
             VivaldiSearchEngineEditorEntryReasonEdit)
                ? l10n_util::GetNSString(
                      IDS_VIVALDI_SEARCH_ENGINE_EDITOR_EDIT_ENGINE_TITLE)
                : l10n_util::GetNSString(
                      IDS_VIVALDI_SEARCH_ENGINE_EDITOR_ADD_ENGINE_TITLE);
  }

  self.controller.title = title;
  self.controller.navigationItem.largeTitleDisplayMode =
      UINavigationItemLargeTitleDisplayModeNever;
  self.controller.modalPresentationStyle = UIModalPresentationPageSheet;
  _navigationController =
      [[UINavigationController alloc] initWithRootViewController:self.controller];

  if (self.allowsCancel) {
    UIBarButtonItem *cancelItem =
        [[UIBarButtonItem alloc]
            initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                 target:self
                                 action:@selector(handleCancelButtonTap)];
    _navigationController.topViewController
        .navigationItem.leftBarButtonItem = cancelItem;
  }
  [self setupSheetPresentationController];
}

- (void)setupMediator {
  self.mediator = [[VivaldiSearchEngineEditorMediator alloc]
      initWithProfile:self.browser->GetProfile()
          entryReason:self.entryReason
           entryPoint:self.entryPoint
                 item:self.item];

  // Connect mediator and view provider
  self.mediator.consumer = self.viewProvider;
  self.viewProvider.consumer = self.mediator;
}

- (void)setupNavigationItems {
  UIBarButtonItem* doneButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                           target:self
                           action:@selector(handleDoneButtonTap)];

  self.controller.navigationItem.rightBarButtonItem = doneButton;
}

- (void)setupSheetPresentationController {
  UISheetPresentationController *sheetPc =
      _navigationController.sheetPresentationController;
  // When iPad full screen or 2/3 SplitView support only large detent because
  // medium detent cuts the contents makes the dialog small and off centered.
  if (IsSplitToolbarMode(self.baseViewController)) {
    auto preferredHeightForSheetContent = ^CGFloat(
        id<UISheetPresentationControllerDetentResolutionContext> context) {
      return MAX(context.maximumDetentValue / 3.0, minimumDetentHeight);
    };
    UISheetPresentationControllerDetent* customDetent =
        [UISheetPresentationControllerDetent
            customDetentWithIdentifier:kCustomDetentIdentifier
                              resolver:preferredHeightForSheetContent];

    sheetPc.detents = @[ customDetent ];
    sheetPc.selectedDetentIdentifier = kCustomDetentIdentifier;
  } else {
    sheetPc.detents = @[ UISheetPresentationControllerDetent.largeDetent ];
  }

  sheetPc.prefersScrollingExpandsWhenScrolledToEdge = NO;
  sheetPc.widthFollowsPreferredContentSizeWhenEdgeAttached = YES;
}

- (void)presentViewController {
  if (self.entryPoint ==
      VivaldiSearchEngineEditorEntryPointContextMenu) {
    [self.baseViewController presentViewController:_navigationController
                                          animated:YES completion:nil];
  } else {
    [self.baseNavigationController pushViewController:self.controller
                                             animated:YES];
  }
}

#pragma mark - Event Handlers

- (void)handleItemStateChange:(VivaldiSearchEngineEditorItem*)item {
  self.isFormDataValid = item.isFormValid;
  if (self.hasActiveChanges) {
    return;
  }

  self.hasActiveChanges = YES;

  auto buttonStyle = UIBarButtonSystemItemSave;
  if (@available(iOS 26, *)) {
    buttonStyle = UIBarButtonSystemItemDone;
  }

  UIBarButtonItem* saveButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:buttonStyle
                           target:self
                           action:@selector(handleSaveButtonTap)];

  self.controller.navigationItem.rightBarButtonItem = saveButton;
}

- (void)handleBackendChange {
  [self.delegate searchEngineEditorShouldDismiss:self];
  [self dismissPresentedController];
}

#pragma mark - Button Actions

- (void)handleDoneButtonTap {
  [self stop];
  [self.delegate searchEngineEditorShouldDismiss:self];
  [self dismissPresentedController];
}

- (void)handleSaveButtonTap {
  if (!self.isFormDataValid)
    return;
  [self.mediator saveChanges];
}

- (void)handleCancelButtonTap {
  if (self.allowsCancel) {
    [self stop];
    [self.baseViewController dismissViewControllerAnimated:YES completion:nil];
  }
}

#pragma mark - Cleanup

- (void)cleanup {
  self.viewProvider.consumer = nil;
  self.viewProvider = nil;
  self.controller = nil;
  self.item = nil;
  _navigationController = nil;

  [self.mediator disconnect];
  self.mediator = nil;

  self.hasActiveChanges = NO;
}

#pragma mark - VivaldiHostingControllerPresentationDelegate

- (void)hostingController:(UIViewController*)hostingController
                didMoveTo:(UIViewController*)parent {
  DCHECK_EQ(self.controller, hostingController);
  if (parent != nil) {
    // TODO: why are we getting nil parent when
    // there is a guard on the swift side?
    [self.delegate searchEngineEditorShouldDismiss:self];
  }
}

#pragma mark - Helpers

- (void)dismissPresentedController {
  if (self.entryPoint == VivaldiSearchEngineEditorEntryPointContextMenu) {
    if (_navigationController) {
      [_navigationController dismissViewControllerAnimated:YES completion:nil];
      _navigationController = nil;
    }
  } else {
    [self.baseNavigationController popViewControllerAnimated:YES];
  }
}

@end
