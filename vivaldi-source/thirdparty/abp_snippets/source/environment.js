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
/**
 * @typedef {object} Environment
 * @property {Array.<Array>} debugCSSProperties Highlighting options.
 * CSS properties to be applied to the targeted element.
 * @property {bool} debugJS Javascript debugging.
 * Enables/disables JS Devtools debugging
 * @property {string} world Target injection world. 'ISOLATED' or 'MAIN'.
 * @property {Function} [sendDetectionEvent] Optional callback invoked
 * when a monitoring snippet detects a match.
 * Receives (type, hostname, specifier).
 * @property {Function} [sendSnippetHitEvent] Optional callback invoked
 * when a snippet successfully acts on a page element.
 * Receives (filter, hostname) where filter is the full filter body
 * e.g. "json-prune ads userId".
 */

/**
 * A configuration object passed by integrators.
 * @type {Environment}
 * @private
 */

// Set the currentEnvironment to the environment
if (typeof currentEnvironment !== "undefined" &&
    currentEnvironment.initial &&
    typeof environment !== "undefined")
  currentEnvironment = environment;

const getLibEnvironment = () => {
  if (typeof currentEnvironment !== "undefined")
    return currentEnvironment;
  if (typeof environment !== "undefined")
    return environment;
  return {};
};

export default getLibEnvironment;
