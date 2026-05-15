// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#import "ios/ui/settings/start_page/vivaldi_start_page_dailymix_helper.h"

#import <UIKit/UIKit.h>

#import "ios/ui/settings/start_page/vivaldi_start_page_constants.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_prefs.h"
#import "ios/ui/settings/start_page/wallpaper_settings/vivaldi_wallpaper_notification_constants.h"

NSString* const kDailyMixCreditPhotographerName = @"photographerName";
NSString* const kDailyMixCreditPhotographerLink = @"photographerLink";
NSString* const kDailyMixCreditProviderName = @"providerName";
NSString* const kDailyMixCreditProviderLink = @"providerLink";

@implementation VivaldiStartPageDailyMixHelper

namespace {
NSString* const kDailyMixMetadataUrl =
    @"https://downloads.vivaldi.com/background/image.json";

// Flag to prevent concurrent refresh operations.
BOOL isRefreshing = NO;

// Serial queue for coordinating refresh state.
dispatch_queue_t GetRefreshQueue() {
  static dispatch_queue_t refreshQueue;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    refreshQueue = dispatch_queue_create("com.vivaldi.ios.startpage.dailymix",
                                         DISPATCH_QUEUE_SERIAL);
  });
  return refreshQueue;
}

NSString* TodayString() {
  NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
  formatter.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
  formatter.dateFormat = @"yyyy-MM-dd";
  return [formatter stringFromDate:[NSDate date]] ?: @"";
}

BOOL IsDailyMixSelected() {
  NSString* selected = [VivaldiStartPagePrefs getWallpaperName] ?: @"";
  return [selected isEqualToString:vDailyMixWallpaperName];
}

NSDictionary* ParseJSONData(NSData* data) {
  if (!data.length) {
    return nil;
  }
  NSError* jsonError = nil;
  id json = [NSJSONSerialization JSONObjectWithData:data
                                            options:0
                                              error:&jsonError];
  if (jsonError || ![json isKindOfClass:[NSDictionary class]]) {
    return nil;
  }
  return (NSDictionary*)json;
}

NSString* ExtractImageURLFromDict(NSDictionary* dict) {
  if (!dict) {
    return nil;
  }
  id urls = dict[@"urls"];
  if ([urls isKindOfClass:[NSDictionary class]]) {
    id full = ((NSDictionary*)urls)[@"full"];
    if ([full isKindOfClass:[NSString class]] && [(NSString*)full length] > 0) {
      return (NSString*)full;
    }
  }
  return nil;
}

NSString* SerializeJSONDict(NSDictionary* dict) {
  if (!dict) {
    return @"";
  }
  NSError* error = nil;
  NSData* data = [NSJSONSerialization dataWithJSONObject:dict
                                                 options:0
                                                   error:&error];
  if (error || !data) {
    return @"";
  }
  return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]
             ?: @"";
}

void PostWallpaperUpdateNotification() {
  dispatch_async(dispatch_get_main_queue(), ^{
    [[NSNotificationCenter defaultCenter]
        postNotificationName:vWallpaperUpdateNotificationName
                      object:nil];
  });
}

NSString* GetDailyMixLastFetchDateOnMainThread() {
  if ([NSThread isMainThread]) {
    return [VivaldiStartPagePrefs getDailyMixLastFetchDate] ?: @"";
  }

  __block NSString* lastFetch = @"";
  dispatch_sync(dispatch_get_main_queue(), ^{
    lastFetch = [VivaldiStartPagePrefs getDailyMixLastFetchDate] ?: @"";
  });
  return lastFetch;
}

UIImage* GetDailyMixWallpaperOnMainThread() {
  if ([NSThread isMainThread]) {
    return [VivaldiStartPagePrefs getDailyMixWallpaper];
  }

  __block UIImage* wallpaper = nil;
  dispatch_sync(dispatch_get_main_queue(), ^{
    wallpaper = [VivaldiStartPagePrefs getDailyMixWallpaper];
  });
  return wallpaper;
}

// Clears the daily mix selection when fetch fails and no cached image exists.
void ClearDailyMixSelection() {
  dispatch_async(dispatch_get_main_queue(), ^{
    [VivaldiStartPagePrefs setDailyMixLastFetchDate:@""];
    [VivaldiStartPagePrefs setWallpaperName:@""];
    [[NSNotificationCenter defaultCenter]
        postNotificationName:vWallpaperUpdateNotificationName
                      object:nil];
  });
}

// Resets the refresh flag on the serial queue.
void ResetRefreshingFlag() {
  dispatch_async(GetRefreshQueue(), ^{
    isRefreshing = NO;
  });
}

}  // namespace

#pragma mark - Private Methods

// Handles fetch failure by checking for cached image or clearing selection.
+ (void)handleFetchFailure {
  // Check cache and keep pref access on the main sequence.
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    UIImage* cached = GetDailyMixWallpaperOnMainThread();
    if (cached) {
      // Have cached image from previous fetch
      PostWallpaperUpdateNotification();
    } else {
      // No cached image available, clear fetch date and deselect Daily Mix.
      ClearDailyMixSelection();
    }
  });
}

// Saves the image and metadata to prefs and posts update notification.
+ (void)saveImage:(UIImage*)image
          forDate:(NSString*)dateString
         metadata:(NSDictionary*)metadata
       completion:(void (^)(void))completion {
  dispatch_async(dispatch_get_main_queue(), ^{
    [VivaldiStartPagePrefs setDailyMixLastFetchDate:dateString];
    [VivaldiStartPagePrefs setDailyMixWallpaper:image];
    [VivaldiStartPagePrefs setDailyMixMetadata:SerializeJSONDict(metadata)];
    PostWallpaperUpdateNotification();
    if (completion) {
      completion();
    }
  });
}

// Downloads the image from the given URL.
+ (void)fetchImageFromURL:(NSURL*)imageURL
               completion:(void (^)(UIImage* image))completion {
  NSURLSessionDataTask* task = [[NSURLSession sharedSession]
        dataTaskWithURL:imageURL
      completionHandler:^(NSData* data, NSURLResponse* response,
                          NSError* error) {
        UIImage* image = nil;
        if (!error && data.length) {
          image = [UIImage imageWithData:data];
        }
        if (completion) {
          completion(image);
        }
      }];
  [task resume];
}

// Fetches the metadata JSON and extracts the image URL and full metadata dict.
// Uses NSURLRequestReloadIgnoringLocalCacheData to bypass iOS local cache,
// and Cache-Control: no-cache header to force CDN revalidation with origin.
+ (void)fetchMetadataWithCompletion:
    (void (^)(NSURL* imageURL, NSDictionary* metadata))completion {
  NSURL* metadataURL = [NSURL URLWithString:kDailyMixMetadataUrl];
  if (!metadataURL) {
    if (completion) {
      completion(nil, nil);
    }
    return;
  }

  NSMutableURLRequest* request = [NSMutableURLRequest
       requestWithURL:metadataURL
          cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
      timeoutInterval:30];
  [request setValue:@"no-cache" forHTTPHeaderField:@"Cache-Control"];
  NSURLSessionDataTask* task = [[NSURLSession sharedSession]
      dataTaskWithRequest:request
        completionHandler:^(NSData* data, NSURLResponse* response,
                            NSError* error) {
          NSURL* imageURL = nil;
          NSDictionary* metadata = nil;
          if (!error && data) {
            metadata = ParseJSONData(data);
            NSString* urlString = ExtractImageURLFromDict(metadata);
            if (urlString.length) {
              imageURL = [NSURL URLWithString:urlString];
            }
          }
          if (completion) {
            completion(imageURL, metadata);
          }
        }];
  [task resume];
}

#pragma mark - Public Methods

+ (nullable NSDictionary<NSString*, NSString*>*)photoCredit {
  NSString* jsonString = [VivaldiStartPagePrefs getDailyMixMetadata];
  if (!jsonString.length) {
    return nil;
  }

  NSData* data = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
  NSDictionary* metadata = ParseJSONData(data);
  if (!metadata) {
    return nil;
  }

  id photographer = metadata[@"photographer"];
  id provider = metadata[@"provider"];
  NSString* name = [photographer isKindOfClass:[NSDictionary class]]
                       ? photographer[@"name"]
                       : nil;
  NSString* link = [photographer isKindOfClass:[NSDictionary class]]
                       ? photographer[@"link"]
                       : nil;
  NSString* provName =
      [provider isKindOfClass:[NSDictionary class]] ? provider[@"name"] : nil;
  NSString* provLink =
      [provider isKindOfClass:[NSDictionary class]] ? provider[@"link"] : nil;

  if (!name.length || !provName.length) {
    return nil;
  }

  return @{
    kDailyMixCreditPhotographerName : name,
    kDailyMixCreditPhotographerLink : link ?: @"",
    kDailyMixCreditProviderName : provName,
    kDailyMixCreditProviderLink : provLink ?: @"",
  };
}

+ (void)refreshIfNeeded {
  [self refreshIfNeededWithForceFetch:NO];
}

+ (void)refreshIfNeededWithForceFetch:(BOOL)forceFetch {
  // Only refresh when Daily Mix is selected.
  if (!IsDailyMixSelected()) {
    return;
  }

  dispatch_async(GetRefreshQueue(), ^{
    if (isRefreshing) {
      return;
    }

    // Check cache and keep pref access on the main sequence.
    NSString* today = TodayString();
    NSString* lastFetch = GetDailyMixLastFetchDateOnMainThread();
    UIImage* cached = GetDailyMixWallpaperOnMainThread();

    if (!forceFetch && [lastFetch isEqualToString:today] && cached) {
      // Already have cached image for today - just notify UI.
      PostWallpaperUpdateNotification();
      return;
    }

    isRefreshing = YES;

    // Step 1: Fetch metadata to get image URL and credit info.
    [self
        fetchMetadataWithCompletion:^(NSURL* imageURL, NSDictionary* metadata) {
          if (!imageURL) {
            [self handleFetchFailure];
            ResetRefreshingFlag();
            return;
          }

          // Step 2: Download the image.
          [self fetchImageFromURL:imageURL
                       completion:^(UIImage* image) {
                         if (!image) {
                           // Image download failed - handle error.
                           [self handleFetchFailure];
                           ResetRefreshingFlag();
                           return;
                         }

                         // Step 3: Verify Daily Mix is still selected and save.
                         dispatch_async(dispatch_get_main_queue(), ^{
                           if (!IsDailyMixSelected()) {
                             ResetRefreshingFlag();
                             return;
                           }
                           // Step 4: Save image + metadata and notify.
                           [self saveImage:image
                                   forDate:today
                                  metadata:metadata
                                completion:^{
                                  ResetRefreshingFlag();
                                }];
                         });
                       }];
        }];
  });
}

@end
