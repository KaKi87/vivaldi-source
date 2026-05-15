// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef IOS_UI_SETTINGS_START_PAGE_VIVALDI_START_PAGE_DAILYMIX_HELPER_H_
#define IOS_UI_SETTINGS_START_PAGE_VIVALDI_START_PAGE_DAILYMIX_HELPER_H_

#import <Foundation/Foundation.h>

extern NSString* _Nonnull const kDailyMixCreditPhotographerName;
extern NSString* _Nonnull const kDailyMixCreditPhotographerLink;
extern NSString* _Nonnull const kDailyMixCreditProviderName;
extern NSString* _Nonnull const kDailyMixCreditProviderLink;

@interface VivaldiStartPageDailyMixHelper : NSObject

+ (void)refreshIfNeeded;
+ (void)refreshIfNeededWithForceFetch:(BOOL)forceFetch;

// Returns parsed photo credit from stored metadata, or nil if unavailable.
+ (nullable NSDictionary<NSString*, NSString*>*)photoCredit;

@end

#endif  // IOS_UI_SETTINGS_START_PAGE_VIVALDI_START_PAGE_DAILYMIX_HELPER_H_
