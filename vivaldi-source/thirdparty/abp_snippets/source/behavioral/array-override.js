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
import {apply, proxy} from "proxy-pants/function";
import {hasOwnProperty} from "proxy-pants/object";

import {getDebugger} from "../introspection/log.js";
import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {profile} from "../introspection/profile.js";
import {matchesStackTrace} from "../utils/execution.js";
import {proxyToStringCalls} from "../utils/toString.js";

const {Error, Object, Array, Map} = $(window);

// Contains all the values to override after the snippet is used at least once
let arrayValues = null;
const hitFilters = new Set();
function sendHitOnce(filter) {
  if (!hitFilters.has(filter)) {
    hitFilters.add(filter);
    sendSnippetHitEvent(filter);
  }
}

/**
 * @description Checks if any property in an object matches the
 * given regex pattern Traverses the object up to a specified depth
 * @param {*} val - The value to check
 * @param {string} needle - The regex pattern to match against
 * @param {string[]} pathSegments - Parsed path to look for the needle
 * @return {boolean} True if a match is found
 */
function hasMatchingProperty(val, needle, pathSegments) {
  // Navigate through the object based on the path
  let current = val;
  for (const segment of pathSegments) {
    // If current is null, or the property doesn't exist, return false
    if (!current || !hasOwnProperty(current, segment))
      return false;
    current = current[segment];
  }

  // Check if the value at the path is a string or number that matches the regex
  if (typeof current === "string" || typeof current === "number"){
    const currStr = current.toString();
    return needle.test(currStr);
  }

  return false;
}

/**
 * @description Traps calls to Array.prototype functions. If the needle matches
 * the parameter to the function call, the snippet changes the behaviour
 * of the function to ignore that call or return another value.
 * @memberof module:snippets/behavioral
 *
 * @param {string} method The Array function to override.
 * Possible values to override the property with:
 *   push
 *   includes
 * @param {string} needle The string or regex used to determine
 * which function calls to trap.
 * @param {?string} [returnValue=false] The return value for the matched
 * function calls.
 * Possible values:
 *   false
 *   true
 * @param {?string} path Path to look for the needle
 * @param {?string} stack Comma-separated list of strings to check in the
 * stack trace. If provided, the override will only apply when the stack
 * trace contains at least one of these patterns.
 *
 * @example
 * array-override push test => Will ignore array.push(“test“)
 * while allowing other values to be added to the array.
 * const arr = [];
 * arr.push(“1“);
 * arr.push(“2“);
 * arr.push(“test“)
 * arr will only have the values “1” and “2” inside of it.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/393969665/array-override} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/behavioral-snippets/array-override} for external documentation.
 * @since Adblock Plus 4.8
 */
export function arrayOverride(method, needle, returnValue = "false",
                              path, stack) {
  if (!method)
    throw new Error("[array-override snippet]: Missing method to override.");

  if (!needle)
    throw new Error("[array-override snippet]: Missing needle.");

  if (!arrayValues)
    arrayValues = new Map();

  let debugLog = getDebugger("array-override");
  const {mark, end} = profile("array-override");
  const formattedArgsToLog = formatArguments(arguments);
  // Array.prototype.push
  if (method === "push" && !arrayValues.has("push")) {
    mark();
    const {push} = Array.prototype;
    arrayValues.set("push", $([]));

    let wrappedPush = proxy(
      push,
      function(val) {
        const overrideVals = arrayValues.get("push");
        for (const {
          needleRegex, pathSegments, stackNeedles, formattedArgs
        } of overrideVals) {
          // Simple check for strings and numbers
          if (!pathSegments.length && (typeof val === "string" ||
              typeof val === "number")) {
            const valStr = val.toString();
            if (valStr.match && valStr.match(needleRegex) &&
                matchesStackTrace(stackNeedles, debugLog)) {
              debugLog("success", `Array.push is ignored for needle: ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
              sendHitOnce("array-override " + formattedArgs);
              return;
            }
          }
          // Deep check for objects
          else if (pathSegments.length && typeof val === "object" &&
                   val !== null) {
            if (hasMatchingProperty(val, needleRegex, pathSegments) &&
                matchesStackTrace(stackNeedles, debugLog)) {
              debugLog("success", `Array.push is ignored for object containing needle: ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
              sendHitOnce("array-override " + formattedArgs);
              return;
            }
          }
        }
        return apply(push, this, arguments);
      }
    );
    proxyToStringCalls(wrappedPush, push);
    Object.defineProperty(window.Array.prototype, "push", {
      value: wrappedPush
    });
    debugLog("info", "Wrapped Array.prototype.push");
    end();
  }
  // Array.prototype.includes
  else if (method === "includes" && !arrayValues.has("includes")) {
    mark();
    const {includes} = Array.prototype;
    arrayValues.set("includes", $([]));

    let wrappedIncludes = proxy(
      includes,
      function(val) {
        const overrideVals = arrayValues.get("includes");
        for (const {
          needleRegex,
          retVal,
          pathSegments,
          stackNeedles,
          formattedArgs
        } of overrideVals) {
          // Simple check for strings and numbers
          if (!pathSegments.length && (typeof val === "string" ||
               typeof val === "number")) {
            if (val.toString().match &&
                val.toString().match(needleRegex) &&
                matchesStackTrace(stackNeedles, debugLog)) {
              debugLog("success", `Array.includes returned ${retVal} for ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
              sendHitOnce("array-override " + formattedArgs);
              return retVal;
            }
          }
          // Deep check for objects
          else if (pathSegments.length && typeof val === "object" &&
                   val !== null) {
            if (hasMatchingProperty(val, needleRegex, pathSegments) &&
                matchesStackTrace(stackNeedles, debugLog)) {
              debugLog("success", `Array.includes returned ${retVal} for object containing ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
              sendHitOnce("array-override " + formattedArgs);
              return retVal;
            }
          }
        }
        return apply(includes, this, arguments);
      }
    );
    proxyToStringCalls(wrappedIncludes, includes);
    Object.defineProperty(window.Array.prototype, "includes", {
      value: wrappedIncludes
    });
    debugLog("info", "Wrapped Array.prototype.includes");
    end();
  }
  // Array.prototype.forEach
  else if (method === "forEach" && !arrayValues.has("forEach")) {
    mark();
    const {forEach} = Array.prototype;
    arrayValues.set("forEach", $([]));

    let wrappedForEach = proxy(
      forEach,
      function(callback, thisArg) {
        const overrideVals = arrayValues.get("forEach");
        // Create a new callback that filters items based on the needles
        const filteredCallback = function(item, index, array) {
          for (const {needleRegex, pathSegments, stackNeedles, formattedArgs} of
            overrideVals) {
            // Simple check for strings and numbers
            if (!pathSegments.length && (typeof item === "string" ||
                typeof item === "number")) {
              const itemStr = item.toString();
              if (itemStr.match &&
                  itemStr.match(needleRegex) &&
                  matchesStackTrace(stackNeedles, debugLog)) {
                debugLog("success", `Array.forEach skipped callback for item matching needle: ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
                sendHitOnce("array-override " + formattedArgs);
                return; // Skip callback for this item
              }
            }
            // Deep check for objects
            else if (pathSegments.length && typeof item === "object" &&
                     item !== null) {
              if (hasMatchingProperty(item, needleRegex, pathSegments) &&
                  matchesStackTrace(stackNeedles, debugLog)) {
                debugLog("success", `Array.forEach skipped callback for object containing needle: ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
                sendHitOnce("array-override " + formattedArgs);
                return; // Skip callback for this item
              }
            }
          }
          // If we didn't match any needles, call the original callback
          return apply(callback, thisArg || this, [item, index, array]);
        };
        return apply(forEach, this, [filteredCallback, thisArg]);
      }
    );
    proxyToStringCalls(wrappedForEach, forEach);
    Object.defineProperty(window.Array.prototype, "forEach", {
      value: wrappedForEach
    });
    debugLog("info", "Wrapped Array.prototype.forEach");
    end();
  }

  const needleRegex = toRegExp(needle);
  let pathSegments = [];
  if (path)
    pathSegments = path.split(".");

  // Parse stack trace needles
  let stackNeedles = [];
  if (stack)
    stackNeedles = stack.split(",").map(s => s.trim());

  const overrideVals = arrayValues.get(method);
  const retVal = returnValue === "true";
  overrideVals.push({needleRegex, retVal, pathSegments, stackNeedles,
                     formattedArgs: formattedArgsToLog});
  arrayValues.set(method, overrideVals);
}
