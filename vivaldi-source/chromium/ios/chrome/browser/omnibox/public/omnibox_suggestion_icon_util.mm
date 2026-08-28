// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/omnibox/public/omnibox_suggestion_icon_util.h"

#import "base/notreached.h"
#import "ios/chrome/browser/shared/ui/symbols/symbols.h"

// Vivaldi
#import "app/vivaldi_apptools.h"
#import "ios/ui/ntp/vivaldi_speed_dial_constants.h"
#import "ios/ui/vivaldi_overflow_menu/vivaldi_oveflow_menu_constants.h"
#import "ios/ui/vivaldi_symbols/vivaldi_symbol_names.h"

using vivaldi::IsVivaldiRunning;
// End Vivaldi

namespace {
const CGFloat kSymbolSize = 18;
}  // namespace

UIImage* GetOmniboxSuggestionIcon(OmniboxSuggestionIconType icon_type) {
  Symbol symbol = SymbolGlobe;
  switch (icon_type) {
    case OmniboxSuggestionIconType::kCalculator:
      symbol = SymbolEqual;
      break;
    case OmniboxSuggestionIconType::kDefaultFavicon:
      symbol = SymbolGlobeAmericas;
      break;
    case OmniboxSuggestionIconType::kSearch:
      symbol = SymbolSearch;
      break;
    case OmniboxSuggestionIconType::kSearchHistory:
      symbol = SymbolHistory;
      break;
    case OmniboxSuggestionIconType::kConversion:
      symbol = SymbolSyncEnabled;
      break;
    case OmniboxSuggestionIconType::kDictionary:
      symbol = SymbolBookClosed;
      break;
    case OmniboxSuggestionIconType::kStock:
      symbol = SymbolSort;
      break;
    case OmniboxSuggestionIconType::kSunrise:
      symbol = SymbolSunFill;
      break;
    case OmniboxSuggestionIconType::kWhenIs:
      symbol = SymbolCalendar;
      break;
    case OmniboxSuggestionIconType::kTranslation:
      symbol = SymbolTranslate;
      break;
    case OmniboxSuggestionIconType::kFallbackAnswer:
      symbol = SymbolSearch;
      break;
    case OmniboxSuggestionIconType::kSearchTrend:
      symbol = SymbolUpTrend;
      break;
    case OmniboxSuggestionIconType::kSearchWithSparkle:
      symbol = SymbolMagnifyingglassSpark;
      break;
    case OmniboxSuggestionIconType::kNotesSpark:
      symbol = SymbolLineThreeSpark;
      break;
    case OmniboxSuggestionIconType::kCount:
      NOTREACHED();

      // Vivaldi
    case OmniboxSuggestionIconType::kDirectMatch:
      break;
      // End Vivaldi

  }

  // Vivaldi
  switch (icon_type) {
    case OmniboxSuggestionIconType::kDefaultFavicon:
      return [[UIImage imageNamed:vNTPSDFallbackFavicon]
                imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    case OmniboxSuggestionIconType::kCount:
      return [[UIImage imageNamed:vNTPSDFallbackFavicon]
                imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    case OmniboxSuggestionIconType::kSearch:
    case OmniboxSuggestionIconType::kFallbackAnswer:
      return CustomSymbolWithPointSize(vSearch, kSymbolSize);
    case OmniboxSuggestionIconType::kTranslation:
      return CustomSymbolWithPointSize(vOverflowTranslate, kSymbolSize);
    default:
      break;
  }
  // End Vivaldi

  return SymbolWithPointSize(symbol, kSymbolSize);
}

#if BUILDFLAG(IOS_USE_BRANDED_ASSETS)
UIImage* GetBrandedGoogleIconForOmnibox() {
  return MakeSymbolMonochrome(
      SymbolWithPointSize(SymbolGoogleIcon, kSymbolSize));
}
#endif  // BUILDFLAG(IOS_USE_BRANDED_ASSETS)
