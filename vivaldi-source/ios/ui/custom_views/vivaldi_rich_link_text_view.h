// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_CUSTOM_VIEWS_VIVALDI_RICH_LINK_TEXT_VIEW_H_
#define IOS_UI_CUSTOM_VIEWS_VIVALDI_RICH_LINK_TEXT_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/ui/custom_views/vivaldi_rich_link_text_models.h"
#import "ui/base/l10n/l10n_util_mac_bridge.h"

NS_ASSUME_NONNULL_BEGIN

@class VivaldiRichLinkTextView;

@protocol VivaldiRichLinkTextViewDelegate <NSObject>

@optional
- (BOOL)richLinkTextView:(VivaldiRichLinkTextView*)view
            shouldHandle:(VivaldiRichLinkTextLink*)link;
- (void)richLinkTextView:(VivaldiRichLinkTextView*)view
                  didTap:(VivaldiRichLinkTextLink*)link;

@end

// A reusable view that renders localized text with tappable links.
@interface VivaldiRichLinkTextView : UIView

@property(nonatomic, weak, nullable) id<VivaldiRichLinkTextViewDelegate>
    delegate;

- (void)renderWithLocalizeID:(MessageID)localizeID
                       links:(NSArray<VivaldiRichLinkTextLink*>*)links
               configuration:(VivaldiRichLinkTextConfiguration*)configuration
    NS_SWIFT_NAME(render(localizeId:links:configuration:));

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_UI_CUSTOM_VIEWS_VIVALDI_RICH_LINK_TEXT_VIEW_H_
