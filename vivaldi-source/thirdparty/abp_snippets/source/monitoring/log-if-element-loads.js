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
import {call} from "proxy-pants/function";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {raceWinner} from "../introspection/race.js";
import {
  formatArguments, sendDetectionEvent, toRegExp
} from "../utils/general.js";

let {Error, MutationObserver, Set} = $(window);

// Captured before page JS can poison Element.prototype.
// In the bundle, this runs inside the callback when the extension
// content script loads (document_start), before any page JS executes.
const elementQSA = Element.prototype.querySelectorAll;

const elementLoadHandlers = new Set();
let sharedElementMo = null;

const candidatesBuffer = $([]);

function fillCandidates(records) {
  candidatesBuffer.length = 0;
  for (let r = 0; r < records.length; r++) {
    const record = records[r];
    const type = record.type;
    if (type === "attributes") {
      const el = record.target;
      if (el.src || el.href)
        candidatesBuffer.push(el);
    }
    else {
      const nodes = record.addedNodes;
      const len = nodes.length;
      for (let i = 0; i < len; i++) {
        const node = nodes[i];
        if (node.nodeType !== 1)
          continue;
        // Check the node itself; QSA covers descendants (excludes root).
        if (node.src || node.href)
          candidatesBuffer.push(node);
        if (node.childElementCount > 0) {
          for (const el of call(elementQSA, node, "[src],[href]"))
            candidatesBuffer.push(el);
        }
      }
    }
  }
}

function addElementHandler(handler) {
  elementLoadHandlers.add(handler);
  if (!sharedElementMo) {
    sharedElementMo = new MutationObserver(records => {
      fillCandidates(records);
      for (const h of elementLoadHandlers)
        h(candidatesBuffer);
    });
    sharedElementMo.observe(document, {
      childList: true,
      subtree: true,
      attributes: true,
      attributeFilter: ["src", "href"]
    });
  }
}

function removeElementHandler(handler) {
  elementLoadHandlers.delete(handler);
  if (elementLoadHandlers.size === 0 && sharedElementMo) {
    sharedElementMo.disconnect();
    sharedElementMo = null;
  }
}

/**
 * @description Detects when any element whose `.src` or `.href` property
 * matches a given URL pattern is inserted into the DOM or has its URL
 * attribute updated in-place, then fires a telemetry event via
 * env.sendDetectionEvent.
 *
 * Reads browser-resolved DOM properties (`.src` / `.href`) rather than raw
 * HTML attributes, so percent-encoded URLs are normalised before matching —
 * encoding-proof by design.
 *
 * `log-if-script-loads`, `log-if-iframe-loads`, and
 * `log-if-anchor-href-matches` are thin wrappers around this snippet that
 * scope matching to their respective element types.
 *
 * @memberof module:snippets/monitoring
 *
 * @param {string} urlPattern URL substring or /regex/ to match against the
 *   element's `.src` or `.href` property. Plain strings are literal
 *   substrings. Wrap in /.../ for a regular expression, or /.../i for
 *   case-insensitive.
 * @param {string} type Detection type label sent in the telemetry event
 *   (e.g. "acme-adwall").
 * @param {string} [tag] Optional element tag name to scope matching
 *   (e.g. "script", "iframe", "a", "video"). When omitted, all elements
 *   with a `.src` or `.href` property are considered.
 * @param {string} [specifier] Optional override for the event specifier
 *   field. When omitted, the matched element URL is used.
 *
 * @example
 * example.com#$#log-if-element-loads acme-wall.example acme-wall
 *
 * @example
 * example.com#$#log-if-element-loads acme-wall.example acme-wall video
 *
 * @since Adblock Plus 4.29.0
 */
export function logIfElementLoads(urlPattern, type,
                                  tag = null, specifier = null) {
  if (!urlPattern)
    throw new Error("[log-if-element-loads snippet]: Missing URL pattern.");
  if (!type)
    throw new Error("[log-if-element-loads snippet]: Missing type.");

  const formattedArguments = formatArguments(arguments);
  const debugLog = getDebugger("log-if-element-loads");
  const {mark, end} = profile("log-if-element-loads");

  const re = toRegExp(urlPattern);
  const tagUpper = tag ? tag.toUpperCase() : null;

  // Used only for the initial sync scan
  const selector = tag ? `${tag}[src],${tag}[href]` : "[src],[href]";

  const urlOf = el => el.src || el.href || null;
  const matchesTag = el => tagUpper === null || el.nodeName === tagUpper;

  // Declared before win so the raceWinner closure can reference it.
  let callback;
  const win = raceWinner("log-if-element-loads", () => {
    removeElementHandler(callback);
  });

  // Receives the shared candidatesBuffer from the MO (or null for the
  // initial sync scan). Each handler only applies its own tag + URL filter.
  let matched = false;
  callback = elements => {
    mark();
    const toCheck = elements !== null ? elements : $$(selector);

    let matchedEl = null;
    for (let i = 0; i < toCheck.length; i++) {
      const el = toCheck[i];
      if (!matchesTag(el))
        continue;
      const url = urlOf(el);
      if (url && re.test(url)) {
        matchedEl = el;
        matched = true;
        break;
      }
    }

    if (matched) {
      const url = specifier !== null ? specifier : urlOf(matchedEl);
      sendDetectionEvent(type, url);
      debugLog("success", "Matched element:", matchedEl, formattedArguments);
      win();
      removeElementHandler(callback);
    }
    end();
  };

  // Sync check after handler setup so nothing is missed
  // between scan and observe()
  callback(null);
  if (matched)
    return;

  addElementHandler(callback);
}
