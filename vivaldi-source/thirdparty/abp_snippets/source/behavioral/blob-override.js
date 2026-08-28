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
import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {proxyToStringCalls} from "../utils/toString.js";

const {Array, Blob, Error, Object, Reflect} = $(window);

// Global array to store all active rules
const blobRules = [];
const hitFilters = new Set();

/**
 * @description Traps calls to the Blob constructor, Replaces the blob
 * content(blobParts) if the content matches a given
 * regular expression pattern.
 * @memberof module:snippets/behavioral
 *
 * @param {string} search - The string or regex pattern to match
 * in the blob content.
 * @param {?string} replacement - The string to replace the matched
 * pattern with.
 * @param {?string} needle - An optional string or regex to check in the
 * blob parts before applying the replacement.
 *
 * @example
 * blob-override '/config = \\{[^}]*\\}/' 'config = {}' '/Sponsored/'
 * => Will override the config content matched via regex pattern with
 * an empty object only if the blob contains also the string Sponsored.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/964657166/blob-override} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/behavioral-snippets/blob-override} for external documentation.
 * @since Adblock Plus 4.22.2
 */

export function blobOverride(search, replacement = "", needle = null) {
  if (!search) {
    throw new Error(
      "[blob-override snippet]: Missing parameter search."
    );
  }
  const debugLog = getDebugger("blob-override");
  const formattedArgsToLog = formatArguments(arguments);
  const {mark, end} = profile("blob-override");
  mark();

  // Store the new rule
  blobRules.push({
    match: toRegExp(search),
    replaceWith: replacement,
    needle: needle ? toRegExp(needle) : null,
    formattedArgs: formattedArgsToLog
  });

  // Patch Blob only once when pushing the first rule
  if (blobRules.length > 1)
    return;

  const OriginalBlob = Blob;
  function PatchedBlob(data, options = {}) {
    if (Array.isArray(data)) {
      let combinedData = $(data).join("");

      for (const rule of $(blobRules)) {
        if (
          (!rule.needle || rule.needle.test(combinedData)) &&
          rule.match.test(combinedData)
        ) {
          combinedData = combinedData.replace(rule.match, rule.replaceWith);
          debugLog("success", `Replaced: ${rule.match} → ${rule.replaceWith},\nFILTER: blob-override ${rule.formattedArgs}`);
          const filter =
            "blob-override " + rule.formattedArgs;
          if (!hitFilters.has(filter)) {
            hitFilters.add(filter);
            sendSnippetHitEvent(filter);
          }
        }
      }
      data = [combinedData];
    }

    const blob = Reflect.construct(OriginalBlob, [data, options]);
    Object.setPrototypeOf(blob, PatchedBlob.prototype);
    return blob;
  }

  PatchedBlob.prototype = OriginalBlob.prototype; // For instances behaviour
  Object.setPrototypeOf(PatchedBlob, OriginalBlob); // For constructor
  proxyToStringCalls(PatchedBlob, window.Blob);
  window.Blob = PatchedBlob;
  debugLog("info", "Wrapped Blob constructor in context ");
  end();
}
