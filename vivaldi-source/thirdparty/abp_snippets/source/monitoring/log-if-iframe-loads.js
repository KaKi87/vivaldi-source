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
 * @description Alias for `log-if-element-loads` scoped to `<iframe src>`.
 *   Detects when an `<iframe src="...">` element whose src URL matches a given
 *   pattern is inserted into the DOM, then fires a telemetry event.
 * @memberof module:snippets/monitoring
 *
 * @param {string} urlPattern URL substring or /regex/ to match against
 *   iframe src attributes.
 * @param {string} type Detection type (e.g. "acme-adwall").
 * @param {string} [specifier] Optional override for the event specifier
 *   field. When omitted, the matched iframe src URL is used.
 *
 * @example
 * example.com#$#log-if-iframe-loads evil-empire.com acme-wall
 *
 * @deprecated Use {@link logIfElementLoads} with tag `"iframe"` instead.
 * @since Adblock Plus 4.27.0
 */
export function logIfIframeLoads(urlPattern, type, specifier = null) {
  logIfElementLoads(urlPattern, type, "iframe", specifier);
}
