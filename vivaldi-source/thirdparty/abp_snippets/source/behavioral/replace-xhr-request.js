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
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {JSONPath} from "../utils/jsonpath.js";
import {addPreSendCallback} from "../utils/xhrManipulation.js";

let {Array, Error, JSON, Object, RegExp} = $(window);
let xhrRequestRules;
const hitFilters = new Set();

/**
 * @description Replaces the request body of XMLHttpRequest (XHR)
 * requests before they are sent, if the body matches a given
 * string.
 * @memberof module:snippets/behavioral
 *
 * @param {string} search - The string or regex pattern to match
 * 	in the request body. If the value starts with "jsonpath(",
 * 	it is treated as a JSONPath query: the body is parsed
 * 	as JSON, matching properties are replaced with the
 * 	replacement value (interpreted via overrideValue), and
 * 	the result is re-serialized.
 * @param {?string} [replacement=""] - The string to replace
 * 	the matched pattern with. In JSONPath mode the value is
 * 	parsed via JSON.parse (e.g. "false" → false,
 * 	"null" → null, "[]" → empty array). If JSON.parse
 * 	fails the raw string is used as-is.
 * @param {?string} [needle=null] - An optional regex to
 * 	pre-filter on the request body content. If specified,
 * 	the replacement is only applied when the needle matches
 * 	the request body.
 * @param {string} [mode="replace"] - JSONPath operation mode.
 * 	"replace" (default) sets the matched property to the
 * 	parsed replacement value. "append" appends the parsed
 * 	value to the existing one: concatenates strings,
 * 	pushes/concats into arrays, merges objects via
 * 	Object.assign.
 *
 * @example
 * // Replaces "trackingId" with "blocked" in the request body
 * // of all XHR requests.
 * replaceXhrRequest(/trackingId/, "blocked");
 *
 * @example
 * // Replaces "trackingId" with "blocked" in the request body
 * // only if the body contains "analytics".
 * replaceXhrRequest(/trackingId/, "blocked", "analytics");
 *
 * @example
 * // Uses JSONPath to set the "enabled" property to false.
 * replaceXhrRequest("jsonpath($.ads.enabled)", "false");
 *
 * @example
 * // Uses JSONPath append mode to push a tag into an array.
 * replaceXhrRequest(
 *   "jsonpath($.tags)", '"blocked"', '', 'append'
 * );
 *
 */
export function replaceXhrRequest(
  search, replacement = "", needle = null,
  mode = "replace"
) {
  const formattedArgsToLog = formatArguments(arguments);
  const debugLog = getDebugger("replace-xhr-request");
  const {mark, end} = profile("replace-xhr-request");

  if (!search)
    throw new Error("[replace-xhr-request]: Missing 'search' parameter");

  function parseJSON(str) {
    try {
      return JSON.parse(str);
    }
    catch (_e) {
      return str;
    }
  }

  function appendValue(parent, key, parsed) {
    let existing = parent[key];
    if (Array.isArray(existing)) {
      if (Array.isArray(parsed))
        parent[key] = $(existing).concat(parsed);
      else
        $(existing).push(parsed);
    }
    else if (
      typeof existing === "object" &&
      existing !== null &&
      typeof parsed === "object" &&
      parsed !== null &&
      !Array.isArray(parsed)
    ) {
      Object.assign(existing, parsed);
    }
    else if (typeof existing === "string") {
      parent[key] = existing + $(parsed).toString();
    }
    else {
      parent[key] = parsed;
    }
  }

  if (!xhrRequestRules) {
    xhrRequestRules = new Map();
    debugLog("info", "XMLHttpRequest proxied");

    addPreSendCallback(body => {
      mark();
      let modifiedBody = body;
      for (const [thisSearch, {
        replacement: thisReplacement,
        needle: thisNeedle,
        formattedArgs,
        isJsonPath,
        jsonPathEngine,
        mode: thisMode
      }] of xhrRequestRules) {
        if (thisNeedle) {
          const needleRegex = toRegExp(thisNeedle);
          if (needleRegex.test(modifiedBody)) {
            debugLog(
              "info",
              `'${thisNeedle}' found in ` +
              "XHR request body"
            );
          }
          else {
            continue;
          }
        }

        if (isJsonPath) {
          try {
            let obj = JSON.parse(modifiedBody);
            const matches =
              jsonPathEngine.evaluate(obj);
            $(matches).forEach(({parent, key}) => {
              let parsed =
                parseJSON(thisReplacement);
              if (thisMode === "append")
                appendValue(parent, key, parsed);
              else
                parent[key] = parsed;
              debugLog(
                "success",
                `JSONPath [${thisMode}] at ` +
                `[${key}] with ` +
                thisReplacement,
                "\nFILTER: replace-xhr-request " +
                formattedArgs
              );
              const filter = "replace-xhr-request " + formattedArgs;
              if (!hitFilters.has(filter)) {
                hitFilters.add(filter);
                sendSnippetHitEvent(filter);
              }
            });
            modifiedBody = JSON.stringify(obj);
          }
          catch (e) {
            debugLog(
              "info",
              "JSONPath: skipping non-JSON " +
              "body or evaluation error: " +
              e.message
            );
          }
        }
        else {
          modifiedBody =
            $(modifiedBody)
              .replace(thisSearch, thisReplacement)
              .toString();
          if (
            body.toString() !==
            modifiedBody.toString()
          ) {
            debugLog(
              "success",
              `'${thisSearch}' replaced ` +
              `with '${thisReplacement}' ` +
              "in XHR request body",
              "\nFILTER: replace-xhr-request " +
              formattedArgs
            );
            const filter = "replace-xhr-request " + formattedArgs;
            if (!hitFilters.has(filter)) {
              hitFilters.add(filter);
              sendSnippetHitEvent(filter);
            }
          }
        }
      }
      end();
      return modifiedBody;
    });
  }

  if ($(search).startsWith("jsonpath(")) {
    let jsonPathEngine;
    try {
      const query =
        $(search).slice(9, -1).toString();
      jsonPathEngine = new JSONPath(query);
    }
    catch (e) {
      debugLog(
        "error",
        `Invalid JSONPath query: ${search}. ` +
        `Error: ${e.message}`
      );
      return;
    }
    xhrRequestRules.set(search, {
      replacement,
      needle,
      formattedArgs: formattedArgsToLog,
      isJsonPath: true,
      jsonPathEngine,
      mode
    });
  }
  else {
    const regex = toRegExp(search);
    // replaceAll is not supported in older browsers,
    // this simulates it.
    const globalisedRegEx = new RegExp(regex, "g");
    xhrRequestRules.set(globalisedRegEx, {
      replacement,
      needle,
      formattedArgs: formattedArgsToLog,
      isJsonPath: false,
      jsonPathEngine: null,
      mode
    });
  }
}
