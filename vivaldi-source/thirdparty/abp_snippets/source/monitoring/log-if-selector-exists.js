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
import {$$, initQueryAll} from "../utils/dom.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {raceWinner} from "../introspection/race.js";
import {formatArguments, sendDetectionEvent} from "../utils/general.js";

let {Error, MutationObserver} = $(window);

/**
 * @description Detects when a CSS or XPath selector appears in the DOM and
 * fires a telemetry event via env.sendDetectionEvent.
 * @memberof module:snippets/monitoring
 *
 * @param {string} selector CSS selector to watch for, or an XPath 1.0 query
 * wrapped as `xpath(the_actual_selector)`. Prefer CSS unless the match needs
 * something CSS cannot express.
 * @param {string} type Detection type (e.g. "acme-adwall").
 * @param {string} [specifier] Optional specifier for BigQuery filtering.
 *
 * @example
 * example.com#$#log-if-selector-exists .adwall-overlay acme-wall
 *
 * @example
 * example.com#$#log-if-selector-exists 'xpath(//div[@class="wall"])' acme-wall
 *
 * @since Adblock Plus 4.27.0
 */
export function logIfSelectorExists(selector, type, specifier = null) {
  if (!selector)
    throw new Error("[log-if-selector-exists snippet]: Missing selector.");
  if (!type)
    throw new Error("[log-if-selector-exists snippet]: Missing type.");

  const formattedArguments = formatArguments(arguments);
  const debugLog = getDebugger("log-if-selector-exists");
  const {mark, end} = profile("log-if-selector-exists");
  let exists;
  if (selector.startsWith("xpath(") && selector.endsWith(")")) {
    let queryAll;
    try {
      queryAll = initQueryAll(selector);
    }
    catch (error) {
      throw new Error("[log-if-selector-exists snippet]: " +
                      "Invalid XPath selector: " + error.message);
    }
    exists = () => queryAll().length > 0;
  }
  else {
    exists = () => $$(selector).length > 0;
  }

  let mo;
  const win = raceWinner("log-if-selector-exists", () => {
    mo.disconnect();
  });

  // Single callback for both MutationObserver and sync paths.
  // Both scan the full document via exists() — no added-nodes optimisation
  // since we're checking for selector existence, not specific nodes.
  // mark()/end() always paired to measure per-invocation cost.
  let matched = false;
  const callback = () => {
    mark();
    if (exists()) {
      sendDetectionEvent(type, specifier);
      debugLog("success", "Matched selector:", selector, formattedArguments);
      win();
      mo.disconnect();
      matched = true;
    }
    end();
  };

  mo = new MutationObserver(callback);

  // Sync check after observer setup so nothing is missed
  // between scan and mo.observe()
  callback();
  if (matched)
    return;

  mo.observe(document, {childList: true, subtree: true});
}
