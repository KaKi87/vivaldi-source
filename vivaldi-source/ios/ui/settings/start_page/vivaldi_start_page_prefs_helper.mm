// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#import "ios/ui/settings/start_page/vivaldi_start_page_prefs_helper.h"

#import "ios/ui/settings/start_page/vivaldi_start_page_dailymix_helper.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_prefs.h"

@implementation VivaldiStartPagePrefsHelper

#pragma mark - Getters

+ (const SpeedDialSortingMode)getSDSortingMode {
  return [VivaldiStartPagePrefs getSDSortingMode];
}

+ (const SpeedDialSortingOrder)getSDSortingOrder {
  return [VivaldiStartPagePrefs getSDSortingOrder];
}

+ (const VivaldiStartPageLayoutStyle)getStartPageLayoutStyle {
  return [VivaldiStartPagePrefs getStartPageLayoutStyle];
}

+ (const VivaldiStartPageLayoutColumn)getStartPageSpeedDialMaximumColumns {
  return [VivaldiStartPagePrefs getStartPageSpeedDialMaximumColumns];
}

+ (BOOL)showFrequentlyVisitedPages {
  return [VivaldiStartPagePrefs showFrequentlyVisitedPages];
}

+ (BOOL)showSpeedDials {
  return [VivaldiStartPagePrefs showSpeedDials];
}

+ (BOOL)showStartPageCustomizeButton {
  return [VivaldiStartPagePrefs showStartPageCustomizeButton];
}

+ (BOOL)showAddButton {
  return [VivaldiStartPagePrefs showAddButton];
}

+ (const VivaldiStartPageStartItemType)getReopenStartPageWithItem {
  return [VivaldiStartPagePrefs getReopenStartPageWithItem];
}

+ (NSString*)getStartPageLastVisitedGroupIdentifier {
  return [VivaldiStartPagePrefs getStartPageLastVisitedGroupIdentifier];
}

+ (NSString*)getWallpaperName {
  return [VivaldiStartPagePrefs getWallpaperName];
}

+ (UIImage*)getPortraitWallpaper {
  return [VivaldiStartPagePrefs getPortraitWallpaper];
}

+ (UIImage*)getLandscapeWallpaper {
  return [VivaldiStartPagePrefs getLandscapeWallpaper];
}

+ (NSString*)getDailyMixLastFetchDate {
  return [VivaldiStartPagePrefs getDailyMixLastFetchDate];
}

+ (UIImage*)getDailyMixWallpaper {
  return [VivaldiStartPagePrefs getDailyMixWallpaper];
}

#pragma mark - Setters

+ (void)setSDSortingMode:(const SpeedDialSortingMode)mode {
  [VivaldiStartPagePrefs setSDSortingMode:mode];
}

+ (void)setSDSortingOrder:(const SpeedDialSortingOrder)order {
  [VivaldiStartPagePrefs setSDSortingOrder:order];
}

+ (void)setStartPageLayoutStyle:(const VivaldiStartPageLayoutStyle)style {
  [VivaldiStartPagePrefs setStartPageLayoutStyle:style];
}

+ (void)setStartPageSpeedDialMaximumColumns:
    (VivaldiStartPageLayoutColumn)columns {
  [VivaldiStartPagePrefs setStartPageSpeedDialMaximumColumns:columns];
}

+ (void)setShowSpeedDials:(BOOL)show {
  [VivaldiStartPagePrefs setShowSpeedDials:show];
}

+ (void)setShowFrequentlyVisitedPages:(BOOL)show {
  [VivaldiStartPagePrefs setShowFrequentlyVisitedPages:show];
}

+ (void)setShowStartPageCustomizeButton:(BOOL)show {
  [VivaldiStartPagePrefs setShowStartPageCustomizeButton:show];
}

+ (void)setShowAddButton:(BOOL)show {
  [VivaldiStartPagePrefs setShowAddButton:show];
}

+ (void)setReopenStartPageWithItem:(const VivaldiStartPageStartItemType)item {
  [VivaldiStartPagePrefs setReopenStartPageWithItem:item];
}

+ (void)setStartPageLastVisitedGroupIdentifier:(NSString*)identifier {
  [VivaldiStartPagePrefs setStartPageLastVisitedGroupIdentifier:identifier];
}

+ (void)setWallpaperName:(NSString*)name {
  [VivaldiStartPagePrefs setWallpaperName:name];
}

+ (void)setPortraitWallpaper:(UIImage*)image {
  [VivaldiStartPagePrefs setPortraitWallpaper:image];
}

+ (void)setLandscapeWallpaper:(UIImage*)image {
  [VivaldiStartPagePrefs setLandscapeWallpaper:image];
}

+ (void)setDailyMixLastFetchDate:(NSString*)date {
  [VivaldiStartPagePrefs setDailyMixLastFetchDate:date];
}

+ (void)setDailyMixWallpaper:(UIImage*)image {
  [VivaldiStartPagePrefs setDailyMixWallpaper:image];
}

+ (NSString*)getDailyMixMetadata {
  return [VivaldiStartPagePrefs getDailyMixMetadata];
}

+ (void)setDailyMixMetadata:(NSString*)jsonString {
  [VivaldiStartPagePrefs setDailyMixMetadata:jsonString];
}

#pragma mark - Other Methods

+ (void)refreshDailyMixWallpaperIfNeeded {
  [VivaldiStartPageDailyMixHelper refreshIfNeeded];
}

+ (void)refreshDailyMixWallpaperForceFetch {
  [VivaldiStartPageDailyMixHelper refreshIfNeededWithForceFetch:YES];
}

@end
