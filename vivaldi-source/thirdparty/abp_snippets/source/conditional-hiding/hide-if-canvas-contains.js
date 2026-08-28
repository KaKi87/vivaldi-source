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
import {proxyToStringCalls} from "../utils/toString.js";

import {$$, hideElement} from "../utils/dom.js";
import {formatArguments, sendSnippetHitEvent, toRegExp}
  from "../utils/general.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";

const {CanvasRenderingContext2D,
       document,
       Map,
       MutationObserver,
       Object,
       requestAnimationFrame,
       Set,
       WeakMap,
       WeakSet} = $(window);

const MAX_BUFFER = 10000;

let canvasRules;
let canvasTextBuffers = new WeakMap();
let matchedCanvases = new WeakSet();
let pendingHideCanvasElements = new Set();
let hideCanvasSeenMap = new WeakSet();
const hitFilters = new Set();

// Data-URL ("data") mode state. Drawing to a canvas does not fire DOM
// mutations and a single draw call only paints part of the final image, so
// instead of matching on each draw we mark the canvas dirty and run one
// toDataURL() check per animation frame (coalescing bursts of draw calls).
let dataModeActive = false;
let dataHooksInstalled = false;
let dataCheckScheduled = false;
let dirtyCanvases = new Set();

function processMatch(canvasElement, rule) {
  matchedCanvases.add(canvasElement);
  canvasTextBuffers.delete(canvasElement);
  const elementToHide = $(canvasElement).closest(rule.selector);
  if (elementToHide && !hideCanvasSeenMap.has(elementToHide)) {
    hideElement(elementToHide);
    hideCanvasSeenMap.add(elementToHide);

    getDebugger("hide-if-canvas-contains")("success", "Matched: ", elementToHide, `\nFILTER: hide-if-canvas-contains ${rule.formattedArguments}`);
    const fStr =
      "hide-if-canvas-contains " +
      rule.formattedArguments;
    if (!hitFilters.has(fStr)) {
      hitFilters.add(fStr);
      sendSnippetHitEvent(fStr);
    }
  }
  else {
    scheduleElementToHide(canvasElement, rule);
  }
}

// Flags a canvas for a data-URL re-check after it was drawn to. No-op unless
// at least one "data" mode rule is registered, and never re-checks a canvas
// that has already matched.
function markDirtyForDataMode(canvas) {
  if (!dataModeActive || !canvas || matchedCanvases.has(canvas))
    return;
  dirtyCanvases.add(canvas);
  if (dataCheckScheduled)
    return;
  dataCheckScheduled = true;
  requestAnimationFrame(runDataCheck);
}

// Reads the data URL of every canvas drawn to since the last frame and tests
// it against the "data" mode rules.
function runDataCheck() {
  dataCheckScheduled = false;
  const canvases = dirtyCanvases;
  dirtyCanvases = new Set();
  const debugLog = getDebugger("hide-if-canvas-contains");
  for (const canvas of canvases) {
    if (matchedCanvases.has(canvas))
      continue;
    // Encode lazily: toDataURL() is a full PNG encode, so we only pay for it
    // once we've confirmed the canvas is under at least one data rule's
    // selector. A drawing canvas that no rule targets (the selector is always
    // the canvas itself or an ancestor, so a null closest means it can't be
    // hidden) is skipped without ever being encoded or re-encoded.
    let dataURL = null;
    let encodeFailed = false;
    for (const [searchRegex, rule] of canvasRules) {
      if (rule.mode !== "data")
        continue;
      if (!$(canvas).closest(rule.selector))
        continue;
      if (encodeFailed)
        continue;
      if (dataURL === null) {
        try {
          // Secured native toDataURL so a tampered page method can't lie
          // about the rendered content. Convert to a primitive before
          // matching.
          dataURL = $(canvas).toDataURL().toString();
        }
        catch (error) {
          // Cross-origin images taint the canvas, making toDataURL() throw a
          // SecurityError. We can't read such canvases, so skip them.
          debugLog("info", "Could not read canvas data URL:", error.message);
          encodeFailed = true;
          continue;
        }
      }
      if (searchRegex.test(dataURL))
        processMatch(canvas, rule);
    }
  }
}

// Proxies the canvas painting methods that aren't already wrapped for text
// mode, so any draw triggers a data-URL re-check. Installed lazily the first
// time a "data" mode rule is registered.
function installDataModeHooks() {
  if (dataHooksInstalled)
    return;
  dataHooksInstalled = true;
  const CanvasProto = CanvasRenderingContext2D.prototype;
  const methods = ["fillRect", "strokeRect", "putImageData", "fill", "stroke"];
  for (const name of methods) {
    const originalFunction = CanvasProto[name];
    if (typeof originalFunction !== "function")
      continue;
    let wrappedFunction = proxy(originalFunction, function(...args) {
      const result = apply(originalFunction, this, args);
      markDirtyForDataMode(this.canvas);
      return result;
    });
    proxyToStringCalls(wrappedFunction, originalFunction);
    // eslint-disable-next-line max-len
    Object.defineProperty(window.CanvasRenderingContext2D.prototype, name, {value: wrappedFunction});
  }
}

/**
 * @description Hides a canvas element, or a parent element specified
 * by the selector, if the canvas contains the specified search term
 * in its `fillText` or `strokeText` calls, or if the image source
 * in a `drawImage` call matches the search term. With `mode="data"` the
 * search term is instead matched against the canvas' rendered data URL
 * (`canvas.toDataURL()`).
 * @memberof module:snippets/conditional-hiding
 *
 * @param {string} search Regex pattern that will be searched for in
 * `fillText` or `strokeText` calls, or matched against the image
 * source (`image.src`) in `drawImage` calls. In `"data"` mode it is matched
 * against the canvas data URL instead.
 * @param {string} [selector="canvas"] - The CSS selector that identifies
 * the element to hide. This can be the canvas element itself or a parent
 * of the canvas. Defaults to the `<canvas>` element if not provided.
 * @param {string} [clearRectBehavior=""] - The snippet keeps a running
 * memory of all text drawn to the canvas, hiding the target element
 * when that accumulated text matches the search pattern. Pages often
 * erase and redraw the canvas (e.g. for animations or changing ad
 * content); `clearRect` is the canvas call that does the erasing.
 * This parameter controls whether the snippet should also forget
 * previously seen text when `clearRect` is called. By default (empty
 * string), text is only forgotten when `clearRect` covers the entire
 * canvas, meaning the page is starting a completely fresh drawing
 * cycle. Use `"always"` to forget text on any `clearRect` call, even
 * partial ones — useful when you only care about what is currently
 * visible on the canvas. Use `"never"` to never forget text, even
 * across full redraws — useful when the ad builds up its content over
 * multiple drawing cycles. **Important:** when multiple filters target
 * the same canvas, their behaviors are aggregated. If *any* active
 * filter uses `"always"`, text is forgotten on every `clearRect` call
 * regardless of what other filters specify. Text is only *never*
 * forgotten when *all* active filters specify `"never"`. Ignored in
 * `"data"` mode, which does not use the text buffer.
 * @param {string} [mode=""] - What the `search` pattern is matched against.
 * The default (empty string) is text mode: the pattern is matched against
 * text drawn via `fillText`/`strokeText` and against `drawImage` image
 * sources. Use `"data"` to match against the canvas' rendered data URL
 * (`canvas.toDataURL()`) instead. In data mode the snippet re-reads the
 * data URL (debounced to once per animation frame) whenever the canvas is
 * drawn to via any painting method (`fillRect`, `strokeRect`, `fillText`,
 * `strokeText`, `drawImage`, `putImageData`, `fill`, `stroke`, `clearRect`),
 * and also checks canvases already present when the filter is applied. Note
 * the data URL is base64-encoded pixel data, not human-readable text, and
 * canvases tainted by cross-origin images cannot be read (`toDataURL()`
 * throws) and are skipped.
 * @example
 * hide-if-canvas-contains /sponsored/ => Hides any canvas element whose
 * text content matches sponsored and has been added with a fillText,
 * strokeText, or drawImage call (matching the image src).
 * @example
 * hide-if-canvas-contains /iVBORw0KGgo/ '#ad' '' data => Hides the closest
 * `#ad` ancestor of any canvas whose rendered data URL matches the pattern.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/290848782/hide-if-canvas-contains} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/conditional-hiding-snippets/hide-if-canvas-contains} for external documentation.
 * @since Adblock Plus 4.7
 */
export function hideIfCanvasContains(
  search, selector = "canvas", clearRectBehavior = "", mode = "") {
  const debugLog = getDebugger("hide-if-canvas-contains");
  const formattedArgsToLog = formatArguments(arguments);
  const {mark, end} = profile("hide-if-canvas-contains");

  if (!search) {
    debugLog("error", "The parameter 'search' is required");
    return;
  }

  if (!canvasRules) {
    mark();
    const CanvasProto = CanvasRenderingContext2D.prototype;
    debugLog("info", "CanvasRenderingContext2D proxied");

    function overrideFunctionInCanvas(functionName){
      const originalFunction = CanvasProto[functionName];

      let wrappedFunction = proxy(originalFunction, function(text, ...args) {
        const canvas = this.canvas;
        // Once a canvas matches, stop accumulating — won't be re-evaluated.
        if (matchedCanvases.has(canvas))
          return apply(originalFunction, this, [text, ...args]);
        const accumulated =
          ((canvasTextBuffers.get(canvas) || "") + text)
            .slice(-MAX_BUFFER);
        canvasTextBuffers.set(canvas, accumulated);
        for (const [searchRegex, rule] of canvasRules) {
          // "data" mode rules match the rendered data URL, not drawn text.
          if (rule.mode === "data")
            continue;
          if (searchRegex.test(accumulated))
            processMatch(canvas, rule);
        }
        const result = apply(originalFunction, this, [text, ...args]);
        markDirtyForDataMode(canvas);
        return result;
      });
      proxyToStringCalls(wrappedFunction, originalFunction);
      // eslint-disable-next-line max-len
      Object.defineProperty(window.CanvasRenderingContext2D.prototype, functionName, {
        value: wrappedFunction
      });
    }

    overrideFunctionInCanvas("fillText");
    overrideFunctionInCanvas("strokeText");

    function overrideClearRect() {
      const originalClearRect = CanvasProto.clearRect;
      let wrappedClearRect = proxy(originalClearRect, function(...args) {
        // Clearing the text buffer on clearRect means previously seen
        // text is forgotten and won't contribute to future matches.
        // Keeping the buffer lets the snippet match text accumulated
        // across multiple drawing cycles separated by clearRect calls.
        let forceAlways = false;
        let forceNever = true;
        for (const {clearRectBehavior: crb} of canvasRules.values()) {
          if (crb === "always")
            forceAlways = true;
          if (crb !== "never")
            forceNever = false;
        }
        // Clear the buffer on every clearRect (forceAlways), or only
        // when clearRect covers the entire canvas (default ""), unless
        // all rules use "never" in which case we skip clearing entirely.
        if (!forceNever) {
          const [x, y, w, h] = args;
          const fullCanvas = x <= 0 && y <= 0 &&
            w >= this.canvas.width && h >= this.canvas.height;
          if (forceAlways || fullCanvas)
            canvasTextBuffers.delete(this.canvas);
        }
        const result = apply(originalClearRect, this, args);
        markDirtyForDataMode(this.canvas);
        return result;
      });
      proxyToStringCalls(wrappedClearRect, originalClearRect);
      // eslint-disable-next-line max-len
      Object.defineProperty(window.CanvasRenderingContext2D.prototype, "clearRect", {
        value: wrappedClearRect
      });
    }

    overrideClearRect();

    function overrideDrawImage() {
      const originalDrawImage = CanvasProto.drawImage;

      let wrappedDrawImage = proxy(originalDrawImage, function(image, ...args) {
        debugLog("info", "drawImage called with arguments:", image, ...args);
        if (image && typeof image.src === "string" && image.src) {
          for (const [searchRegex, rule] of canvasRules) {
            // "data" mode rules match the rendered data URL, not the image src.
            if (rule.mode === "data")
              continue;
            if (searchRegex.test(image.src))
              processMatch(this.canvas, rule);
          }
        }
        const result = apply(originalDrawImage, this, [image, ...args]);
        markDirtyForDataMode(this.canvas);
        return result;
      });
      proxyToStringCalls(wrappedDrawImage, originalDrawImage);
      // eslint-disable-next-line max-len
      Object.defineProperty(window.CanvasRenderingContext2D.prototype, "drawImage", {
        value: wrappedDrawImage
      });
    }

    overrideDrawImage();
    canvasRules = new Map();

    // Initialize a single MutationObserver
    const mo = new MutationObserver(mutationsList => {
      for (let mutation of $(mutationsList)) {
        if (mutation.type === "childList") {
          // Check all pending elements when DOM is mutated
          checkPendingElements();
        }
      }
    });

    // Start observing the document for subtree childList mutations
    mo.observe(document, {childList: true, subtree: true});
    end();
  }

  const searchRegex = toRegExp(search);
  // eslint-disable-next-line max-len
  canvasRules.set(searchRegex, {selector, formattedArguments: formattedArgsToLog, clearRectBehavior, mode});

  if (mode === "data") {
    dataModeActive = true;
    installDataModeHooks();
    // Check canvases that were already drawn before this rule was registered;
    // their draw calls happened before our hooks could mark them dirty.
    for (const canvas of $$("canvas"))
      markDirtyForDataMode(canvas);
  }
}

function scheduleElementToHide(canvasElement, rule) {
  pendingHideCanvasElements.add({canvasElement, rule});
}

function checkPendingElements() {
  pendingHideCanvasElements.forEach(entry => {
    const elementToHide = $(entry.canvasElement).closest(entry.rule.selector);
    if (elementToHide && !hideCanvasSeenMap.has(elementToHide)) {
      hideElement(elementToHide);
      hideCanvasSeenMap.add(elementToHide);
      pendingHideCanvasElements.delete(entry);

      getDebugger("hide-if-canvas-contains")("success", "Matched: ", elementToHide, `\nFILTER: hide-if-canvas-contains ${entry.rule.formattedArguments}`);
      const fStr =
        "hide-if-canvas-contains " +
        entry.rule.formattedArguments;
      if (!hitFilters.has(fStr)) {
        hitFilters.add(fStr);
        sendSnippetHitEvent(fStr);
      }
    }
  });
}
