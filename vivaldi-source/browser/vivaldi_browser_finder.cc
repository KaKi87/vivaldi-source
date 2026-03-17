// Copyright (c) 2016 Vivaldi. All rights reserved.

#include "browser/vivaldi_browser_finder.h"

#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/content/vivaldi_tab_check.h"
#include "ui/vivaldi_browser_window.h"

using content::WebContents;

namespace vivaldi {

Browser* FindBrowserForEmbedderWebContents(const WebContents* web_contents) {
  if (!web_contents)
    return nullptr;
  VivaldiBrowserWindow* window = FindWindowForEmbedderWebContents(web_contents);
  return window ? window->browser() : nullptr;
}

VivaldiBrowserWindow* FindWindowForEmbedderWebContents(
    const content::WebContents* web_contents) {
  VivaldiBrowserWindow* vivaldi_window = nullptr;
  ForEachCurrentAndNewBrowserWindowInterfaceOrderedByActivation(
      [&](BrowserWindowInterface* browser) {
        VivaldiBrowserWindow* window =
            static_cast<VivaldiBrowserWindow*>(browser->GetWindow());
        if (window->web_contents() == web_contents) {
          vivaldi_window = window;
          return false;  // stop iterating
        }
        return true;  // continue iterating
      });
  return vivaldi_window;
}

Browser* FindBrowserWithTab(const content::WebContents* web_contents) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  Browser* browser = chrome::FindBrowserWithTab(web_contents);

  // NOTE(espen@vivaldi.com): Some elements (e.g., within panels) will not match
  // in the function above. We have to find the window that contains the web
  // content and use that information to look up the browser.
  if (!browser) {
    return FindBrowserWithNonTabContent(web_contents);
  }

  return browser;
#else
  return nullptr;
#endif
}

Browser* FindBrowserWithNonTabContent(
    const content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }
  if (VivaldiTabCheck::IsOwnedByDevTools(
          const_cast<content::WebContents*>(web_contents))) {
    return nullptr;
  }

  Browser* browser = nullptr;
  guest_view::GuestViewBase* gvb = guest_view::GuestViewBase::FromWebContents(
      const_cast<content::WebContents*>(web_contents));
  if (gvb) {
    WebContents* embedder_web_contents = gvb->embedder_web_contents();
    content::BrowserContext* browser_context = gvb->browser_context();
    if (embedder_web_contents && browser_context) {
      browser = FindBrowserForEmbedderWebContents(embedder_web_contents);
    }
  }
  return browser;
}

Browser* FindBrowserByWindowId(int32_t window_id) {
  return chrome::FindBrowserWithID(SessionID::FromSerializedValue(window_id));
}

int GetBrowserCountOfType(Browser::Type type) {
  int count = 0;
  ForEachCurrentBrowserWindowInterfaceOrderedByActivation(
      [&](BrowserWindowInterface* browser) {
        if (browser->GetType() == type) {
          count++;
        }
        return true;
      });
  return count;
}

}  // namespace vivaldi
