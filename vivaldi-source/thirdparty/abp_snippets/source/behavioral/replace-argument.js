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

import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {matchesStackTrace, overrideValue} from "../utils/execution.js";
import {proxyToStringCalls} from "../utils/toString.js";

let {Array, Map, Object, parseInt, RegExp, Set} = $(window);

// methodPath -> [{argPosition, search, replacement, wholeValue, filterStr,
//                 stackNeedles}]
const rulesByMethod = new Map();
// methodPaths whose native method has already been wrapped once
const patchedMethods = new Set();
// dedupe hit events per filter
const hitFilters = new Set();

// Always replace every occurrence, not just the first.
function toGlobalRegExp(pattern) {
  const base = toRegExp(pattern);
  return new RegExp(base.source, base.flags + "g");
}

/**
 * @description Wraps a native method and rewrites one of its call arguments
 * before the method runs. It has two modes on a single rule:
 *
 * - Substitution (pattern non-empty): replaces text matching the pattern
 *     inside the stringified argument, keeping the rest. Object and array
 *     arguments are left untouched in this mode. Use whole-value
 *     mode for those instead.
 * - Whole-value (pattern empty): replaces the whole argument with a typed value
 *     drawn from overrideValue's vocabulary (false, true, null,
 *     undefined, noopFunc, trueFunc, falseFunc, emptyArray, emptyObj, falseStr,
 *     trueStr, a positive integer), otherwise a literal string.
 *
 * On the first rule for a given method the method is resolved from the page
 * global and wrapped once with a Proxy; further filters targeting the same
 * method append rules. On each call the first rule that actually changes the
 * target argument wins, the original method is always invoked with the modified
 * arguments, and the wrapper never throws.
 * @memberof module:snippets/behavioral
 *
 * @param {string} methodPath Dotted path to the native method,
 *   Must resolve to a function.
 * @param {string} argPosition 0-based index of the argument to rewrite.
 * @param {?string} pattern `/regex/` or literal text, matched against
 *   the stringified argument. An empty string selects
 *   whole-value mode. Substitution always replaces every match.
 * @param {?string} replacement In substitution mode, the replacement text.
 *   In whole-value mode, a value token from overrideValue's vocabulary
 *   injected as a real typed value, otherwise a literal string.
 * @param {?string} [stackNeedle=""] A comma-separated list of strings or
 *   regex which must be present in the callstack for the rule to apply.
 * @example
 * example.com#$#replace-argument adConfig.setEnabled 0 '' false
 *
 * @since Adblock Plus X.X.X
 */
export function replaceArgument(methodPath, argPosition,
                                pattern = "", replacement = "",
                                stackNeedle = "") {
  const debugLog = getDebugger("[replace-argument snippet]");
  const formattedArguments = formatArguments(arguments);
  const filterStr = "replace-argument " + formattedArguments;
  const {mark, end} = profile("replace-argument");

  if (!methodPath || typeof methodPath !== "string") {
    debugLog("error", `methodPath param must be a string.\nFILTER: ${filterStr}`);
    return;
  }
  const posStr = "" + argPosition;
  if (!(/^\d+$/.test(posStr))) {
    debugLog(
      "error",
      `argPosition param must be a non-negative integer.\nFILTER: ${filterStr}`
    );
    return;
  }
  const position = parseInt(posStr, 10);

  const parts = $(methodPath).split(".");
  const method = parts[parts.length - 1];
  let parent = window;
  for (let i = 0; i < parts.length - 1; i++) {
    if (parent == null)
      break;
    parent = parent[parts[i]];
  }

  if (parent == null || typeof parent[method] !== "function") {
    debugLog("warn", `could not resolve ${methodPath}\nFILTER: ${filterStr}`);
    return;
  }

  const stackNeedles = stackNeedle ?
    $(stackNeedle).split(",").map(s => s.trim()) : [];

  const wholeValue = pattern === "";
  const rule = {
    argPosition: position,
    search: wholeValue ? null : toGlobalRegExp(pattern),
    replacement,
    wholeValue,
    filterStr,
    stackNeedles
  };

  let rules = rulesByMethod.get(methodPath);
  if (!rules) {
    rules = new Array();
    rulesByMethod.set(methodPath, rules);
  }
  rules.push(rule);
  debugLog("info", `Added rule for ${methodPath}\nFILTER: ${filterStr}`);

  if (!patchedMethods.has(methodPath)) {
    mark();
    patchedMethods.add(methodPath);

    const nativeMethod = parent[method];
    const wrappedMethod = proxy(nativeMethod, function() {
      let applyArgs = arguments;
      try {
        const methodRules = rulesByMethod.get(methodPath);
        if (methodRules) {
          for (const thisRule of methodRules) {
            if (arguments.length <= thisRule.argPosition ||
                !matchesStackTrace(thisRule.stackNeedles, debugLog))
              continue;

            const original = arguments[thisRule.argPosition];
            let replaced;
            if (thisRule.wholeValue) {
              replaced = overrideValue(thisRule.replacement);
            }
            else {
              // Substitution mode operates on text. An object/array
              // argument can't be safely rewritten by regex and reassembled
              // without risking a type mismatch at the call site: use
              // whole-value mode for those instead.
              if (original !== null && typeof original === "object")
                continue;

              const originalAsStr = "" + original;
              replaced = $(originalAsStr)
                .replace(thisRule.search, thisRule.replacement).toString();
              // Self-gating: no substitution happened, leave this rule be.
              if (replaced === originalAsStr)
                continue;
            }

            const newArgs = Array.from(arguments);
            newArgs[thisRule.argPosition] = replaced;
            if (!hitFilters.has(thisRule.filterStr)) {
              hitFilters.add(thisRule.filterStr);
              sendSnippetHitEvent(thisRule.filterStr);
            }
            debugLog(
              "success",
              `argument ${thisRule.argPosition} of ${methodPath} replaced` +
              `\nFILTER: ${thisRule.filterStr}`
            );
            applyArgs = newArgs;
            // First rule that actually changes the argument wins.
            break;
          }
        }
      }
      catch (e) {
        // Never break the page: fall through to the untouched call.
        applyArgs = arguments;
      }
      return apply(nativeMethod, this, applyArgs);
    });
    proxyToStringCalls(wrappedMethod, nativeMethod);
    Object.defineProperty(parent, method, {value: wrappedMethod});
    debugLog("info", `${methodPath} wrapped`);
    end();
  }
}
