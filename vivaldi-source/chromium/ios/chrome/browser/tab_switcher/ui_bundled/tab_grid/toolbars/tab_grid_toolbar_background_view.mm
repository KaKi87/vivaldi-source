// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/toolbars/tab_grid_toolbar_background_view.h"

#import "ios/chrome/browser/shared/ui/elements/gradient/gradient_blur.h"
#import "ios/chrome/browser/shared/ui/elements/gradient/gradient_view.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#import "ios/chrome/common/ui/util/constraints_ui_util.h"
#import "ios/chrome/common/ui/util/ui_util.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/grid/grid_constants.h"
// End Vivaldi

namespace {

// The distance at the start of which the background starts dimming out.
constexpr CGFloat kScrollToEdgeAlphaUpdateDistance = 60;

// The extra distance the lowest background should extend to.
constexpr CGFloat kLowestBackgroundExtraDistance = 75;

// The percentage of the blur effect.
constexpr CGFloat kBlurEffectPercentage = 0.1;

// The length of the blur effect.
constexpr CGFloat kBlurLength = 0.3;

// The length of the top background gradient.
constexpr CGFloat kTopBackgroundGradientLength = 0.5;

// The alpha of the black color for the top background gradient.
constexpr CGFloat kTopBackgroundBlackAlpha = 0.5;

}  // namespace

@implementation TabGridToolbarBackgroundView {
  // The background view standing at the bottom.
  UIView* _lowestBackground;
  // The blur, covering _lowestBackground.
  UIView* _blurBackground;
  // The background view standing at the top, covering _blurBackground.
  UIView* _topBackground;

  // Vivaldi
  // Forces dark mode gradients/blur when showing incognito.
  BOOL _forceDarkModeGradient;
  // End Vivaldi

}

- (instancetype)initWithPosition:(TabGridToolbarBackgroundPosition)position {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    BOOL isTop = position == TabGridToolbarBackgroundPosition::kTop;

    _lowestBackground = [self createLowestBackgroundForTop:isTop];
    [self addSubview:_lowestBackground];
    AddSameConstraintsWithInsets(
        self, _lowestBackground,
        NSDirectionalEdgeInsetsMake(
            isTop ? 0 : kLowestBackgroundExtraDistance, 0,
            isTop ? kLowestBackgroundExtraDistance : 0, 0));

    _blurBackground = [self createBlurBackgroundForTop:isTop];
    [self addSubview:_blurBackground];
    AddSameConstraints(self, _blurBackground);

    _topBackground = [self createTopBackgroundForTop:isTop];
    [self addSubview:_topBackground];
    AddSameConstraints(self, _topBackground);
  }
  return self;
}

- (void)setRemainingScrollDistance:(CGFloat)remainingScrollDistance {
  _remainingScrollDistance = remainingScrollDistance;
  _lowestBackground.alpha =
      remainingScrollDistance / kScrollToEdgeAlphaUpdateDistance;
}

#pragma mark - Private

// Creates the lowest background view.
- (UIView*)createLowestBackgroundForTop:(BOOL)isTop {

  if (vivaldi::IsVivaldiRunning()) {
    return [self createVivaldiLowestBackgroundForTop:isTop];
  } // End Vivaldi

  CGPoint lowestStartPoint = isTop ? CGPointMake(0.5, 1) : CGPointMake(0.5, 0);
  CGPoint lowestEndPoint = isTop ? CGPointMake(0.5, 0) : CGPointMake(0.5, 1);
  UIView* lowestBackground = [[GradientView alloc]
      initWithStartColor:UIColor.clearColor
                endColor:UIColor.blackColor
              startPoint:lowestStartPoint
                endPoint:lowestEndPoint
            gradientType:GradientLayerType::kEaseInThenLinear];
  lowestBackground.translatesAutoresizingMaskIntoConstraints = NO;
  return lowestBackground;
}

// Creates the blur background view.
- (UIView*)createBlurBackgroundForTop:(BOOL)isTop {

  if (vivaldi::IsVivaldiRunning()) {
    return [self createVivaldiBlurBackgroundForTop:isTop];
  } // End Vivaldi

  CGPoint blurStartPoint =
      isTop ? CGPointMake(0.5, 1 - kBlurLength) : CGPointMake(0.5, kBlurLength);
  CGPoint blurEndPoint = isTop ? CGPointMake(0.5, 1) : CGPointMake(0.5, 0);
  UIBlurEffect* targetEffect =
      [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemChromeMaterialDark];
  UIView* blurBackground =
      [[GradientBlur alloc] initWithEffect:targetEffect
                          effectPercentage:kBlurEffectPercentage
                                startPoint:blurStartPoint
                                  endPoint:blurEndPoint
                              gradientType:GradientLayerType::kLinear];
  blurBackground.translatesAutoresizingMaskIntoConstraints = NO;
  return blurBackground;
}

// Creates the top background view.
- (UIView*)createTopBackgroundForTop:(BOOL)isTop {

  if (vivaldi::IsVivaldiRunning()) {
    return [self createVivaldiTopBackgroundForTop:isTop];
  } // End Vivaldi

  CGPoint topStartPoint = isTop ? CGPointMake(0.5, 0) : CGPointMake(0.5, 1);
  CGPoint topEndPoint =
      isTop ? CGPointMake(0.5, 0 + kTopBackgroundGradientLength)
            : CGPointMake(0.5, 1 - kTopBackgroundGradientLength);
  UIView* topBackground = [[GradientView alloc]
      initWithStartColor:[UIColor.blackColor
                             colorWithAlphaComponent:kTopBackgroundBlackAlpha]
                endColor:UIColor.clearColor
              startPoint:topStartPoint
                endPoint:topEndPoint
            gradientType:GradientLayerType::kLinear];
  topBackground.translatesAutoresizingMaskIntoConstraints = NO;
  return topBackground;
}

// Vivaldi
- (void)setForceDarkModeGradient:(BOOL)forceDarkModeGradient {
  if (_forceDarkModeGradient == forceDarkModeGradient) {
    return;
  }
  _forceDarkModeGradient = forceDarkModeGradient;
  [self updateGradientInterfaceStyle];
}

// Updates the trait style used to resolve dynamic colors and material effects.
- (void)updateGradientInterfaceStyle {
  UIUserInterfaceStyle style = _forceDarkModeGradient
                                   ? UIUserInterfaceStyleDark
                                   : UIUserInterfaceStyleUnspecified;
  self.overrideUserInterfaceStyle = style;
  _lowestBackground.overrideUserInterfaceStyle = style;
  _blurBackground.overrideUserInterfaceStyle = style;
  _topBackground.overrideUserInterfaceStyle = style;
}

- (UIView*)createVivaldiLowestBackgroundForTop:(BOOL)isTop {
  CGPoint lowestStartPoint = isTop ? CGPointMake(0.5, 1) : CGPointMake(0.5, 0);
  CGPoint lowestEndPoint = isTop ? CGPointMake(0.5, 0) : CGPointMake(0.5, 1);
  UIColor* backgroundColor = [UIColor colorNamed:kGridBackgroundColor];
  UIView* lowestBackground = [[GradientView alloc]
      initWithStartColor:[backgroundColor colorWithAlphaComponent:0]
                endColor:backgroundColor
              startPoint:lowestStartPoint
                endPoint:lowestEndPoint
            gradientType:GradientLayerType::kEaseInThenLinear];
  lowestBackground.translatesAutoresizingMaskIntoConstraints = NO;
  return lowestBackground;
}

- (UIView*)createVivaldiBlurBackgroundForTop:(BOOL)isTop {
  CGPoint blurStartPoint =
      isTop ? CGPointMake(0.5, 1 - kBlurLength) : CGPointMake(0.5, kBlurLength);
  CGPoint blurEndPoint = isTop ? CGPointMake(0.5, 1) : CGPointMake(0.5, 0);
  UIBlurEffect* targetEffect =
      [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemChromeMaterial];
  UIView* blurBackground =
      [[GradientBlur alloc] initWithEffect:targetEffect
                          effectPercentage:kBlurEffectPercentage
                                startPoint:blurStartPoint
                                  endPoint:blurEndPoint
                              gradientType:GradientLayerType::kLinear];
  blurBackground.translatesAutoresizingMaskIntoConstraints = NO;
  return blurBackground;
}

- (UIView*)createVivaldiTopBackgroundForTop:(BOOL)isTop {
  CGPoint topStartPoint = isTop ? CGPointMake(0.5, 0) : CGPointMake(0.5, 1);
  CGPoint topEndPoint =
      isTop ? CGPointMake(0.5, 0 + kTopBackgroundGradientLength)
            : CGPointMake(0.5, 1 - kTopBackgroundGradientLength);
  UIColor* backgroundColor = [UIColor colorNamed:kGridBackgroundColor];
  UIView* topBackground = [[GradientView alloc]
      initWithStartColor:[backgroundColor
                             colorWithAlphaComponent:kTopBackgroundBlackAlpha]
                endColor:[backgroundColor colorWithAlphaComponent:0]
              startPoint:topStartPoint
                endPoint:topEndPoint
            gradientType:GradientLayerType::kLinear];
  topBackground.translatesAutoresizingMaskIntoConstraints = NO;
  return topBackground;
}
// End Vivaldi

@end
