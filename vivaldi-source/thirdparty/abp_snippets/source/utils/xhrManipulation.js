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
import {proxyToStringCalls} from "./toString.js";

let {XMLHttpRequest, WeakMap, Object} = $(window);

let hasXhrBeenProxied = false;

/**
 * Callbacks that will be executed before
 * XMLHttpRequest.send() to modify the request body.
 */
const preSendCallbacks = [];

/**
 * Callbacks that will be executed when
 * XMLHttpRequest.response is read, to modify the response text.
 * Each is invoked as `fn(responseText, reqInfo)` where `reqInfo` is
 * `{url}` — the URL passed to `open()`. Callbacks that only care
 * about the text can ignore the second argument.
 */
const postResponseCallbacks = [];

const xhrData = new WeakMap();

/**
 * @description Proxies XMLHttpRequest: since we have
 * more than 1 snippet manipulating XHR, here we proxy
 * it only once, allowing multiple snippets to add
 * their callbacks without interfering with each other.
 */
const proxyXhr = () => {
  if (hasXhrBeenProxied)
    return;

  /* Override window.XMLHttpRequest only once.
   * Overriding send() also prevents it from becoming
   * read-only, which has been observed on some pages
   * when XMLHttpRequest is imported via $(window).
   */
  const XMLHttpRequestWrapper = class extends XMLHttpRequest {
    open(method, url, ...args) {
      xhrData.set(this, {method, url});
      return super.open(method, url, ...args);
    }
    send(body) {
      let modifiedBody = body;
      if (
        typeof body === "string" &&
        preSendCallbacks.length > 0
      ) {
        for (const fn of preSendCallbacks)
          modifiedBody = fn(modifiedBody);
      }
      return super.send(modifiedBody);
    }
    get response() {
      const innerResponse = super.response;
      if (postResponseCallbacks.length === 0)
        return innerResponse;

      const data = xhrData.get(this);
      if (typeof data === "undefined")
        return innerResponse;

      // Cache: skip re-transformation when the
      // underlying response hasn't changed.
      const responseLength =
        typeof innerResponse === "string" ?
          innerResponse.length : void 0;
      if (
        data.lastResponseLength !== responseLength
      ) {
        data.cachedResponse = void 0;
        data.lastResponseLength = responseLength;
      }

      if (typeof data.cachedResponse !== "undefined")
        return data.cachedResponse;

      if (typeof innerResponse !== "string")
        return (data.cachedResponse = innerResponse);

      let transformed = innerResponse;
      for (const fn of postResponseCallbacks)
        transformed = fn(transformed, {url: data.url});

      return (data.cachedResponse = transformed);
    }
    get responseText() {
      const response = this.response;
      if (typeof response !== "string")
        return super.responseText;

      return response;
    }
  };
  proxyToStringCalls(XMLHttpRequestWrapper, window.XMLHttpRequest);
  proxyToStringCalls(
    XMLHttpRequestWrapper.prototype.open,
    window.XMLHttpRequest.prototype.open
  );
  proxyToStringCalls(
    XMLHttpRequestWrapper.prototype.send,
    window.XMLHttpRequest.prototype.send
  );
  proxyToStringCalls(
    Object.getOwnPropertyDescriptor(
      XMLHttpRequestWrapper.prototype, "response"
    ).get,
    Object.getOwnPropertyDescriptor(
      window.XMLHttpRequest.prototype, "response"
    ).get
  );
  proxyToStringCalls(
    Object.getOwnPropertyDescriptor(
      XMLHttpRequestWrapper.prototype, "responseText"
    ).get,
    Object.getOwnPropertyDescriptor(
      window.XMLHttpRequest.prototype, "responseText"
    ).get
  );

  window.XMLHttpRequest = XMLHttpRequestWrapper;

  hasXhrBeenProxied = true;
};

export const addPreSendCallback = callback => {
  preSendCallbacks.push(callback);
  proxyXhr();
};

export const addPostResponseCallback = callback => {
  postResponseCallbacks.push(callback);
  proxyXhr();
};
