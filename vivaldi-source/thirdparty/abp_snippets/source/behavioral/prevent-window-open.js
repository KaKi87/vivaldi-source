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
import {bound} from "proxy-pants/bound";

import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {proxyToStringCalls} from "../utils/toString.js";

// Proxy, Reflect, and Symbol.toStringTag can't be captured via the
// usual $(window) securing (each fails it for a different reason);
// grab them directly from the globals, once, before a page can
// poison them after injection.
const NativeProxy = Proxy;
const {toStringTag} = Symbol;
const {
  defineProperty: reflectDefineProperty,
  deleteProperty: reflectDeleteProperty,
  get: reflectGet,
  getOwnPropertyDescriptor: reflectGetOwnPropertyDescriptor,
  has: reflectHas,
  set: reflectSet
} = bound(Reflect);
const {Array, Error, Map, Object, Set,
       document, parseFloat, setTimeout} = $(window);

// new Array(), not a literal: a literal's prototype is the live,
// poisonable Array.prototype; $(window)'s Array has a frozen,
// module-load-time prototype instead.
const activeFilters = new Array();
const hitFilters = new Set();
function sendHitOnce(filter) {
  if (!hitFilters.has(filter)) {
    hitFilters.add(filter);
    sendSnippetHitEvent(filter);
  }
}

// The fakePopup facade owns these names: get() serves them, and set()/
// defineProperty() reject them so they can never become own props of the
// benign target and contradict the getter (a Proxy invariant hazard).
const facadeNames = new Set([
  "closed", "close", "opener", "frameElement",
  "parent", "top", "self", "window", "globalThis", "frames",
  "location", "document", "history", toStringTag
]);

/**
 * @description Prevents window.open calls whose arguments match a pattern,
 * optionally returning a decoy pop-up handle so the page cannot detect the
 * block by inspecting the returned window.
 * @memberof module:snippets/behavioral
 *
 * @param {?string} pattern Matched against all window.open arguments joined
 * with spaces. `/.../` is a regexp, `/.../i` a case-insensitive regexp,
 * otherwise an escaped literal. A leading `!` inverts the match. Empty or
 * omitted matches every call.
 * @param {?string} delay Milliseconds. If omitted, prevented calls return
 * null; otherwise a decoy pop-up handle is returned and removed after
 * `delay` ms.
 * @param {?string} decoy One of `iframe` (default), `obj`, or `blank`. Empty or
 * omitted is equivalent to `iframe`.
 *
 * @example
 * example.com#$#prevent-window-open /popunder/ 2000 obj
 * => returns a decoy <object> for pop-under calls, removed after 2000 ms
 *
 * @since Adblock Plus X.X.X
 */
export function preventWindowOpen(pattern = "", delay = "", decoy = "iframe") {
  if (decoy === "")
    decoy = "iframe";
  if (decoy !== "iframe" && decoy !== "obj" && decoy !== "blank") {
    throw new Error(
      "[prevent-window-open snippet]: decoy must be iframe, obj or blank."
    );
  }

  let invert = false;
  if ($(pattern).startsWith("!")) {
    invert = true;
    pattern = $(pattern).slice(1);
  }

  activeFilters.push({
    regex: toRegExp(pattern),
    invert,
    hasDelay: delay !== "",
    autoRemoveAfter: parseFloat(delay) || 0,
    decoy,
    formattedArgs: formatArguments(arguments)
  });

  if (activeFilters.length > 1)
    return;

  const debugLog = getDebugger("[prevent-window-open]");
  const {mark, end} = profile("prevent-window-open");

  const openDescriptor = Object.getOwnPropertyDescriptor(window, "open");
  if (!openDescriptor || typeof openDescriptor.value !== "function" ||
      !openDescriptor.configurable) {
    debugLog("warn", "window.open not wrappable, bailing out");
    return;
  }
  const nativeOpen = openDescriptor.value;

  const fakePopup = (autoRemoveAfter = 0, cleanup = () => {}) => {
    let isClosed = false;
    // idempotent: a manual popup.close() and the auto-expire timer both funnel
    // through here, so cleanup (e.g. removing a backing decoy element) runs
    // exactly once regardless of which fires first — also keeps popup.close
    // stable so popup.close === popup.close
    const closePopup = () => {
      if (isClosed)
        return;
      isClosed = true;
      cleanup();
    };
    // setTimeout clamps non-positive delays to ~0, so this also covers a
    // "0"/NaN/negative delay (closed flips / cleanup runs immediately)
    setTimeout(closePopup, autoRemoveAfter);

    // benign, non-navigating location so popup.location.* cannot touch the host
    const fakeLocation = {
      href: "about:blank",
      assign() {}, replace() {}, reload() {},
      toString() {
        return "about:blank";
      }
    };
    // benign document/history so popup.document.* / popup.history.* stay inert
    const fakeDocument = {
      location: fakeLocation,
      defaultView: null, // wired to the proxy below
      cookie: "",
      open() {}, write() {}, writeln() {}, close() {}
    };
    const fakeHistory = {
      length: 0, state: null, scrollRestoration: "auto",
      back() {}, forward() {}, go() {}, pushState() {}, replaceState() {}
    };
    // one stable no-op per property, so popup.focus === popup.focus
    // (but !== popup.blur)
    const noops = new Map();

    // Proxy a benign object we own, not the real window, so
    // isolation is structural: its prototype is a detached,
    // null-proto object, not Window.prototype. Symbol.toStringTag
    // is spoofed in get() below, since `popup instanceof Window`
    // is no longer true (accepted residual).
    const popupTarget = Object.create(Object.create(null));

    const popup = new NativeProxy(popupTarget, {
      get(target, prop, receiver) {
        // honor real own props first — invariant-safe for any (even
        // non-configurable) own prop a caller managed to place on the target
        if (reflectGetOwnPropertyDescriptor(target, prop))
          return reflectGet(target, prop, receiver);
        // facade props (authoritative; set/defineProperty keep them off target)
        if (prop === "closed")
          return isClosed;
        if (prop === "close")
          return closePopup;
        if (prop === "opener")
          return window;
        if (prop === "frameElement")
          return null;
        // popup instanceof Window is false (detached proto); spoof the brand
        // so Object.prototype.toString.call(popup) still reads Window
        if (prop === toStringTag)
          return "Window";
        // a real popup is its own top-level context
        if (prop === "parent" || prop === "top" || prop === "self" ||
            prop === "window" || prop === "globalThis" || prop === "frames")
          return receiver;
        if (prop === "location")
          return fakeLocation;
        if (prop === "document")
          return fakeDocument;
        if (prop === "history")
          return fakeHistory;
        // generic passthrough from the real window (get-side only,
        // not a mutation): executes any page-defined accessor and
        // mirrors page expandos onto popup (tracked residual);
        // guarded because some host getters (e.g. localStorage) throw
        let value;
        try {
          value = reflectGet(window, prop);
        }
        catch (error) {
          return void 0;
        }
        if (typeof value === "function") {
          let noop = noops.get(prop);
          if (!noop) {
            noop = () => {};
            noops.set(prop, noop);
          }
          return noop;
        }
        // never hand back live host objects (storage, navigator, ...)
        if (value !== null && typeof value === "object")
          return void 0;
        return value;
      },
      set(target, prop, value) {
        // swallow navigation writes: forwarding would redirect the host tab
        if (prop === "location" || prop === "opener")
          return true;
        // facade names stay authoritative; ordinary writes land on the target
        if (facadeNames.has(prop))
          return true;
        return reflectSet(target, prop, value);
      },
      defineProperty(target, prop, descriptor) {
        // a facade name must never become an own prop (would contradict get)
        if (facadeNames.has(prop))
          return false;
        return reflectDefineProperty(target, prop, descriptor);
      },
      deleteProperty(target, prop) {
        // only ever touches the benign target; its result honors invariants
        return reflectDeleteProperty(target, prop);
      },
      has(target, prop) {
        // same documented residual as the get-trap passthrough: this
        // leaks page-expando *presence* ("__probe" in popup mirrors
        // window.__probe) independently of the get-trap leak.
        return facadeNames.has(prop) ||
               reflectHas(target, prop) || reflectHas(window, prop);
      },
      // reject: matches a real Window (cannot be reparented/frozen), and keeps
      // the facade traps invariant-safe (a non-extensible target would
      // make has()/get() of non-own facade names violate Proxy invariants)
      setPrototypeOf() {
        return false;
      },
      preventExtensions() {
        return false;
      }
    });
    fakeDocument.defaultView = popup;
    return popup;
  };

  const wrappedOpen = proxy(nativeOpen, function(url) {
    mark();
    // built by index, not Array.from(arguments)/spread: both use the
    // iterator protocol, whose shared %ArrayIteratorPrototype%.next a
    // page could replace to hide arguments from us.
    const callArgs = new Array(arguments.length);
    for (let i = 0; i < arguments.length; i++)
      callArgs[i] = arguments[i];
    const haystack = callArgs.join(" ");
    // indexed, not for...of: activeFilters' own Symbol.iterator is
    // secured, but for...of's .next still comes from the shared,
    // poisonable %ArrayIteratorPrototype%.
    for (let index = 0; index < activeFilters.length; index++) {
      const rule = activeFilters[index];
      if (rule.regex.test(haystack) === rule.invert)
        continue;

      sendHitOnce("prevent-window-open " + rule.formattedArgs);
      debugLog("success", `Prevented window.open(${haystack})`, `\nFILTER: prevent-window-open ${rule.formattedArgs}`);
      end();

      if (!rule.hasDelay)
        return null;

      if (rule.decoy === "blank") {
        callArgs[0] = "about:blank";
        const realPopup = apply(nativeOpen, this, callArgs);
        // capture close synchronously, before returning realPopup to the
        // caller: a same-origin about:blank handle lets the page overwrite
        // .close (a writable WebIDL operation) to defeat the auto-expire or
        // to throw an observable async error when the timer invokes it
        const closePopup = realPopup && realPopup.close;
        if (typeof closePopup === "function") {
          setTimeout(
            () => apply(closePopup, realPopup, []), rule.autoRemoveAfter
          );
        }
        return realPopup;
      }

      const tag = rule.decoy === "obj" ? "object" : "iframe";
      const urlProp = rule.decoy === "obj" ? "data" : "src";
      let decoyElem;
      try {
        decoyElem = $(document).createElement(tag);
        // nullish url would coerce to the string "undefined" and fetch
        // "/undefined"
        decoyElem[urlProp] =
          (url === void 0 || url === null) ? "about:blank" : url;
        // "HTMLElement" hint required: style is on HTMLElement.prototype, not
        // Element.prototype, so the default hint's chain wouldn't secure it
        const {style} = $(decoyElem, "HTMLElement");
        const $style = $(style, "CSSStyleDeclaration");
        $style.setProperty("height", "1px", "important");
        $style.setProperty("position", "fixed", "important");
        $style.setProperty("top", "-1px", "important");
        $style.setProperty("width", "1px", "important");
        const parent = $(document).body || $(document).documentElement;
        $(parent).appendChild(decoyElem);
      }
      catch (error) {
        // construction/append can fail (e.g. a page that removed
        // document.documentElement before this call); don't let that
        // exception distinguish us from the native window.open, and don't
        // let best-effort cleanup itself throw out of this boundary
        if (decoyElem) {
          try {
            $(decoyElem).remove();
          }
          catch (cleanupError) {
            // best-effort cleanup only
          }
        }
        return fakePopup(rule.autoRemoveAfter);
      }

      // decoyElem.contentWindow can't serve as the handle: Window.top
      // is non-configurable per spec. fakePopup is the sole handle
      // source; decoyElem only hosts/fetches the URL, and its removal
      // is fakePopup's cleanup (one lifecycle timer, not two).
      return fakePopup(rule.autoRemoveAfter, () => $(decoyElem).remove());
    }
    debugLog("info", `Allowed window.open(${haystack})`);
    end();
    return apply(nativeOpen, this, arguments);
  });
  proxyToStringCalls(wrappedOpen, nativeOpen);
  Object.defineProperty(
    window, "open", {...openDescriptor, value: wrappedOpen}
  );
  debugLog("info", "Wrapped window.open");
}
