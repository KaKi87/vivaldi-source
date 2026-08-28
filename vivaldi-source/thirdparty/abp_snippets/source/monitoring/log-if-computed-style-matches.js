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
import {$$} from "../utils/dom.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {raceWinner} from "../introspection/race.js";
import {formatArguments, sendDetectionEvent} from "../utils/general.js";

let {Error, MutationObserver, getComputedStyle, Set} = $(window);

const computedStyleHandlers = new Set();
let sharedComputedStyleMo = null;

const addedElementsBuffer = $([]);

// Only direct addedNodes (elements only) — recursively walking every
// subtree may cause performance issues on some sites.
function fillAddedElements(records) {
  addedElementsBuffer.length = 0;
  for (let r = 0; r < records.length; r++) {
    const record = records[r];
    const nodes = record.addedNodes;
    const len = nodes.length;
    for (let i = 0; i < len; i++) {
      const node = nodes[i];
      if (node.nodeType === 1)
        addedElementsBuffer.push(node);
    }
  }
}

function addComputedStyleHandler(handler) {
  computedStyleHandlers.add(handler);
  if (!sharedComputedStyleMo) {
    sharedComputedStyleMo = new MutationObserver(records => {
      fillAddedElements(records);
      for (const h of computedStyleHandlers)
        h(addedElementsBuffer);
    });
    sharedComputedStyleMo.observe(document, {
      childList: true,
      subtree: true
    });
  }
}

function removeComputedStyleHandler(handler) {
  computedStyleHandlers.delete(handler);
  if (computedStyleHandlers.size === 0 && sharedComputedStyleMo) {
    sharedComputedStyleMo.disconnect();
    sharedComputedStyleMo = null;
  }
}

/**
 * @description Detects when an element whose computed CSS style matches ALL
 * of the given property/value pairs appears in the DOM, then fires a
 * telemetry event via env.sendDetectionEvent.
 *
 * Performance note: the MutationObserver only checks direct addedNodes, not
 * their descendants, as recursively walking every subtree may cause
 * performance issues on some sites. A full one-time check is done
 * synchronously to begin. This does carry the risk that a matching element
 * embedded as a child will be missed. This snippet should be combined with
 * other detection methods to avoid false negatives.
 * @memberof module:snippets/monitoring
 *
 * @param {string} type Detection type (e.g. "acme-adwall").
 * @param {string} specifier Event specifier for BigQuery filtering. Always
 *   positionally required because it precedes the css style pairs. Pass
 *   "null" to omit, or a descriptive label.
 * @param {...string} pairs An even number of additional arguments treated as
 *   alternating CSS property/value pairs: prop1, val1, prop2, val2, ...
 *   At least one pair (two arguments) is required.
 *
 * @example
 * // Single pair — detect any fixed element
 * example.com#$#log-if-computed-style-matches acme-wall null position fixed
 *
 * @example
 * // Two pairs — must match both position AND z-index
 * example.com#$#log-if-computed-style-matches acme-wall fixed-overlay \
 *   position fixed z-index 999999
 *
 * @since Adblock Plus 4.27.0
 */
export function logIfComputedStyleMatches(type, specifier, ...pairs) {
  if (!type)
    throw new Error("[log-if-computed-style-matches snippet]: Missing type.");
  if (pairs.length === 0 || pairs.length % 2 !== 0)
    throw new Error("[log-if-computed-style-matches snippet]: Uneven pairs.");

  const formattedArguments = formatArguments(arguments);
  const debugLog = getDebugger("log-if-computed-style-matches");
  const {mark, end} = profile("log-if-computed-style-matches");

  const conditions = $([]);
  for (let i = 0; i < pairs.length; i += 2)
    conditions.push({property: pairs[i], value: pairs[i + 1]});

  const resolvedSpecifier = specifier === "null" ? null : specifier;

  const matchesAllConditions = el => {
    try {
      const style = getComputedStyle(el);
      return conditions.every(({property, value}) =>
        style[property] === value);
    }
    catch (e) {
      return false;
    }
  };

  // Declared before win so the raceWinner closure can reference it.
  let callback;
  const win = raceWinner("log-if-computed-style-matches", () => {
    removeComputedStyleHandler(callback);
  });

  // Receives the shared addedElementsBuffer from the MO (or null for
  // the initial sync scan).
  let matched = false;
  callback = elements => {
    mark();
    const toCheck = elements !== null ? elements : $$("*");
    let matchedEl = null;
    for (let i = 0; i < toCheck.length; i++) {
      const el = toCheck[i];
      if (matchesAllConditions(el)) {
        matchedEl = el;
        matched = true;
        break;
      }
    }
    if (matched) {
      sendDetectionEvent(type, resolvedSpecifier);
      debugLog(
        "success",
        "Matched computed style:",
        matchedEl,
        formattedArguments
      );
      win();
      removeComputedStyleHandler(callback);
    }
    end();
  };

  // Sync check after handler setup so nothing is missed
  // between scan and observe()
  callback(null);
  if (matched)
    return;

  addComputedStyleHandler(callback);
}
