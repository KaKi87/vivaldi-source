// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_CUSTOM_VIEWS_VIVALDI_RICH_LINK_TEXT_MODELS_H_
#define IOS_UI_CUSTOM_VIEWS_VIVALDI_RICH_LINK_TEXT_MODELS_H_

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

// Model representing one link replacement inside a localized template text.
@interface VivaldiRichLinkTextLink : NSObject

@property(nonatomic, copy, readonly) NSString* identifier;
@property(nonatomic, copy, readonly) NSString* displayText;
@property(nonatomic, strong, readonly, nullable) NSURL* URL;

- (instancetype)initWithIdentifier:(NSString*)identifier
                       displayText:(NSString*)displayText
                               URL:(nullable NSURL*)URL
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

// Visual configuration for rich link text rendering.
@interface VivaldiRichLinkTextConfiguration : NSObject

@property(nonatomic, strong) UIFont* font;
@property(nonatomic, strong) UIColor* textColor;
@property(nonatomic, strong) UIColor* linkColor;
@property(nonatomic, assign) NSTextAlignment textAlignment;
@property(nonatomic, assign) NSInteger numberOfLines;
@property(nonatomic, assign, getter=isUnderlineEnabled) BOOL underlineEnabled;

- (instancetype)init NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_UI_CUSTOM_VIEWS_VIVALDI_RICH_LINK_TEXT_MODELS_H_
