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

const {Function, Object, WeakMap} = $(window);

let toStringProxied = false;
const nativeFns = new WeakMap();

function proxyToString() {
  const {toString} = Function.prototype;

  const wrappedToString = proxy(toString, function() {
    const native = nativeFns.get(this);
    if (typeof native !== "undefined")
      return apply(toString, native, arguments);
    return apply(toString, this, arguments);
  });

  Object.defineProperty(window.Function.prototype, "toString", {
    value: wrappedToString
  });

  nativeFns.set(wrappedToString, toString);

  toStringProxied = true;
}

/**
 * Register a wrapper function so it appears native via
 * Function.prototype.toString.
 * @param {Function} wrapped   The wrapper / proxy function
 * @param {Function} native    The original native function to impersonate
 */
export function proxyToStringCalls(wrapped, native) {
  if (!toStringProxied)
    proxyToString();

  nativeFns.set(wrapped, native);
}
