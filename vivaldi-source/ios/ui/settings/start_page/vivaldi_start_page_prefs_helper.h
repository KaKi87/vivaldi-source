// Copyright (c) 2024 Vivaldi Technologies AS. All rights reserved

#ifndef IOS_UI_SETTINGS_START_PAGE_VIVALDI_START_PAGE_PREFS_HELPER_H_
#define IOS_UI_SETTINGS_START_PAGE_VIVALDI_START_PAGE_PREFS_HELPER_H_

#import <UIKit/UIKit.h>

#import "ios/ui/ntp/vivaldi_speed_dial_sorting_mode.h"
#import "ios/ui/settings/start_page/layout_settings/vivaldi_start_page_layout_column.h"
#import "ios/ui/settings/start_page/layout_settings/vivaldi_start_page_layout_style.h"
#import "ios/ui/settings/start_page/vivaldi_start_page_start_item_type.h"

@interface VivaldiStartPagePrefsHelper : NSObject

#pragma mark - Getters
/// Returns the speed dial sorting mode from prefs.
+ (const SpeedDialSortingMode)getSDSortingMode;
/// Returns the speed dial sorting order from prefs.
+ (const SpeedDialSortingOrder)getSDSortingOrder;
/// Returns the start page layout setting
+ (const VivaldiStartPageLayoutStyle)getStartPageLayoutStyle;
/// Returns the start page speed dial maximum columns
+ (const VivaldiStartPageLayoutColumn)getStartPageSpeedDialMaximumColumns;
/// Returns whether frequently visited pages are visible on the start page.
+ (BOOL)showFrequentlyVisitedPages;
/// Returns whether speed dials are visible on the start page.
+ (BOOL)showSpeedDials;
/// Returns whether start page customize button is visible on the start page.
+ (BOOL)showStartPageCustomizeButton;
/// Returns whether Add (SD/Folder) button is visible on the start page.
+ (BOOL)showAddButton;

/// Returns the option to open start page with.
+ (const VivaldiStartPageStartItemType)getReopenStartPageWithItem;

/// Returns the last visited group identifier. UUID is preferred,
/// falling back to primary id when needed.
+ (NSString*)getStartPageLastVisitedGroupIdentifier;

/// Returns the startup wallpaper
+ (NSString*)getWallpaperName;
/// Retrieves the UIImage from the stored Base64 encoded string
+ (UIImage*)getPortraitWallpaper;
+ (UIImage*)getLandscapeWallpaper;

/// Daily Mix wallpaper (downloaded once per day).
+ (NSString*)getDailyMixLastFetchDate;
+ (UIImage*)getDailyMixWallpaper;
/// Returns the raw JSON metadata string for the current daily mix image.
+ (NSString*)getDailyMixMetadata;

#pragma mark - Setters
/// Sets the speed dial sorting mode to the prefs.
+ (void)setSDSortingMode:(const SpeedDialSortingMode)mode;
/// Sets the speed dial sorting order to the prefs.
+ (void)setSDSortingOrder:(const SpeedDialSortingOrder)order;
/// Sets the start page layout style.
+ (void)setStartPageLayoutStyle:(const VivaldiStartPageLayoutStyle)style;
/// Sets the start page speed dial maximum columns.
+ (void)setStartPageSpeedDialMaximumColumns:
    (VivaldiStartPageLayoutColumn)columns;
/// Sets whether frequently visited pages are visible on the start page.
+ (void)setShowFrequentlyVisitedPages:(BOOL)show;
/// Sets whether speed dials are visible on the start page.
+ (void)setShowSpeedDials:(BOOL)show;
/// Sets whether start page customize button is visible on the start page.
+ (void)setShowStartPageCustomizeButton:(BOOL)show;

/// Sets the option to open start page with.
+ (void)setReopenStartPageWithItem:(const VivaldiStartPageStartItemType)item;

/// Sets the last visited group identifier. UUID is preferred,
/// falling back to primary id when needed.
+ (void)setStartPageLastVisitedGroupIdentifier:(NSString*)identifier;

/// Sets the wallpaper name for starup wallpaper
+ (void)setWallpaperName:(NSString*)name;
/// Stores the UIImage as a Base64 encoded string
+ (void)setPortraitWallpaper:(UIImage*)image;
+ (void)setLandscapeWallpaper:(UIImage*)image;

/// Daily Mix wallpaper (downloaded once per day).
+ (void)setDailyMixLastFetchDate:(NSString*)date;
+ (void)setDailyMixWallpaper:(UIImage*)image;
/// Stores the raw JSON metadata string for the current daily mix image.
+ (void)setDailyMixMetadata:(NSString*)jsonString;

#pragma mark - Other Methods
/// Triggers a refresh if Daily Mix is selected and the cached image is stale.
+ (void)refreshDailyMixWallpaperIfNeeded;
/// Triggers a refresh even when a same-day cached image exists.
+ (void)refreshDailyMixWallpaperForceFetch;
@end

#endif  // IOS_UI_SETTINGS_START_PAGE_VIVALDI_START_PAGE_PREFS_HELPER_H_
