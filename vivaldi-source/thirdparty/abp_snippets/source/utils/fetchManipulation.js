/*!
 * This file is part of eyeo's Anti-Circumvention Snippets module (@eyeo/snippets),
 * Copyright (C) 2006-present eyeo GmbH
 * 
 * @eyeo/snippets is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 * 
 * @eyeo/snippets is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with @eyeo/snippets.  If not, see <http://www.gnu.org/licenses/>.
 */
import $ from "../$.js";
import {apply, proxy} from "proxy-pants/function";
import {proxyToStringCalls} from "./toString.js";

let {fetch} = $(window);

let hasFetchBeenProxied = false;

/**
 * Callbacks that will be executed for strip-fetch-query-parameter
 */
const preFetchCallbacks = [];

/**
 * Callbacks that will be executed for replace-fetch-response.
 * Each is invoked as `fn(response, reqInfo)` where `reqInfo` is
 * `{url}` — the request URL that produced the response. Callbacks
 * that only care about the response can ignore the second argument.
 */
const postFetchCallbacks = [];


/**
 * @description Proxies the fetch API: since we have more than
 * 1 snippet manipulating fetch, here we proxy it only once,
 * allowing multiple snippets to add their callbacks
 * without interfering with each other.
 */
const proxyFetch = () => {
  // override the `window.fetch` only once
  if (!hasFetchBeenProxied) {
    let wrappedFetch = proxy(fetch, (...args) => {
      let [source] = args;
      // Resolve the request URL up-front so postFetch callbacks can
      // tell which endpoint produced the response. `source` is either
      // a URL string or a Request; for a Request we read `.url` (which
      // reflects any page-side override of that getter).
      let requestUrl =
        typeof source === "string" ? source :
          (source && typeof source.url === "string" ? source.url : "");
      if (preFetchCallbacks.length > 0 && typeof source === "string") {
        let url;
        try {
          url = new URL(source);
        }
        catch (e) {
          if (e instanceof TypeError)
            url = new URL(source, $(document).location);
          else
            throw e;
        }
        preFetchCallbacks.forEach(fn => fn(url));
        args[0] = url.href;
        requestUrl = url.href;
      }

      const promise = apply(fetch, self, args).then(origResponse => {
        let transformedResponse = origResponse;
        postFetchCallbacks.forEach(fn => {
          transformedResponse = fn(transformedResponse, {url: requestUrl});
        });
        return transformedResponse;
      });
      return promise;
    });
    proxyToStringCalls(wrappedFetch, window.fetch);
    window.fetch = wrappedFetch;
    hasFetchBeenProxied = true;
  }
};

export const addPreFetchCallback = callback => {
  preFetchCallbacks.push(callback);
  proxyFetch();
};

export const addPostFetchCallback = callback => {
  postFetchCallbacks.push(callback);
  proxyFetch();
};

