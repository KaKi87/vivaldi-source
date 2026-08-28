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

import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";

const {
  Date,
  MutationObserver,
  Set,
  document,
  parseInt,
  setTimeout,
  window: w
} = $(window);

let installed = false;

// Hard cap on how long the cold arm will keep polling. After this,
// the cold-load MutationObserver disconnects; the nav arm keeps
// listening for `yt-navigate-finish` for the rest of the session.
const COLD_GIVE_UP_MS = 10000;

// Per-nav retry budget. `yt-navigate-finish` may fire before the
// player has its response, so we re-check periodically.
const NAV_RETRY_MAX = 30;
const NAV_RETRY_INTERVAL_MS = 100;

/**
 * @description Forces an immediate `player.loadVideoById(videoId,
 * startSeconds)` whenever the player presents a video we haven't
 * already loaded in this session. Fires on both cold page load AND
 * SPA navigation between `/watch?` URLs.
 *
 * The intent is to make the player re-request media so playback
 * starts promptly instead of trickling in. `loadVideoById` issues a
 * fresh request cycle. See internal-docs for the mechanism.
 *
 * State tracked: `lastFiredVideoId`. The snippet only fires when
 * the player's current `getPlayerResponse().videoDetails.videoId`
 * is non-empty AND differs from `lastFiredVideoId` AND the player
 * is in a pre-playback state. The pre-playback decision uses
 * `getPlayerState()` (YouTube's canonical state enum):
 *
 *   -1 UNSTARTED  → fire (fresh, about to start)
 *    5 CUED       → fire (cued, waiting)
 *    3 BUFFERING  → fire UNLESS `currentTime >= 1` AND
 *                   `loadedFraction >= 0.05`. Both must hold to skip:
 *                   a fresh nav to `/watch?v=…&t=18s` presets
 *                   currentTime before any data loads, so we also
 *                   need the buffer signal to call it mid-playback.
 *    1 PLAYING    → skip
 *    2 PAUSED     → skip
 *    0 ENDED      → skip
 *
 * After firing — or after detecting the player is already in a
 * playing-ish state — `lastFiredVideoId` is set to the new video
 * so the snippet stops re-checking. This means:
 *
 *   - Cold load → fires once (state=-1 or =3 at currentTime=0).
 *   - SPA nav to a different video → fires once for the new video.
 *   - SPA nav back to the same video → does NOT re-fire.
 *   - Player already playing the video when we detect it
 *     (state=1/2/0, or state=3 mid-playback) → does NOT fire.
 *   - Internal player re-init during our own `loadVideoById` →
 *     does NOT re-fire.
 *
 * Every decision logs a snapshot of `getPlayerState()`,
 * `getCurrentTime()`, `getVideoLoadedFraction()`, `getDuration()`,
 * and `getPlayerStateObject().isBuffering` so the gate's behaviour
 * is auditable in the trace.
 *
 * Three arms run in parallel:
 *
 *   - **Cold arm** — a one-shot `MutationObserver` on `document`
 *     that calls `tryFire()` on every DOM mutation until either
 *     readiness fires or `COLD_GIVE_UP_MS` elapses.
 *
 *   - **Nav arm** — listens for the YouTube-specific
 *     `yt-navigate-finish` event on `document`, then retries
 *     `tryFire()` up to `NAV_RETRY_MAX` times at
 *     `NAV_RETRY_INTERVAL_MS` intervals.
 *
 *   - **Error arm** — opt-in via `errorMode` (default `"none"`;
 *     set to `"dom"`, `"player"`, or `"both"` to enable). Attaches
 *     a `MutationObserver` to `movie_player` as soon as the cold/
 *     nav arms have found it, and on every change calls
 *     `tryErrorFire()`. Fires `loadVideoById` when the chosen
 *     signal (DOM `.ytp-error` class, `playabilityStatus.status`,
 *     or both) reports an error state. Independent of `mode` —
 *     even after `mode=first` disables the cold/nav arms, the
 *     error arm keeps recovering. Budget: 1 reload per videoId.
 *
 * Readiness check on each `tryFire()`:
 *   - we're on a `/watch?` URL,
 *   - `<div id="movie_player">` exists,
 *   - `player.loadVideoById` is a function,
 *   - `player.getPlayerResponse().videoDetails.videoId` is a
 *     non-empty string that differs from `lastFiredVideoId`,
 *   - `player.getProgressState().current === 0` (no playback yet).
 *
 * **Temporary site-specific snippet.** `tmp-` indicates tied to a
 * single site (YouTube), unmaintained, removable at any time.
 *
 * @memberof module:snippets/behavioral
 *
 * @param {?string} [mode="first"] When to fire:
 *   - `"first"` (default, alias `"once"`): fire only on the first
 *     video, then disable. Bypasses the cold-load drip without
 *     re-loading on every SPA nav. `tmp-yt-buffering-spoof`
 *     handles the throttle and `muteOnStart` on subsequent
 *     warm-nav videos through its own response-side and
 *     video-element hooks, so a per-nav reload isn't needed.
 *   - `"every"`: fire on every new video — cold load AND every
 *     SPA navigation. Opt in if you want each video routed
 *     through a fresh `/player` call.
 *
 * @param {?string} [errorMode="none"] Reload trigger for the error
 *   arm. Default `"none"` keeps the snippet's pre-error-arm
 *   behavior unchanged — opt in explicitly:
 *   - `"none"` (default) — disable the error arm entirely.
 *   - `"dom"` — watch `movie_player` for the `.ytp-error` class on
 *     itself or any descendant. Tracks the visible UI state so we
 *     only react to errors the user can see.
 *   - `"player"` — read
 *     `getPlayerResponse().playabilityStatus.status` and treat any
 *     value other than `"OK"` / `"OK_LIMITED"` as an error.
 *   - `"both"` — require BOTH signals before firing. Strictest;
 *     useful if `dom` is firing on transient overlays you don't
 *     want to recover from.
 *   Budget: 1 reload per videoId. After firing once for video X,
 *   subsequent errors on X are ignored until SPA navigation
 *   selects a new videoId. The arm respects the `paths` filter
 *   and the always-excluded list (shorts/tv/embed).
 *
 * @param {?string} [delayMs="0"] Optional additional delay (ms)
 *   after readiness is detected, before `loadVideoById` is called.
 *   Default 0 — fire immediately. Increase if the load sequence is
 *   racy on your build.
 *
 * @param {?string} [paths=""] Space-separated URL-path allow/deny
 *   filter, re-checked at fire time so SPA navs are honored. Each
 *   token is a first-path-segment name; prefix `!` to make it
 *   negative.
 *   - Only positives (e.g. `"watch playlist"`) — fires only on
 *     those paths; everything else is denied.
 *   - Only negatives (e.g. `"!shorts !embed"`) — fires everywhere
 *     except those paths.
 *   - Mixed — negatives always override; positives must match.
 *   - Empty / unset — fire on any path (subject to the existing
 *     `/watch?` guard).
 *   Quote the value in the filter so it stays a single arg.
 *
 * @example
 * youtube.com#$#tmp-yt-force-reload
 *
 * @example
 * youtube.com#$#tmp-yt-force-reload every
 *
 * @example
 * youtube.com#$#tmp-yt-force-reload first dom
 *
 * @example
 * youtube.com#$#tmp-yt-force-reload every player 200
 *
 * @example
 * youtube.com#$#tmp-yt-force-reload every none 0 'watch !shorts'
 */
export function tmpYtForceReload(mode, errorMode, delayMs, paths) {
  if (installed)
    return;
  installed = true;

  const debugLog = getDebugger("tmp-yt-force-reload");
  const {mark, end} = profile("tmp-yt-force-reload");
  mark();

  const extraDelay = (() => {
    const raw = typeof delayMs === "string" ? delayMs.toString() : "0";
    const n = parseInt(raw, 10);
    return isNaN(n) || n < 0 ? 0 : n;
  })();

  // `every` → fire on every new video (cold + every SPA nav).
  // Anything else (including unset / empty / typos / `first` /
  // `once`) → default `first`: disable after the first successful
  // fire.
  const normalizedMode = (() => {
    const raw = typeof mode === "string" ? mode.toString() : "";
    const lower = raw.toLowerCase();
    if (lower === "every")
      return "every";
    return "first";
  })();

  // Error-arm trigger selector. Default `none` keeps the snippet's
  // pre-error-arm behavior unchanged so existing filters that
  // don't pass this parameter aren't quietly opted into anything.
  // Opt in with one of:
  //   `dom`    — watch `movie_player` for the `.ytp-error` class
  //   `player` — read playabilityStatus.status
  //   `both`   — require both signals
  // Anything else (typos / empty) → default `none`.
  const normalizedErrorMode = (() => {
    const raw =
      typeof errorMode === "string" ? errorMode.toString() : "";
    const lower = raw.toLowerCase();
    if (lower === "dom" || lower === "player" || lower === "both")
      return lower;
    return "none";
  })();

  // Parse the single `paths` arg as a space-separated list of
  // positive (e.g. `watch`) and negative (e.g. `!shorts`) tokens.
  const pathRules = parsePathRules(paths);

  const installedAt = Date.now();
  let lastFiredVideoId = "";
  let fireCount = 0;
  // In `first` mode this flips true after the first successful fire
  // so subsequent tryFire() invocations short-circuit out. We don't
  // bother detaching the nav listener — the early-return makes it a
  // no-op.
  let disabled = false;
  // Error-arm state. `errorRetried` holds videoIds we've already
  // reloaded in response to an error overlay — per the 1-reload-
  // per-video budget. `errorObserverInstalled` guards against
  // attaching the MutationObserver more than once.
  const errorRetried = new Set();
  let errorObserverInstalled = false;
  let errorFireCount = 0;

  // Evaluate whether the player is in an error state, per the
  // currently configured `normalizedErrorMode`. Returns false fast
  // when the mode is `none`. The DOM check looks at both
  // `movie_player.classList` and any descendant with the
  // `.ytp-error` class — YT applies the class in both spots on
  // different error overlays. The player check reads
  // `getPlayerResponse().playabilityStatus.status` and treats
  // anything other than `OK` / `OK_LIMITED` as an error.
  const checkErrorState = player => {
    if (normalizedErrorMode === "none" || !player)
      return false;
    let domErr = false;
    let playerErr = false;
    if (normalizedErrorMode === "dom" ||
        normalizedErrorMode === "both") {
      try {
        domErr =
          player.classList.contains("ytp-error") ||
          player.querySelector(".ytp-error") !== null;
      }
      catch (e) {
        // never break the page
      }
    }
    if (normalizedErrorMode === "player" ||
        normalizedErrorMode === "both") {
      try {
        const pr = typeof player.getPlayerResponse === "function" ?
          player.getPlayerResponse() : null;
        const status =
          pr && pr.playabilityStatus && pr.playabilityStatus.status;
        playerErr =
          typeof status === "string" &&
          status !== "OK" && status !== "OK_LIMITED";
      }
      catch (e) {
        // never break the page
      }
    }
    if (normalizedErrorMode === "both")
      return domErr && playerErr;
    if (normalizedErrorMode === "player")
      return playerErr;
    return domErr;
  };

  // Error-arm fire path. Independent of `mode` — even when the
  // normal arms have `disabled=true`, errors keep being recovered.
  // Per-videoId budget of 1 reload (tracked in `errorRetried`) so
  // we never loop on a genuinely broken video. Respects the same
  // URL/path gate as `tryFire`.
  const tryErrorFire = () => {
    if (normalizedErrorMode === "none")
      return;
    if (w.location.href.indexOf("/watch?") === -1)
      return;
    if (!pathAllowed(w.location.href, pathRules))
      return;
    const player = document.getElementById("movie_player");
    if (!player || typeof player.loadVideoById !== "function")
      return;
    if (!checkErrorState(player))
      return;
    let pr;
    try {
      pr = typeof player.getPlayerResponse === "function" ?
        player.getPlayerResponse() : null;
    }
    catch (e) {
      pr = null;
    }
    const videoId = pr && pr.videoDetails && pr.videoDetails.videoId;
    if (typeof videoId !== "string" || videoId === "")
      return;
    if (errorRetried.has(videoId)) {
      // Logged at debug level once per transition to error state
      // would be ideal, but cheaply: only log if the snapshot logger
      // hasn't run for this videoId yet. Skipping for simplicity —
      // the initial fire already logged the videoId, and seeing
      // repeated mutations on a known-error video is expected.
      return;
    }
    errorRetried.add(videoId);
    const startSeconds =
      (pr.playerConfig && pr.playerConfig.playbackStartConfig &&
       pr.playerConfig.playbackStartConfig.startSeconds) || 0;
    errorFireCount++;
    const seq = errorFireCount;
    const elapsed = Date.now() - installedAt;
    const playabilityStatus =
      pr && pr.playabilityStatus && pr.playabilityStatus.status;
    debugLog("info",
             `error#${seq} [+${elapsed}ms] Error detected for ` +
             `"${videoId}" (signal=${normalizedErrorMode}, ` +
             `playabilityStatus=${playabilityStatus}). Firing ` +
             `loadVideoById("${videoId}", ${startSeconds}).`);
    try {
      player.loadVideoById(videoId, startSeconds);
    }
    catch (e) {
      debugLog("error", `error#${seq} loadVideoById threw: ${e}`);
    }
  };

  // Lazily attach the error-arm observer to `movie_player` the
  // first time the cold/nav arms find it. Idempotent. The observer
  // scope is narrow: we only need `attributes` (class changes on
  // the player root) and `childList` + `subtree` (error overlay
  // appearing as a descendant). Much cheaper than a document-wide
  // observer for the rest of the session.
  const ensureErrorObserver = player => {
    if (normalizedErrorMode === "none")
      return;
    if (errorObserverInstalled || !player)
      return;
    errorObserverInstalled = true;
    const obs = new MutationObserver(() => {
      tryErrorFire();
    });
    obs.observe(player, {
      attributes: true,
      attributeFilter: ["class"],
      childList: true,
      subtree: true
    });
    debugLog("info",
             "Error arm attached to movie_player " +
             `(signal=${normalizedErrorMode}).`);
    // Initial poke — covers the case where the error overlay is
    // already on screen when we attach (e.g., we missed the
    // mutation that put it there during our cold-arm warm-up).
    tryErrorFire();
  };

  const tryFire = () => {
    // In `first` mode, after the initial fire we want to be inert.
    // Returning `true` here makes the cold-arm observer disconnect,
    // and the nav listener sees a no-op tick.
    if (disabled)
      return true;
    if (w.location.href.indexOf("/watch?") === -1)
      return false;
    // User-supplied path allow/deny filter — re-checked here (not at
    // install time) so SPA navs work. Returning false makes the cold
    // arm keep polling and the nav arm tick again; eventually the URL
    // changes to an allowed path and we proceed, or we time out.
    if (!pathAllowed(w.location.href, pathRules))
      return false;
    const player = document.getElementById("movie_player");
    if (!player || typeof player.loadVideoById !== "function")
      return false;
    // Once we've confirmed a usable player, make sure the error-arm
    // observer is attached for the rest of the session. Independent
    // of whether we actually fire a normal reload below.
    ensureErrorObserver(player);
    let pr;
    try {
      pr = typeof player.getPlayerResponse === "function" ?
        player.getPlayerResponse() : null;
    }
    catch (e) {
      pr = null;
    }
    const videoId = pr && pr.videoDetails && pr.videoDetails.videoId;
    if (typeof videoId !== "string" || videoId === "")
      return false;
    if (videoId === lastFiredVideoId)
      return false;
    // Snapshot the player's state-machine view at the exact moment
    // we're about to decide whether to reload. Logged on every
    // decision (fire and skip) so we can correlate the gate with
    // the actual values YT reports. Reads:
    //   - getPlayerState(): canonical enum
    //       -1 UNSTARTED, 0 ENDED, 1 PLAYING, 2 PAUSED,
    //       3 BUFFERING, 5 CUED
    //   - getCurrentTime(): playback position in seconds
    //   - getVideoLoadedFraction(): 0..1 buffered fraction
    //   - getDuration(): total duration in seconds
    //   - getPlayerStateObject(): {isBuffering, ...} verbose view
    const state = safeCall(player, "getPlayerState");
    const currentTime = safeCall(player, "getCurrentTime");
    const loadedFraction = safeCall(player, "getVideoLoadedFraction");
    const duration = safeCall(player, "getDuration");
    const stateObj = safeCall(player, "getPlayerStateObject");
    const stateSnapshot =
      `state=${state}, current=${currentTime}, ` +
      `loadedFraction=${loadedFraction}, duration=${duration}, ` +
      `isBuffering=${stateObj && stateObj.isBuffering}`;
    // Skip if the player is in a "playing-ish" state. Reload only
    // helps a video that hasn't started playing yet; reloading
    // mid-playback would interrupt healthy playback for no benefit.
    //
    // States that should SKIP reload:
    //   1 PLAYING — actively playing
    //   2 PAUSED  — user paused
    //   0 ENDED   — finished
    // States that should FIRE reload:
    //   -1 UNSTARTED — fresh, about to start
    //    5 CUED      — cued, waiting
    //    3 BUFFERING — could be initial OR mid-playback rebuffer.
    //                  `currentTime` ALONE is not enough: a fresh
    //                  nav to `/watch?v=…&t=18s` pre-sets currentTime
    //                  to 18 BEFORE any chunk arrives, even though
    //                  nothing is loaded. Use `loadedFraction` as the
    //                  authoritative "real playback has started"
    //                  signal — if loadedFraction is ~0, nothing is
    //                  loaded yet so this is fresh, not a rebuffer.
    const MIN_PLAYBACK_LOADED_FRACTION = 0.05;
    const hasMeaningfulBuffer =
      typeof loadedFraction === "number" &&
      loadedFraction >= MIN_PLAYBACK_LOADED_FRACTION;
    let shouldFire;
    let reason;
    if (state === 1 || state === 2 || state === 0) {
      shouldFire = false;
      reason = "already playing/paused/ended";
    }
    else if (state === 3 && typeof currentTime === "number" &&
             currentTime >= 1 && hasMeaningfulBuffer) {
      shouldFire = false;
      reason = "mid-playback buffer";
    }
    else {
      shouldFire = true;
      reason = "fresh / pre-playback";
    }
    if (!shouldFire) {
      lastFiredVideoId = videoId;
      debugLog("info",
               `Skipping reload for "${videoId}": ` +
               `${reason}. ${stateSnapshot}`);
      return true;
    }
    const startSeconds =
      (pr.playerConfig && pr.playerConfig.playbackStartConfig &&
       pr.playerConfig.playbackStartConfig.startSeconds) || 0;
    // Commit lastFiredVideoId before scheduling the call so a
    // re-entrant tick can't race and fire twice for the same id.
    lastFiredVideoId = videoId;
    fireCount++;
    const seq = fireCount;
    const idForLog = videoId;
    const startForLog = startSeconds;
    const fire = () => {
      try {
        const elapsed = Date.now() - installedAt;
        debugLog("info",
                 `#${seq} [+${elapsed}ms] Firing ` +
                 `loadVideoById("${idForLog}", ${startForLog}). ` +
                 `${stateSnapshot}`);
        player.loadVideoById(idForLog, startForLog);
      }
      catch (e) {
        debugLog("error", `#${seq} loadVideoById threw: ${e}`);
      }
    };
    if (extraDelay > 0)
      setTimeout(fire, extraDelay);
    else
      fire();
    if (normalizedMode === "first") {
      disabled = true;
      debugLog("info",
               "first-mode: disabling further reloads after this fire.");
    }
    return true;
  };

  // Cold arm — disconnects on first fire OR after COLD_GIVE_UP_MS.
  const coldArm = () => {
    if (tryFire())
      return;
    let observer = new MutationObserver(() => {
      if (tryFire() && observer) {
        observer.disconnect();
        observer = null;
      }
    });
    observer.observe(document, {childList: true, subtree: true});
    setTimeout(() => {
      if (observer) {
        observer.disconnect();
        observer = null;
      }
    }, COLD_GIVE_UP_MS);
  };

  if (document.readyState === "loading")
    document.addEventListener("DOMContentLoaded", coldArm);
  else
    coldArm();

  // Nav arm — YouTube fires `yt-navigate-finish` on document after
  // every SPA nav. Player may not be immediately ready, so retry-
  // poll briefly.
  document.addEventListener("yt-navigate-finish", () => {
    let attempts = NAV_RETRY_MAX;
    const tick = () => {
      if (tryFire())
        return;
      attempts--;
      if (attempts <= 0)
        return;
      setTimeout(tick, NAV_RETRY_INTERVAL_MS);
    };
    setTimeout(tick, NAV_RETRY_INTERVAL_MS);
  });

  debugLog("info",
           "Installed. Mode=" + normalizedMode + ". " +
           (normalizedMode === "first" ?
             "Fires once on the first video, then disables." :
             "Fires on every new video (cold load + SPA nav).") +
           (extraDelay > 0 ? ` +${extraDelay}ms delay.` : "") +
           (normalizedErrorMode === "none" ?
             " Error arm disabled." :
             ` Error arm via ${normalizedErrorMode} signal ` +
             "(1 reload/video).") +
           describePathRules(pathRules));
  end();
}

// Call `name` on `obj` and return its result. Returns undefined if
// `obj` or method is missing, or if the call throws. Used to take
// non-fatal snapshots of player state methods that may or may not
// be present.
function safeCall(obj, name) {
  if (obj === null || typeof obj === "undefined")
    return void 0;
  const fn = obj[name];
  if (typeof fn !== "function")
    return void 0;
  try {
    return fn.call(obj);
  }
  catch (e) {
    return void 0;
  }
}

// Parse a space-separated string of path tokens into allow/deny
// arrays. A leading `!` marks a token as negative (deny). Empty or
// missing input yields empty arrays (which means "no filtering").
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

// Extract the first path segment from a URL string. For
// `https://www.youtube.com/watch?v=X` → `"watch"`. For
// `https://www.youtube.com/shorts/abc` → `"shorts"`. Empty string
// when the URL has no path segment (e.g. site root).
function firstPathSegment(href) {
  if (typeof href !== "string" || href.length === 0)
    return "";
  let p = href;
  // Strip the query and hash so they don't confuse the segment.
  const q = p.indexOf("?");
  if (q !== -1)
    p = p.slice(0, q);
  const h = p.indexOf("#");
  if (h !== -1)
    p = p.slice(0, h);
  // Drop the scheme + host: everything up to and including the
  // first `/` after `://`.
  const ss = p.indexOf("://");
  if (ss !== -1)
    p = p.slice(ss + 3);
  const slash = p.indexOf("/");
  if (slash === -1)
    return "";
  const path = p.slice(slash);
  const m = /^\/([^/]+)/.exec(path);
  return m ? m[1].toLowerCase() : "";
}

// Decide whether a URL is allowed under the given path rules.
//   - If the URL's first segment matches any deny rule → false.
//   - Else if there are no allow rules → true (default-allow).
//   - Else only true if the segment matches some allow rule.
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
