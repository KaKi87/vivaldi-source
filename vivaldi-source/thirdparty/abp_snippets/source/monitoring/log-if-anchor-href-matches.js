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
import {logIfElementLoads} from "./log-if-element-loads.js";

/**
 * @description Alias for `log-if-element-loads` scoped to `<a href>`.
 *   Detects when an `<a>` element whose resolved `.href` matches a given
 *   pattern is inserted into the DOM or has its href updated in-place, then
 *   fires a telemetry event.
 * @memberof module:snippets/monitoring
 *
 * @param {string} urlPattern URL substring or /regex/ to match against
 *   the resolved href.
 * @param {string} type Detection type (e.g. "acme-adwall").
 * @param {string} [specifier] Optional override for the event specifier
 *   field. When omitted, the matched href URL is used.
 *
 * @example
 * example.com#$#log-if-anchor-href-matches example.com acme-wall
 *
 * @deprecated Use {@link logIfElementLoads} with tag `"a"` instead.
 * @since Adblock Plus 4.28.0
 */
export function logIfAnchorHrefMatches(urlPattern, type, specifier = null) {
  logIfElementLoads(urlPattern, type, "a", specifier);
}
