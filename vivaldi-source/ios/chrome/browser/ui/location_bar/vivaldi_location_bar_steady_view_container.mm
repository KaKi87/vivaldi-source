// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/chrome/browser/ui/location_bar/vivaldi_location_bar_steady_view_container.h"

#import "ios/chrome/browser/ui/location_bar/location_bar_constants+vivaldi.h"

@interface VivaldiLocationBarLeadingButtonContainer : UIView

@property(nonatomic, weak) UIButton* leadingButton;

@end

@implementation VivaldiLocationBarLeadingButtonContainer

- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  if (self.leadingButton.hidden || !self.leadingButton.enabled) {
    return nil;
  }
  UIView* hitView = [super hitTest:point withEvent:event];
  return hitView == self ? self.leadingButton : hitView;
}

@end

void AttachVivaldiLocationBarLeadingButtonContainer(UIView* locationButton,
                                                    UIButton* leadingButton) {
  VivaldiLocationBarLeadingButtonContainer* container =
      [[VivaldiLocationBarLeadingButtonContainer alloc] init];
  container.translatesAutoresizingMaskIntoConstraints = NO;
  container.leadingButton = leadingButton;
  [locationButton addSubview:container];
  [container addSubview:leadingButton];

  [NSLayoutConstraint activateConstraints:@[
    [container.leadingAnchor
        constraintEqualToAnchor:locationButton.leadingAnchor],
    [container.topAnchor constraintEqualToAnchor:locationButton.topAnchor],
    [container.bottomAnchor
        constraintEqualToAnchor:locationButton.bottomAnchor],
    [container.widthAnchor
        constraintEqualToConstant:vLocationBarLeadingButtonTouchTargetWidth],
  ]];
}
