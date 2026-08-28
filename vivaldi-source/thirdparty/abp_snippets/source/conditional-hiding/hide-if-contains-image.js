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

import {$$, $closest, hideElement} from "../utils/dom.js";
import {raceWinner} from "../introspection/race.js";
import {formatArguments, sendSnippetHitEvent, toRegExp}
  from "../utils/general.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {fetchContent} from "../utils/execution.js";
import {uint8ArrayToHex} from "../utils/general.js";

let {
  getComputedStyle,
  MutationObserver,
  Uint8Array
} = $(window);
const hitFilters = new Set();

/**
 * @description Hides any HTML element or one of its ancestors matching
 * a CSS selector if the background image of the element
 * matches a given pattern.
 * @memberof module:snippets/conditional-hiding
 *
 * @param {string} search The pattern to look for in the background images of
 *   HTML elements. This must be the hexadecimal representation of the image
 *   data for which to look. If the string begins and ends with a slash (`/`),
 *   the text in between is treated as a regular expression.
 * @param {string} selector The CSS selector that an HTML element must match
 *   for it to be hidden.
 * @param {?string} [searchSelector] The CSS selector that an HTML element
 *   containing the given pattern must match. Defaults to the value of the
 *   `selector` argument.
 * @example
 * hide-if-contains-image ffd8ffe1001845 div => Hides any div element
 * whose background-image's hex matches the ffd8ffe1001845 pattern.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/69962622/hide-if-contains-image} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/conditional-hiding-snippets/hide-if-contains-image} for external documentation.
 * @since Adblock Plus 3.4.2
 */
export function hideIfContainsImage(search, selector, searchSelector) {
  if (searchSelector == null)
    searchSelector = selector;

  let searchRegExp = toRegExp(search);

  const formattedArguments = formatArguments(arguments);
  const debugLog = getDebugger("hide-if-contains-image");
  const {mark, end} = profile("hide-if-contains-image");

  let callback = () => {
    mark();
    for (const {element, rootParents} of $$(searchSelector, true)) {
      let style = getComputedStyle(element);
      let match = $(style["background-image"]).match(/^url\("(.*)"\)$/);
      if (match) {
        fetchContent(match[1]).then(content => {
          if (searchRegExp.test(uint8ArrayToHex(new Uint8Array(content)))) {
            let closest = $closest($(element), selector, rootParents);
            if (closest) {
              win();
              hideElement(closest);
              debugLog("success",
                       "Matched: ",
                       closest,
                       "\nFILTER: hide-if-contains-image",
                       formattedArguments);
              const filter =
                "hide-if-contains-image " +
                formattedArguments;
              if (!hitFilters.has(filter)) {
                hitFilters.add(filter);
                sendSnippetHitEvent(filter);
              }
            }
          }
        });
      }
    }
    end();
  };

  let mo = new MutationObserver(callback);
  let win = raceWinner(
    "hide-if-contains-image",
    () => mo.disconnect()
  );
  mo.observe(document, {childList: true, subtree: true});
  callback();
}

