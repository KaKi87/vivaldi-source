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
import {apply, proxy, call} from "proxy-pants/function";

import {getDebugger} from "../introspection/log.js";
import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {profile} from "../introspection/profile.js";
import {matchesStackTrace} from "../utils/execution.js";
import {proxyToStringCalls} from "../utils/toString.js";

const {Error, Object, parseInt, isNaN} = $(window);

const {toString} = Function.prototype;

const origSetTimeout = window.setTimeout;
const origSetInterval = window.setInterval;

const MODES = {
  TIMEOUT: "timeout",
  INTERVAL: "interval",
  BOTH: "both"
};

let timerOverrides = null;
const hitFilters = new Set();

/**
 * @description Overrides setTimeout and/or setInterval
 * to change their delay duration or replace their callback
 * with a no-op.
 * @memberof module:snippets/behavioral
 *
 * @param {string} timerValue New timer delay duration in ms.
 * @param {?string} [needle] Override only happens if this
 *   text or regex is found in the callback function or the
 *   original delay value. If the string starts and ends with
 *   a slash (`/`), it is treated as a regular expression.
 * @param {?string} [callbackFunc] If "noop", replaces the
 *   callback with `() => {}`.
 * @param {?string} [mode="both"] Which timer API to
 *   override. Accepts: "timeout", "interval", "both".
 * @param {?string} [stackNeedle] A list of space-separated
 *   strings or regex which must be present in the callstack
 *   for the override to occur.
 *
 * @example
 * timer-override 1 5000 => setTimeout/setInterval with
 * delay 5000 finishes in 1ms instead
 *
 * @example
 * timer-override 1 preventAdsFromClosing => Override
 * timers containing "preventAdsFromClosing" in callback
 *
 */
export function timerOverride(timerValue,
                              needle = "",
                              callbackFunc = "",
                              mode = MODES.BOTH,
                              stackNeedle = "") {
  if (!timerValue) {
    throw new Error(
      "[timer-override snippet]: " +
      "Missing required parameter timerValue."
    );
  }

  if (!Object.values(MODES).includes(mode)) {
    throw new Error(
      "[timer-override snippet]: " +
      "Invalid mode. Acceptable values are: " +
      Object.values(MODES).join(", ")
    );
  }

  const newDelay = parseInt(timerValue, 10);
  if (isNaN(newDelay)) {
    throw new Error(
      "[timer-override snippet]: " +
      "timerValue must be a number."
    );
  }

  if (!timerOverrides) {
    timerOverrides = $([]);

    const debugLog = getDebugger("timer-override");
    const {mark, end} = profile("timer-override");
    mark();

    function getCbStr(callback) {
      try {
        if (typeof callback === "function")
          return call(toString, callback);
        return "" + callback;
      }
      catch (e) {
        return "";
      }
    }

    function handleTimer(ctx, origFn, apiName,
                         modes, callback,
                         delay, args) {
      const cbStr = getCbStr(callback);
      // If multiple filters target the same callback
      // first matching config wins
      for (const config of timerOverrides) {
        if (modes.indexOf(config.mode) < 0)
          continue;

        if (config.needleRegex) {
          const delayStr = "" + delay;
          if (!config.needleRegex.test(cbStr) &&
              !config.needleRegex.test(delayStr))
            continue;

          debugLog(
            "info",
            config.needle +
            " found in " + cbStr
          );
        }

        if (config.stackNeedles.length > 0 &&
            !matchesStackTrace(
              config.stackNeedles, debugLog
            ))
          continue;

        let finalCb = callback;
        const finalDelay = config.newDelay;

        if (config.isNoop) {
          finalCb = () => {};
          debugLog(
            "success",
            "Callback replaced with noop for " +
            cbStr
          );
        }

        debugLog(
          "success",
          apiName + " replaced with " +
          finalDelay + " for " + cbStr
        );
        const filter =
          "timer-override " + config.formattedArgs;
        if (!hitFilters.has(filter)) {
          hitFilters.add(filter);
          sendSnippetHitEvent(filter);
        }

        const newArgs = $([finalCb, finalDelay]);
        for (let i = 2; i < args.length; i++)
          newArgs.push(args[i]);
        return apply(origFn, ctx, newArgs);
      }
      return null;
    }

    const timeoutModes = $([MODES.TIMEOUT, MODES.BOTH]);
    let wrappedSetTimeout = proxy(origSetTimeout, function(cb, dl) {
      const r = handleTimer(
        this,
        origSetTimeout,
        "setTimeout",
        timeoutModes,
        cb,
        dl,
        arguments
      );
      if (r !== null)
        return r;
      return apply(origSetTimeout, this, arguments);
    });
    proxyToStringCalls(wrappedSetTimeout, origSetTimeout);
    Object.defineProperty(window, "setTimeout", {
      value: wrappedSetTimeout
    });

    const intervalModes = $([MODES.INTERVAL, MODES.BOTH]);
    let wrappedSetInterval = proxy(origSetInterval, function(cb, dl) {
      const r = handleTimer(
        this,
        origSetInterval,
        "setInterval",
        intervalModes,
        cb,
        dl,
        arguments
      );
      if (r !== null)
        return r;
      return apply(origSetInterval, this, arguments);
    });
    proxyToStringCalls(wrappedSetInterval, origSetInterval);
    Object.defineProperty(window, "setInterval", {
      value: wrappedSetInterval
    });

    debugLog("info", "timer APIs proxied");
    end();
  }



  let stackNeedles = [];
  if (stackNeedle)
    stackNeedles = stackNeedle.split(/ +/);

  timerOverrides.push({
    newDelay,
    needle,
    needleRegex: needle ? toRegExp(needle) : null,
    mode,
    isNoop: callbackFunc === "noop",
    stackNeedles,
    formattedArgs: formatArguments(arguments)
  });
}
