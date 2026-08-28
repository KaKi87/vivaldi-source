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

import {addPostFetchCallback} from "../utils/fetchManipulation.js";
import {addPostResponseCallback} from "../utils/xhrManipulation.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {proxyToStringCalls} from "../utils/toString.js";

// Only globals that exist in the Node-based build-time sandbox
// (see bundle/utils.js) are destructured at module-load time.
// Browser-only constructors (HTMLIFrameElement, Request, etc.) are
// destructured inside the install function so the build's
// `getSnippets` evaluation doesn't trip on undefined classes.
//
// `Proxy` and `Reflect` are NOT destructured from $() because the
// `$()` chain tries to build a subclass of every wrapped value via
// `class X extends value{}` — Reflect has no .prototype and Proxy's
// extension trips the same path at build-evaluation time. We
// reference the globals directly for those two.
const {
  Array,
  Date,
  Object,
  Set,
  WeakSet,
  document,
  parseInt,
  window: w
} = $(window);

let installed = false;

// Spoof states.
const S_FIRST = "param_first";
const S_SECOND = "param_second";
const S_PYV = "pyv";
const S_CLIENT_SCREEN = "client_screen";
const S_AD_TYPE = "ad_type";
const S_NONE = "none";

// Opaque request values written into the player request body.
const PARAMS_FIRST = "eAFgAQ";
const PARAMS_SECOND = "8AUB";
const CLIENT_SCREEN_CHANNEL = "CHANNEL";

// Response markers that mean the current state needs to advance.
const ERROR_MARKERS = ["playerErrorMessageRenderer", "UNPLAYABLE"];

// Install a hook (a closure that performs the prototype/global write
// under raw `window`). Logs success / failure but never throws so that
// later hooks still install if one of them blows up.
//
// Each call site uses `Object.defineProperty(target, key, {value,
// writable: true, configurable: true})` rather than plain assignment.
// This is the same composability pattern `json-prune` / `json-override`
// use: explicit writable+configurable lets any snippet that installs
// AFTER us further wrap the slot without first having to test whether
// our descriptor accidentally locked it. Plain assignment would also
// work in the happy case, but it produces a property whose original
// descriptor (writable/configurable) depends on what was there before
// us — defineProperty makes the contract visible.
function installHook(installFn, name, debugLog) {
  try {
    installFn();
  }
  catch (e) {
    debugLog("error", `Failed to install ${name}: ${e}`);
  }
}

/**
 * @description Fixes YouTube video-player start-up buffering and the
 * audio side effects that come with it. Adjusts outgoing player
 * requests and cleans up the matching responses so playback starts
 * promptly and stays in the audio state the user expects, including
 * on in-page navigations between videos.
 *
 * Always excluded: `/shorts/`, `/tv`, `/embed/`.
 *
 * **Supersedes `tmp-yt-disguise`** — do not enable both.
 *
 * **Temporary site-specific snippet.** `tmp-` indicates tied to a
 * single site (YouTube), unmaintained, removable at any time.
 *
 * @memberof module:snippets/behavioral
 *
 * @param {?string} [disabledHooks=""] Space-separated hook numbers to
 *   disable, for debugging. By default all hooks are enabled; each
 *   number listed turns that hook off. Hook numbers:
 *   1 JSON.stringify, 2 JSON.parse, 3 TextEncoder.encode, 4 Request,
 *   5 XMLHttpRequest.send, 6 Promise.then, 7 Node.appendChild,
 *   8 fetch-postFetch, 9 xhr-postResponse, 10 video-unmute. Non-
 *   numeric tokens are ignored. E.g. `"8 9 10"` disables the
 *   response-rebuild and unmute hooks.
 *
 * @param {?string} [lateMuteWindowMs="5000"] Window (ms) after a
 *   video starts during which an unexpected mute is reverted. After
 *   the window, mutes are left alone. Larger = revert for longer;
 *   smaller = hand off to the user sooner. Non-numeric / negative /
 *   unset → default 5000.
 *
 * @param {?string} [userGestureWindowMs="600"] Window (ms) after the
 *   user activates a mute control (clicking `.ytp-mute-button` or the
 *   `m`/`M` key) during which a resulting mute is attributed to the
 *   user and left in place, even inside `lateMuteWindowMs`. Raise it
 *   if genuine user mutes are still being reverted on slower
 *   machines. Non-numeric / negative / unset → default 600.
 *
 * @param {?string} [paths="!homepage !shorts watch"] Space-separated
 *   URL-path allow/deny filter. Each token is a first-path-segment
 *   name; prefix `!` to make it negative. The special token
 *   `homepage` matches the site root (`youtube.com/`), which has no
 *   path segment of its own.
 *   - Only positives (e.g. `"watch playlist"`) — active only on
 *     those paths; everything else is left untouched.
 *   - Only negatives (e.g. `"!embed"`) — active everywhere except
 *     those paths.
 *   - Mixed — negatives always override; positives must match.
 *   - Empty / unset — uses the default `"!homepage !shorts watch"`,
 *     i.e. /watch only. This keeps the response-side cleanup and the
 *     video-element unmute backstop off the homepage/search hover-
 *     preview players, which are muted by design — fighting them
 *     caused a feedback-loop slowdown.
 *   Scopes the response-side cleanup and the video-element unmute
 *   backstop. The outgoing-request spoof is NOT path-gated (only the
 *   always-excluded list applies to it): the /player request must be
 *   spoofed even when issued mid-navigation, before the URL flips to
 *   /watch — otherwise the server throttles and the video buffers.
 *   Quote the value in the filter so it stays a single arg.
 *
 * @example
 * youtube.com#$#tmp-yt-buffering-spoof
 *
 * @example
 * youtube.com#$#tmp-yt-buffering-spoof '6 10'
 *
 * @example
 * youtube.com#$#tmp-yt-buffering-spoof '' 8000 1000
 *
 * @example
 * youtube.com#$#tmp-yt-buffering-spoof '' '' '' 'watch shorts'
 */
export function tmpYtBufferingSpoof(
  disabledHooks, lateMuteWindowMs, userGestureWindowMs, paths
) {
  if (installed)
    return;
  installed = true;

  // Browser-only globals — pulled in here (at call time, in a real
  // browser) rather than at module load so the Node build-time
  // evaluation of the bundle doesn't trip on undefined classes.
  const {
    Document,
    HTMLIFrameElement,
    Response
  } = $(window);

  const debugLog = getDebugger("tmp-yt-buffering-spoof");
  const {mark, end} = profile("tmp-yt-buffering-spoof");
  mark();

  // --- Module state — closed over by every hook below. ---
  let currentState = S_FIRST;
  let lastVideoId = null;
  let mutationCount = 0;
  let responseCount = 0;

  // Capture the natives from raw `window.*` (not the `$()`-secured
  // copies) so we compose with any other snippet that wrapped the
  // same slot before us. See internal-docs for the rationale.
  const nativeStringify = window.JSON.stringify;
  const nativeParse = window.JSON.parse;

  // --- document.visibilityState override. ---
  const origVisibilityDescriptor = Object.getOwnPropertyDescriptor(
    Document.prototype, "visibilityState"
  );

  const forceVisible = () => {
    try {
      Object.defineProperty(document, "visibilityState", {
        get() {
          return "visible";
        },
        configurable: true
      });
    }
    catch (e) {
      // never break the page
    }
  };

  const restoreVisibility = () => {
    try {
      if (origVisibilityDescriptor) {
        Object.defineProperty(
          document, "visibilityState", origVisibilityDescriptor
        );
      }
    }
    catch (e) {
      // never break the page
    }
  };

  // --- Safe nested property access (no optional chaining for
  // FF68 compat). Returns undefined as soon as any link is null. ---
  const dig = function(obj) {
    for (let i = 1; i < arguments.length; i++) {
      if (obj === null || typeof obj === "undefined")
        return void 0;
      obj = obj[arguments[i]];
    }
    return obj;
  };

  // --- User-supplied path allow/deny filter (parsed once at install
  // time, evaluated every hook call). Layered on top of the hard-
  // coded exclusions below. When unset/empty it defaults to
  // restricting the snippet to /watch (and explicitly off the
  // homepage + shorts), which is where the start-up mute occurs and
  // keeps the snippet away from the hover-preview players. ---
  const DEFAULT_PATHS = "!homepage !shorts watch";
  const effectivePaths =
    typeof paths === "string" && paths.replace(/\s+/g, "").length > 0 ?
      paths : DEFAULT_PATHS;
  const pathRules = parsePathRules(effectivePaths);

  // --- Per-hook disable list (debugging). Space-separated hook
  // numbers; each listed number turns that hook off. Empty/unset →
  // all enabled. Non-numeric tokens are ignored. ---
  const disabledHookSet = new Set();
  if (typeof disabledHooks === "string") {
    const tokens = disabledHooks.split(/\s+/);
    for (let i = 0; i < tokens.length; i++) {
      const n = parseInt(tokens[i], 10);
      if (n >= 1)
        disabledHookSet.add(n);
    }
  }
  const hookEnabled = n => !disabledHookSet.has(n);

  // Install a numbered hook only when it is enabled.
  const installHookIf = (n, installFn, name) => {
    if (hookEnabled(n))
      installHook(installFn, name, debugLog);
  };

  // --- Hard exclusion: shorts, tv, embed use different code paths
  // that must never be touched. This is the ONLY gate on the
  // request-mutation hooks (JSON.stringify / TextEncoder / Request /
  // XHR.send): the outgoing /player request must be spoofed even when
  // it is issued mid-navigation, while `location.href` may still read
  // the previous page (e.g. the homepage). Gating those hooks on the
  // path filter caused buffering — the request fired before the URL
  // flipped to /watch, so the spoof was skipped and the server
  // engaged the throttle. ---
  const hardExcluded = () => {
    const href = w.location.href;
    return href.indexOf("/shorts/") !== -1 ||
      href.indexOf("youtube.com/tv") !== -1 ||
      href.indexOf("youtube.com/embed/") !== -1;
  };

  // --- Full exclusion: hard exclusion PLUS the configurable path
  // filter. Used by the response-side cleanup and the video-element
  // unmute backstop, which fire once the URL has settled and which
  // must stay off non-watch surfaces (notably the homepage hover
  // previews). ---
  const isExcluded = () =>
    hardExcluded() || !pathAllowed(w.location.href, pathRules);

  // --- Read the player's current playabilityStatus.status. Used to
  // decide whether to surrender the spoof (login/captcha required). ---
  const getPlayabilityStatus = () => {
    try {
      const player = document.getElementById("movie_player");
      if (!player || typeof player.getPlayerResponse !== "function")
        return null;
      const pr = player.getPlayerResponse();
      return dig(pr, "playabilityStatus", "status");
    }
    catch (e) {
      return null;
    }
  };

  // --- Remove the appInstallData field on every mutation. ---
  const deleteFingerprint = body => {
    if (!body.playbackContext && !body.playerRequest)
      return;
    const configInfo = dig(body, "context", "client", "configInfo");
    if (configInfo && configInfo.appInstallData)
      delete configInfo.appInstallData;
  };

  // --- Track videoId across requests; reset state to FIRST on a
  // new video so each fresh video gets the cheapest spoof first. ---
  const trackVideoId = body => {
    const vid = body.videoId;
    if (typeof vid !== "string" || vid.length === 0)
      return;
    if (lastVideoId !== null && lastVideoId !== vid) {
      debugLog("info",
               `New video ${vid} (was ${lastVideoId}) — ` +
               `reset to ${S_FIRST}`);
      currentState = S_FIRST;
    }
    lastVideoId = vid;
  };

  // --- Advance the state machine on UNPLAYABLE / errors. ---
  const advanceState = reason => {
    let next;
    if (currentState === S_FIRST)
      next = S_SECOND;
    else if (currentState === S_SECOND)
      next = S_PYV;
    else if (currentState === S_PYV)
      next = S_CLIENT_SCREEN;
    else if (currentState === S_CLIENT_SCREEN)
      next = S_AD_TYPE;
    else
      next = S_NONE;
    debugLog("info", `State: ${currentState} → ${next} (${reason})`);
    currentState = next;
  };

  // --- Core mutator. `body` = the InnerTube request body; `pbCtx` =
  // body.playbackContext (or body.playerRequest.playbackContext when
  // the body wraps it inside playerRequest). ---
  const mutateBody = (body, pbCtx) => {
    try {
      if (!body || !pbCtx)
        return;
      trackVideoId(body);

      // If YT requires login or content-check, give up — don't make
      // a recoverable error worse.
      let effective = currentState;
      const status = getPlayabilityStatus();
      if (status === "LOGIN_REQUIRED" ||
          status === "CONTENT_CHECK_REQUIRED")
        effective = S_NONE;

      const csCurrent =
        dig(body, "context", "client", "clientScreen");

      const refreshLact = () => {
        if (pbCtx.contentPlaybackContext) {
          // Template literal, NOT String(): the `String` from $() is
          // a secured class and throws if invoked without `new`.
          pbCtx.contentPlaybackContext.lactMilliseconds =
            `${Date.now()}`;
        }
      };

      if (effective === S_FIRST &&
          csCurrent !== CLIENT_SCREEN_CHANNEL) {
        body.params = PARAMS_FIRST;
        if (body.playerRequest &&
            body.playerRequest.params !== PARAMS_FIRST)
          body.playerRequest.params = PARAMS_FIRST;
        if (body.playbackContext &&
            body.playbackContext.params !== PARAMS_FIRST)
          body.playbackContext.params = PARAMS_FIRST;
        refreshLact();
        forceVisible();
        deleteFingerprint(body);
        mutationCount++;
        return;
      }

      if (effective === S_SECOND &&
          csCurrent !== CLIENT_SCREEN_CHANNEL) {
        if (body.params !== PARAMS_SECOND)
          body.params = PARAMS_SECOND;
        if (body.playerRequest &&
            body.playerRequest.params !== PARAMS_SECOND)
          body.playerRequest.params = PARAMS_SECOND;
        if (body.playbackContext &&
            body.playbackContext.params !== PARAMS_SECOND)
          body.playbackContext.params = PARAMS_SECOND;
        if (!body.playlistId && body.context && body.context.client)
          body.context.client.clientScreen = CLIENT_SCREEN_CHANNEL;
        refreshLact();
        forceVisible();
        deleteFingerprint(body);
        mutationCount++;
        return;
      }

      if (effective === S_PYV &&
          csCurrent !== CLIENT_SCREEN_CHANNEL) {
        const pcParams = pbCtx.params;
        const startsWithSpoofedParams =
          typeof pcParams === "string" &&
          (pcParams.indexOf(PARAMS_FIRST) === 0 ||
           pcParams.indexOf(PARAMS_SECOND) === 0);
        if (startsWithSpoofedParams)
          return;
        pbCtx.adPlaybackContext = {pyv: true};
        refreshLact();
        deleteFingerprint(body);
        mutationCount++;
        return;
      }

      if (effective === S_CLIENT_SCREEN) {
        const clientName =
          dig(body, "context", "client", "clientName");
        if (clientName !== "WEB")
          return;
        body.context.client.clientScreen = CLIENT_SCREEN_CHANNEL;
        refreshLact();
        forceVisible();
        deleteFingerprint(body);
        mutationCount++;
        return;
      }

      if (effective === S_AD_TYPE) {
        pbCtx.adPlaybackContext = {adType: "AD_TYPE_INSTREAM"};
        refreshLact();
        forceVisible();
        deleteFingerprint(body);
        mutationCount++;
        return;
      }

      if (effective === S_NONE) {
        if (pbCtx.adPlaybackContext)
          delete pbCtx.adPlaybackContext;
        restoreVisibility();
      }
    }
    catch (e) {
      // never break the page
    }
  };

  // --- Convenience: apply mutateBody to both possible
  // playbackContext nesting paths in a body. ---
  const applyToBody = body => {
    if (!body || !body.context || !body.context.client)
      return;
    if (body.playbackContext &&
        typeof body.playbackContext.adPlaybackContext === "undefined")
      mutateBody(body, body.playbackContext);
    if (body.playerRequest &&
        body.playerRequest.playbackContext &&
        typeof body.playerRequest.playbackContext.adPlaybackContext ===
          "undefined")
      mutateBody(body, body.playerRequest.playbackContext);
  };

  // ====================================================================
  // Hook #1: JSON.stringify — typical serialization path for the
  // request body before fetch sends it.
  // ====================================================================
  const wrappedStringify = proxy(nativeStringify, function() {
    if (hardExcluded())
      return apply(nativeStringify, this, arguments);
    try {
      const arg = arguments[0];
      if (arg && typeof arg === "object")
        applyToBody(arg);
    }
    catch (e) {
      // never break the page
    }
    return apply(nativeStringify, this, arguments);
  });
  proxyToStringCalls(wrappedStringify, nativeStringify);
  // Write to the RAW window.JSON, not the secured `w.JSON`: the
  // secured proxy's property descriptors are non-writable, so
  // Object.defineProperty / assignment both fail with TypeError.
  // The page reads from raw window.JSON anyway, so the raw write is
  // what's needed to actually intercept anything.
  installHookIf(1, () => {
    Object.defineProperty(window.JSON, "stringify", {
      value: wrappedStringify,
      writable: true,
      configurable: true
    });
  }, "JSON.stringify");

  // ====================================================================
  // Hook #2: JSON.parse — inspect responses; advance state on error
  // markers and apply state-specific response fixes.
  // ====================================================================
  const wrappedParse = proxy(nativeParse, function() {
    if (isExcluded() || currentState === S_NONE)
      return apply(nativeParse, this, arguments);
    let result;
    try {
      result = apply(nativeParse, this, arguments);
    }
    catch (e) {
      // Surface parse failures to the caller untouched.
      return apply(nativeParse, this, arguments);
    }
    try {
      if (!result || typeof result !== "object")
        return result;
      if (!result.responseContext && !result.playabilityStatus)
        return result;
      responseCount++;
      const stringified = nativeStringify(result);
      let hasError = false;
      for (const m of ERROR_MARKERS) {
        if (stringified.indexOf(m) !== -1) {
          hasError = true;
          break;
        }
      }
      const hasContentCheck =
        stringified.indexOf("CONTENT_CHECK_REQUIRED") !== -1;
      if (hasError && !hasContentCheck) {
        advanceState("response had error marker");
        return result;
      }
      // Clean response — apply state-specific side-fixes:
      if (currentState === S_FIRST) {
        const audioConfig =
          dig(result, "playerConfig", "audioConfig");
        if (audioConfig && audioConfig.muteOnStart) {
          const onWatch = w.location.href.indexOf("/watch") !== -1;
          const isMiniplayer =
            dig(result, "playabilityStatus", "miniplayer");
          if (onWatch || (result.cards && !isMiniplayer)) {
            delete audioConfig.muteOnStart;
            const messages = result.messages;
            if (messages && messages[0] && messages[0].youThereRenderer)
              delete messages[0].youThereRenderer;
          }
        }
      }
      if (currentState === S_AD_TYPE) {
        // Keep the playback-rate range usable in this state.
        const gvsc =
          dig(result, "playerConfig", "granularVariableSpeedConfig");
        if (gvsc) {
          gvsc.maximumPlaybackRate = 200;
          gvsc.minimumPlaybackRate = 25;
        }
      }
    }
    catch (e) {
      // never break the page
    }
    return result;
  });
  proxyToStringCalls(wrappedParse, nativeParse);
  installHookIf(2, () => {
    Object.defineProperty(window.JSON, "parse", {
      value: wrappedParse,
      writable: true,
      configurable: true
    });
  }, "JSON.parse");

  // ====================================================================
  // Hook #3: TextEncoder.encode — some YT code paths serialize bodies
  // via TextEncoder rather than JSON.stringify. We parse the input
  // string, mutate, and re-stringify.
  // ====================================================================
  // Capture from raw window (see internal-docs).
  const nativeEncode = window.TextEncoder.prototype.encode;
  const wrappedEncode = proxy(nativeEncode, function() {
    if (hardExcluded())
      return apply(nativeEncode, this, arguments);
    try {
      const text = arguments[0];
      if (typeof text === "string" &&
          (text.indexOf("\"contentPlaybackContext\"") !== -1 ||
           text.indexOf("\"adSignalsInfo\"") !== -1)) {
        const parsed = nativeParse(text);
        if (parsed && parsed.context && parsed.context.client) {
          applyToBody(parsed);
          arguments[0] = nativeStringify(parsed);
        }
      }
    }
    catch (e) {
      // never break the page
    }
    return apply(nativeEncode, this, arguments);
  });
  proxyToStringCalls(wrappedEncode, nativeEncode);
  installHookIf(3, () => {
    Object.defineProperty(window.TextEncoder.prototype, "encode", {
      value: wrappedEncode,
      writable: true,
      configurable: true
    });
  }, "TextEncoder.prototype.encode");

  // ====================================================================
  // Hook #4: Request constructor — YT often builds requests via
  // `new Request(url, {body: "..."})` and feeds the Request to fetch.
  // Intercept construction, mutate init.body before the Request is
  // created.
  // ====================================================================
  // Proxy the RAW Request, not the $()-secured one (see internal-docs).
  const wrappedRequest = new Proxy(window.Request, {
    construct(target, args, newTarget) {
      try {
        if (hardExcluded())
          return Reflect.construct(target, args, newTarget);
        const url = args[0];
        const init = args[1];
        const urlStr =
          typeof url === "string" ? url :
            (url && typeof url.url === "string" ? url.url : "");
        const body = init && init.body;
        if (urlStr.indexOf("youtubei") !== -1 &&
            typeof body === "string" &&
            (body.indexOf("\"contentPlaybackContext\"") !== -1 ||
             body.indexOf("\"adSignalsInfo\"") !== -1)) {
          const parsed = nativeParse(body);
          if (parsed && parsed.context && parsed.context.client) {
            applyToBody(parsed);
            init.body = nativeStringify(parsed);
          }
        }
      }
      catch (e) {
        // never break the page
      }
      return Reflect.construct(target, args, newTarget);
    }
  });
  installHookIf(4, () => {
    Object.defineProperty(window, "Request", {
      value: wrappedRequest,
      writable: true,
      configurable: true
    });
  }, "Request");

  // ====================================================================
  // Hook #5: XMLHttpRequest.send — legacy XHR path. send() takes the
  // body string (or array for some YT calls); rewrite it before
  // forwarding.
  // ====================================================================
  // Capture from raw window (see internal-docs).
  const nativeSend = window.XMLHttpRequest.prototype.send;
  const wrappedSend = proxy(nativeSend, function() {
    if (hardExcluded())
      return apply(nativeSend, this, arguments);
    try {
      const first = arguments[0];
      const wasArray = Array.isArray(first);
      const text = wasArray ? first[0] : first;
      if (typeof text === "string" &&
          (text.indexOf("\"contentPlaybackContext\"") !== -1 ||
           text.indexOf("\"adSignalsInfo\"") !== -1)) {
        const parsed = nativeParse(text);
        if (parsed && parsed.context && parsed.context.client) {
          applyToBody(parsed);
          const newText = nativeStringify(parsed);
          if (wasArray)
            arguments[0][0] = newText;
          else
            arguments[0] = newText;
        }
      }
    }
    catch (e) {
      // never break the page
    }
    return apply(nativeSend, this, arguments);
  });
  proxyToStringCalls(wrappedSend, nativeSend);
  installHookIf(5, () => {
    Object.defineProperty(window.XMLHttpRequest.prototype, "send", {
      value: wrappedSend,
      writable: true,
      configurable: true
    });
  }, "XMLHttpRequest.prototype.send");

  // ====================================================================
  // Hook #6: Promise.then — response-side cleanup on the chained-data
  // plumbing. Two callback handlers (object-level + string-level),
  // selected by matching the callback's source. See internal-docs.
  // ====================================================================
  const jspbResponseHandler = {
    apply(target, thisArg, args) {
      const result = Reflect.apply(target, thisArg, args);
      try {
        if (result && result.responseContext) {
          delete result.adSlots;
          delete result.playerAds;
          const audioConfig = dig(result, "playerConfig", "audioConfig");
          if (audioConfig && audioConfig.muteOnStart) {
            const onWatch = w.location.href.indexOf("/watch") !== -1;
            const isMiniplayer =
              dig(result, "playabilityStatus", "miniplayer");
            if (onWatch || (result.cards && !isMiniplayer)) {
              delete audioConfig.muteOnStart;
              const messages = result.messages;
              if (messages && messages[0] &&
                  messages[0].youThereRenderer)
                delete messages[0].youThereRenderer;
            }
          }
        }
      }
      catch (e) {
        // never break the page
      }
      return result;
    }
  };
  const stringValueHandler = {
    apply(target, thisArg, args) {
      try {
        const arg = args[0];
        if (arg && typeof arg.value === "string" &&
            arg.value.indexOf("playerResponse") !== -1) {
          let s = arg.value;
          const onWatch = w.location.href.indexOf("/watch") !== -1;
          const cardsButNotMiniplayer =
            s.indexOf("cards") !== -1 &&
            s.indexOf("\"miniplayer\"") === -1;
          if ((onWatch || cardsButNotMiniplayer) &&
              s.indexOf("\"muteOnStart\":true") !== -1) {
            s = s.replace(
              "\"muteOnStart\":true", "\"muteOnStart\":false"
            );
            if (s.indexOf("\"youThereRenderer\":") !== -1) {
              s = s.replace(
                "\"youThereRenderer\":", "\"no_youThereRenderer\":"
              );
            }
          }
          s = s.replace(
            /"(adSlots|playerAds)":/g, "\"no_ads\":"
          );
          arg.value = s;
          args[0] = arg;
        }
      }
      catch (e) {
        // never break the page
      }
      return Reflect.apply(target, thisArg, args);
    }
  };
  // Capture from raw window (see internal-docs).
  const nativeThen = window.Promise.prototype.then;
  const wrappedThen = proxy(nativeThen, function() {
    // URL gate — wrapping a matched callback changes its identity, so
    // stay inert on excluded URLs. See internal-docs.
    if (isExcluded())
      return apply(nativeThen, this, arguments);
    try {
      const cb = arguments[0];
      if (typeof cb === "function") {
        const src = cb.toString();
        if (src.indexOf("jspbResponseCtor") !== -1)
          arguments[0] = new Proxy(cb, jspbResponseHandler);
        else if (src.indexOf(".next(") !== -1)
          arguments[0] = new Proxy(cb, stringValueHandler);
      }
    }
    catch (e) {
      // never break the page
    }
    return apply(nativeThen, this, arguments);
  });
  proxyToStringCalls(wrappedThen, nativeThen);
  installHookIf(6, () => {
    Object.defineProperty(window.Promise.prototype, "then", {
      value: wrappedThen,
      writable: true,
      configurable: true
    });
  }, "Promise.prototype.then");

  // ====================================================================
  // Hook #7: Node.appendChild — propagate our wrapped fetch/Request
  // into freshly-appended about:blank iframes. See internal-docs.
  // ====================================================================
  // Capture from raw window (see internal-docs).
  const nativeAppend = window.Node.prototype.appendChild;
  const wrappedAppend = proxy(nativeAppend, function() {
    const result = apply(nativeAppend, this, arguments);
    // Infrastructure (keeps the player from escaping our request
    // hooks via a pristine iframe) — gated like the request hooks.
    if (hardExcluded())
      return result;
    try {
      if (result instanceof HTMLIFrameElement &&
          result.src === "about:blank" &&
          result.contentWindow) {
        result.contentWindow.fetch = w.fetch;
        result.contentWindow.Request = w.Request;
      }
    }
    catch (e) {
      // contentWindow may be cross-origin / inaccessible
    }
    return result;
  });
  proxyToStringCalls(wrappedAppend, nativeAppend);
  installHookIf(7, () => {
    Object.defineProperty(window.Node.prototype, "appendChild", {
      value: wrappedAppend,
      writable: true,
      configurable: true
    });
  }, "Node.prototype.appendChild");

  // ====================================================================
  // Hook #8: fetch postFetch callback — rebuild player JSON responses
  // with the cleanup applied (strip muteOnStart / adSlots / playerAds,
  // re-inject the URL start time). See internal-docs.
  // ====================================================================
  // Substring-matched. `/get_watch` also covers `/youtubei/v1/get_watch`.
  const PLAYER_ENDPOINTS = [
    "/youtubei/v1/player",
    "/get_watch",
    "/get_video_info"
  ];
  let muteCleanupCount = 0;
  let startSecondsInjectCount = 0;
  let honeypotBypassCount = 0;
  addPostFetchCallback((response, reqInfo) => {
    // Disabled, missing request descriptor, or path-excluded (like
    // Hooks #2/#6, so we never touch homepage hover-preview responses).
    if (!hookEnabled(8) || !reqInfo ||
        typeof reqInfo.url !== "string" || isExcluded())
      return response;
    let matched = false;
    for (const ep of PLAYER_ENDPOINTS) {
      if (reqInfo.url.indexOf(ep) !== -1) {
        matched = true;
        break;
      }
    }
    if (!matched)
      return response;
    if (typeof response.url === "string" &&
        response.url.indexOf("data:") === 0) {
      honeypotBypassCount++;
      return response;
    }
    const ctype =
      (response.headers.get("content-type") || "").toLowerCase();
    if (ctype.indexOf("json") === -1)
      return response;
    // Read the URL ONCE per request — if the user races a SPA nav
    // while a /player call is in flight, we'd otherwise re-read at
    // response time and inject the WRONG video's start time.
    const desiredStart = parseStartSecondsFromHref(w.location.href);
    return response.clone().json().then(obj => {
      let touched = false;
      // Handle both top-level player response AND the array form
      // (/get_watch wraps playerResponse inside an array element).
      const targets = [];
      if (obj && obj.playabilityStatus)
        targets.push(obj);
      if (Array.isArray(obj)) {
        for (const entry of obj) {
          if (entry && entry.playerResponse &&
              entry.playerResponse.playabilityStatus)
            targets.push(entry.playerResponse);
        }
      }
      for (const t of targets) {
        const cleaned = cleanPlayerResponse(t);
        const seekInjected = injectStartSeconds(t, desiredStart);
        if (cleaned)
          touched = true;
        if (seekInjected) {
          touched = true;
          startSecondsInjectCount++;
        }
      }
      if (!touched)
        return response;
      muteCleanupCount++;
      const reconstructed = new Response(nativeStringify(obj), {
        status: response.status,
        statusText: response.statusText,
        headers: response.headers
      });
      // Preserve runtime-only properties that `new Response()`
      // resets but YT inspects.
      Object.defineProperties(reconstructed, {
        ok: {value: response.ok},
        redirected: {value: response.redirected},
        type: {value: response.type},
        url: {value: response.url}
      });
      return reconstructed;
    }).catch(() => response);
  });

  // ====================================================================
  // Hook #9: XHR postResponse callback — same mute cleanup as Hook #8
  // but on the XMLHttpRequest path. YT uses both fetch AND XHR for
  // /youtubei/v1/player calls; we have to intercept both or some
  // responses arrive with `muteOnStart: true` intact.
  // ====================================================================
  addPostResponseCallback((responseText, reqInfo) => {
    // Disabled, missing request descriptor, or path-excluded (like
    // Hooks #2/#6) — leave non-watch surfaces alone.
    if (!hookEnabled(9) || !reqInfo ||
        typeof reqInfo.url !== "string" || isExcluded())
      return responseText;
    let matched = false;
    for (const ep of PLAYER_ENDPOINTS) {
      if (reqInfo.url.indexOf(ep) !== -1) {
        matched = true;
        break;
      }
    }
    if (!matched)
      return responseText;
    if (reqInfo.url.indexOf("data:") === 0) {
      honeypotBypassCount++;
      return responseText;
    }
    if (typeof responseText !== "string" ||
        responseText.length === 0)
      return responseText;
    // Heuristic: only parse if the body looks like JSON that might
    // contain a player response. Cheap substring guard so we don't
    // pay the JSON.parse cost on unrelated XHRs.
    if (responseText.indexOf("playerResponse") === -1 &&
        responseText.indexOf("playabilityStatus") === -1)
      return responseText;
    const desiredStart = parseStartSecondsFromHref(w.location.href);
    try {
      const obj = nativeParse(responseText);
      let touched = false;
      const targets = [];
      if (obj && obj.playabilityStatus)
        targets.push(obj);
      if (Array.isArray(obj)) {
        for (const entry of obj) {
          if (entry && entry.playerResponse &&
              entry.playerResponse.playabilityStatus)
            targets.push(entry.playerResponse);
        }
      }
      for (const t of targets) {
        const cleaned = cleanPlayerResponse(t);
        const seekInjected = injectStartSeconds(t, desiredStart);
        if (cleaned)
          touched = true;
        if (seekInjected) {
          touched = true;
          startSecondsInjectCount++;
        }
      }
      if (!touched)
        return responseText;
      muteCleanupCount++;
      return nativeStringify(obj);
    }
    catch (e) {
      return responseText;
    }
  });

  // ====================================================================
  // Hook #10: video-element unmute backstop. Last line of defense for
  // in-page navigations whose payload never crosses the wire hooks
  // above. Two listeners on the YT <video> element:
  //   (a) `playing` — if muted at start, unmute once per videoId
  //       (tracked in `unmutedVideoIds`).
  //   (b) `volumechange` — for `LATE_MUTE_WINDOW_MS` after `playing`,
  //       revert an unexpected mute, unless it was user-initiated
  //       (see the gesture handling below). Re-arms across SPA navs.
  // See internal-docs for the full rationale.
  // ====================================================================
  // Both windows are filter-configurable (see JSDoc). Non-negative
  // int, else fallback. `parseInt` → NaN for junk, and `NaN >= 0` is
  // false, so `>= 0` covers both NaN and negatives.
  const parseWindowMs = (raw, fallback) => {
    if (typeof raw === "undefined" || raw === null)
      return fallback;
    // Template literal, NOT String(): the `String` from $() is a
    // secured class and throws if invoked without `new` (which is
    // why passing the windows as "" used to crash install).
    const n = parseInt(`${raw}`, 10);
    return n >= 0 ? n : fallback;
  };
  const LATE_MUTE_WINDOW_MS = parseWindowMs(lateMuteWindowMs, 5000);
  const USER_GESTURE_WINDOW_MS = parseWindowMs(userGestureWindowMs, 600);
  const unmutedVideoIds = new Set();
  // VideoIds the user deliberately muted. Persists for the whole
  // video (not just the gesture window) so a later mute-state event
  // for the same id is respected; a user unmute clears it. Keyed by
  // videoId so a fresh video is still handled. See internal-docs.
  const userMutedVideoIds = new Set();
  const watchedVideos = new WeakSet();
  let unmuteCount = 0;
  // Stamp of the last user mute-control activation (0 = none yet).
  let lastUserGestureAt = 0;

  // Cap on how many late re-mutes we revert per playback session.
  // Safety net against a unmute↔re-mute feedback loop (e.g. the
  // homepage hover-preview, which YT keeps muted by design and would
  // otherwise drive a per-millisecond volumechange storm).
  const MAX_LATE_REVERTS = 5;

  const currentVideoId = () => {
    try {
      const player = document.getElementById("movie_player");
      const pr =
        player && typeof player.getPlayerResponse === "function" ?
          player.getPlayerResponse() : null;
      return (pr && pr.videoDetails && pr.videoDetails.videoId) || "";
    }
    catch (e) {
      return "";
    }
  };

  // Scope gate for the unmute backstop — the same `paths` filter the
  // request/response hooks use, evaluated at event time so SPA navs
  // are honoured. The default (`!homepage !shorts watch`) keeps it on
  // /watch only, off the hover-preview players (which are muted by
  // design); a custom `paths` reconfigures it. The MAX_LATE_REVERTS
  // cap is the hard guard against a runaway loop regardless.
  const unmuteInScope = () => !isExcluded();

  // Unmute via the player API (`movie_player.unMute()`), not by
  // poking `video.muted`, so the player's own state (and the volume
  // bar) stays consistent. Falls back to the element if the API is
  // absent. See internal-docs.
  const unmuteNow = video => {
    let usedApi = false;
    try {
      const player = document.getElementById("movie_player");
      if (player && typeof player.unMute === "function") {
        player.unMute();
        usedApi = true;
        // If volume is 0 after unmute, nudge it back to audible.
        if (typeof player.getVolume === "function" &&
            typeof player.setVolume === "function" &&
            player.getVolume() === 0)
          player.setVolume(100);
      }
    }
    catch (e) {
      // fall through to the element fallback
    }
    try {
      if (video && video.muted)
        video.muted = false;
    }
    catch (e) {
      // never break the page
    }
    return usedApi;
  };

  const armVideoUnmuteWatcher = () => {
    if (isExcluded())
      return;
    const video =
      document.querySelector("video.html5-main-video") ||
      document.querySelector("video.video-stream");
    if (!video || watchedVideos.has(video))
      return;
    watchedVideos.add(video);
    // Per-element state — closed over by both listeners. `playingAt`
    // stamps the start; `lateReverts` counts how many late re-mutes
    // we've reverted this session (capped, see MAX_LATE_REVERTS).
    let playingAt = 0;
    let lateReverts = 0;
    video.addEventListener("playing", () => {
      try {
        playingAt = Date.now();
        lateReverts = 0;
        if (!unmuteInScope())
          return;
        if (!video.muted)
          return;
        const vid = currentVideoId();
        // The user deliberately muted this video — a seek that re-
        // fires `playing` must not undo their mute.
        if (vid && userMutedVideoIds.has(vid))
          return;
        if (vid && unmutedVideoIds.has(vid))
          return;
        if (vid)
          unmutedVideoIds.add(vid);
        unmuteCount++;
        const usedApi = unmuteNow(video);
        debugLog("info",
                 "[video.playing] muted at first playing for " +
                 `videoId=${vid || "?"} — unmuted (via ` +
                 `${usedApi ? "player.unMute()" : "element"}).`);
      }
      catch (e) {
        // never break the page
      }
    });
    video.addEventListener("volumechange", () => {
      try {
        // Scope gate (configurable via `paths`); never the previews.
        if (!unmuteInScope())
          return;
        const vid = currentVideoId();
        const recentGesture =
          lastUserGestureAt !== 0 &&
          (Date.now() - lastUserGestureAt) < USER_GESTURE_WINDOW_MS;

        if (!video.muted) {
          // Now unmuted. If the user just did this, forget any
          // remembered mute preference so audio protection resumes.
          if (recentGesture && vid)
            userMutedVideoIds.delete(vid);
          return;
        }

        // Muted from here. If the user just activated a mute control,
        // this mute is theirs: remember it for the whole video (so a
        // later seek can't undo it) and respect it now.
        if (recentGesture) {
          if (vid)
            userMutedVideoIds.add(vid);
          debugLog("info",
                   "[video.volumechange] mute within user-gesture " +
                   "window — remembering + respecting user mute " +
                   `(videoId=${vid || "?"}).`);
          return;
        }

        // The user muted this video earlier (outside the gesture
        // window now) — e.g. they muted, then seeked. Keep respecting
        // it.
        if (vid && userMutedVideoIds.has(vid)) {
          debugLog("info",
                   "[video.volumechange] mute on user-muted video " +
                   `— respecting (videoId=${vid}).`);
          return;
        }

        // Otherwise: an unexpected mute within the window — revert,
        // up to MAX_LATE_REVERTS times per session. The cap stops a
        // unmute↔re-mute feedback loop from spinning the CPU.
        if (playingAt === 0)
          return;
        const sincePlaying = Date.now() - playingAt;
        if (sincePlaying >= LATE_MUTE_WINDOW_MS)
          return;
        if (lateReverts >= MAX_LATE_REVERTS)
          return;
        lateReverts++;
        unmuteCount++;
        const usedApi = unmuteNow(video);
        debugLog("info",
                 "[video.volumechange] late mute at " +
                 `+${sincePlaying}ms after playing for videoId=` +
                 `${vid || "?"} — unmuted (via ` +
                 `${usedApi ? "player.unMute()" : "element"}).`);
      }
      catch (e) {
        // never break the page
      }
    });
    debugLog("info",
             "[video-watcher] attached to <video> element " +
             `(late-mute window=${LATE_MUTE_WINDOW_MS}ms).`);
  };
  // Hook #10 activation — gated so a disabled hook installs no
  // observer or listeners at all (the helpers above stay defined but
  // inert).
  if (hookEnabled(10)) {
    // Try immediately, then keep trying until the element exists.
    armVideoUnmuteWatcher();
    const videoWaitObs = new MutationObserver(() => {
      armVideoUnmuteWatcher();
    });
    videoWaitObs.observe(document, {childList: true, subtree: true});
    // Also re-check on each SPA nav, since YT can occasionally
    // recreate the element across page-type boundaries.
    document.addEventListener("yt-navigate-finish", () => {
      armVideoUnmuteWatcher();
    });

    // --- User mute-gesture detection. Document-level, capture phase
    // so we still see the event even if YT stops propagation on the
    // control. Stamps `lastUserGestureAt`, which the volumechange
    // handler consults to avoid fighting a mute the user asked for. ---
    const markUserGesture = () => {
      lastUserGestureAt = Date.now();
    };
    document.addEventListener("click", evt => {
      try {
        const target = evt.target;
        if (target && typeof target.closest === "function" &&
            target.closest(".ytp-mute-button"))
          markUserGesture();
      }
      catch (e) {
        // never break the page
      }
    }, true);
    document.addEventListener("keydown", evt => {
      try {
        const key = evt.key;
        if (key !== "m" && key !== "M")
          return;
        // Ignore the shortcut while the user is typing (search box,
        // comment field, etc.) — YT does the same.
        const active = document.activeElement;
        const tag = active && active.tagName ? active.tagName : "";
        if (tag === "INPUT" || tag === "TEXTAREA" ||
            (active && active.isContentEditable))
          return;
        markUserGesture();
      }
      catch (e) {
        // never break the page
      }
    }, true);
  }

  debugLog("info",
           `Installed. Starting state: ${currentState}. Hooks: ` +
           "JSON.{stringify,parse}, TextEncoder.encode, Request, " +
           "XMLHttpRequest.send, Promise.then, Node.appendChild, " +
           "fetch-postFetch, xhr-postResponse, video-unmute. " +
           `Counters: ${mutationCount} mutations, ` +
           `${responseCount} responses inspected, ` +
           `${muteCleanupCount} response-rewrites, ` +
           `${startSecondsInjectCount} startSeconds-injects, ` +
           `${honeypotBypassCount} honeypot bypasses, ` +
           `${unmuteCount} video-element unmutes. ` +
           `Windows: late-mute=${LATE_MUTE_WINDOW_MS}ms, ` +
           `user-gesture=${USER_GESTURE_WINDOW_MS}ms.` +
           (disabledHookSet.size > 0 ?
             ` Disabled hooks: ${[...disabledHookSet].join(",")}.` :
             "") +
           describePathRules(pathRules));
  end();
}

// Parse a space-separated string of path tokens into allow/deny
// arrays. A leading `!` marks a token as negative (deny). Empty or
// missing input yields empty arrays ("no filtering").
//
//   parsePathRules("watch !shorts")
//     → {allow: ["watch"], deny: ["shorts"]}
//   parsePathRules("!shorts")
//     → {allow: [], deny: ["shorts"]}
//   parsePathRules("")
//     → {allow: [], deny: []}
function parsePathRules(paths) {
  const allow = [];
  const deny = [];
  if (typeof paths !== "string" || paths.length === 0)
    return {allow, deny};
  const tokens = paths.split(/\s+/);
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!t)
      continue;
    if (t.charAt(0) === "!" && t.length > 1)
      deny.push(t.slice(1).toLowerCase());
    else
      allow.push(t.toLowerCase());
  }
  return {allow, deny};
}

// Extract the first path segment of an absolute URL. Returns the
// lowercased segment, or "" if there is none.
//   https://www.youtube.com/watch?v=X → "watch"
//   https://www.youtube.com/shorts/X  → "shorts"
//   https://www.youtube.com/          → ""
// Returns the special token "homepage" when the URL has no first
// path segment (the site root, e.g. youtube.com/), so it can be
// allow/deny-matched by name like any other segment.
function firstPathSegment(href) {
  if (typeof href !== "string" || href.length === 0)
    return "homepage";
  let p = href;
  const q = p.indexOf("?");
  if (q !== -1)
    p = p.slice(0, q);
  const h = p.indexOf("#");
  if (h !== -1)
    p = p.slice(0, h);
  const ss = p.indexOf("://");
  if (ss !== -1)
    p = p.slice(ss + 3);
  const slash = p.indexOf("/");
  if (slash === -1)
    return "homepage";
  const path = p.slice(slash);
  const m = /^\/([^/]+)/.exec(path);
  return m ? m[1].toLowerCase() : "homepage";
}

// Decide whether a URL passes the path rules.
//   - If the first-segment matches any deny rule → false.
//   - Else if there are no allow rules → true.
//   - Else only true if the first-segment matches some allow rule.
function pathAllowed(href, rules) {
  const segment = firstPathSegment(href);
  for (let i = 0; i < rules.deny.length; i++) {
    if (rules.deny[i] === segment)
      return false;
  }
  if (rules.allow.length === 0)
    return true;
  for (let i = 0; i < rules.allow.length; i++) {
    if (rules.allow[i] === segment)
      return true;
  }
  return false;
}

// Render the path rules as a human-readable suffix for the install
// banner. Empty rules → empty string.
function describePathRules(rules) {
  if (rules.allow.length === 0 && rules.deny.length === 0)
    return "";
  const parts = [];
  if (rules.allow.length > 0)
    parts.push("allow=[" + rules.allow.join(",") + "]");
  if (rules.deny.length > 0)
    parts.push("deny=[" + rules.deny.join(",") + "]");
  return " Path filter: " + parts.join(" ") + ".";
}

// Clean a playerResponse-shaped object in place. Returns true if
// anything changed (so the caller knows to rebuild the Response).
function cleanPlayerResponse(pr) {
  if (!pr || typeof pr !== "object")
    return false;
  let touched = false;
  if (pr.adSlots) {
    delete pr.adSlots;
    touched = true;
  }
  if (pr.playerAds) {
    delete pr.playerAds;
    touched = true;
  }
  const audioConfig =
    pr.playerConfig && pr.playerConfig.audioConfig;
  if (audioConfig && audioConfig.muteOnStart) {
    delete audioConfig.muteOnStart;
    touched = true;
  }
  const messages = pr.messages;
  if (messages && messages[0] && messages[0].youThereRenderer) {
    delete messages[0].youThereRenderer;
    touched = true;
  }
  return touched;
}

// Re-assert the URL `&t=…` start time on a playerResponse-shaped
// object (the outgoing request no longer carries it). Returns true
// if the field changed, so the caller knows to rebuild the Response.
// See internal-docs.
function injectStartSeconds(pr, startSeconds) {
  if (!pr || typeof pr !== "object")
    return false;
  if (startSeconds === null || !(startSeconds > 0))
    return false;
  if (!pr.playerConfig)
    pr.playerConfig = {};
  if (!pr.playerConfig.playbackStartConfig)
    pr.playerConfig.playbackStartConfig = {};
  const cfg = pr.playerConfig.playbackStartConfig;
  if (cfg.startSeconds === startSeconds)
    return false;
  cfg.startSeconds = startSeconds;
  return true;
}

// Parse `&t=…` from a YouTube URL into seconds. Supports the formats
// YT accepts on /watch links:
//   &t=5            (pure integer = seconds)
//   &t=5s
//   &t=1m30s
//   &t=1h2m3s
// Returns null if no `t=` is present or the value can't be parsed.
function parseStartSecondsFromHref(href) {
  if (typeof href !== "string" || href.length === 0)
    return null;
  const m = /[?&]t=([^&#]+)/.exec(href);
  if (!m)
    return null;
  let raw = m[1];
  try {
    raw = decodeURIComponent(raw);
  }
  catch (e) {
    // keep raw as-is
  }
  // Bare integer → seconds.
  if (/^\d+$/.test(raw))
    return parseInt(raw, 10);
  // h/m/s combos (any subset, in order).
  const hms = /^(?:(\d+)h)?(?:(\d+)m)?(?:(\d+)s)?$/i.exec(raw);
  if (!hms || (!hms[1] && !hms[2] && !hms[3]))
    return null;
  const h = parseInt(hms[1] || "0", 10);
  const min = parseInt(hms[2] || "0", 10);
  const s = parseInt(hms[3] || "0", 10);
  return h * 3600 + min * 60 + s;
}
