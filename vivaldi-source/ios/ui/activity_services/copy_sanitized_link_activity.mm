// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#import "ios/ui/activity_services/copy_sanitized_link_activity.h"

#import "base/check.h"
#import "base/functional/bind.h"
#import "base/functional/callback_helpers.h"
#import "components/omnibox/vivaldi_url_utils.h"
#import "ios/chrome/browser/shared/ui/symbols/symbols.h"
#import "ios/chrome/browser/shared/ui/util/pasteboard_util.h"
#import "ios/chrome/browser/sharing/ui_bundled/activity_services/data/share_to_data.h"
#import "ios/ui/vivaldi_symbols/vivaldi_symbol_names.h"
#import "ui/base/l10n/l10n_util_mac.h"
#import "url/gurl.h"
#import "vivaldi/ios/grit/vivaldi_ios_native_strings.h"

namespace {

NSString* const kCopySanitizedLinkActivityType =
    @"com.vivaldi.ios.copySanitizedLinkActivity";

}  // namespace

@interface CopySanitizedLinkActivity ()

@property(nonatomic, strong) NSArray<ShareToData*>* dataItems;

@end

@implementation CopySanitizedLinkActivity

- (instancetype)initWithDataItems:(NSArray<ShareToData*>*)dataItems {
  DCHECK(dataItems);
  DCHECK(dataItems.count);
  self = [super init];
  if (self) {
    _dataItems = dataItems;
  }
  return self;
}

- (NSString*)activityType {
  return kCopySanitizedLinkActivityType;
}

- (NSString*)activityTitle {
  return l10n_util::GetNSString(IDS_VIVALDI_IOS_COPY_SANITIZED_LINK);
}

- (UIImage*)activityImage {
  return CustomSymbolWithPointSize(vMenuCopy, kSymbolActionPointSize);
}

- (BOOL)canPerformWithActivityItems:(NSArray*)activityItems {
  return !!self.dataItems && self.dataItems.count;
}

- (void)prepareWithActivityItems:(NSArray*)activityItems {
}

+ (UIActivityCategory)activityCategory {
  return UIActivityCategoryAction;
}

- (void)performActivity {
  __weak __typeof(self) weakSelf = self;
  if (self.dataItems.count == 1 && self.dataItems.firstObject.additionalText) {
    StoreInPasteboard(
        self.dataItems.firstObject.additionalText,
        CopyUrlWithoutParameters(self.dataItems.firstObject.shareURL),
        base::BindOnce(^{
          [weakSelf activityDidFinish:YES];
        }));
  } else {
    std::vector<GURL> urls;
    for (ShareToData* shareToData in self.dataItems) {
      urls.push_back(CopyUrlWithoutParameters(shareToData.shareURL));
    }
    StoreURLsInPasteboard(urls, base::BindOnce(^{
                            [weakSelf activityDidFinish:YES];
                          }));
  }
}

@end
