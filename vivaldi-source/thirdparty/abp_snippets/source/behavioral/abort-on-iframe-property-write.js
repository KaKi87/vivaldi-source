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
import {abortOnIframe} from "../utils/execution.js";
import {profile} from "../introspection/profile.js";

/**
 * @description Patches a list of properties on the iframes'
 * window object to abort execution
 * when the property is written.
 * No error is printed to the console.
 * @memberof module:snippets/behavioral
 *
 * @param {...string} properties The list with the properties.
 * @example
 * abort-on-iframe-property-write atob btoa => The code that
 * sets the atob or btoa function inside any iframe throws an exception.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/69960151/abort-on-iframe-property-write} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/behavioral-snippets/abort-on-iframe-property-write} for external documentation.
 * @since Adblock Plus 3.10.1
 */
export function abortOnIframePropertyWrite(...properties) {
  const {mark, end} = profile("abort-on-iframe-property-write");
  mark();
  abortOnIframe(properties, false, true);
  end();
}
