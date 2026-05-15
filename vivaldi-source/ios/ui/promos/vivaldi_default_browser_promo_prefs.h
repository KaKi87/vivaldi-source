// Copyright 2026 Vivaldi Technologies. All rights reserved.

#ifndef IOS_UI_PROMOS_VIVALDI_DEFAULT_BROWSER_PROMO_PREFS_H_
#define IOS_UI_PROMOS_VIVALDI_DEFAULT_BROWSER_PROMO_PREFS_H_

#import <Foundation/Foundation.h>

class PrefRegistrySimple;

@interface VivaldiDefaultBrowserPromoPrefs : NSObject

+ (void)registerLocalStatePrefs:(PrefRegistrySimple*)registry;

@end

#endif  // IOS_UI_PROMOS_VIVALDI_DEFAULT_BROWSER_PROMO_PREFS_H_
