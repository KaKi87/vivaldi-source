// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ntp/ui_bundled/incognito/incognito_view_controller.h"

#import "ios/chrome/browser/ntp/ui_bundled/incognito/incognito_view.h"
#import "ios/chrome/browser/ntp/ui_bundled/new_tab_page_constants.h"
#import "ios/chrome/browser/ntp/ui_bundled/new_tab_page_url_loader_delegate.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"
#import "ios/ui/ntp/vivaldi_ntp_constants.h"
#import "ios/ui/ntp/vivaldi_private_ntp_view.h"

using vivaldi::IsVivaldiRunning;
// End Vivaldi

@interface IncognitoViewController ()

// The scrollview containing the actual views.
@property(nonatomic, strong) UIScrollView* incognitoView;

@end

@implementation IncognitoViewController

- (void)viewDidLoad {
  self.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;

  if (IsVivaldiRunning()) {
    UIImageView* bgView = [UIImageView new];
    bgView.contentMode = UIViewContentModeScaleAspectFill;
    bgView.image = [UIImage imageNamed:vNTPPrivateTabBG];
    bgView.backgroundColor = UIColor.clearColor;
    [self.view addSubview:bgView];
    [bgView fillSuperview];

    VivaldiPrivateNTPView* privateView = [VivaldiPrivateNTPView new];
    [self.view addSubview:privateView];
    [privateView fillSuperview];
    privateView.URLLoaderDelegate = self.URLLoaderDelegate;
  } else {
  IncognitoView* view = [[IncognitoView alloc] initWithFrame:self.view.bounds];
  view.URLLoaderDelegate = self.URLLoaderDelegate;
  self.incognitoView = view;
  self.incognitoView.accessibilityIdentifier = kNTPIncognitoViewIdentifier;
  [self.incognitoView setAutoresizingMask:UIViewAutoresizingFlexibleHeight |
                                          UIViewAutoresizingFlexibleWidth];
  self.incognitoView.backgroundColor = [UIColor colorNamed:kBackgroundColor];
  [self.view addSubview:self.incognitoView];
  } // End Vivaldi

}

- (void)dealloc {
  [_incognitoView setDelegate:nil];
}

@end
