// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include "pinned_tab_throttle.h"

#include "app/vivaldi_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/mojom/loader/referrer.mojom.h"
#include "ui/base/page_transition_types.h"
#include "vivaldi/prefs/vivaldi_gen_prefs.h"

using ThrottleCheckResult = content::NavigationThrottle::ThrottleCheckResult;

PinnedTabsThrottle::PinnedTabsThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

PinnedTabsThrottle::~PinnedTabsThrottle() = default;

const char* PinnedTabsThrottle::GetNameForLogging() {
  return "PinnedTabsThrottle";
}

std::optional<bool> GetExtDataRetainDomain(content::WebContents* web_contents) {
  if (!vivaldi::TabExtData::Has(web_contents)) {
    return false;
  }

  vivaldi::TabExtData* ext_data = vivaldi::TabExtData::Get(web_contents);
  if (ext_data->GetGroupId()) {
    // VB-124511 Disable restricted pinned tabs for tab stacks
    return false;
  }
  return ext_data->IsPinnedTabRestricted();
}

bool IsTabPinned(content::WebContents* web_contents) {
  BrowserWindowInterface* const browser =
      GlobalBrowserCollection::GetInstance()->FindBrowserWithTab(web_contents);

  if (!browser) {
    return false;
  }

  TabStripModel* tab_strip = browser->GetTabStripModel();
  int index = tab_strip->GetIndexOfWebContents(web_contents);

  if (index != TabStripModel::kNoTab) {
    return tab_strip->IsTabPinned(index);
  }

  return false;
}

bool PinnedTabsThrottle::IsInternalURL(const GURL& url) {
  return url.SchemeIs(vivaldi::kVivaldiUIScheme) ||
         url.SchemeIs(content::kChromeUIScheme) ||
         url.SchemeIs(extensions::kExtensionScheme);
}

ThrottleCheckResult PinnedTabsThrottle::WillStartRequest() {
  content::NavigationHandle* handle = navigation_handle();

  if (!handle->IsInMainFrame()) {
    return PROCEED;
  }

  content::WebContents* source_contents = handle->GetWebContents();

  if (!source_contents) {
    return PROCEED;
  }

  if (vivaldi::TabExtData::Has(source_contents) &&
      vivaldi::TabExtData::Get(source_contents)
          ->GetFollowerExtId()
          .has_value()) {
    return PROCEED;
  }

  auto retain_tab_ext_data = GetExtDataRetainDomain(source_contents);
  if (retain_tab_ext_data.has_value()) {
    if (!retain_tab_ext_data.value()) {
      return PROCEED;
    }
  } else {
    Profile* profile =
        Profile::FromBrowserContext(source_contents->GetBrowserContext());

    if (!profile) {
      return PROCEED;
    }
    PrefService* prefs = profile->GetPrefs();
    bool prefs_retain_tabs =
        prefs->GetBoolean(vivaldiprefs::kTabsRestrictPinnedTabs);

    if (!prefs_retain_tabs) {
      return PROCEED;
    }
  }

  const GURL& target_url = handle->GetURL();
  bool is_pin = IsTabPinned(handle->GetWebContents());

  if (!is_pin) {
    return PROCEED;
  }

  const GURL& current_url = handle->GetWebContents()->GetLastCommittedURL();
  // New tab
  if (current_url.is_empty()) {
    return PROCEED;
  }

  if (PinnedTabsThrottle::IsInternalURL(current_url)) {
    return PROCEED;
  }

  bool same_domain = net::registry_controlled_domains::SameDomainOrHost(
      current_url, target_url,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (same_domain) {
    return PROCEED;
  }

  content::OpenURLParams params(target_url,
                                content::Referrer(handle->GetReferrer().url,
                                                  handle->GetReferrer().policy),
                                WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                ui::PAGE_TRANSITION_LINK, false);

  source_contents->OpenURL(params, base::NullCallback());
  return CANCEL_AND_IGNORE;
}
