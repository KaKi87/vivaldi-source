// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/ui/custom_views/vivaldi_rich_link_text_view.h"

#import "base/check_op.h"

namespace {
const UIEdgeInsets kTextContainerInset = UIEdgeInsetsZero;
constexpr CGFloat kTextContainerPadding = 0;
// VRLT = Vivaldi Rich Link Text.
// Placeholder tokens look like "__VRLT_0__" / "__VRLT_1__".
// We inject these into localized format args first, then replace them with
// actual link display text in the attributed string.
NSString* const kPlaceholderPrefix = @"__VRLT_";
NSString* const kPlaceholderSuffix = @"__";
}  // namespace

// Current L10nUtils bridge supports up to two format arguments.
// For linksCount > 2, only the first two placeholders are localized.
@interface VivaldiRichLinkTextView () <UITextViewDelegate>

@property(nonatomic, strong) UITextView* textView;
@property(nonatomic, copy) NSArray<VivaldiRichLinkTextLink*>* links;

@end

@implementation VivaldiRichLinkTextView

- (instancetype)init {
  return [self initWithFrame:CGRectZero];
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self setUpUI];
  }
  return self;
}

- (void)setUpUI {
  self.backgroundColor = UIColor.clearColor;

  UITextView* textView = [[UITextView alloc] initWithFrame:CGRectZero];
  textView.translatesAutoresizingMaskIntoConstraints = NO;
  textView.backgroundColor = UIColor.clearColor;
  textView.editable = NO;
  textView.scrollEnabled = NO;
  textView.selectable = YES;
  textView.delegate = self;
  textView.textContainerInset = kTextContainerInset;
  textView.textContainer.lineFragmentPadding = kTextContainerPadding;
  textView.textDragInteraction.enabled = NO;
  textView.adjustsFontForContentSizeCategory = YES;
  [self addSubview:textView];
  _textView = textView;

  [NSLayoutConstraint activateConstraints:@[
    [textView.topAnchor constraintEqualToAnchor:self.topAnchor],
    [textView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [textView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
    [textView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
  ]];
}

- (NSString*)placeholderForIndex:(NSUInteger)index {
  return [NSString stringWithFormat:@"%@%lu%@", kPlaceholderPrefix,
                                    static_cast<unsigned long>(index),
                                    kPlaceholderSuffix];
}

- (NSString*)localizedTemplateForLocalizeID:(MessageID)localizeID
                                 linksCount:(NSUInteger)linksCount {
  switch (linksCount) {
    case 0:
      return [L10nUtils stringWithFixupForMessageID:localizeID];
    case 1:
      return [L10nUtils formatStringForMessageID:localizeID
                                        argument:[self placeholderForIndex:0]];
    default:
      return [L10nUtils formatStringForMessageID:localizeID
                                        argument:[self placeholderForIndex:0]
                                        argument:[self placeholderForIndex:1]];
  }
}

- (void)renderWithLocalizeID:(MessageID)localizeID
                       links:(NSArray<VivaldiRichLinkTextLink*>*)links
               configuration:(VivaldiRichLinkTextConfiguration*)configuration {
  CHECK_LE(links.count, static_cast<NSUInteger>(2));
  self.links = [links copy];
  NSString* templateText = [self localizedTemplateForLocalizeID:localizeID
                                                     linksCount:links.count];

  NSMutableParagraphStyle* paragraphStyle =
      [[NSMutableParagraphStyle alloc] init];
  paragraphStyle.alignment = configuration.textAlignment;

  NSDictionary* textAttributes = @{
    NSFontAttributeName : configuration.font,
    NSForegroundColorAttributeName : configuration.textColor,
    NSParagraphStyleAttributeName : paragraphStyle
  };

  NSMutableAttributedString* attributedText =
      [[NSMutableAttributedString alloc] initWithString:templateText
                                             attributes:textAttributes];

  NSUInteger linkIndex = 0;
  for (VivaldiRichLinkTextLink* link in links) {
    NSString* placeholder = [self placeholderForIndex:linkIndex];
    NSRange placeholderRange =
        [[attributedText string] rangeOfString:placeholder];
    if (placeholderRange.location == NSNotFound) {
      linkIndex += 1;
      continue;
    }

    [attributedText replaceCharactersInRange:placeholderRange
                                  withString:link.displayText];
    NSRange replacementRange =
        NSMakeRange(placeholderRange.location, link.displayText.length);

    if (link.URL) {
      [attributedText addAttribute:NSLinkAttributeName
                             value:link.URL
                             range:replacementRange];
    }

    if (configuration.underlineEnabled) {
      [attributedText addAttribute:NSUnderlineStyleAttributeName
                             value:@(NSUnderlineStyleSingle)
                             range:replacementRange];
    }
    linkIndex += 1;
  }

  NSMutableDictionary* linkAttributes =
      [NSMutableDictionary dictionaryWithObject:configuration.linkColor
                                         forKey:NSForegroundColorAttributeName];
  if (configuration.underlineEnabled) {
    linkAttributes[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
  }
  self.textView.linkTextAttributes = [linkAttributes copy];
  self.textView.textContainer.maximumNumberOfLines =
      configuration.numberOfLines;
  self.textView.textContainer.lineBreakMode = configuration.numberOfLines == 1
                                                  ? NSLineBreakByTruncatingTail
                                                  : NSLineBreakByWordWrapping;
  self.textView.attributedText = attributedText;
  [self invalidateIntrinsicContentSize];
}

- (CGSize)intrinsicContentSize {
  const CGFloat fittingWidth = CGRectGetWidth(self.bounds) > 0
                                   ? CGRectGetWidth(self.bounds)
                                   : CGFLOAT_MAX;
  CGSize fittingSize =
      [self.textView sizeThatFits:CGSizeMake(fittingWidth, CGFLOAT_MAX)];
  return CGSizeMake(UIViewNoIntrinsicMetric, ceil(fittingSize.height));
}

- (VivaldiRichLinkTextLink*)linkForURL:(NSURL*)URL {
  for (VivaldiRichLinkTextLink* link in self.links) {
    if ([link.URL isEqual:URL]) {
      return link;
    }
  }

  return nil;
}

#pragma mark - UITextViewDelegate

- (UIAction*)textView:(UITextView*)textView
    primaryActionForTextItem:(UITextItem*)textItem
               defaultAction:(UIAction*)defaultAction {
  if (!textItem.link) {
    return defaultAction;
  }
  VivaldiRichLinkTextLink* tappedLink = [self linkForURL:textItem.link];
  if (!tappedLink) {
    return defaultAction;
  }

  if ([self.delegate respondsToSelector:@selector(richLinkTextView:
                                                      shouldHandle:)] &&
      ![self.delegate richLinkTextView:self shouldHandle:tappedLink]) {
    return nil;
  }

  __weak __typeof(self) weakSelf = self;
  return [UIAction actionWithHandler:^(UIAction* action) {
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    if ([strongSelf.delegate respondsToSelector:@selector(richLinkTextView:
                                                                    didTap:)]) {
      [strongSelf.delegate richLinkTextView:strongSelf didTap:tappedLink];
    }
  }];
}

- (UITextItemMenuConfiguration*)textView:(UITextView*)textView
            menuConfigurationForTextItem:(UITextItem*)textItem
                             defaultMenu:(UIMenu*)defaultMenu {
  return nil;
}

- (void)textViewDidChangeSelection:(UITextView*)textView {
  textView.selectedTextRange = nil;
}

@end
