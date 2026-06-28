// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#include "brave/common/url_cleaning.h"

#include "base/logging.h"
#include "brave/components/debounce/core/browser/debounce_service.h"
#include "brave/components/debounce/core/browser/debounce_service_factory.h"
#include "brave/components/query_filter/browser/utils.h"
#include "brave/components/url_sanitizer/url_sanitizer_service.h"
#include "brave/components/url_sanitizer/url_sanitizer_service_factory.h"
#include "chrome/browser/profiles/profile.h"

GURL CleanURL(Profile* profile, const GURL& url) {
  if (!profile || !url.SchemeIsHTTPOrHTTPS()) {
    return url;
  }

  GURL final_url = url;

  // Step 1: Apply debounce rules (rewrite tracking shorteners).
  auto* debounce_service =
      debounce::DebounceServiceFactory::GetForBrowserContext(profile);
  if (debounce_service && !debounce_service->Debounce(url, &final_url)) {
    VLOG(1) << "URLCleaning: Unable to apply debounce rules";
  }

  // Step 2: Apply query filter (strip tracking params).
  auto filtered_url = query_filter::ApplyQueryFilter(final_url);
  if (filtered_url.has_value()) {
    final_url = filtered_url.value();
  }

  // Step 3: Sanitize url (clean-urls rules).
  auto* sanitizer =
      url_sanitizer::URLSanitizerServiceFactory::GetForBrowserContext(profile);
  if (sanitizer) {
    final_url = sanitizer->SanitizeURL(final_url);
  }

  return final_url;
}
