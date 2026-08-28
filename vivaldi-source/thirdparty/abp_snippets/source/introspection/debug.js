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
import {toRegExp} from "../utils/general.js";

/**
 * @description Whether debug mode is enabled.
 * @type {boolean}
 * @private
 */
let debugging = false;

/**
 * @description The active debug filter, or null if unfiltered.
 * @type {?RegExp}
 * @private
 */
let filter = null;

/**
 * @description Tells if the debug mode is inactive.
 * @memberOf module:snippets/introspection
 * @returns {boolean}
 */
export function debug() {
  return debugging;
}

/**
 * @description Returns the active debug filter regexp,
 * or null if no filter is set.
 * @returns {?RegExp}
 */
export function debugFilter() {
  return filter;
}

/**
 * @description Enables debug mode. If a pattern is provided,
 * only log messages matching the pattern will be shown.
 * @memberOf module:snippets/introspection
 *
 * @param {string} [pattern] Optional filter pattern.
 *   Plain strings match literally; `/regex/` patterns
 *   are treated as regular expressions.
 *
 * @example
 * example.com#$#debug; log 'Hello, world!'
 * example.com#$#debug; abort-on-property-read atob => activates
 * debug mode for the abort-on-property-read snippet
 * example.com#$#debug json-prune; json-prune ads => activates
 * debug mode only for json-prune logs
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/69959924/debug} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/debugging-snippets/debug} for external documentation.
 * @since Adblock Plus 3.8
 */
export function setDebug(pattern) {
  debugging = true;
  if (pattern)
    filter = toRegExp(pattern);
}
