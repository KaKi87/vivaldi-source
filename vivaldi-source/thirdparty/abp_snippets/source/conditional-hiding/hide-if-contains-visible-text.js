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

import {$childNodes,
        hideIfMatches,
        isVisible,
        getTransformMatrix,
        isFontVisible,
        isTextVisible,
        isContained} from "../utils/dom.js";
import {formatArguments, sendSnippetHitEvent, toRegExp}
  from "../utils/general.js";
import {waitUntilEvent} from "../utils/execution.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {raceWinner} from "../introspection/race.js";

let {getComputedStyle, Map, WeakSet, parseFloat, Math} = $(window);
const hitFilters = new Set();

const {ELEMENT_NODE, TEXT_NODE} = Node;

/**
 * @description Hides any HTML element matching a CSS selector
 * if the visible text content of the element contains a given string.
 * @memberof module:snippets/conditional-hiding
 *
 * @param {string} search The string to match to the visible text. Is considered
 *   visible text that isn't hidden by CSS properties or other means.
 *   If the string begins and ends with a slash (`/`), the text in between is
 *   treated as a regular expression.
 * @param {string} selector The CSS selector that an HTML element must match
 *   for it to be hidden.
 * @param {?string} [searchSelector] The CSS selector that an HTML element
 *   containing the given string must match. Defaults to the value of the
 *   `selector` argument.
 * @param {?Array.<string>} [attributes] The CSS attributes a computed style
 *   map of a node should have in order to consider that node hidden.
 *   Syntax: <key>:<value>, where <value> can be a string or a regex (if it
 *   starts and ends with a `/`).
 * @example
 * // eslint-disable-next-line max-len
 * hide-if-contains-visible-text /regex/ li.serp-item
 * 'li.serp-item div.label' 'color:rgb(255, 255, 255)' =>
 * Hides any li.serp-item element which has an 'li.serp-item div.label'
 * element inside its subtree whose visible text content matches
 * the /regex/ regex and which font color is not white.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/69960779/hide-if-contains-visible-text} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/conditional-hiding-snippets/hide-if-contains-visible-text} for external documentation.
 * @since Adblock Plus 3.6
 */
export function hideIfContainsVisibleText(search, selector,
                                          searchSelector = null,
                                          ...attributes) {
  const {mark, end} = profile("hide-if-contains-visible-text");
  const formattedArguments = formatArguments(arguments);
  const debugLog = getDebugger("hide-if-contains-visible-text");
  let entries = $([]);
  const optionalParams = new Map([
    ["-snippet-box-margin", "2"],
    ["-disable-bg-color-check", "false"],
    ["-check-is-contained", "false"],
    ["-pseudo-box-margin", "2"],
    ["-ignore-padding", "false"],
    ["-disable-font-check", "false"],
    ["-wait-until", ""]
  ]);
  let defaultEntries = $([
    ["opacity", "0"],
    ["font-size", "0px"],
    // if color is transparent...
    ["color", "rgba(0, 0, 0, 0)"]
  ]);
  for (let attr of attributes) {
    attr = $(attr);
    let markerIndex = attr.indexOf(":");
    if (markerIndex < 0)
      continue;
    let key = attr.slice(0, markerIndex).trim().toString();
    let value = attr.slice(markerIndex + 1).trim().toString();
    if (key && value) {
      if (optionalParams.has(key))
        optionalParams.set(key, value);
      else
        entries.push([key, value]);
    }
  }
  let attributesMap = new Map(defaultEntries.concat(entries));

  /**
   * @description Check if a pseudo element has visible text via `content`.
   *
   * @param {Element} element The element to check visibility of.
   * @param {string} pseudo The `::before` or `::after` pseudo selector.
   * @param {DOMMatrix} parentMatrix The combined DOMMatrix of the
   * parent elements.
   * @return {string} The pseudo content or an empty string.
   * @private
   */
  function getPseudoContent(element, pseudo, parentMatrix,
                            {bgColorCheck = true,
                             transThresh = 2,
                             fontCheck = true} = {}) {
    let style = getComputedStyle(element, pseudo);

    if (!isVisible(element, style) ||
     !isTextVisible(element,
                    style,
                    attributesMap,
                    {bgColorCheck, fontCheck: false}))
      return "";

    let {content} = $(style);
    if (content && content !== "none") {
      let strings = $([]);

      const domMatrix = getTransformMatrix(element, pseudo);
      const resultMatrix = parentMatrix.multiply(domMatrix);

      const angle = Math.atan2(resultMatrix.b, resultMatrix.a);
      const angleDegrees = angle * (180 / Math.PI);
      const rotated = Math.abs(angleDegrees) > 5;

      if (rotated)
        return "";

      const translated = Math.abs(resultMatrix.e) > transThresh ||
                         Math.abs(resultMatrix.f) > transThresh;
      if (translated)
        return "";

      // remove all strings, in quotes, including escaping chars, putting
      // instead `\x01${string-index}` in place, which is not valid CSS,
      // so that it's safe to parse it back at the end of the operation.
      content = $(content).trim().replace(
        /(["'])(?:(?=(\\?))\2.)*?\1/g,
        value => `\x01${strings.push($(value).slice(1, -1)) - 1}`
      );

      // replace attr(...) with the attribute value or an empty string,
      // ignoring units and fallback values, as these do not work, or have,
      // any meaning in the CSS `content` property value.
      content = content.replace(
        /\s*attr\(\s*([^\s,)]+)[^)]*?\)\s*/g,
        (_, name) => $(element).getAttribute(name) || ""
      );

      // replace back all `\x01${string-index}` values with their corresponding
      // strings, so that the outcome is a real, cleaned up, `content` value.
      const finalText = content.replace(
        /\x01(\d+)/g,
        (_, index) => strings[index]);

      if (fontCheck && finalText && !isFontVisible(element, style, finalText))
        return "";

      return finalText;
    }
    return "";
  }

  /**
   * @description Returns the visible text content from an element and
   * its descendants.
   *
   * @param {Element} element The element whose visible text we want.
   * @param {Element} closest The closest parent to reach while checking
   *   for visibility.
   * @param {?CSSStyleDeclaration} style The computed style of element. If
   *   falsey it will be queried.
   * @param {Element} parentOverflowNode The closest parent with overflow hidden
   * @param {Element} originalElement The original element
   * @param {?Array.<Element>} shadowRootParents Optional list of
   *    shadow root parents.
   * @param {DOMMatrix} domMatrix The combined transformation matrix
   *  of the parent elements.
   * @param {?Object} conf Configuration object
   * @param {?Number} conf.boxMargin The optional parameter that
   *   can be used to specify how much to stretch the bounding box of the
   *   overflow parent in pixels. Used to counter the hiding methods that
   *   involve pushing decoy elements outside an overflow-y:hidden parent
   *   to make them invisible. Default is 2 pixels.
   * @returns {string} The text that is visible.
   * @private
   */
  function getVisibleContent(element,
                             closest,
                             style,
                             parentOverflowNode,
                             originalElement,
                             shadowRootParents,
                             domMatrix,
                             {
                               boxMargin = 2,
                               bgColorCheck,
                               checkIsContained,
                               fontCheck,
                               transThresh
                             } = {}) {
    let checkClosest = !style;
    if (checkClosest)
      style = getComputedStyle(element);

    if (!isVisible(element, style, checkClosest && closest, shadowRootParents))
      return "";

    if (!parentOverflowNode &&
      (
        $(style).getPropertyValue("overflow-x") === "hidden" ||
        $(style).getPropertyValue("overflow-y") === "hidden"
      )
    )
      parentOverflowNode = element;

    if (!domMatrix)
      domMatrix = getTransformMatrix(element);

    else
      domMatrix = domMatrix.multiply(getTransformMatrix(element));

    let text = getPseudoContent(element,
                                ":before",
                                domMatrix,
                                {bgColorCheck,
                                 transThresh,
                                 fontCheck});
    for (let node of $childNodes($(element))) {
      switch ($(node).nodeType) {
        case ELEMENT_NODE:
          text += getVisibleContent(node,
                                    element,
                                    getComputedStyle(node),
                                    parentOverflowNode,
                                    originalElement,
                                    shadowRootParents,
                                    domMatrix,
                                    {
                                      boxMargin,
                                      bgColorCheck,
                                      checkIsContained,
                                      transThresh,
                                      fontCheck
                                    }
          );
          break;
        case TEXT_NODE:
          // If there is a parent with overflow:hidden, it is possible to push
          // elements out of the boundary box of that parent to make them
          // invisible. This clause checks against that. We fallback to the
          // current behaviour if no overflow parent.
          if (parentOverflowNode) {
            if (isContained(element, parentOverflowNode, {
              boxMargin,
              ignorePadding
            }) && isTextVisible(element,
                                style,
                                attributesMap,
                                {bgColorCheck, fontCheck}))
              text += $(node).nodeValue;
          }
          else if (isTextVisible(element,
                                 style,
                                 attributesMap,
                                 {bgColorCheck, fontCheck})) {
            if (checkIsContained && !isContained(element, originalElement, {
              boxMargin,
              ignorePadding
            }))
              continue;
            text += $(node).nodeValue;
          }
          break;
      }
    }
    text += getPseudoContent(element,
                             ":after",
                             domMatrix,
                             {bgColorCheck,
                              transThresh,
                              fontCheck});
    return text;
  }

  const boxMargin = parseFloat(optionalParams.get("-snippet-box-margin")) || 0;
  const bgColorCheck = optionalParams.get("-disable-bg-color-check") !== "true";
  const fontCheck = optionalParams.get("-disable-font-check") !== "true";
  const checkIsContained = optionalParams.get("-check-is-contained") === "true";
  const ignorePadding = optionalParams.get("-ignore-padding") === "true";
  const transThresh = parseFloat(optionalParams.get("-pseudo-box-margin")) || 0;

  let searchRegex = toRegExp(search);
  let seen = new WeakSet();

  const mainLogic = async() => {
    const mo = hideIfMatches(
      (element, closest, rootParents) => {
        mark();
        if (seen.has(element))
          return false;

        seen.add(element);
        let text = getVisibleContent(
          element, closest, null, null, element, rootParents, null, {
            boxMargin,
            bgColorCheck,
            checkIsContained,
            transThresh,
            fontCheck
          }
        );
        let result = searchRegex.test(text);
        if (text.length) {
          if (result) {
            debugLog("success", result, searchRegex, text, "\nFILTER: hide-if-contains-visible-text", formattedArguments); // eslint-disable-line max-len
            const filter =
              "hide-if-contains-visible-text " +
              formattedArguments;
            if (!hitFilters.has(filter)) {
              hitFilters.add(filter);
              sendSnippetHitEvent(filter);
            }
          }
          else {
            debugLog("info", result, searchRegex, text);
          }
        }
        end();
        return result;
      },
      selector,
      searchSelector
    );
    mo.race(raceWinner(
      "hide-if-contains-visible-text",
      () => {
        mo.disconnect();
      }
    ));
  };
  const waitUntil = optionalParams.get("-wait-until");
  waitUntilEvent(debugLog, mainLogic, waitUntil);
}
