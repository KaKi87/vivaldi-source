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

import {findOwner} from "../utils/execution.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {
  formatArguments, sendSnippetHitEvent
} from "../utils/general.js";
import {matchesStackTrace} from "../utils/execution.js";
import {JSONPath} from "../utils/jsonpath.js";
import {proxyToStringCalls} from "../utils/toString.js";

let {Array, Error, JSON, Map, Object, Response} = $(window);

// will be a Map of all paths, once the snippet is used at least once
let paths = null;
const hitFilters = new Set();
function sendHitOnce(filter) {
  if (!hitFilters.has(filter)) {
    hitFilters.add(filter);
    sendSnippetHitEvent(filter);
  }
}

/**
 * @description Traps calls to JSON.parse, and if the result
 * of the parsing is an Object, it will remove specified
 * properties from the result before returning to the caller.
 *
 * The idea originates from
 * [uBlock Origin](https://github.com/gorhill/uBlock/commit/2fd86a66).
 * @memberof module:snippets/behavioral
 *
 * @param {string} rawPrunePaths A list of space-separated properties to remove.
 *  Can include placeholders {} and [] to iterate over nested objects and arrays
 *  respectively.
 * @param {?string} [rawNeedlePaths] A list of space-separated properties which
 *   must be all present for the pruning to occur.
 * @param {?string} [rawNeedleStack] A list of space-separated strings or regex
 *   which must be present in the callstack for the pruning to occur.
 * @example
 * json-prune ads userId => Removes any ads property (if existent)
 * from every object that is parsed with JSON.parse and
 * has an userId property.
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/69960013/json-prune} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/behavioral-snippets/json-prune} for external documentation.
 * @since Adblock Plus 3.9.0
 */
export function jsonPrune(rawPrunePaths,
                          rawNeedlePaths = "",
                          rawNeedleStack = "") {
  if (!rawPrunePaths)
    throw new Error("Missing paths to prune");

  let debugLog = getDebugger("json-prune");
  const {mark, end} = profile("json-prune");

  if (!paths) {
    mark();
    function pruneObject(obj) {
      for (let {prune,
                needle,
                jsonPathObjects,
                stackNeedle,
                formattedArgs} of paths.values()) {
        // Check if needle paths are present
        if ($(needle).length > 0 &&
          $(needle).some(path => !findOwner(obj, path)))
          return obj;

        // Check if the call stack matches the rawNeedleStack condition
        if ($(stackNeedle) &&
            $(stackNeedle).length > 0 &&
            !matchesStackTrace(stackNeedle, debugLog))
          return obj;

        for (let path of prune) {
          if (path.startsWith("jsonpath(")) {
            try {
              const engine = jsonPathObjects.get(path);
              const matches = engine.evaluate(obj);
              matches.forEach(({parent, key}) => {
                debugLog("success", `JSONPath match found and deleted at [${key}]`, `\nFILTER: json-prune ${formattedArgs}`);
                sendHitOnce("json-prune " + formattedArgs);
                delete parent[key];
              });
            }
            catch (e) {
              debugLog("error", `JSONPath evaluation failed for: ${path}. Error: ${e.message}`);
            }
          }
          else if (path.includes("{}") || path.includes("[]") ||
              path.includes("{-}") || path.includes("[-]")) {
            prunePathWithPlaceholders(obj, path, formattedArgs);
          }
          else {
            prunePathSimple(obj, path, formattedArgs);
          }
        }
      }
      return obj;
    }

    function prunePathWithPlaceholders(obj, path, formattedArgs) {
      let pathParts = $(path).split(".");
      let currentObj = obj;

      for (let i = 0; i < pathParts.length; i++) {
        let part = pathParts[i];

        if (part === "[]") {
          if (Array.isArray(currentObj)) {
            debugLog("info", `Iterating over array at: ${part}`);
            $(currentObj).forEach(item =>
              prunePathWithPlaceholders(item,
                                        pathParts.slice(i + 1).join("."),
                                        formattedArgs));
          }
          return;
        }
        else if (part === "[-]") {
          if (Array.isArray(currentObj)) {
            debugLog("info", `Iterating over array with element removal at: ${part}`);
            let remainingPath = pathParts.slice(i + 1).join(".");
            let indicesToRemove = [];

            $(currentObj).forEach((item, index) => {
              if (shouldRemoveElement(item, remainingPath))
                indicesToRemove.push(index);
            });

            // Remove elements in reverse order to maintain correct indices
            for (let j = indicesToRemove.length - 1; j >= 0; j--) {
              debugLog("success", `Found element at index ${indicesToRemove[j]} matching ${remainingPath} and removed entire element, \nFILTER: json-prune ${formattedArgs}`);
              sendHitOnce("json-prune " + formattedArgs);
              currentObj.splice(indicesToRemove[j], 1);
            }
          }
          return;
        }
        else if (part === "{}") {
          if (typeof currentObj === "object" && currentObj !== null) {
            debugLog("info", `Iterating over object at: ${part}`);
            Object.keys(currentObj).forEach(key =>
              prunePathWithPlaceholders(currentObj[key],
                                        pathParts.slice(i + 1).join("."),
                                        formattedArgs));
          }
          return;
        }
        else if (part === "{-}") {
          if (typeof currentObj === "object" && currentObj !== null) {
            debugLog("info", `Iterating over object with element removal at: ${part}`);
            let remainingPath = pathParts.slice(i + 1).join(".");
            let keysToRemove = [];

            Object.keys(currentObj).forEach(key => {
              if (shouldRemoveElement(currentObj[key], remainingPath))
                keysToRemove.push(key);
            });

            // Remove keys
            keysToRemove.forEach(key => {
              debugLog("success", `Found object key ${key} matching ${remainingPath} and removed entire element, \nFILTER: json-prune ${formattedArgs}`);
              sendHitOnce("json-prune " + formattedArgs);
              delete currentObj[key];
            });
          }
          return;
        }
        else if (currentObj && typeof currentObj === "object" &&
          hasOwnProperty(currentObj, part)) {
          if (i === pathParts.length - 1) {
            debugLog("success", `Found ${path} and deleted, \nFILTER: json-prune ${formattedArgs}`);
            sendHitOnce("json-prune " + formattedArgs);
            delete currentObj[part];
          }
          else {
            currentObj = currentObj[part];
          }
        }
        else {
          return;
        }
      }
    }

    function shouldRemoveElement(obj, path) {
      if (!path || path === "")
        return true;

      let pathParts = $(path).split(".");
      let currentObj = obj;

      for (let i = 0; i < pathParts.length; i++) {
        let part = pathParts[i];

        if (part === "[]") {
          if (Array.isArray(currentObj)) {
            return $(currentObj).some(item =>
              shouldRemoveElement(item, pathParts.slice(i + 1).join(".")));
          }
          return false;
        }
        else if (part === "{}") {
          if (typeof currentObj === "object" && currentObj !== null) {
            return Object.keys(currentObj).some(key =>
              shouldRemoveElement(currentObj[key],
                                  pathParts.slice(i + 1).join(".")));
          }
          return false;
        }
        else if (currentObj && typeof currentObj === "object" &&
          hasOwnProperty(currentObj, part)) {
          if (i === pathParts.length - 1)
            return true; // Found the property to check
          currentObj = currentObj[part];
        }
        else {
          return false;
        }
      }

      return false;
    }

    function prunePathSimple(obj, path, formattedArgs) {
      let details = findOwner(obj, path);
      if (typeof details != "undefined") {
        debugLog("success", `Found ${path} and deleted`, `\nFILTER: json-prune ${formattedArgs}`);
        sendHitOnce("json-prune " + formattedArgs);
        delete details[0][details[1]];
      }
    }

    // allow both jsonPrune and jsonOverride to work together
    let {parse} = JSON;
    paths = new Map();

    let wrappedParse = proxy(parse, function() {
      let result = apply(parse, this, arguments);
      return pruneObject(result);
    });
    proxyToStringCalls(wrappedParse, parse);
    Object.defineProperty(window.JSON, "parse", {
      value: wrappedParse
    });
    debugLog("info", "Wrapped JSON.parse for prune");

    let {json} = Response.prototype;
    let wrappedJson = proxy(json, function() {
      let resultPromise = apply(json, this, arguments);
      return resultPromise.then(obj => pruneObject(obj));
    });
    proxyToStringCalls(wrappedJson, json);
    Object.defineProperty(window.Response.prototype, "json", {
      value: wrappedJson
    });
    debugLog("info", "Wrapped Response.json for prune");
    end();
  }

  const formattedArgs = formatArguments(arguments);
  // allow a single unique rawPrunePaths definition per domain
  // TBD: should we throw an error if it was already set?

  const pruneList = $(rawPrunePaths).split(/ +/);
  const jsonPathObjects = new Map();
  for (const p of pruneList) {
    if (p.startsWith("jsonpath(")) {
      try {
        jsonPathObjects.set(p, new JSONPath(p.slice(9, -1)));
      }
      catch (e) {
        debugLog("error", `Invalid JSONPath query: ${p}. Error: ${e.message}`);
      }
    }
  }

  paths.set(rawPrunePaths, {
    formattedArgs,
    prune: pruneList,
    jsonPathObjects,
    needle: rawNeedlePaths.length ? $(rawNeedlePaths).split(/ +/) : [],
    stackNeedle: rawNeedleStack.length ? $(rawNeedleStack).split(/ +/) : []
  });
}
