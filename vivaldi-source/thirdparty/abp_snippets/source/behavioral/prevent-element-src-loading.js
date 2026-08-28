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
import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";

const {
  Array,
  addEventListener,
  Error,
  Object,
  Reflect,
  Set,
  WeakSet
} = $(window);
// Global WeakSet to store all matching targets
const matchedElements = new WeakSet();
const activeFilters = new Array(); // Global array to store filter rules
const hitFilters = new Set();
// Global Set to store patched element prototypes and avoid overriding
const patchedPrototypes = new Set();
/**
 * @description Prevent targeted elements(iframe/scripts/img/link)
 * from loading resources without triggering onerror events.
 * @memberof module:snippets/behavioral
 *
 * @param {string} tagName Target element tagName which
 * src/href property resource loading will be silently prevented.
 * Accepts: img/link/iframe/script
 * @param {string} search String or regular expression
 * for matching the URL.
 * @example
 * #$#prevent-element-src-loading 'script' '/search-regex/'
 *
 * @since Adblock Plus X.X.X
 */

export function preventElementSrcLoading(tagName, search) {
  if (!tagName || typeof tagName !== "string") {
    throw new Error(
      "[prevent-element-src-loading snippet]: tagName param must be a string."
    );
  }
  if (!search) {
    throw new Error(
      "[prevent-element-src-loading snippet]: Missing search parameter."
    );
  }
  tagName = $(tagName).toString().toLowerCase();
  if (!$(["script", "img", "iframe", "link"]).includes(tagName)) {
    throw new Error(
      "[prevent-element-src-loading snippet]: tagName parameter is incorrect."
    );
  }
  const srcMockData = {
    // "KCk9Pnt9" decodes to "()=>{}"
    script: "data:text/javascript;base64,KCk9Pnt9",
    // Empty 1x1 image
    img: "data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==", // eslint-disable-line
    // Empty div tag
    iframe: "data:text/html;base64,PGRpdj48L2Rpdj4=",
    // Empty data
    link: "data:text/plain;base64,"
  };

  const constructors = {
    script: window.HTMLScriptElement,
    img: window.HTMLImageElement,
    iframe: window.HTMLIFrameElement,
    link: window.HTMLLinkElement
  };
  const instance = constructors[tagName];

  const sourcePropertyName = tagName === "link" ? "href" : "src";
  const onerrorPropertyName = "onerror";
  const debugLog = getDebugger("[prevent-element-src-loading snippet]");
  const formattedArgsToLog = formatArguments(arguments);
  const filterStr =
    "prevent-element-src-loading " + formattedArgsToLog;
  const {mark, end} = profile("prevent-element-src-loading");
  mark();
  const searchRegex = toRegExp(search);
  activeFilters.push({tagName, searchRegex});
  debugLog("info", `Added filter rule\nFILTER: prevent-element-src-loading ${formattedArgsToLog}`);

  // Runs only during first call for each tagName
  if (!patchedPrototypes.has(tagName)) {
    patchedPrototypes.add(tagName);
    let setAttributeWrapper = (target, thisArg, args) => {
      // Check if arguments are present
      if (!args[0] || !args[1])
        return Reflect.apply(target, thisArg, args);

      const nodeName = thisArg.nodeName.toLowerCase();
      const attrName = args[0].toLowerCase();
      const attrValue = args[1];
      const isMatched = attrName === sourcePropertyName &&
      activeFilters.some(f =>
        nodeName === f.tagName &&
        f.searchRegex.test(attrValue)
      );
      if (!isMatched)
        return Reflect.apply(target, thisArg, args);
      matchedElements.add(thisArg);
      // Forward the URI that corresponds with element's type
      debugLog(
        "success",
        `Replaced setAttribute for ${attrName}: ${attrValue} → ${srcMockData[nodeName]}`);
      if (!hitFilters.has(filterStr)) {
        hitFilters.add(filterStr);
        sendSnippetHitEvent(filterStr);
      }
      return Reflect.apply(target, thisArg, [attrName, srcMockData[nodeName]]);
    };
    const setAttributeHandler = {
      apply: setAttributeWrapper
    };
    instance.prototype.setAttribute =
      new Proxy(instance.prototype.setAttribute, setAttributeHandler);
    debugLog("info", "Wrapped setAttribute function");

    const origSrcDescriptor =
      Object.getOwnPropertyDescriptor(instance.prototype, sourcePropertyName);
    if (!origSrcDescriptor)
      return;
    Object.defineProperty(instance.prototype, sourcePropertyName, {
      enumerable: true,
      configurable: true,
      get() {
        return origSrcDescriptor.get.call(this);
      },
      set(urlValue) {
        const nodeName = this.nodeName.toLowerCase();
        const isMatched = activeFilters.some(f =>
          nodeName === f.tagName &&
          f.searchRegex.test(urlValue)
        );
        if (!isMatched) {
          origSrcDescriptor.set.call(this, urlValue);
          return;
        }

        matchedElements.add(this);
        debugLog("success", `Replaced in src/href setter ${urlValue} → ${srcMockData[nodeName]}`);
        if (!hitFilters.has(filterStr)) {
          hitFilters.add(filterStr);
          sendSnippetHitEvent(filterStr);
        }
        origSrcDescriptor.set.call(this, srcMockData[nodeName]);
      }
    });
    debugLog("info", "Wrapped src/href property setter");
  }

  // Runs only during first snippet function call
  if (activeFilters.length === 1) {
    const origOnerrorDescriptor =
      Object.getOwnPropertyDescriptor(
        HTMLElement.prototype,
        onerrorPropertyName);
    if (!origOnerrorDescriptor)
      return;
    Object.defineProperty(HTMLElement.prototype, onerrorPropertyName, {
      enumerable: true,
      configurable: true,
      get() {
        return origOnerrorDescriptor.get.call(this);
      },
      set(cb) {
        const isMatched = matchedElements.has(this);

        if (!isMatched) {
          origOnerrorDescriptor.set.call(this, cb);
          return;
        }
        debugLog("success", `Replaced in onerror setter ${cb} → () => {}`);
        if (!hitFilters.has(filterStr)) {
          hitFilters.add(filterStr);
          sendSnippetHitEvent(filterStr);
        }
        origOnerrorDescriptor.set.call(this, () => {});
      }
    });
    debugLog("info", "Wrapped onerror property setter");

    const addEventListenerWrapper = (target, thisArg, args) => {
      // Check if arguments are present
      if (!args[0] || !args[1] || !thisArg)
        return Reflect.apply(target, thisArg, args);

      const eventName = args[0];
      const isMatched = typeof thisArg.getAttribute === "function" &&
        matchedElements.has(thisArg) &&
        eventName === "error";

      if (isMatched) {
        debugLog("success", `Replaced error event handler on ${thisArg} with () => {}`);
        if (!hitFilters.has(filterStr)) {
          hitFilters.add(filterStr);
          sendSnippetHitEvent(filterStr);
        }
        return Reflect.apply(target, thisArg, [eventName, () => {}]);
      }
      return Reflect.apply(target, thisArg, args);
    };
    const addEventListenerHandler = {
      apply: addEventListenerWrapper
    };
    EventTarget.prototype.addEventListener =
      new Proxy(
        EventTarget.prototype.addEventListener,
        addEventListenerHandler);
    debugLog("info", "Wrapped addEventListener");

    const preventInlineOnerror = () => {
      addEventListener("error", event => {
        const target = event.target;
        if (!target || !target.nodeName)
          return;
        const url = target.src || target.href;
        const nodeName = target.nodeName.toLowerCase();
        const isMatched = activeFilters.some(f =>
          nodeName === f.tagName && url && f.searchRegex.test(url)
        );
        if (!isMatched)
          return;
        target.onerror = () => {};
      }, true);
      debugLog("info", "Added event listener to defuse global errors");
    };
    preventInlineOnerror();
  }
  end();
}
