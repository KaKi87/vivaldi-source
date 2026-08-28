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
import {apply, call, proxy} from "proxy-pants/function";
import {hasOwnProperty} from "proxy-pants/object";

import {getDebugger} from "../introspection/log.js";
import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {profile} from "../introspection/profile.js";
import {matchesStackTrace} from "../utils/execution.js";
import {proxyToStringCalls} from "../utils/toString.js";

const {Error, Object, Map} = $(window);

// Contains all the values to override after the snippet is used at least once
let mapValues = null;
const hitFilters = new Set();
function sendHitOnce(filter) {
  if (!hitFilters.has(filter)) {
    hitFilters.add(filter);
    sendSnippetHitEvent(filter);
  }
}

/**
 * @description Checks if a value matches the given regex pattern
 * @param {*} val - The value to check
 * @param {RegExp} needle - The regex pattern to match against
 * @param {string[]} pathSegments - Parsed path to look for the needle
 * @return {boolean} True if a match is found
 */
function isMatchingValue(val, needle, pathSegments) {
  // If no path is specified, check the value directly
  if (!pathSegments.length) {
    if (typeof val === "string" || typeof val === "number") {
      const valStr = val.toString();
      return needle.test(valStr);
    }
    return false;
  }

  // Navigate through the object based on the path
  let current = val;
  for (const segment of pathSegments) {
    // If current is null, or the property doesn't exist, return false
    if (!current || !hasOwnProperty(current, segment))
      return false;
    current = current[segment];
  }

  // Check if the value at the path matches the regex
  if (typeof current === "string" || typeof current === "number") {
    const currStr = current.toString();
    return needle.test(currStr);
  }

  return false;
}

/**
 * @description Traps calls to Map.prototype functions. If the needle matches
 * the parameter to the function call, the snippet changes the behaviour
 * of the function to ignore that call or return another value.
 * @memberof module:snippets/behavioral
 *
 * @param {string} method The Map function to override.
 * Possible values to override the property with:
 *   set
 *   get
 *   has
 * @param {string} needle The string or regex used to determine
 * which function calls to trap. For 'set', this matches the value.
 * For 'get' and 'has', this matches the key.
 * @param {?string} [returnValue=""] The return value for the matched
 * function calls. For 'set', ignored. For 'get', the value to return.
 * For 'has', "true" or "false".
 * @param {?string} path Path to look for the needle (only for 'set' method
 * when checking object values)
 * @param {?string} stack Comma-separated list of strings to check in the
 * stack trace. If provided, the override will only apply when the stack
 * trace contains at least one of these patterns.
 * @example
 * map-override get key1 customValue => map.get(“key1“) will
 * always return “customValue”.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/1059127297/map-override} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/behavioral-snippets/map-override} for external documentation.
 * @since Adblock Plus 4.24.0
 */
export function mapOverride(method, needle, returnValue = "", path,
                            stack) {
  if (!method)
    throw new Error("[map-override snippet]: Missing method to override.");

  if (!needle)
    throw new Error("[map-override snippet]: Missing needle.");

  if (!mapValues)
    mapValues = new Map();

  let debugLog = getDebugger("map-override");
  const {mark, end} = profile("map-override");
  const {set, get, has} = Map.prototype;
  const formattedArgsToLog = formatArguments(arguments);

  // Map.prototype.set
  if (method === "set" && !mapValues.has("set")) {
    mark();
    call(set, mapValues, "set", $([]));

    let wrappedSet = proxy(set, function(key, val) {
      const overrideVals = call(get, mapValues, "set");
      for (const {needleRegex, pathSegments, stackNeedles} of overrideVals) {
        if (isMatchingValue(val, needleRegex, pathSegments) &&
            matchesStackTrace(stackNeedles, debugLog)) {
          debugLog("success", `Map.set is ignored for value matching needle: ${needleRegex}\nFILTER: map-override ${formattedArgsToLog}`);
          sendHitOnce("map-override " + formattedArgsToLog);
          return this; // Return the Map instance as per spec
        }
      }
      return apply(set, this, arguments);
    });
    proxyToStringCalls(wrappedSet, set);
    Object.defineProperty(window.Map.prototype, "set", {
      value: wrappedSet
    });
    debugLog("info", "Wrapped Map.prototype.set");
    end();
  }
  // Map.prototype.get
  else if (method === "get" && !mapValues.has("get")) {
    mark();
    call(set, mapValues, "get", $([]));

    let wrappedGet = proxy(get, function(key) {
      const overrideVals = call(get, mapValues, "get");
      for (const {needleRegex, retVal, stackNeedles} of overrideVals) {
        if (typeof key === "string" || typeof key === "number") {
          const keyStr = key.toString();
          if (needleRegex.test(keyStr) &&
              matchesStackTrace(stackNeedles, debugLog)) {
            debugLog("success", `Map.get returned ${retVal} for key: ${keyStr}\nFILTER: map-override ${formattedArgsToLog}`);
            sendHitOnce("map-override " + formattedArgsToLog);
            return retVal;
          }
        }
      }
      return apply(get, this, arguments);
    });
    proxyToStringCalls(wrappedGet, get);
    Object.defineProperty(window.Map.prototype, "get", {
      value: wrappedGet
    });
    debugLog("info", "Wrapped Map.prototype.get");
    end();
  }
  // Map.prototype.has
  else if (method === "has" && !mapValues.has("has")) {
    mark();
    call(set, mapValues, "has", $([]));

    let wrappedHas = proxy(has, function(key) {
      const overrideVals = call(get, mapValues, "has");
      for (const {needleRegex, retVal, stackNeedles} of overrideVals) {
        if (typeof key === "string" || typeof key === "number") {
          const keyStr = key.toString();
          if (needleRegex.test(keyStr) &&
              matchesStackTrace(stackNeedles, debugLog)) {
            debugLog("success", `Map.has returned ${retVal} for key: ${keyStr}\nFILTER: map-override ${formattedArgsToLog}`);
            sendHitOnce("map-override " + formattedArgsToLog);
            return retVal;
          }
        }
      }
      return apply(has, this, arguments);
    });
    proxyToStringCalls(wrappedHas, has);
    Object.defineProperty(window.Map.prototype, "has", {
      value: wrappedHas
    });
    debugLog("info", "Wrapped Map.prototype.has");
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

  const overrideVals = call(get, mapValues, method);

  let retVal;
  if (method === "get") {
    // For get, return the specified value or undefined
    retVal = returnValue === "" ? void 0 : returnValue;
  }
  else if (method === "has") {
    // For has, return boolean
    retVal = returnValue === "true";
  }

  overrideVals.push({needleRegex, retVal, pathSegments, stackNeedles});
  call(set, mapValues, method, overrideVals);
}
