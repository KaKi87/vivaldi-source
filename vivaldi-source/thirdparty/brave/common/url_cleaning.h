// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef BRAVE_COMMON_URL_CLEANING_H_
#define BRAVE_COMMON_URL_CLEANING_H_

#include "url/gurl.h"

class Profile;

// Cleans a URL by sequentially applying three independent filters:
//  1. Debounce       – rewrites tracking URL shorteners to their targets
//  2. Query Filter   – strips known tracking query parameters
//  3. URL Sanitizer  – removes additional tracking params from clean-urls rules
//
// Returns the cleaned GURL (may be identical to |url| if no changes apply).
GURL CleanURL(Profile* profile, const GURL& url);

#endif  // BRAVE_COMMON_URL_CLEANING_H_
