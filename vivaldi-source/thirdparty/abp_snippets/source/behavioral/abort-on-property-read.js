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
import {abortOnRead} from "../utils/execution.js";
import {formatArguments} from "../utils/general.js";
import {profile} from "../introspection/profile.js";

/**
 * @description Patches a property on the window object to
 * abort execution when the property is read.
 *
 * No error is printed to the console.
 *
 * The idea originates from
 * [uBlock Origin](https://github.com/uBlockOrigin/uAssets/blob/80b195436f8f8d78ba713237bfc268ecfc9d9d2b/filters/resources.txt#L1703).
 * @memberof module:snippets/behavioral
 *
 * @param {string} property The name of the property.
 * @param {?string} setConfigurable Value of the configurable attribute. Sets
 *   the flag to false if "false" is given, the flag is set to true
 *   for all other cases.
 * @example
 * abort-on-property-read R false => The code that reads/calls R throws
 * an exception. This example comes from a website that was
 * circumventing and false prevents the circumvention attempt.
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/69959928/abort-on-property-read} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/behavioral-snippets/abort-on-property-read} for external documentation.
 * @since Adblock Plus 3.4.1
 */
export function abortOnPropertyRead(property, setConfigurable) {
  const configurableFlag = !(setConfigurable === "false");
  const formattedArguments = formatArguments(arguments);
  const {mark, end} = profile("abort-on-property-read");
  mark();
  abortOnRead("abort-on-property-read",
              window,
              property,
              formattedArguments,
              configurableFlag);
  end();
}
