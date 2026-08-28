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
import {debug} from "../introspection/debug.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {overrideValue} from "../utils/execution.js";
import {JSONPath} from "../utils/jsonpath.js";
import {addPostResponseCallback} from "../utils/xhrManipulation.js";

let {JSON, RegExp} = $(window);
let xhrRules;
const hitFilters = new Set();

/**
 * @description Replaces the response of XMLHttpRequest (XHR) requests
 * if the response text matches a given string.
 * @memberof module:snippets/behavioral
 *
 * @param {string} search - The string or regex pattern to match
 * 	in the response text. If the value starts with "jsonpath(",
 * 	it is treated as a JSONPath query: the response is parsed
 * 	as JSON, matching properties are replaced with the
 * 	replacement value (interpreted via overrideValue), and
 * 	the result is re-serialized.
 * @param {?string} [replacement=""] - The string to replace
 * 	the matched pattern with. When using JSONPath, this value
 * 	is interpreted via overrideValue (supports "false", "true",
 * 	"null", "emptyArray", "emptyObj", etc.).
 * @param {?string} [needle=null] - An optional string
 * representing properties to match in the XMLHttpRequest details.
 *
 * @example
 * // Replaces "Hello" with "Hi" in the response text
 * // of all XHR requests.
 * replaceXhrResponse(/Hello/, "Hi");
 *
 * @example
 * // Replaces "Hello" with "Hi" in the response text of
 * // XHR requests only if the URL includes "example.com".
 * replaceXhrResponse(/Hello/, "Hi", "url:example.com");
 *
 * @example
 * replace-xhr-response '/ads:\\[(.*?)\\]/' => Will remove
 * anything that looks like ads:[<<any-string>>] from every
 * response body.
 *
 * @example
 * // Uses JSONPath to set all "enabled" properties inside
 * // the "ads" array to false.
 * replaceXhrResponse("jsonpath($.ads[*].enabled)", "false");
 *
 * @example
 * // Uses JSONPath with needle: only applies if the response
 * // contains "tracking".
 * replaceXhrResponse(
 *   "jsonpath($..trackingId)", "null", "tracking"
 * );
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/176422979/replace-xhr-response} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/behavioral-snippets/replace-xhr-response} for external documentation.
 * @since Adblock Plus 4.4
 */
export function replaceXhrResponse(search, replacement = "", needle = null) {
  const formattedArgsToLog = formatArguments(arguments);
  const debugLog = getDebugger("replace-xhr-response");
  const {mark, end} = profile("replace-xhr-response");

  if (!search) {
    debugLog("error", "The parameter 'pattern' is required");
    return;
  }

  if (!xhrRules) {
    xhrRules = new Map();
    debugLog("info", "XMLHttpRequest proxied");

    addPostResponseCallback(responseText => {
      mark();
      let replacedText = responseText;
      for (const [thisSearch, {
        replacement: thisReplacement,
        needle: thisNeedle,
        formattedArgs,
        isJsonPath,
        jsonPathEngine
      }] of xhrRules) {
        if (thisNeedle) {
          const needleRegex = toRegExp(thisNeedle);
          if (needleRegex.test(replacedText)) {
            if (debug()) {
              console.groupCollapsed(`DEBUG [replace-xhr-response] success: '${thisNeedle}' found in XHR response`);
              debugLog("info", replacedText);
              console.groupEnd();
            }
          }
          else {
            if (debug()) {
              console.groupCollapsed(`DEBUG [replace-xhr-response] warn: '${thisNeedle}' not found in XHR response`);
              debugLog("warn", replacedText);
              console.groupEnd();
            }
            continue;
          }
        }

        if (isJsonPath) {
          try {
            let obj = JSON.parse(replacedText);
            const matches =
              jsonPathEngine.evaluate(obj);
            $(matches).forEach(({parent, key}) => {
              parent[key] =
                overrideValue(thisReplacement);
              debugLog(
                "success",
                "JSONPath match at " +
                `[${key}], replaced with ` +
                thisReplacement,
                "\nFILTER: replace-xhr-response " +
                formattedArgs
              );
              const filter = "replace-xhr-response " + formattedArgs;
              if (!hitFilters.has(filter)) {
                hitFilters.add(filter);
                sendSnippetHitEvent(filter);
              }
            });
            replacedText = JSON.stringify(obj);
          }
          catch (e) {
            debugLog(
              "info",
              "JSONPath: skipping non-JSON " +
              "response or evaluation error: " +
              e.message
            );
          }
        }
        else {
          replacedText =
            $(replacedText)
              .replace(thisSearch, thisReplacement)
              .toString();
          if (
            responseText.toString() !==
            replacedText.toString()
          ) {
            const filter = "replace-xhr-response " + formattedArgs;
            if (!hitFilters.has(filter)) {
              hitFilters.add(filter);
              sendSnippetHitEvent(filter);
            }
            if (debug()) {
              console.groupCollapsed(`DEBUG [replace-xhr-response] success: '${thisSearch}' replaced with '${thisReplacement}' in XHR response`,
                                     "\nFILTER: replace-xhr-response " +
                formattedArgs);
              debugLog("success", replacedText);
              console.groupEnd();
            }
          }
        }
      }
      end();
      return replacedText.toString();
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
    xhrRules.set(search, {
      replacement,
      needle,
      formattedArgs: formattedArgsToLog,
      isJsonPath: true,
      jsonPathEngine
    });
  }
  else {
    const regex = toRegExp(search);
    // replaceAll is not supported in older browsers,
    // this simulates it.
    const globalisedRegEx = new RegExp(regex, "g");
    xhrRules.set(globalisedRegEx, {
      replacement,
      needle,
      formattedArgs: formattedArgsToLog,
      isJsonPath: false,
      jsonPathEngine: null
    });
  }
}
