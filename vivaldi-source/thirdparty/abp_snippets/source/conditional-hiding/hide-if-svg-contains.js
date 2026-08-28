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

import {$$, $closest, hideElement} from "../utils/dom.js";
import {raceWinner} from "../introspection/race.js";
import {formatArguments, sendSnippetHitEvent, toRegExp}
  from "../utils/general.js";
import {waitUntilEvent} from "../utils/execution.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {fetchContent} from "../utils/execution.js";

let {
  getComputedStyle,
  MutationObserver,
  WeakSet,
  DOMParser,
  Math,
  Map
} = $(window);
const hitFilters = new Set();

/**
 * @description Hides any HTML element or one of its ancestors matching
 * a CSS selector if a child element's background SVG contains a
 * specific, visible text string and/or specific attributes.
 * @memberof module:snippets/conditional-hiding
 *
 * @param {string} search The text that needs to exist in the SVG image
 * for it to be hidden. If the string begins and ends with a slash (`/`),
 * the text in between is treated as a regular expression.
 * @param {string} selector The CSS selector that an HTML element must match
 * for it to be hidden.
 * @param {?string} [searchSelector] The CSS selector that an HTML element
 * containing the SVG with the given text must match. Defaults to
 * the value of the `selector` argument.
 * @param {?Array.<string>} attributes Override values of the default parameters
 * plus the CSS attributes a computed style map of a node should
 * have in order to consider that node hidden.
 * Syntax: <key>:<value>, where <value> can be a string or a regex (if it
 * starts and ends with a `/`).
 * Default parameters that can be Overriden:
 * -position-threshold: 500, max number the text computed X or Y can have
 * to be considered visible.
 *
 * -disable-contained-check: false, Disable text position check.
 *
 * -opacity-alpha-threshold: 0.1, the maximum value the opacity or alpha
 * parameter can have for the text to be considered hidden
 * (Takes precedence over CSS entries passed for opacity and for
 * alpha param in color/fill)
 *
 * -font-size-threshold: 1, the maximum value the font-size
 * can have for the text to be considered hidden
 * (Takes precedence over CSS entry passed for font-size)
 *
 * -wait-until: "", can be used to delay the running of the snippet
 * until the given state is reached.
 * Accepts: loading, interactive, complete, load or any event name
 *
 * @example
 * hide-if-svg-contains Sponsored div.parent div.child => Checks all
 * div.child elements that have a SVG background-image. If the element has a
 * background-image and that image contains the visible word
 * “Sponsored“ div.parent will be hidden.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/1104019458/hide-if-svg-contains} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/conditional-hiding-snippets/hide-if-svg-contains} for external documentation.
 * @since Adblock Plus 4.26.0
 */

export function hideIfSvgContains(
  search,
  selector,
  searchSelector,
  ...attributes
) {
  if (searchSelector == null)
    searchSelector = selector;

  const textSearchRegExp = toRegExp(search);
  const formattedArguments = formatArguments(arguments);
  const hiddenMap = new WeakSet();
  const debugLog = getDebugger("hide-if-svg-contains");
  const {mark, end} = profile("hide-if-svg-contains");
  const defaultOptionalParameters = new Map([
    ["-position-threshold", "500"],
    ["-disable-contained-check", "false"],
    ["-wait-until", ""],
    ["-opacity-alpha-threshold", "0.1"],
    ["-font-size-threshold", "1"]
  ]);
  let entries = $([]);
  for (let attr of attributes) {
    attr = $(attr);
    let markerIndex = attr.indexOf(":");
    if (markerIndex < 0)
      continue;

    let key = attr.slice(0, markerIndex).trim();
    let value = attr.slice(markerIndex + 1).trim();

    if (key && value) {
      if (defaultOptionalParameters.has(key))
        defaultOptionalParameters.set(key, value);
      else
        entries.push([key, value]);
    }
  }
  // CSS Attributes to be checked by default
  let defaultCSSEntries = $([
    ["display", "none"],
    ["visibility", "hidden"],
    ["opacity", "0"],
    ["fill", "none"],
    ["font-size", "0"]
  ]);
  let attributesMap = new Map(defaultCSSEntries.concat(entries));

  // Parse numeric params
  const positionThresh =
    parseFloat(defaultOptionalParameters.get("-position-threshold")) || 0;
  const disableContainedCheck =
    (defaultOptionalParameters.get("-disable-contained-check") === "true");
  const opacityAlphaThreshold =
    parseFloat(defaultOptionalParameters.get("-opacity-alpha-threshold")) || 0; // eslint-disable-line
  const fontSizeThreshold =
    parseFloat(defaultOptionalParameters.get("-font-size-threshold")) || 0; // eslint-disable-line

  // Map to cache Fetch results for faster processing
  const svgFetchCache = new Map();

  const mainLogic = async() => {
    // Create Isolated element that will host the SVGs to evaluate styles
    let host;
    let callback = async() => {
      mark();
      for (const {element, rootParents} of $$(searchSelector, true)) {
        if (!host)
          host = createIsolatedHost();
        if (hiddenMap.has(element))
          continue;
        let isMatchAndVisible = false;
        try {
          const backgroundImage = $(getComputedStyle(element).backgroundImage);
          const urlMatch = backgroundImage.match(/url\("?(.+?)"?\)/);
          if (!urlMatch)
            continue;
          const url = urlMatch[1];
          // Get cached SVG fetching promise if resolved
          let svgFetchPromise = svgFetchCache.get(url);
          if (!svgFetchPromise) {
            svgFetchPromise = fetchContent(url, {as: "text"});
            svgFetchCache.set(url, svgFetchPromise);
          }
          const svgContent = await svgFetchPromise;
          if (hiddenMap.has(element))
            continue;
          // Use DOMParser to parse the string as an SVG/XML document
          const parser = new DOMParser();
          const svgDoc = parser.parseFromString(svgContent, "image/svg+xml");
          if (svgDoc.querySelector("parsererror")) {
            debugLog(
              "warn",
              "Skipping malformed SVG:",
              url,
              "for element:",
              element);
            continue;
          }
          // Clear the host and append the *valid* SVG document
          host.svgContainer.innerHTML = "";
          host.svgContainer.appendChild(svgDoc.documentElement);
          // Extract text elements within SVG
          const textElements =
            host.svgContainer.querySelectorAll("text, tspan");
          for (const textEl of textElements) {
            if (
              isElementVisibleAndTextMatchesInSvg(
                textEl,
                textSearchRegExp
              )) {
              isMatchAndVisible = true;
              debugLog(
                "info",
                "Condition met: Text found visible in SVG of element",
                element);
              break;
            }
          }
        }
        catch (error) {
          debugLog(
            "warn",
            "An error occurred while processing element:",
            element,
            error);
          continue;
        }
        if (isMatchAndVisible && !hiddenMap.has(element)) {
          const closestToHide = $closest($(element), selector, rootParents);
          if (closestToHide) {
            win();
            hideElement(closestToHide);
            hiddenMap.add(element);
            debugLog("success",
                     "Matched: ",
                     closestToHide,
                     "\nFILTER: hide-if-svg-contains",
                     formattedArguments);
            const filter =
              "hide-if-svg-contains " +
              formattedArguments;
            if (!hitFilters.has(filter)) {
              hitFilters.add(filter);
              sendSnippetHitEvent(filter);
            }
          }
        }
      }
      end();
    };

    let mo = new MutationObserver(callback);
    let win = raceWinner(
      "hide-if-svg-contains",
      () => {
        mo.disconnect();
        // Cleanup hosting elements
        host.cleanup();
      }
    );
    mo.observe(document, {childList: true, subtree: true});
    callback();
  };
  const waitUntil = defaultOptionalParameters.get("-wait-until");
  waitUntilEvent(debugLog, mainLogic, waitUntil);

  /**
   * @description Manually checks a text element in a virtual SVG DOM
   * for visibility, considering inline styles, presentation attributes,
   * and custom attributes.
   * @param {Element} element The text element to check(from virtual SVG DOM):
   * @param {RegExp} searchRegExp The regex to test against text content.
   * @param {Object} styles The element computed styles object.
   * @param {Object} bounds The element boundingClientRect.
   * @returns {boolean} True if the element is visible and
   * its text matches, false otherwise.
   * @private
   */
  function isElementVisibleAndTextMatchesInSvg(
    element,
    searchRegExp
  ) {
    if (!searchRegExp.test(element.textContent))
      return false;
    const styles = getComputedStyle(element);
    const elementCoordinates = element.getBoundingClientRect();
    const parentCoordinates = element.ownerSVGElement.getBoundingClientRect();
    const rgbaRegex = /\b(rgba?|hsla?|hwb)\b/;

    for (const [key, value] of attributesMap) {
      const styleValue = styles[key];
      const floatStyleValue = parseFloat(styleValue);
      switch (key) {
        case "color":
        case "fill": {
          if (styleValue && rgbaRegex.test(styleValue) &&
          opacityAlphaThreshold != 0) {
            const alphaStr = styleValue.split(",")[3] ||
            styleValue.split("/")[1];
            const alphaValue = alphaStr ? parseFloat(alphaStr) : 1.0;
            if (!isNaN(alphaValue) && alphaValue <= opacityAlphaThreshold)
              return false;
          }
          else if (styleValue && toRegExp(value).test(styleValue)) {
            return false;
          }
          break;
        }
        case "opacity": {
          if (!isNaN(floatStyleValue) && opacityAlphaThreshold > 0) {
            if (floatStyleValue <= opacityAlphaThreshold)
              return false;
          }
          break;
        }
        case "font-size": {
          if (!isNaN(floatStyleValue) && fontSizeThreshold > 0) {
            if (floatStyleValue <= fontSizeThreshold)
              return false;
          }
          break;
        }
        // Fallback to regex matching for other attributes
        default: {
          if (styleValue && toRegExp(value).test(styleValue))
            return false;
          break;
        }
      }
    }
    if (!disableContainedCheck && positionThresh > 0) {
      const finalX = elementCoordinates.x - parentCoordinates.x;
      const finalY = elementCoordinates.y - parentCoordinates.y;

      if (Math.abs(finalX) > positionThresh ||
      Math.abs(finalY) > positionThresh)
        return false;
    }
    return true;
  }
  /**
   * @description Creates isolated element to host SVGs for evaluating styles
   * @returns {Object} Object contains element and Cleanup function
   * @private
   */
  function createIsolatedHost() {
    debugLog(
      "info",
      "Creating Isolated element to host SVGs"
    );
    const shadowRootHost = document.createElement("div");
    shadowRootHost.style.cssText = "position: absolute; " +
    "top: -9999px; left: -9999px; " +
    "border: none; " +
    "pointer-events: none;";
    document.body.appendChild(shadowRootHost);
    const attachedShadowRoot = shadowRootHost.attachShadow({mode: "closed"});
    attachedShadowRoot.innerHTML = `
      <div id="container" style="{ all: initial; }">
      </div>
    `;
    let svgContainer = attachedShadowRoot.querySelector("#container");
    return {
      svgContainer,
      cleanup: () => shadowRootHost.remove()
    };
  }
}
