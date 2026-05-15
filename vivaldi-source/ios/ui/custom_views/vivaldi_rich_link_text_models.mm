// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/ui/custom_views/vivaldi_rich_link_text_models.h"

@interface VivaldiRichLinkTextLink ()

@property(nonatomic, copy, readwrite) NSString* identifier;
@property(nonatomic, copy, readwrite) NSString* displayText;
@property(nonatomic, strong, readwrite, nullable) NSURL* URL;

@end

@implementation VivaldiRichLinkTextLink

- (instancetype)initWithIdentifier:(NSString*)identifier
                       displayText:(NSString*)displayText
                               URL:(NSURL*)URL {
  self = [super init];
  if (self) {
    _identifier = [identifier copy];
    _displayText = [displayText copy];
    _URL = URL;
  }
  return self;
}

@end

@implementation VivaldiRichLinkTextConfiguration

- (instancetype)init {
  self = [super init];
  if (self) {
    _font = [UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    _textColor = UIColor.secondaryLabelColor;
    _linkColor = UIColor.secondaryLabelColor;
    _textAlignment = NSTextAlignmentLeft;
    _numberOfLines = 0;
    _underlineEnabled = YES;
  }
  return self;
}

@end
