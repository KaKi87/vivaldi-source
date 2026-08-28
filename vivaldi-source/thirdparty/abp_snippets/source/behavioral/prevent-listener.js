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

import {debug} from "../introspection/debug.js";
import {
  formatArguments, sendSnippetHitEvent, toRegExp
} from "../utils/general.js";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {proxyToStringCalls} from "../utils/toString.js";

let {Error, Map, Object, console} = $(window);

let {toString} = Function.prototype;
let EventTargetProto = EventTarget.prototype;
let {addEventListener} = EventTargetProto;

// will be a Map of all events, once the snippet is used at least once
let events = null;
const hitFilters = new Set();

/**
 * @description Prevents adding event listeners.
 * @memberof module:snippets/behavioral
 *
 * @param {string} event Pattern that matches the type(s) of event
 * we want to prevent. If the string starts and ends with a slash (`/`),
 * the text in between is treated as a regular expression.
 * @param {?string} eventHandler Pattern that matches the event handler's
 * declaration. If the string starts and ends with a slash (`/`),
 * the text in between is treated as a regular expression.
 * @param {?string} selector The CSS selector that the event target must match.
 * If the event target is not an HTML element the event handler is added.
 * @example
 * prevent-listener click  console => No listener will be added
 * for click events who's handler matches console:
 *  Won't be added:
 *    someElement.addEventListener("click", () => console.log("click"))
 *  Will be added:
 *    someElement.addEventListener("click", () => alert("click"))
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/69960109/prevent-listener} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/behavioral-snippets/prevent-listener} for external documentation.
 * @since Adblock Plus 3.11.2
 */
export function preventListener(event, eventHandler, selector) {
  if (!event)
    throw new Error("[prevent-listener snippet]: No event type.");

  if (!events) {
    events = new Map();

    let debugLog = getDebugger("[prevent]");
    const {mark, end} = profile("prevent-listener");

    let wrappedAddEventListener = proxy(
      addEventListener,
      function(type, listener) {
        mark();
        for (let {evt, handlers, selectors, formattedArgs} of events.values()) {
          // bail out ASAP if current type doesn't match
          if (!evt.test(type))
            continue;

          let isElement = this instanceof Element;

          // check every possible handler and selector per same event type
          for (let i = 0; i < handlers.length; i++) {
            const handler = handlers[i];
            const sel = selectors[i];

            // If we have a selcetor and we don't match an element,
            // we don't prevent the event from being added.
            if (sel && !(isElement && $(this).matches(sel)))
              continue;

            if (handler) {
              const proxiedHandlerMatch = function() {
                try {
                  const proxiedHandlerString = call(
                    toString,
                    typeof listener === "function" ?
                      listener : listener.handleEvent
                  );
                  return handler.test(proxiedHandlerString);
                }
                catch (e) {
                  debugLog("error",
                           "Error while trying to stringify listener: ",
                           e);
                  return false;
                }
              };

              const actualHandlerMatch = function() {
                try {
                  const actualHandlerString = String(
                    typeof listener === "function" ?
                      listener : listener.handleEvent
                  );
                  return handler.test(actualHandlerString);
                }
                catch (e) {
                  debugLog("error",
                           "Error while trying to stringify listener: ",
                           e);
                  return false;
                }
              };

              // If an eventHandler is provided and we don't find a match,
              // we don't prevent the event from being added.
              if (!proxiedHandlerMatch() && !actualHandlerMatch())
                continue;
            }

            const filter =
              "prevent-listener " + formattedArgs;
            if (!hitFilters.has(filter)) {
              hitFilters.add(filter);
              sendSnippetHitEvent(filter);
            }
            if (debug()) {
              console.groupCollapsed("DEBUG [prevent] was successful", `\nFILTER: prevent-listener ${formattedArgs}`);
              debugLog("success", `type: ${type} matching ${evt}`);
              debugLog("success", "handler:", listener);
              if (handler)
                debugLog("success", `matching ${handler}`);
              if (sel)
                debugLog("success", "on element: ", this, ` matching ${sel}`);
              debugLog("success", "was prevented from being added");
              console.groupEnd();
            }
            return;
          }
        }
        end();
        return apply(addEventListener, this, arguments);
      }
    );
    proxyToStringCalls(wrappedAddEventListener, addEventListener);
    Object.defineProperty(EventTargetProto, "addEventListener", {
      value: wrappedAddEventListener
    });

    debugLog("info", "Wrapped addEventListener");
  }
  // Map each argument to a string format for logging purposes
  const formattedArgsToLog = formatArguments(arguments);

  if (!events.has(event)) {
    events.set(event,
               {evt: toRegExp(event),
                handlers: [],
                selectors: [],
                formattedArgs: formattedArgsToLog});
  }

  let {handlers, selectors} = events.get(event);

  handlers.push(eventHandler ? toRegExp(eventHandler) : null);
  selectors.push(selector);
}
