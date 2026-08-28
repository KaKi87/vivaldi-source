// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include "follower_tab_throttle.h"

#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/ext_data/tab_ext_data.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

using ThrottleCheckResult = content::NavigationThrottle::ThrottleCheckResult;

FollowerTabThrottle::FollowerTabThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

FollowerTabThrottle::~FollowerTabThrottle() = default;

const char* FollowerTabThrottle::GetNameForLogging() {
  return "FollowerTabThrottle";
}

ThrottleCheckResult FollowerTabThrottle::WillStartRequest() {
  using vivaldi::TabExtData;

  ui::PageTransition transition = navigation_handle()->GetPageTransition();
  if (transition & ui::PAGE_TRANSITION_CLIENT_REDIRECT) {
    return PROCEED;
  }

  if (ui::PageTransitionCoreTypeIs(transition,
                                   ui::PAGE_TRANSITION_FORM_SUBMIT) ||
      ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_TYPED)) {
    return PROCEED;
  }
  content::NavigationHandle* handle = navigation_handle();

  if (!handle->IsRendererInitiated()) {
    return PROCEED;
  }

  content::WebContents* source_contents = handle->GetWebContents();
  if (!source_contents) {
    return PROCEED;
  }

  std::optional<std::string> parent_follower_ext_id =
      TabExtData::Get(source_contents)->GetFollowerExtId();

  // No follower tab
  if (!parent_follower_ext_id) {
    return PROCEED;
  }

  BrowserWindowInterface* const browser =
      GlobalBrowserCollection::GetInstance()->FindBrowserWithTab(
          source_contents);
  if (!browser) {
    return PROCEED;
  }

  TabStripModel* tab_strip = browser->GetTabStripModel();
  int tab_idx = -1;
  for (int i = 0; i < tab_strip->count(); ++i) {
    content::WebContents* tab = tab_strip->GetWebContentsAt(i);
    CHECK(tab);

    if (TabExtData::Get(tab)->GetExtId() == *parent_follower_ext_id) {
      tab_idx = i;
      break;
    }
  }
  if (tab_idx == -1) {
    return PROCEED;
  }

  content::WebContents* target_contents = tab_strip->GetWebContentsAt(tab_idx);

  if (!target_contents) {
    return PROCEED;
  }

  const GURL& target_url = handle->GetURL();
  content::NavigationController::LoadURLParams load_params(target_url);
  load_params.transition_type = ui::PAGE_TRANSITION_LINK;
  load_params.source_site_instance = source_contents->GetSiteInstance();

  target_contents->GetController().LoadURLWithParams(load_params);

  return CANCEL_AND_IGNORE;
}
