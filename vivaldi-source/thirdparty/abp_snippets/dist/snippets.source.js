(environment, ...filters) => {
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
   */const $$1 = Proxy;

  const {apply: a, bind: b, call: c} = Function;
  const apply$2 = c.bind(a);
  const bind = c.bind(b);
  const call = c.bind(c);

  const callerHandler = {
    get(target, name) {
      return bind(c, target[name]);
    }
  };
  const caller = target => new $$1(target, callerHandler);

  const proxy = (source, target) => new $$1(source, {
    apply: (_, self, args) => apply$2(target, self, args)
  });

  const handler$2 = {
    get(target, name) {
      return bind(target[name], target);
    }
  };
  const bound = target => new $$1(target, handler$2);

  const {
    assign: assign$1,
    defineProperties: defineProperties$1,
    freeze: freeze$1,
    getOwnPropertyDescriptor: getOwnPropertyDescriptor$3,
    getOwnPropertyDescriptors: getOwnPropertyDescriptors$1,
    getPrototypeOf
  } = bound(Object);

  const {hasOwnProperty} = caller({});

  const {species} = Symbol;

  const handler$1 = {
    get(target, name) {
      const Native = target[name];
      class Secure extends Native {}

      const proto = getOwnPropertyDescriptors$1(Native.prototype);
      delete proto.constructor;
      freeze$1(defineProperties$1(Secure.prototype, proto));

      const statics = getOwnPropertyDescriptors$1(Native);
      delete statics.length;
      delete statics.prototype;
      statics[species] = {value: Secure};
      return freeze$1(defineProperties$1(Secure, statics));
    }
  };

  const secure = target => new $$1(target, handler$1);

  if (typeof currentEnvironment !== "undefined" &&
      currentEnvironment.initial &&
      typeof environment !== "undefined")
    currentEnvironment = environment;

  const getLibEnvironment = () => {
    if (typeof currentEnvironment !== "undefined")
      return currentEnvironment;
    if (typeof environment !== "undefined")
      return environment;
    return {};
  };

  if (typeof globalThis === "undefined")
    window.globalThis = window;

  const {apply: apply$1, ownKeys} = bound(Reflect);
  const libEnvironment = getLibEnvironment();
  const worldEnvDefined = "world" in libEnvironment;
  const isIsolatedWorld = worldEnvDefined && libEnvironment.world === "ISOLATED";
  const isMainWorld = worldEnvDefined && libEnvironment.world === "MAIN";

  const chromeObjAvailable = typeof chrome === "object" && !!chrome.runtime;
  const browserObjAvailable = typeof browser === "object" && !!browser.runtime;
  const isExtensionContext$2 = !isMainWorld &&
    (isIsolatedWorld || chromeObjAvailable || browserObjAvailable);
  const copyIfExtension = value => isExtensionContext$2 ?
    value :
    create(value, getOwnPropertyDescriptors(value));

  const {
    create,
    defineProperties,
    defineProperty,
    freeze,
    getOwnPropertyDescriptor: getOwnPropertyDescriptor$2,
    getOwnPropertyDescriptors
  } = bound(Object);

  const invokes = bound(globalThis);
  const classes = isExtensionContext$2 ? globalThis : secure(globalThis);
  const {Map: Map$i, RegExp: RegExp$6, Set: Set$c, WeakMap: WeakMap$8, WeakSet: WeakSet$f} = classes;

  const augment = (source, target, method = null) => {
    const known = ownKeys(target);
    for (const key of ownKeys(source)) {
      if (known.includes(key))
        continue;

      const descriptor = getOwnPropertyDescriptor$2(source, key);
      if (method && "value" in descriptor) {
        const {value} = descriptor;
        if (typeof value === "function")
          descriptor.value = method(value);
      }
      defineProperty(target, key, descriptor);
    }
  };

  const primitive = name => {
    const Super = classes[name];
    class Class extends Super {}
    const {toString, valueOf} = Super.prototype;
    defineProperties(Class.prototype, {
      toString: {value: toString},
      valueOf: {value: valueOf}
    });
    const type = name.toLowerCase();
    const method = callback => function() {
      const result = apply$1(callback, this, arguments);
      return typeof result === type ? new Class(result) : result;
    };
    augment(Super, Class, method);
    augment(Super.prototype, Class.prototype, method);
    return Class;
  };

  const variables$3 = freeze({
    frozen: new WeakMap$8(),
    hidden: new WeakSet$f(),
    iframePropertiesToAbort: {
      read: new Set$c(),
      write: new Set$c()
    },
    abortedIframes: new WeakMap$8()
  });

  const startsCapitalized = new RegExp$6("^[A-Z]");
  const extensionApi = (
    isExtensionContext$2 && (
      (chromeObjAvailable && chrome) ||
      (browserObjAvailable && browser)
    )
  ) || void 0;

  var env = new Proxy(new Map$i([

    ["chrome", extensionApi],
    ["browser", extensionApi],
    ["isExtensionContext", isExtensionContext$2],
    ["variables", variables$3],

    ["console", copyIfExtension(console)],
    ["document", globalThis.document],
    ["JSON", copyIfExtension(JSON)],
    ["Map", Map$i],
    ["Math", copyIfExtension(Math)],
    ["Number", isExtensionContext$2 ? Number : primitive("Number")],
    ["RegExp", RegExp$6],
    ["Set", Set$c],
    ["String", isExtensionContext$2 ? String : primitive("String")],
    ["WeakMap", WeakMap$8],
    ["WeakSet", WeakSet$f],

    ["MouseEvent", MouseEvent]
  ]), {
    get(map, key) {
      if (map.has(key))
        return map.get(key);

      let value = globalThis[key];
      if (typeof value === "function")
        value = (startsCapitalized.test(key) ? classes : invokes)[key];

      map.set(key, value);
      return value;
    },
    has(map, key) {
      return map.has(key);
    }
  });

  class WeakValue {
    has() { return false; }
    set() {}
  }

  const helpers = {WeakSet, WeakMap, WeakValue};
  const {apply} = Reflect;

  function transformOnce (callback) {  const {WeakSet, WeakMap, WeakValue} = (this || helpers);
    const ws = new WeakSet;
    const wm = new WeakMap;
    const wv = new WeakValue;
    return function (any) {
      if (ws.has(any))
        return any;

      if (wm.has(any))
        return wm.get(any);

      if (wv.has(any))
        return wv.get(any);

      const value = apply(callback, this, arguments);
      ws.add(value);
      if (value !== any)
        (typeof any === 'object' && any ? wm : wv).set(any, value);
      return value;
    };
  }

  const {Map: Map$h, WeakMap: WeakMap$7, WeakSet: WeakSet$e, setTimeout: setTimeout$5} = env;

  let cleanup = true;
  let cleanUpCallback = map => {
    map.clear();
    cleanup = !cleanup;
  };

  var transformer = transformOnce.bind({
    WeakMap: WeakMap$7,
    WeakSet: WeakSet$e,

    WeakValue: class extends Map$h {
      set(key, value) {
        if (cleanup) {
          cleanup = !cleanup;
          setTimeout$5(cleanUpCallback, 0, this);
        }
        return super.set(key, value);
      }
    }
  });

  const {concat, includes, join, reduce, unshift} = caller([]);

  const {Map: Map$g, WeakMap: WeakMap$6} = secure(globalThis);

  const map = new Map$g;
  const descriptors = target => {
    const chain = [];
    let current = target;
    while (current) {
      if (map.has(current))
        unshift(chain, map.get(current));
      else {
        const descriptors = getOwnPropertyDescriptors$1(current);
        map.set(current, descriptors);
        unshift(chain, descriptors);
      }
      current = getPrototypeOf(current);
    }
    unshift(chain, {});
    return apply$2(assign$1, null, chain);
  };

  const chain = source => {
    const target = typeof source === 'function' ? source.prototype : source;
    const chained = descriptors(target);
    const handler = {
      get(target, key) {
        if (key in chained) {
          const {value, get} = chained[key];
          if (get)
            return call(get, target);
          if (typeof value === 'function')
            return bind(value, target);
        }
        return target[key];
      },
      set(target, key, value) {
        if (key in chained) {
          const {set} = chained[key];
          if (set) {
            call(set, target, value);
            return true;
          }
        }
        target[key] = value;
        return true;
      }
    };
    return target => new Proxy(target, handler);
  };

  const {
    isExtensionContext: isExtensionContext$1,
    Array: Array$e,
    Number: Number$1,
    String: String$1,
    Object: Object$p
  } = env;

  const {isArray} = Array$e;
  const {getOwnPropertyDescriptor: getOwnPropertyDescriptor$1, setPrototypeOf: setPrototypeOf$1} = Object$p;

  const {toString: toString$2} = Object$p.prototype;
  const {slice} = String$1.prototype;
  const getBrand = value => call(slice, call(toString$2, value), 8, -1);

  const {get: nodeType} = getOwnPropertyDescriptor$1(Node.prototype, "nodeType");

  const chained = isExtensionContext$1 ? {} : {
    Attr: chain(Attr),
    CanvasRenderingContext2D: chain(CanvasRenderingContext2D),
    CSSStyleDeclaration: chain(CSSStyleDeclaration),
    Document: chain(Document),
    Element: chain(Element),
    HTMLCanvasElement: chain(HTMLCanvasElement),
    HTMLElement: chain(HTMLElement),
    HTMLImageElement: chain(HTMLImageElement),
    HTMLScriptElement: chain(HTMLScriptElement),
    MutationRecord: chain(MutationRecord),
    Node: chain(Node),
    ShadowRoot: chain(ShadowRoot),

    get CSS2Properties() {
      return chained.CSSStyleDeclaration;
    }
  };

  const upgrade = (value, hint) => {
    if (hint !== "Element" && hint in chained)
      return chained[hint](value);

    if (isArray(value))
      return setPrototypeOf$1(value, Array$e.prototype);

    const brand = getBrand(value);
    if (brand in chained)
      return chained[brand](value);

    if (brand in env)
      return setPrototypeOf$1(value, env[brand].prototype);

    if ("nodeType" in value) {
      switch (call(nodeType, value)) {
        case 1:
          if (!(hint in chained))
            throw new Error("unknown hint " + hint);
          return chained[hint](value);
        case 2:
          return chained.Attr(value);
        case 3:
          return chained.Node(value);
        case 9:
          return chained.Document(value);
      }
    }

    throw new Error("unknown brand " + brand);
  };

  var $ = isExtensionContext$1 ?
    value => (value === window || value === globalThis ? env : value) :
    transformer((value, hint = "Element") => {
      if (value === window || value === globalThis)
        return env;

      switch (typeof value) {
        case "object":
          return value && upgrade(value, hint);

        case "string":
          return new String$1(value);

        case "number":
          return new Number$1(value);

        default:
          throw new Error("unsupported value");
      }
    });

  const handler = {
    get(target, name) {
      const context = target;
      while (!hasOwnProperty(target, name))
        target = getPrototypeOf(target);
      const {get, set} = getOwnPropertyDescriptor$3(target, name);
      return function () {
        return arguments.length ?
                apply$2(set, context, arguments) :
                call(get, context);
      };
    }
  };

  const accessor = target => new $$1(target, handler);

  let {Math: Math$5, setInterval: setInterval$1, performance} = $(window);

  const noopProfile = {
    mark() {},
    end() {},
    toString() {
      return "{mark(){},end(){}}";
    }
  };

  let inactive = true;

  function setProfile() {
    inactive = false;
  }

  function profile(id, rate = 10) {
    if (inactive)
      return noopProfile;
    function processSamples() {
      let samples = $([]);

      for (let {name, duration} of performance.getEntriesByType("measure"))
        samples.push({name, duration});

      if (samples.length)
        performance.clearMeasures();
    }

    if (!profile[id]) {
      profile[id] = setInterval$1(processSamples,
                                Math$5.round(60000 / Math$5.min(60, rate)));
    }

    return {
      mark() {
        performance.mark(id);
      },
      end(clear = false) {
        performance.measure(id, id);
        const measures = performance.getEntriesByName(id, "measure");
        const measureObj = measures.length > 0 ?
                           measures[measures.length - 1] : null;
        console.log("PROFILER:", measureObj);
        performance.clearMarks(id);
        if (clear) {
          clearInterval(profile[id]);
          delete profile[id];
          processSamples();
        }
      }
    };
  }

  let {Array: Array$d, document: document$6, Math: Math$4, RegExp: RegExp$5} = $(window);

  function regexEscape(string) {
    return $(string).replace(/[-/\\^$*+?.()|[\]{}]/g, "\\$&");
  }

  function toRegExp(pattern) {
    let {length} = pattern;

    if (length > 1 && pattern[0] === "/") {
      let isCaseSensitive = pattern[length - 1] === "/";

      if (isCaseSensitive || (length > 2 && $(pattern).endsWith("/i"))) {
        let args = [$(pattern).slice(1, isCaseSensitive ? -1 : -2)];
        if (!isCaseSensitive)
          args.push("i");

        return new RegExp$5(...args);
      }
    }

    return new RegExp$5(regexEscape(pattern));
  }

  function sendDetectionEvent(type, specifier) {
    const env = getLibEnvironment();
    if (typeof env.sendDetectionEvent !== "function")
      return;
    try {
      env.sendDetectionEvent(type, document$6.location.hostname, specifier);
    }
    catch (e) {

    }
  }

  function sendSnippetHitEvent(filter) {
    const env = getLibEnvironment();
    if (typeof env.sendSnippetHitEvent !== "function")
      return;
    try {
      env.sendSnippetHitEvent(filter, document$6.location.hostname);
    }
    catch (e) {

    }
  }

  function randomId() {

    return $(Math$4.floor(Math$4.random() * 2116316160 + 60466176)).toString(36);
  }

  function formatArguments(args) {
    return $(Array$d.from(args)).map(arg => `'${arg}'`).join(" ");
  }

  function toHex(number, length = 2) {
    let hex = $(number).toString(16);

    if (hex.length < length)
      hex = $("0").repeat(length - hex.length) + hex;

    return hex;
  }

  function uint8ArrayToHex(uint8Array) {
    return uint8Array.reduce((hex, byte) => hex + toHex(byte), "");
  }

  let debugging = false;

  let filter = null;

  function debug() {
    return debugging;
  }

  function debugFilter() {
    return filter;
  }

  function setDebug(pattern) {
    debugging = true;
    if (pattern)
      filter = toRegExp(pattern);
  }

  const {console: console$4} = $(window);

  const noop = () => {};

  function log(...args) {
    let {mark, end} = profile("log");
    if (debug()) {
      const logArgs = ["%c DEBUG", "font-weight: bold;"];

      const isErrorIndex = args.indexOf("error");
      const isWarnIndex = args.indexOf("warn");
      const isSuccessIndex = args.indexOf("success");
      const isInfoIndex = args.indexOf("info");

      if (isErrorIndex !== -1) {
        logArgs[0] += " - ERROR";
        logArgs[1] += "color: red; border:2px solid red";
        $(args).splice(isErrorIndex, 1);
      }
      else if (isWarnIndex !== -1) {
        logArgs[0] += " - WARNING";
        logArgs[1] += "color: orange; border:2px solid orange ";
        $(args).splice(isWarnIndex, 1);
      }
      else if (isSuccessIndex !== -1) {
        logArgs[0] += " - SUCCESS";
        logArgs[1] += "color: green; border:2px solid green";
        $(args).splice(isSuccessIndex, 1);
      }
      else if (isInfoIndex !== -1) {
        logArgs[1] += "color: black;";
        $(args).splice(isInfoIndex, 1);
      }

      $(args).unshift(...logArgs);

      const activeFilter = debugFilter();
      if (activeFilter) {
        const matches = $(args).some(
          arg => $(activeFilter).test(arg)
        );
        if (!matches)
          return;
      }
    }
    mark();
    console$4.log(...args);
    end();
  }

  function getDebugger(name) {
    return bind(debug() ? log : noop, null, name);
  }

  const {Function: Function$1, Object: Object$o, WeakMap: WeakMap$5} = $(window);

  let toStringProxied = false;
  const nativeFns = new WeakMap$5();

  function proxyToString() {
    const {toString} = Function$1.prototype;

    const wrappedToString = proxy(toString, function() {
      const native = nativeFns.get(this);
      if (typeof native !== "undefined")
        return apply$2(toString, native, arguments);
      return apply$2(toString, this, arguments);
    });

    Object$o.defineProperty(window.Function.prototype, "toString", {
      value: wrappedToString
    });

    nativeFns.set(wrappedToString, toString);

    toStringProxied = true;
  }

  function proxyToStringCalls(wrapped, native) {
    if (!toStringProxied)
      proxyToString();

    nativeFns.set(wrapped, native);
  }

  let {
    parseFloat: parseFloat$6,
    variables: variables$2,
    clearTimeout,
    fetch: fetch$1,
    setTimeout: setTimeout$4,
    Array: Array$c,
    Error: Error$m,
    Map: Map$f,
    Object: Object$n,
    ReferenceError: ReferenceError$2,
    Set: Set$b,
    WeakMap: WeakMap$4
  } = $(window);

  let {onerror} = accessor(window);

  let NodeProto$1 = Node.prototype;
  let ElementProto$2 = Element.prototype;

  let propertyAccessors = null;

  function wrapPropertyAccess(object, property, descriptor,
                                     setConfigurable = true) {
    let $property = $(property);
    let dotIndex = $property.indexOf(".");
    if (dotIndex == -1) {

      let currentDescriptor = Object$n.getOwnPropertyDescriptor(object, property);
      if (currentDescriptor && !currentDescriptor.configurable)
        return;

      let newDescriptor = Object$n.assign({}, descriptor, {
        configurable: setConfigurable
      });

      if (!currentDescriptor && !newDescriptor.get && newDescriptor.set) {
        let propertyValue = object[property];
        newDescriptor.get = () => propertyValue;
      }

      Object$n.defineProperty(object, property, newDescriptor);
      return;
    }

    let name = $property.slice(0, dotIndex).toString();
    property = $property.slice(dotIndex + 1).toString();
    let value = object[name];
    if (value && (typeof value == "object" || typeof value == "function"))
      wrapPropertyAccess(value, property, descriptor);

    let currentDescriptor = Object$n.getOwnPropertyDescriptor(object, name);
    if (currentDescriptor && !currentDescriptor.configurable)
      return;

    if (!propertyAccessors)
      propertyAccessors = new WeakMap$4();

    if (!propertyAccessors.has(object))
      propertyAccessors.set(object, new Map$f());

    let properties = propertyAccessors.get(object);
    if (properties.has(name)) {
      properties.get(name).set(property, descriptor);
      return;
    }

    let toBeWrapped = new Map$f([[property, descriptor]]);
    properties.set(name, toBeWrapped);
    Object$n.defineProperty(object, name, {
      get: () => value,
      set(newValue) {
        value = newValue;
        if (value && (typeof value == "object" || typeof value == "function")) {

          for (let [prop, desc] of toBeWrapped)
            wrapPropertyAccess(value, prop, desc);
        }
      },
      configurable: setConfigurable
    });
  }

  function overrideOnError(magic) {
    let prev = onerror();
    onerror((...args) => {
      let message = args.length && args[0];
      if (typeof message == "string" && $(message).includes(magic))
        return true;
      if (typeof prev == "function")
        return apply$2(prev, this, args);
    });
  }

  function abortOnRead(loggingPrefix, context,
                              property, formattedProperties = "",
                              setConfigurable = true) {
    let debugLog = getDebugger(loggingPrefix);

    if (!property) {
      debugLog("error", "no property to abort on read");
      return;
    }

    let rid = randomId();
    let hitEventSent = false;

    function abort() {
      debugLog("success", `${property} access aborted`, `\nFILTER: ${loggingPrefix} ${formattedProperties}`);
      if (!hitEventSent) {
        hitEventSent = true;
        sendSnippetHitEvent(`${loggingPrefix} ${formattedProperties}`);
      }
      throw new ReferenceError$2(rid);
    }

    debugLog("info", `aborting on ${property} access`);

    wrapPropertyAccess(context,
                       property,
                       {get: abort, set() {}},
                       setConfigurable);
    overrideOnError(rid);
  }

  function abortOnWrite(loggingPrefix,
                               context, property,
                               formattedProperties = "",
                               setConfigurable = true) {
    let debugLog = getDebugger(loggingPrefix);

    if (!property) {
      debugLog("error", "no property to abort on write");
      return;
    }

    let rid = randomId();
    let hitEventSent = false;

    function abort() {
      debugLog("success", `setting ${property} aborted`, `\nFILTER: ${loggingPrefix} ${formattedProperties}`);
      if (!hitEventSent) {
        hitEventSent = true;
        sendSnippetHitEvent(`${loggingPrefix} ${formattedProperties}`);
      }
      throw new ReferenceError$2(rid);
    }

    debugLog("info", `aborting when setting ${property}`);

    wrapPropertyAccess(context, property, {set: abort}, setConfigurable);
    overrideOnError(rid);
  }

  function abortOnIframe(
    properties,
    abortRead = false,
    abortWrite = false
  ) {
    let abortedIframes = variables$2.abortedIframes;
    let iframePropertiesToAbort = variables$2.iframePropertiesToAbort;

    const formattedPropertiesToLog = formatArguments(properties);

    for (let frame of Array$c.from(window.frames)) {
      if (abortedIframes.has(frame)) {
        for (let property of properties) {
          if (abortRead)

            abortedIframes.get(frame).read.add({property, formattedProperties: formattedPropertiesToLog});
          if (abortWrite)

            abortedIframes.get(frame).write.add({property, formattedProperties: formattedPropertiesToLog});
        }
      }
    }

    for (let property of properties) {
      if (abortRead)

        iframePropertiesToAbort.read.add({property, formattedProperties: formattedPropertiesToLog});
      if (abortWrite)

        iframePropertiesToAbort.write.add({property, formattedProperties: formattedPropertiesToLog});
    }

    queryAndProxyIframe();
    if (!abortedIframes.has(document)) {
      abortedIframes.set(document, true);
      addHooksOnDomAdditions(queryAndProxyIframe);
    }

    function queryAndProxyIframe() {
      for (let frame of Array$c.from(window.frames)) {

        if (!abortedIframes.has(frame)) {
          abortedIframes.set(frame, {
            read: new Set$b(iframePropertiesToAbort.read),
            write: new Set$b(iframePropertiesToAbort.write)
          });
        }

        let readProps = abortedIframes.get(frame).read;
        if (readProps.size > 0) {
          let props = Array$c.from(readProps);
          readProps.clear();
          for (let {property, formattedProperties} of props) {
            abortOnRead("abort-on-iframe-property-read",
                        frame,
                        property,
                        formattedProperties);
          }
        }

        let writeProps = abortedIframes.get(frame).write;
        if (writeProps.size > 0) {
          let props = Array$c.from(writeProps);
          writeProps.clear();
          for (let {property, formattedProperties} of props) {
            abortOnWrite("abort-on-iframe-property-write",
                         frame,
                         property,
                         formattedProperties);
          }
        }
      }
    }
  }

  function addHooksOnDomAdditions(endCallback) {
    let descriptor;

    wrapAccess(NodeProto$1, ["appendChild", "insertBefore", "replaceChild"]);
    wrapAccess(ElementProto$2, ["append", "prepend", "replaceWith", "after",
                              "before", "insertAdjacentElement",
                              "insertAdjacentHTML"]);

    descriptor = getInnerHTMLDescriptor(ElementProto$2, "innerHTML");
    wrapPropertyAccess(ElementProto$2, "innerHTML", descriptor);

    descriptor = getInnerHTMLDescriptor(ElementProto$2, "outerHTML");
    wrapPropertyAccess(ElementProto$2, "outerHTML", descriptor);

    function wrapAccess(prototype, names) {
      for (let name of names) {
        let desc = getAppendChildDescriptor(prototype, name);
        wrapPropertyAccess(prototype, name, desc);
      }
    }

    function getAppendChildDescriptor(target, property) {
      let currentValue = target[property];
      let wrappedValue = function(...args) {
        let result;
        result = apply$2(currentValue, this, args);
        endCallback && endCallback();
        return result;
      };
      proxyToStringCalls(wrappedValue, currentValue);
      return {
        get() {
          return wrappedValue;
        }
      };
    }

    function getInnerHTMLDescriptor(target, property) {
      let desc = Object$n.getOwnPropertyDescriptor(target, property);
      let {set: prevSetter} = desc || {};
      return {
        set(val) {
          let result;
          result = call(prevSetter, this, val);
          endCallback && endCallback();
          return result;
        }
      };
    }
  }

  let {Object: NativeObject} = window;
  function findOwner(root, path) {
    if (!(root instanceof NativeObject))
      return;

    let object = root;
    let chain = $(path).split(".");

    if (chain.length === 0)
      return;

    for (let i = 0; i < chain.length - 1; i++) {
      let prop = chain[i];

      if (!hasOwnProperty(object, prop))
        return;

      object = object[prop];

      if (!(object instanceof NativeObject))
        return;
    }

    let prop = chain[chain.length - 1];

    if (hasOwnProperty(object, prop))
      return [object, prop];
  }

  const decimals = $(/^\d+$/);

  function overrideValue(value) {
    switch (value) {
      case "false":
        return false;
      case "true":
        return true;
      case "falseStr":
        return "false";
      case "trueStr":
        return "true";
      case "null":
        return null;
      case "noopFunc":
        return () => {};
      case "trueFunc":
        return () => true;
      case "falseFunc":
        return () => false;
      case "emptyArray":
        return [];
      case "emptyObj":
        return {};
      case "undefined":
        return void 0;
      case "":
        return value;
      default:
        if (decimals.test(value))
          return parseFloat$6(value);
        return value;
    }
  }

  function getPromiseFromEvent(item, event) {
    return new Promise(
      resolve => {
        const listener = () => {
          item.removeEventListener(event, listener);
          resolve();
        };
        item.addEventListener(event, listener);
      }
    );
  }

  function waitUntilEvent(
    debugLog,
    mainLogic,
    waitUntil) {
    if (waitUntil) {

      if (waitUntil === "load") {
        debugLog("info", "Waiting until window.load");

        window.addEventListener("load", () => {
          debugLog("info", "Window.load fired.");
          mainLogic();
        });
      }

      else if (waitUntil === "loading" ||
              waitUntil === "interactive" ||
              waitUntil === "complete") {
        debugLog("info", "Waiting document state until :", waitUntil);

        document.addEventListener("readystatechange", () => {
          debugLog("info", "Document state changed:", document.readyState);
          if (document.readyState === waitUntil)
            mainLogic();
        });
      }

      else {
        debugLog("info",
                 "Waiting until ",
                 waitUntil,
                 " event is triggered on document");
        getPromiseFromEvent(document, waitUntil).then(() => {
          debugLog("info",
                   waitUntil,
                   " is triggered on document, starting the snippet");
          mainLogic();
        }).catch(err => {
          debugLog("error",
                   "There was an error while waiting for the event.",
                   err);
        });
      }
    }
    else {

      mainLogic();
    }
  }

  function matchesStackTrace(stackNeedle, debugLog) {
    if (!stackNeedle || !stackNeedle.length)
      return true;

    const token = randomId();
    const error = new Error$m(token);

    const locHref = new URL(self.location.href);
    locHref.hash = "";

    const lineRegex = /(.*?@)?(\S+)(:\d+):\d+\)?$/;
    const lines = [];
    for (let line of error.stack.split(/[\n\r]+/)) {
      if ($(line).includes(token))
        continue;

      line = $(line).trim();
      const match = $(lineRegex).exec(line);
      if (match === null)
        continue;

      let url = match[2];
      if ($(url).startsWith("("))
        url = $(url).slice(1);

      if (url === locHref.href)
        url = "inlineScript";
      else if ($(url).startsWith("<anonymous>"))
        url = "injectedScript";

      let functionName = match[1] ?
        $(match[1]).slice(0, -1) :
        $(line).slice(0, $(match).index).trim();

      if ($(functionName).startsWith("at"))
        functionName = $(functionName).slice(2).trim();

      let linePosition = match[3];
      $(lines).push(" " + `${functionName} ${url}${linePosition}:1`.trim());
    }

    lines[0] = `stackDepth:${lines.length - 1}`;
    const normalizedStack = $(lines).join("\n");

    for (let needle of stackNeedle) {
      const regex = toRegExp(needle);
      if (regex.test(normalizedStack)) {
        debugLog("info", `Found needle in stack trace: ${needle}`);
        return true;
      }
    }

    debugLog("info", `Stack trace does not match any needle. Stack trace: ${normalizedStack}`);
    return false;
  }

  let fetchContentMap = new Map$f();

  function fetchContent(url, {as = "arrayBuffer", cleanup = 60000} = {}) {

    let uid = as + ":" + url;
    let details = fetchContentMap.get(uid) || {
      remove: () => fetchContentMap.delete(uid),
      result: null,
      timer: 0
    };
    clearTimeout(details.timer);
    details.timer = setTimeout$4(details.remove, cleanup);
    if (!details.result) {
      details.result = fetch$1(url).then(res => res[as]()).catch(details.remove);
      fetchContentMap.set(uid, details);
    }
    return details.result;
  }

  let {HTMLScriptElement: HTMLScriptElement$1, Object: Object$m, ReferenceError: ReferenceError$1} = $(window);
  let Script = Object$m.getPrototypeOf(HTMLScriptElement$1);

  function abortCurrentInlineScript(api, search = null) {
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("abort-current-inline-script");
    const {mark, end} = profile("abort-current-inline-script");
    const re = search ? toRegExp(search) : null;

    const rid = randomId();
    const us = $(document).currentScript;
    let hitEventSent = false;

    let object = window;
    const path = $(api).split(".");
    const name = $(path).pop();

    for (let node of $(path)) {
      object = object[node];
      if (
        !object || !(typeof object == "object" || typeof object == "function")) {
        debugLog("warn", path, " is not found");
        return;
      }
    }

    const {get: prevGetter, set: prevSetter} =
      Object$m.getOwnPropertyDescriptor(object, name) || {};

    let currentValue = object[name];
    if (typeof currentValue === "undefined")
      debugLog("warn", "The property", name, "doesn't exist yet. Check typos.");

    const abort = () => {
      const element = $(document).currentScript;
      if (element instanceof Script &&
          $(element, "HTMLScriptElement").src == "" &&
          element != us &&
          (!re || re.test($(element).textContent))) {
        debugLog("success",
                 path,
                 " is aborted \n",
                 element,
                 "\nFILTER: abort-current-inline-script",
                 formattedArguments);
        if (!hitEventSent) {
          hitEventSent = true;
          sendSnippetHitEvent(
            "abort-current-inline-script " + formattedArguments
          );
        }
        throw new ReferenceError$1(rid);
      }
    };

    const descriptor = {
      get() {
        abort();

        if (prevGetter)
          return call(prevGetter, this);

        return currentValue;
      },
      set(value) {
        abort();

        if (prevSetter)
          call(prevSetter, this, value);
        else
          currentValue = value;
      }
    };

    mark();
    wrapPropertyAccess(object,
                       name,
                       descriptor);
    end();

    overrideOnError(rid);
  }

  function abortOnIframePropertyRead(...properties) {
    const {mark, end} = profile("abort-on-iframe-property-read");
    mark();
    abortOnIframe(properties, true, false);
    end();
  }

  function abortOnIframePropertyWrite(...properties) {
    const {mark, end} = profile("abort-on-iframe-property-write");
    mark();
    abortOnIframe(properties, false, true);
    end();
  }

  function abortOnPropertyRead(property, setConfigurable) {
    const configurableFlag = !(setConfigurable === "false");
    const formattedArguments = formatArguments(arguments);
    const {mark, end} = profile("abort-on-property-read");
    mark();
    abortOnRead("abort-on-property-read",
                window,
                property,
                formattedArguments,
                configurableFlag);
    end();
  }

  function abortOnPropertyWrite(property, setConfigurable) {
    const formattedArguments = formatArguments(arguments);
    const {mark, end} = profile("abort-on-property-write");
    const configurableFlag = !(setConfigurable === "false");
    mark();
    abortOnWrite("abort-on-property-write",
                 window,
                 property,
                 formattedArguments,
                 configurableFlag);
    end();
  }

  const {Error: Error$l, Object: Object$l, Array: Array$b, Map: Map$e} = $(window);

  let arrayValues = null;
  const hitFilters$p = new Set();
  function sendHitOnce$5(filter) {
    if (!hitFilters$p.has(filter)) {
      hitFilters$p.add(filter);
      sendSnippetHitEvent(filter);
    }
  }

  function hasMatchingProperty(val, needle, pathSegments) {

    let current = val;
    for (const segment of pathSegments) {

      if (!current || !hasOwnProperty(current, segment))
        return false;
      current = current[segment];
    }

    if (typeof current === "string" || typeof current === "number"){
      const currStr = current.toString();
      return needle.test(currStr);
    }

    return false;
  }

  function arrayOverride(method, needle, returnValue = "false",
                                path, stack) {
    if (!method)
      throw new Error$l("[array-override snippet]: Missing method to override.");

    if (!needle)
      throw new Error$l("[array-override snippet]: Missing needle.");

    if (!arrayValues)
      arrayValues = new Map$e();

    let debugLog = getDebugger("array-override");
    const {mark, end} = profile("array-override");
    const formattedArgsToLog = formatArguments(arguments);

    if (method === "push" && !arrayValues.has("push")) {
      mark();
      const {push} = Array$b.prototype;
      arrayValues.set("push", $([]));

      let wrappedPush = proxy(
        push,
        function(val) {
          const overrideVals = arrayValues.get("push");
          for (const {
            needleRegex, pathSegments, stackNeedles, formattedArgs
          } of overrideVals) {

            if (!pathSegments.length && (typeof val === "string" ||
                typeof val === "number")) {
              const valStr = val.toString();
              if (valStr.match && valStr.match(needleRegex) &&
                  matchesStackTrace(stackNeedles, debugLog)) {
                debugLog("success", `Array.push is ignored for needle: ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
                sendHitOnce$5("array-override " + formattedArgs);
                return;
              }
            }

            else if (pathSegments.length && typeof val === "object" &&
                     val !== null) {
              if (hasMatchingProperty(val, needleRegex, pathSegments) &&
                  matchesStackTrace(stackNeedles, debugLog)) {
                debugLog("success", `Array.push is ignored for object containing needle: ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
                sendHitOnce$5("array-override " + formattedArgs);
                return;
              }
            }
          }
          return apply$2(push, this, arguments);
        }
      );
      proxyToStringCalls(wrappedPush, push);
      Object$l.defineProperty(window.Array.prototype, "push", {
        value: wrappedPush
      });
      debugLog("info", "Wrapped Array.prototype.push");
      end();
    }

    else if (method === "includes" && !arrayValues.has("includes")) {
      mark();
      const {includes} = Array$b.prototype;
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

            if (!pathSegments.length && (typeof val === "string" ||
                 typeof val === "number")) {
              if (val.toString().match &&
                  val.toString().match(needleRegex) &&
                  matchesStackTrace(stackNeedles, debugLog)) {
                debugLog("success", `Array.includes returned ${retVal} for ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
                sendHitOnce$5("array-override " + formattedArgs);
                return retVal;
              }
            }

            else if (pathSegments.length && typeof val === "object" &&
                     val !== null) {
              if (hasMatchingProperty(val, needleRegex, pathSegments) &&
                  matchesStackTrace(stackNeedles, debugLog)) {
                debugLog("success", `Array.includes returned ${retVal} for object containing ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
                sendHitOnce$5("array-override " + formattedArgs);
                return retVal;
              }
            }
          }
          return apply$2(includes, this, arguments);
        }
      );
      proxyToStringCalls(wrappedIncludes, includes);
      Object$l.defineProperty(window.Array.prototype, "includes", {
        value: wrappedIncludes
      });
      debugLog("info", "Wrapped Array.prototype.includes");
      end();
    }

    else if (method === "forEach" && !arrayValues.has("forEach")) {
      mark();
      const {forEach} = Array$b.prototype;
      arrayValues.set("forEach", $([]));

      let wrappedForEach = proxy(
        forEach,
        function(callback, thisArg) {
          const overrideVals = arrayValues.get("forEach");

          const filteredCallback = function(item, index, array) {
            for (const {needleRegex, pathSegments, stackNeedles, formattedArgs} of
              overrideVals) {

              if (!pathSegments.length && (typeof item === "string" ||
                  typeof item === "number")) {
                const itemStr = item.toString();
                if (itemStr.match &&
                    itemStr.match(needleRegex) &&
                    matchesStackTrace(stackNeedles, debugLog)) {
                  debugLog("success", `Array.forEach skipped callback for item matching needle: ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
                  sendHitOnce$5("array-override " + formattedArgs);
                  return;
                }
              }

              else if (pathSegments.length && typeof item === "object" &&
                       item !== null) {
                if (hasMatchingProperty(item, needleRegex, pathSegments) &&
                    matchesStackTrace(stackNeedles, debugLog)) {
                  debugLog("success", `Array.forEach skipped callback for object containing needle: ${needleRegex}\nFILTER: array-override ${formattedArgs}`);
                  sendHitOnce$5("array-override " + formattedArgs);
                  return;
                }
              }
            }

            return apply$2(callback, thisArg || this, [item, index, array]);
          };
          return apply$2(forEach, this, [filteredCallback, thisArg]);
        }
      );
      proxyToStringCalls(wrappedForEach, forEach);
      Object$l.defineProperty(window.Array.prototype, "forEach", {
        value: wrappedForEach
      });
      debugLog("info", "Wrapped Array.prototype.forEach");
      end();
    }

    const needleRegex = toRegExp(needle);
    let pathSegments = [];
    if (path)
      pathSegments = path.split(".");

    let stackNeedles = [];
    if (stack)
      stackNeedles = stack.split(",").map(s => s.trim());

    const overrideVals = arrayValues.get(method);
    const retVal = returnValue === "true";
    overrideVals.push({needleRegex, retVal, pathSegments, stackNeedles,
                       formattedArgs: formattedArgsToLog});
    arrayValues.set(method, overrideVals);
  }

  const {Array: Array$a, Blob, Error: Error$k, Object: Object$k, Reflect: Reflect$3} = $(window);

  const blobRules = [];
  const hitFilters$o = new Set();

  function blobOverride(search, replacement = "", needle = null) {
    if (!search) {
      throw new Error$k(
        "[blob-override snippet]: Missing parameter search."
      );
    }
    const debugLog = getDebugger("blob-override");
    const formattedArgsToLog = formatArguments(arguments);
    const {mark, end} = profile("blob-override");
    mark();

    blobRules.push({
      match: toRegExp(search),
      replaceWith: replacement,
      needle: needle ? toRegExp(needle) : null,
      formattedArgs: formattedArgsToLog
    });

    if (blobRules.length > 1)
      return;

    const OriginalBlob = Blob;
    function PatchedBlob(data, options = {}) {
      if (Array$a.isArray(data)) {
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
            if (!hitFilters$o.has(filter)) {
              hitFilters$o.add(filter);
              sendSnippetHitEvent(filter);
            }
          }
        }
        data = [combinedData];
      }

      const blob = Reflect$3.construct(OriginalBlob, [data, options]);
      Object$k.setPrototypeOf(blob, PatchedBlob.prototype);
      return blob;
    }

    PatchedBlob.prototype = OriginalBlob.prototype;
    Object$k.setPrototypeOf(PatchedBlob, OriginalBlob);
    proxyToStringCalls(PatchedBlob, window.Blob);
    window.Blob = PatchedBlob;
    debugLog("info", "Wrapped Blob constructor in context ");
    end();
  }

  let {Error: Error$j, URL: URL$1} = $(window);
  let {cookie: documentCookies} = accessor(document);

  function cookieRemover(cookie, autoRemoveCookie = false) {
    if (!cookie)
      throw new Error$j("[cookie-remover snippet]: No cookie to remove.");

    const formattedArguments = formatArguments(arguments);
    let debugLog = getDebugger("cookie-remover");
    const {mark, end} = profile("cookie-remover");
    let re = toRegExp(cookie);
    let hitEventSent = false;

    if (!$(/^http|^about/).test(location.protocol)) {
      debugLog("warn", "Snippet only works for http or https and about.");
      return;
    }

    function getCookieMatches() {
      const arr = $(documentCookies()).split(";");
      return arr.filter(str => re.test($(str).split("=")[0]));
    }

    const mainLogic = () => {
      debugLog("info", "Parsing cookies for matches");
      mark();
      for (const pair of $(getCookieMatches())) {
        let $hostname = $(location.hostname);

        if (!$hostname &&
          $(location.ancestorOrigins) && $(location.ancestorOrigins[0]))
          $hostname = new URL$1($(location.ancestorOrigins[0])).hostname;
        const name = $(pair).split("=")[0];
        const expires = "expires=Thu, 01 Jan 1970 00:00:00 GMT";
        const path = "path=/";
        const domainParts = $hostname.split(".");

        for (let numDomainParts = domainParts.length;
          numDomainParts > 0; numDomainParts--) {
          const domain =
            domainParts.slice(domainParts.length - numDomainParts).join(".");
          documentCookies(`${$(name).trim()}=;${expires};${path};domain=${domain}`);
          documentCookies(`${$(name).trim()}=;${expires};${path};domain=.${domain}`);
          debugLog("success", `Set expiration date on ${name}`, "\nFILTER: cookie-remover", formattedArguments);
          if (!hitEventSent) {
            hitEventSent = true;
            sendSnippetHitEvent(
              "cookie-remover " + formattedArguments
            );
          }
        }
      }
      end();
    };

    mainLogic();

    if (autoRemoveCookie) {

      let lastCookie = getCookieMatches();
      setInterval(() => {
        let newCookie = getCookieMatches();
        if (newCookie !== lastCookie) {
          try {
            mainLogic();
          }
          finally {
            lastCookie = newCookie;
          }
        }
      }, 1000);
    }
  }

  const {Map: Map$d, Object: Object$j, Reflect: Reflect$2, WeakMap: WeakMap$3} = $(window);

  const originalAddEventListener = window.EventTarget.prototype.addEventListener;
  const originalRemoveEventListener = window.EventTarget.
                                      prototype.removeEventListener;

  const listenerMap = new WeakMap$3();
  let eventOverrides = [];
  const hitFilters$n = new Set();
  function sendHitOnce$4(filter) {
    if (!hitFilters$n.has(filter)) {
      hitFilters$n.add(filter);
      sendSnippetHitEvent(filter);
    }
  }

  function eventOverride(eventType,
                                mode,
                                needle = null) {
    const formattedArgs = formatArguments(arguments);
    const overrideConfig = {
      eventType,
      mode,
      needle: needle ? toRegExp(needle) : null,
      formattedArgs
    };

    if (!eventOverrides.includes(overrideConfig))
      eventOverrides.push(overrideConfig);

    if (eventOverrides.length > 1)
      return;

    let debugLog = getDebugger("[event-override]");
    const {mark, end} = profile("event-override");

    const addEventListenerDescriptor = Object$j.getOwnPropertyDescriptor(
      window.EventTarget.prototype,
      "addEventListener"
    );

    if (addEventListenerDescriptor.configurable) {
      let wrappedAddEventListener = proxy(
        originalAddEventListener,
        function(type, listener, options) {
          mark();

          const filteredEvents = eventOverrides.filter(
            ev => ev.eventType === type
          );

          if (!filteredEvents.length || type !== filteredEvents[0].eventType) {
            end();
            return apply$2(originalAddEventListener, this, arguments);
          }

          const disabledEvent = filteredEvents.find(
            ev =>
              (ev.mode === "disable") &&
              (ev.needle ? ev.needle.test(listener.toString()) : true)
          );

          if (disabledEvent) {
            debugLog("success", `Disabling ${disabledEvent.eventType} event, \nFILTER: event-override ${disabledEvent.formattedArgs}`);
            sendHitOnce$4("event-override " + disabledEvent.formattedArgs);
            end();
            return;
          }

          const changedEvents = filteredEvents.filter(
            ev =>
              (ev.mode === "trusted") &&
              (ev.needle ? ev.needle.test(listener.toString()) : true)
          );

          if (typeof listener !== "function" &&
              !(listener && typeof listener.handleEvent === "function") ||
              !changedEvents.length || type !== changedEvents[0].eventType) {
            end();
            return apply$2(originalAddEventListener, this, arguments);
          }

          const wrappedListener = function(originalEvent) {
            const customEvent = new Proxy(originalEvent, {
              get(target, prop) {
                if (prop === "isTrusted") {
                  debugLog("success", `Providing trusted value for ${originalEvent.type} event`);

                  sendHitOnce$4("event-override " + changedEvents[0].formattedArgs);
                  return true;
                }

                const val = Reflect$2.get(target, prop);

                if (typeof val === "function") {
                  return function(...args) {
                    return apply$2(val, target, args);
                  };
                }

                return val;
              }
            });

            if (typeof listener === "function")
              return call(listener, this, customEvent);

            return call(listener.handleEvent, listener, customEvent);
          };

          wrappedListener.originalListener = listener;

          if (!listenerMap.has(listener))
            listenerMap.set(listener, new Map$d());

          listenerMap.get(listener).set(type, wrappedListener);
          debugLog("info", `\nWrapping event listener for ${type}`);

          end();
          return apply$2(
            originalAddEventListener,
            this,
            [type, wrappedListener, options]
          );
        });
      proxyToStringCalls(wrappedAddEventListener, originalAddEventListener);
      Object$j.defineProperty(window.EventTarget.prototype, "addEventListener", {
        ...addEventListenerDescriptor,
        value: wrappedAddEventListener
      });
    }

    const removeEventListenerDescriptor = Object$j.getOwnPropertyDescriptor(
      window.EventTarget.prototype,
      "removeEventListener"
    );
    if (removeEventListenerDescriptor.configurable) {
      let wrappedRemoveEventListener = proxy(
        originalRemoveEventListener,
        function(type, listener, options) {
          if (listener &&
            listenerMap.has(listener) && listenerMap.get(listener).has(type)) {
            const wrappedListener = listenerMap.get(listener).get(type);
            listenerMap.get(listener).delete(type);
            return apply$2(
              originalRemoveEventListener,
              this,
              [type, wrappedListener, options]
            );
          }

          return apply$2(originalRemoveEventListener, this, arguments);
        });
      proxyToStringCalls(wrappedRemoveEventListener, originalRemoveEventListener);
      Object$j.defineProperty(window.EventTarget.prototype, "removeEventListener", {
        ...removeEventListenerDescriptor,
        value: wrappedRemoveEventListener
      });
    }

    debugLog("info", "Initialized event-override snippet");
  }

  let {
    console: console$3,
    document: document$5,
    getComputedStyle: getComputedStyle$7,
    isExtensionContext,
    variables: variables$1,
    Array: Array$9,
    MutationObserver: MutationObserver$j,
    Object: Object$i,
    DOMMatrix,
    XPathEvaluator,
    XPathExpression,
    XPathResult
  } = $(window);

  const {querySelectorAll} = document$5;
  const document$$ = querySelectorAll && bind(querySelectorAll, document$5);

  function $openOrClosedShadowRoot(element, failSilently = false) {
    try {
      const shadowRoot = (navigator.userAgent.includes("Firefox")) ?
        element.openOrClosedShadowRoot :
        browser.dom.openOrClosedShadowRoot(element);
      if (shadowRoot === null && ((debug() && !failSilently)))
        console$3.log("Shadow root not found or not added in element yet", element);
      return shadowRoot;
    }
    catch (error) {
      if (debug() && !failSilently)
        console$3.log("Error while accessing shadow root", element, error);
      return null;
    }
  }

  function $$(selector, returnRoots = false) {

    return $$recursion(
      selector,
      document$$.bind(document$5),
      document$5,
      returnRoots
    );
  }

  function isArrayEmptyStrings(arr) {
    return !arr || arr.length === 0 || arr.every(item => item.trim() === "");
  }

  function executeSvgCommand(
    nestedCommands,
    rootParent,
    resultNodes,
    rootParents
  ) {
    const xlinkHref = rootParent.getAttribute("xlink:href") ||
            rootParent.getAttribute("href");
    if (xlinkHref) {
      const matchingElement = document$$(xlinkHref)[0];
      if (!matchingElement && debug()) {
        console$3.log("No elements found matching", xlinkHref);
        return false;
      }

      if (isArrayEmptyStrings(nestedCommands)) {
        const oldRootParents = rootParents.length > 0 ? rootParents : [];
        resultNodes.push({
          element: matchingElement,
          rootParents: [...oldRootParents, rootParent]
        });
        return false;
      }
      const next$$ = matchingElement.querySelectorAll.bind(matchingElement);
      return {
        nextBoundElement: matchingElement,
        nestedSelectorsString: nestedCommands.join("^^"),
        next$$
      };
    }
  }

  function executeShadowRootCommand(nestedCommands, rootParent) {
    const shadowRoot = $openOrClosedShadowRoot(rootParent);
    if (shadowRoot) {
      const {querySelectorAll: shadowRootQuerySelectorAll} = shadowRoot;
      const next$$ = shadowRootQuerySelectorAll &&
        bind(shadowRootQuerySelectorAll, shadowRoot).bind(shadowRoot);
      return {
        nextBoundElement: rootParent,
        nestedSelectorsString: ":host " + nestedCommands.join("^^"),
        next$$
      };
    }

    return false;
  }

  function $$recursion(
    selector,
    bound$$,
    boundElement,
    returnRoots,
    rootParents = []
  ) {
    if (selector.includes("^^")) {
      const [currentSelector, currentCommand, ...nestedCommands] =
        selector.split("^^");
      let newRootParents;

      let commandFn;
      switch (currentCommand) {
        case "svg": {
          commandFn = executeSvgCommand;
          break;
        }
        case "sh": {
          commandFn = executeShadowRootCommand;
          break;
        }
        default: {
          if (debug()) {
            console$3.log(
              currentCommand,
              " is not supported. Supported commands are: \n^^sh^^\n^^svg^^"
            );
          }
          return [];
        }
      }

      if (currentSelector.trim() === "")
        newRootParents = [boundElement];
      else
        newRootParents = bound$$(currentSelector);

      const resultNodes = [];

      for (const rootParent of newRootParents) {
        const res =
          commandFn(nestedCommands, rootParent, resultNodes, rootParents);
        if (!res)
          continue;
        const {next$$, nestedSelectorsString, nextBoundElement} = res;
        const nestedElements = $$recursion(
          nestedSelectorsString,
          next$$,
          nextBoundElement,
          returnRoots,
          [...rootParents, rootParent]
        );
        if (nestedElements)
          resultNodes.push(...nestedElements);
      }
      return resultNodes;
    }
    const foundElements = bound$$(selector);
    if (returnRoots) {
      return [...foundElements].map(element => (
        {element, rootParents: rootParents.length > 0 ? rootParents : []})
      );
    }
    return foundElements;
  }

  function $closest(element, selector, shadowRootParents = []) {
    if (selector.includes("^^svg^^"))
      selector = selector.split("^^svg^^")[0];

    if (selector.includes("^^sh^^")) {

      const splitSelector = selector.split("^^sh^^");
      const numShadowRootsToCross = splitSelector.length - 1;
      selector = `:host ${splitSelector[numShadowRootsToCross]}`;

      if (numShadowRootsToCross === shadowRootParents.length) {

        return element.closest(selector);
      }

      const shadowRootParent = shadowRootParents[numShadowRootsToCross];
      return shadowRootParent.closest(selector);
    }
    if (shadowRootParents[0])
      return shadowRootParents[0].closest(selector);
    return element.closest(selector);
  }

  function $childNodes(element, failSilently = true) {
    const shadowRoot = $openOrClosedShadowRoot(element, failSilently);
    if (shadowRoot)
      return shadowRoot.childNodes;

    return $(element).childNodes;
  }

  const {assign, setPrototypeOf} = Object$i;

  class $XPathExpression extends XPathExpression {
    evaluate(...args) {
      return setPrototypeOf(
        apply$2(super.evaluate, this, args),
        XPathResult.prototype
      );
    }
  }

  class $XPathEvaluator extends XPathEvaluator {
    createExpression(...args) {
      return setPrototypeOf(
        apply$2(super.createExpression, this, args),
        $XPathExpression.prototype
      );
    }
  }

  function hideElement(element) {
    if (variables$1.hidden.has(element))
      return false;

    notifyElementHidden(element);

    variables$1.hidden.add(element);

    let {style} = $(element);
    let $style = $(style, "CSSStyleDeclaration");
    let properties = $([]);
    const libEnvironment = getLibEnvironment();
    let {debugCSSProperties} = libEnvironment;

    for (let [key, value] of (debugCSSProperties || [["display", "none"]])) {
      $style.setProperty(key, value, "important");
      properties.push([key, $style.getPropertyValue(key)]);
    }

    new MutationObserver$j(() => {
      for (let [key, value] of properties) {
        let propertyValue = $style.getPropertyValue(key);
        let propertyPriority = $style.getPropertyPriority(key);
        if (propertyValue != value || propertyPriority != "important")
          $style.setProperty(key, value, "important");
      }
    }).observe(element, {attributes: true,
                         attributeFilter: ["style"]});
    return true;
  }

  function notifyElementHidden(element) {
    if (isExtensionContext && typeof checkElement === "function")
      checkElement(element);
  }

  function initQueryAndApply(selector) {
    let $selector = selector;
    if ($selector.startsWith("xpath(") &&
        $selector.endsWith(")")) {
      let xpathQuery = $selector.slice(6, -1);
      let evaluator = new $XPathEvaluator();
      let expression = evaluator.createExpression(xpathQuery, null);

      let flag = XPathResult.ORDERED_NODE_SNAPSHOT_TYPE;

      return cb => {
        if (!cb)
          return;
        let result = expression.evaluate(document$5, flag, null);
        let {snapshotLength} = result;
        for (let i = 0; i < snapshotLength; i++)
          cb(result.snapshotItem(i));
      };
    }
    return cb => $$(selector).forEach(cb);
  }

  function initQueryAll(selector) {
    let $selector = selector;
    if ($selector.startsWith("xpath(") &&
        $selector.endsWith(")")) {
      let queryAndApply = initQueryAndApply(selector);
      return () => {
        let elements = $([]);
        queryAndApply(e => elements.push(e));
        return elements;
      };
    }
    return () => Array$9.from($$(selector));
  }

  function hideIfMatches(match, selector, searchSelector, onHideCallback) {
    if (searchSelector == null)
      searchSelector = selector;

    let won;
    const callback = () => {
      for (const {element, rootParents} of $$(searchSelector, true)) {
        const closest = $closest($(element), selector, rootParents);
        if (closest && match(element, closest, rootParents)) {
          won();
          if (hideElement(closest) && typeof onHideCallback === "function")
            onHideCallback(closest);
        }
      }
    };
    return assign(
      new MutationObserver$j(callback),
      {
        race(win) {
          won = win;
          this.observe(document$5, {childList: true,
                                  characterData: true,
                                  subtree: true});
          callback();
        }
      }
    );
  }

  function isVisible(element, style, closest, shadowRootParents) {
    let $style = $(style, "CSSStyleDeclaration");
    if ($style.getPropertyValue("display") == "none")
      return false;

    let visibility = $style.getPropertyValue("visibility");
    if (visibility == "hidden" || visibility == "collapse")
      return false;

    if (!closest || element == closest)
      return true;

    let parent = $(element).parentElement;
    if (!parent) {

      if (shadowRootParents && shadowRootParents.length) {
        parent = shadowRootParents[shadowRootParents.length - 1];
        shadowRootParents = shadowRootParents.slice(0, -1);
      }
      else {
        return true;
      }
    }

    return isVisible(
      parent, getComputedStyle$7(parent), closest, shadowRootParents
    );
  }

  function getComputedCSSText(element) {
    let style = getComputedStyle$7(element);
    let {cssText} = style;

    if (cssText)
      return cssText;

    for (let property of style)
      cssText += `${property}: ${style[property]}; `;

    return $(cssText).trim();
  }

  function getTransformMatrix(element, pseudo = null) {
    const style = getComputedStyle$7(element, pseudo);
    let transform = style.transform;
    return (transform === "none") ? new DOMMatrix() : new DOMMatrix(transform);
  }

  let fontVisibilityCanvas = null;
  let fontVisibilityCtx = null;

  function isFontVisible(element, style, overrideText = null) {
    try {
      let text = overrideText || element.innerText;
      if (!text)
        return false;

      text = text.trim();
      if (!fontVisibilityCanvas || !fontVisibilityCtx) {
        fontVisibilityCanvas = document$5.createElement("canvas");
        fontVisibilityCtx = fontVisibilityCanvas.getContext("2d", {
          alpha: true,
          willReadFrequently: true
        });
      }
      let fontString = style.font || [
        style.fontStyle,
        style.fontVariant,
        style.fontWeight,
        style.fontSize,
        style.fontFamily
      ].join(" ");
      fontVisibilityCtx.font = fontString;
      const metrics = fontVisibilityCtx.measureText(text);

      if (metrics.width <= 0)
        return false;

      fontVisibilityCanvas.width =
        Math.min(metrics.width, 800);
      fontVisibilityCanvas.height =
        Math.max(1, parseInt(style.fontSize, 10) * 1.5 || 20);

      fontVisibilityCtx.font = fontString;
      fontVisibilityCtx.fillStyle = "#000";
      fontVisibilityCtx.textBaseline = "top";
      fontVisibilityCtx.fillText(text, 0, 0);

      const data = fontVisibilityCtx.getImageData(0, 0, fontVisibilityCanvas.width, fontVisibilityCanvas.height).data;

      for (let i = 3; i < data.length; i += 4) {

        if (data[i] > 0)
          return true;
      }

      return false;
    }
    catch (error) {
      if (debug())
        console$3.log("Font visibility check failed:", element, error.message);

      return true;
    }
  }

  function isTextVisible(element,
                                style,
                                attributesMap,
                                {bgColorCheck = true,
                                 pseudoElemCheck = false,
                                 fontCheck = true} = {}) {
    if (!style)
      style = getComputedStyle$7(element);
    style = $(style);
    for (const [key, value] of attributesMap) {
      let valueAsRegex = toRegExp(value);
      if (valueAsRegex.test(style.getPropertyValue(key)))
        return false;
    }
    const color = style.getPropertyValue("color");
    if (bgColorCheck && style.getPropertyValue("background-color") === color)
      return false;

    if (!pseudoElemCheck) {
      const firstLineStyle = getComputedStyle$7(element, "::first-line");
      if (firstLineStyle) {
        return isTextVisible(element,
                             firstLineStyle,
                             attributesMap,
                             {bgColorCheck,
                              pseudoElemCheck: true,
                              fontCheck});
      }
    }

    if (fontCheck && !isFontVisible(element, style))
      return false;

    const textShadow = style.getPropertyValue("text-shadow");
    if (color.includes("rgba(0, 0, 0, 0)") &&
        (textShadow === "none" ||
        textShadow.includes("rgba(0, 0, 0, 0)"))
    )
      return false;
    return true;
  }

  function isContained(childNode, parentNode, {
    boxMargin = 2,
    ignorePadding = false
  } = {}) {
    let child = $(childNode).getBoundingClientRect();
    if (ignorePadding) {
      const style = getComputedStyle$7(childNode);
      const paddingTop = parseFloat(style.paddingTop) || 0;
      const paddingRight = parseFloat(style.paddingRight) || 0;
      const paddingBottom = parseFloat(style.paddingBottom) || 0;
      const paddingLeft = parseFloat(style.paddingLeft) || 0;

      child = {
        left: child.left + paddingLeft,
        right: child.right - paddingRight,
        top: child.top + paddingTop,
        bottom: child.bottom - paddingBottom
      };
    }

    const parent = $(parentNode).getBoundingClientRect();
    const stretchedParent = {
      left: parent.left - boxMargin,
      right: parent.right + boxMargin,
      top: parent.top - boxMargin,
      bottom: parent.bottom + boxMargin
    };

    return (
      (stretchedParent.left <= child.left &&
          child.left <= stretchedParent.right &&
        stretchedParent.top <= child.top &&
          child.top <= stretchedParent.bottom) &&
      (stretchedParent.top <= child.bottom &&
          child.bottom <= stretchedParent.bottom &&
        stretchedParent.left <= child.right &&
          child.right <= stretchedParent.right)
    );
  }

  let {ELEMENT_NODE: ELEMENT_NODE$3, TEXT_NODE: TEXT_NODE$1, prototype: NodeProto} = Node;
  let {prototype: ElementProto$1} = Element;
  let {prototype: HTMLElementProto} = HTMLElement;

  let {
    console: console$2,
    variables,
    DOMParser: DOMParser$1,
    Error: Error$i,
    MutationObserver: MutationObserver$i,
    Object: Object$h,
    ReferenceError
  } = $(window);

  let {getOwnPropertyDescriptor} = Object$h;

  function freezeElement(selector, options = "", ...exceptions) {
    const formattedArguments = formatArguments(arguments);
    let observer;
    let subtree = false;
    let shouldAbort = false;
    let exceptionSelectors = $(exceptions).filter(e => !isRegex(e));
    let regexExceptions = $(exceptions).filter(e => isRegex(e)).map(toRegExp);
    let rid = randomId();
    let targetNodes;
    let queryAll = initQueryAll(selector);

    checkOptions();
    let data = {
      selector,
      shouldAbort,
      rid,
      exceptionSelectors,
      regexExceptions,
      changeId: 0
    };
    if (!variables.frozen.has(document)) {
      variables.frozen.set(document, true);
      proxyNativeProperties();
    }
    observer = new MutationObserver$i(searchAndAttach);
    observer.observe(document, {childList: true, subtree: true});
    searchAndAttach();

    function isRegex(s) {
      return s.length >= 2 && s[0] == "/" && s[s.length - 1] == "/";
    }

    function checkOptions() {
      let optionsChunks = $(options).split("+");
      if (optionsChunks.length === 1 && optionsChunks[0] === "")
        optionsChunks = [];
      for (let chunk of optionsChunks) {
        switch (chunk) {
          case "subtree":
            subtree = true;
            break;
          case "abort":
            shouldAbort = true;
            break;
          default:
            throw new Error$i("[freeze] Unknown option passed to the snippet." +
                            " [selector]: " + selector +
                            " [option]: " + chunk);
        }
      }
    }

    function proxyNativeProperties() {
      let descriptor;

      descriptor = getAppendChildDescriptor(
        NodeProto, "appendChild", isFrozen, getSnippetData
      );
      wrapPropertyAccess(NodeProto, "appendChild", descriptor);

      descriptor = getAppendChildDescriptor(
        NodeProto, "insertBefore", isFrozen, getSnippetData
      );
      wrapPropertyAccess(NodeProto, "insertBefore", descriptor);

      descriptor = getAppendChildDescriptor(
        NodeProto, "replaceChild", isFrozen, getSnippetData
      );
      wrapPropertyAccess(NodeProto, "replaceChild", descriptor);

      descriptor = getAppendDescriptor(
        ElementProto$1, "append", isFrozen, getSnippetData
      );
      wrapPropertyAccess(ElementProto$1, "append", descriptor);

      descriptor = getAppendDescriptor(
        ElementProto$1, "prepend", isFrozen, getSnippetData
      );
      wrapPropertyAccess(ElementProto$1, "prepend", descriptor);

      descriptor = getAppendDescriptor(
        ElementProto$1,
        "replaceWith",
        isFrozenOrHasFrozenParent,
        getSnippetDataFromNodeOrParent
      );
      wrapPropertyAccess(ElementProto$1, "replaceWith", descriptor);

      descriptor = getAppendDescriptor(
        ElementProto$1,
        "after",
        isFrozenOrHasFrozenParent,
        getSnippetDataFromNodeOrParent
      );
      wrapPropertyAccess(ElementProto$1, "after", descriptor);

      descriptor = getAppendDescriptor(
        ElementProto$1,
        "before",
        isFrozenOrHasFrozenParent,
        getSnippetDataFromNodeOrParent
      );
      wrapPropertyAccess(ElementProto$1, "before", descriptor);

      descriptor = getInsertAdjacentDescriptor(
        ElementProto$1,
        "insertAdjacentElement",
        isFrozenAndInsideTarget,
        getSnippetDataBasedOnTarget
      );
      wrapPropertyAccess(ElementProto$1, "insertAdjacentElement", descriptor);

      descriptor = getInsertAdjacentDescriptor(
        ElementProto$1,
        "insertAdjacentHTML",
        isFrozenAndInsideTarget,
        getSnippetDataBasedOnTarget
      );
      wrapPropertyAccess(ElementProto$1, "insertAdjacentHTML", descriptor);

      descriptor = getInsertAdjacentDescriptor(
        ElementProto$1,
        "insertAdjacentText",
        isFrozenAndInsideTarget,
        getSnippetDataBasedOnTarget
      );
      wrapPropertyAccess(ElementProto$1, "insertAdjacentText", descriptor);

      descriptor = getInnerHTMLDescriptor(
        ElementProto$1, "innerHTML", isFrozen, getSnippetData
      );
      wrapPropertyAccess(ElementProto$1, "innerHTML", descriptor);

      descriptor = getInnerHTMLDescriptor(
        ElementProto$1,
        "outerHTML",
        isFrozenOrHasFrozenParent,
        getSnippetDataFromNodeOrParent
      );
      wrapPropertyAccess(ElementProto$1, "outerHTML", descriptor);

      descriptor = getTextContentDescriptor(
        NodeProto, "textContent", isFrozen, getSnippetData
      );
      wrapPropertyAccess(NodeProto, "textContent", descriptor);

      descriptor = getTextContentDescriptor(
        HTMLElementProto, "innerText", isFrozen, getSnippetData
      );
      wrapPropertyAccess(HTMLElementProto, "innerText", descriptor);

      descriptor = getTextContentDescriptor(
        NodeProto, "nodeValue", isFrozen, getSnippetData
      );
      wrapPropertyAccess(NodeProto, "nodeValue", descriptor);

      function isFrozen(node) {
        return node && variables.frozen.has(node);
      }

      function isFrozenOrHasFrozenParent(node) {
        try {
          return node &&
                 (variables.frozen.has(node) ||
                 variables.frozen.has($(node).parentNode));
        }
        catch (_error) {
          return false;
        }
      }

      function isFrozenAndInsideTarget(node, isInsideTarget) {
        try {
          return node &&
                 (variables.frozen.has(node) && isInsideTarget ||
                  variables.frozen.has($(node).parentNode) &&
                  !isInsideTarget);
        }
        catch (_error) {
          return false;
        }
      }

      function getSnippetData(node) {
        return variables.frozen.get(node);
      }

      function getSnippetDataFromNodeOrParent(node) {
        try {
          if (variables.frozen.has(node))
            return variables.frozen.get(node);
          let parent = $(node).parentNode;
          return variables.frozen.get(parent);
        }
        catch (_error) {}
      }

      function getSnippetDataBasedOnTarget(node, isInsideTarget) {
        try {
          if (variables.frozen.has(node) && isInsideTarget)
            return variables.frozen.get(node);
          let parent = $(node).parentNode;
          return variables.frozen.get(parent);
        }
        catch (_error) {}
      }
    }

    function searchAndAttach() {
      targetNodes = queryAll();
      markNodes(targetNodes, false);
    }

    function markNodes(nodes, isChild = true) {
      for (let node of nodes) {
        if (!variables.frozen.has(node)) {
          variables.frozen.set(node, data);
          if (!isChild && subtree) {
            new MutationObserver$i(mutationsList => {
              for (let mutation of $(mutationsList))
                markNodes($(mutation, "MutationRecord").addedNodes);
            }).observe(node, {childList: true, subtree: true});
          }
          if (subtree && $(node).nodeType === ELEMENT_NODE$3)
            markNodes($(node).childNodes);
        }
      }
    }

    function logPrefixed(id, ...args) {
      log(`[freeze][${id}] `, ...args);
    }

    function logChange(nodeOrDOMString, target, property, snippetData) {
      let targetSelector = snippetData.selector;
      let chgId = snippetData.changeId;
      let isDOMString = typeof nodeOrDOMString == "string";
      let action = snippetData.shouldAbort ? "aborting" : "watching";
      console$2.groupCollapsed(`[freeze][${chgId}] ${action}: ${targetSelector}`);
      switch (property) {
        case "appendChild":
        case "append":
        case "prepend":
        case "insertBefore":
        case "replaceChild":
        case "insertAdjacentElement":
        case "insertAdjacentHTML":
        case "insertAdjacentText":
        case "innerHTML":
        case "outerHTML":
          logPrefixed(chgId,
                      isDOMString ? "text: " : "node: ",
                      nodeOrDOMString);
          logPrefixed(chgId, "added to node: ", target);
          break;
        case "replaceWith":
        case "after":
        case "before":
          logPrefixed(chgId,
                      isDOMString ? "text: " : "node: ",
                      nodeOrDOMString);
          logPrefixed(chgId, "added to node: ", $(target).parentNode);
          break;
        case "textContent":
        case "innerText":
        case "nodeValue":
          logPrefixed(chgId, "content of node: ", target);
          logPrefixed(chgId, "changed to: ", nodeOrDOMString);
          break;
      }
      logPrefixed(chgId, `using the function "${property}"`);
      console$2.groupEnd();
      snippetData.changeId++;
    }

    function isExceptionNode(element, expSelectors) {
      if (expSelectors) {
        let $element = $(element);
        for (let exception of expSelectors) {
          if ($element.matches(exception))
            return true;
        }
      }
      return false;
    }

    function isExceptionText(string, regExceptions) {
      if (regExceptions) {
        for (let exception of regExceptions) {
          if (exception.test(string))
            return true;
        }
      }
      return false;
    }

    let hitEventSent = false;

    function abort(id) {
      if (!hitEventSent) {
        hitEventSent = true;
        sendSnippetHitEvent("freeze-element " + formattedArguments);
      }
      throw new ReferenceError(id);
    }

    function checkHTML(htmlText, parent, property, snippetData) {
      let domparser = new DOMParser$1();
      let {body} = $(domparser.parseFromString(htmlText, "text/html"));
      let nodes = $(body).childNodes;
      let accepted = checkMultiple(nodes, parent, property, snippetData);
      let content = $(accepted).map(node => {
        switch ($(node).nodeType) {
          case ELEMENT_NODE$3:
            return $(node).outerHTML;
          case TEXT_NODE$1:
            return $(node).textContent;
          default:
            return "";
        }
      });
      return content.join("");
    }

    function checkMultiple(nodesOrDOMStrings, parent, property, snippetData) {
      let accepted = $([]);
      for (let nodeOrDOMString of nodesOrDOMStrings) {
        if (checkShouldInsert(nodeOrDOMString, parent, property, snippetData))
          accepted.push(nodeOrDOMString);
      }
      return accepted;
    }

    function checkShouldInsert(nodeOrDOMString, parent, property, snippetData) {
      let aborting = snippetData.shouldAbort;
      let regExceptions = snippetData.regexExceptions;
      let expSelectors = snippetData.exceptionSelectors;
      let id = snippetData.rid;
      if (typeof nodeOrDOMString == "string") {
        let domString = nodeOrDOMString;
        if (isExceptionText(domString, regExceptions))
          return true;
        if (debug())
          logChange(domString, parent, property, snippetData);
        if (aborting)
          abort(id);
        return debug();
      }

      let node = nodeOrDOMString;
      switch ($(node).nodeType) {
        case ELEMENT_NODE$3:
          if (isExceptionNode(node, expSelectors))
            return true;
          if (aborting) {
            if (debug())
              logChange(node, parent, property, snippetData);
            abort(id);
          }
          if (debug()) {
            hideElement(node);
            logChange(node, parent, property, snippetData);
            return true;
          }
          return false;
        case TEXT_NODE$1:
          if (isExceptionText($(node).textContent, regExceptions))
            return true;
          if (debug())
            logChange(node, parent, property, snippetData);
          if (aborting)
            abort(id);
          return false;
        default:
          return true;
      }
    }

    function getAppendChildDescriptor(target, property, shouldValidate,
                                      getSnippetData) {
      let desc = getOwnPropertyDescriptor(target, property) || {};
      let origin = desc.get && call(desc.get, target) || desc.value;
      if (!origin)
        return;

      return {
        get() {
          return function(...args) {
            if (shouldValidate(this)) {
              let snippetData = getSnippetData(this);
              if (snippetData) {
                let incomingNode = args[0];
                if (!checkShouldInsert(incomingNode, this, property, snippetData))
                  return incomingNode;
              }
            }
            return apply$2(origin, this, args);
          };
        }
      };
    }

    function getAppendDescriptor(
      target, property, shouldValidate, getSnippetData
    ) {
      let desc = getOwnPropertyDescriptor(target, property) || {};
      let origin = desc.get && call(desc.get, target) || desc.value;
      if (!origin)
        return;
      return {
        get() {
          return function(...nodesOrDOMStrings) {
            if (!shouldValidate(this))
              return apply$2(origin, this, nodesOrDOMStrings);

            let snippetData = getSnippetData(this);
            if (!snippetData)
              return apply$2(origin, this, nodesOrDOMStrings);

            let accepted = checkMultiple(
              nodesOrDOMStrings, this, property, snippetData
            );
            if (accepted.length > 0)
              return apply$2(origin, this, accepted);
          };
        }
      };
    }

    function getInsertAdjacentDescriptor(
      target, property, shouldValidate, getSnippetData
    ) {
      let desc = getOwnPropertyDescriptor(target, property) || {};
      let origin = desc.get && call(desc.get, target) || desc.value;
      if (!origin)
        return;

      return {
        get() {
          return function(...args) {
            let [position, value] = args;
            let isInsideTarget =
                position === "afterbegin" || position === "beforeend";
            if (shouldValidate(this, isInsideTarget)) {
              let snippetData = getSnippetData(this, isInsideTarget);
              if (snippetData) {
                let parent = isInsideTarget ?
                             this :
                             $(this).parentNode;
                let finalValue;
                switch (property) {
                  case "insertAdjacentElement":
                    if (!checkShouldInsert(value, parent, property, snippetData))
                      return value;
                    break;

                  case "insertAdjacentHTML":
                    finalValue = checkHTML(value, parent, property, snippetData);
                    if (finalValue)
                      return call(origin, this, position, finalValue);

                    return;

                  case "insertAdjacentText":
                    if (!checkShouldInsert(value, parent, property, snippetData))
                      return;
                    break;
                }
              }
            }
            return apply$2(origin, this, args);
          };
        }
      };
    }

    function getInnerHTMLDescriptor(
      target, property, shouldValidate, getSnippetData
    ) {
      let desc = getOwnPropertyDescriptor(target, property) || {};
      let {set: prevSetter} = desc;
      if (!prevSetter)
        return;

      return {
        set(htmlText) {
          if (!shouldValidate(this))
            return call(prevSetter, this, htmlText);

          let snippetData = getSnippetData(this);
          if (!snippetData)
            return call(prevSetter, this, htmlText);
          let finalValue = checkHTML(htmlText, this, property, snippetData);
          if (finalValue)
            return call(prevSetter, this, finalValue);
        }
      };
    }

    function getTextContentDescriptor(
      target, property, shouldValidate, getSnippetData
    ) {
      let desc = getOwnPropertyDescriptor(target, property) || {};
      let {set: prevSetter} = desc;
      if (!prevSetter)
        return;

      return {
        set(domString) {
          if (!shouldValidate(this))
            return call(prevSetter, this, domString);

          let snippetData = getSnippetData(this);
          if (!snippetData)
            return call(prevSetter, this, domString);
          if (checkShouldInsert(domString, this, property, snippetData))
            return call(prevSetter, this, domString);
        }
      };
    }
  }

  const {CanvasRenderingContext2D: CanvasRenderingContext2D$1,
         document: document$4,
         Map: Map$c,
         MutationObserver: MutationObserver$h,
         Object: Object$g,
         requestAnimationFrame,
         Set: Set$a,
         WeakMap: WeakMap$2,
         WeakSet: WeakSet$d} = $(window);

  const MAX_BUFFER = 10000;

  let canvasRules;
  let canvasTextBuffers = new WeakMap$2();
  let matchedCanvases = new WeakSet$d();
  let pendingHideCanvasElements = new Set$a();
  let hideCanvasSeenMap = new WeakSet$d();
  const hitFilters$m = new Set$a();

  let dataModeActive = false;
  let dataHooksInstalled = false;
  let dataCheckScheduled = false;
  let dirtyCanvases = new Set$a();

  function processMatch(canvasElement, rule) {
    matchedCanvases.add(canvasElement);
    canvasTextBuffers.delete(canvasElement);
    const elementToHide = $(canvasElement).closest(rule.selector);
    if (elementToHide && !hideCanvasSeenMap.has(elementToHide)) {
      hideElement(elementToHide);
      hideCanvasSeenMap.add(elementToHide);

      getDebugger("hide-if-canvas-contains")("success", "Matched: ", elementToHide, `\nFILTER: hide-if-canvas-contains ${rule.formattedArguments}`);
      const fStr =
        "hide-if-canvas-contains " +
        rule.formattedArguments;
      if (!hitFilters$m.has(fStr)) {
        hitFilters$m.add(fStr);
        sendSnippetHitEvent(fStr);
      }
    }
    else {
      scheduleElementToHide(canvasElement, rule);
    }
  }

  function markDirtyForDataMode(canvas) {
    if (!dataModeActive || !canvas || matchedCanvases.has(canvas))
      return;
    dirtyCanvases.add(canvas);
    if (dataCheckScheduled)
      return;
    dataCheckScheduled = true;
    requestAnimationFrame(runDataCheck);
  }

  function runDataCheck() {
    dataCheckScheduled = false;
    const canvases = dirtyCanvases;
    dirtyCanvases = new Set$a();
    const debugLog = getDebugger("hide-if-canvas-contains");
    for (const canvas of canvases) {
      if (matchedCanvases.has(canvas))
        continue;

      let dataURL = null;
      let encodeFailed = false;
      for (const [searchRegex, rule] of canvasRules) {
        if (rule.mode !== "data")
          continue;
        if (!$(canvas).closest(rule.selector))
          continue;
        if (encodeFailed)
          continue;
        if (dataURL === null) {
          try {

            dataURL = $(canvas).toDataURL().toString();
          }
          catch (error) {

            debugLog("info", "Could not read canvas data URL:", error.message);
            encodeFailed = true;
            continue;
          }
        }
        if (searchRegex.test(dataURL))
          processMatch(canvas, rule);
      }
    }
  }

  function installDataModeHooks() {
    if (dataHooksInstalled)
      return;
    dataHooksInstalled = true;
    const CanvasProto = CanvasRenderingContext2D$1.prototype;
    const methods = ["fillRect", "strokeRect", "putImageData", "fill", "stroke"];
    for (const name of methods) {
      const originalFunction = CanvasProto[name];
      if (typeof originalFunction !== "function")
        continue;
      let wrappedFunction = proxy(originalFunction, function(...args) {
        const result = apply$2(originalFunction, this, args);
        markDirtyForDataMode(this.canvas);
        return result;
      });
      proxyToStringCalls(wrappedFunction, originalFunction);

      Object$g.defineProperty(window.CanvasRenderingContext2D.prototype, name, {value: wrappedFunction});
    }
  }

  function hideIfCanvasContains(
    search, selector = "canvas", clearRectBehavior = "", mode = "") {
    const debugLog = getDebugger("hide-if-canvas-contains");
    const formattedArgsToLog = formatArguments(arguments);
    const {mark, end} = profile("hide-if-canvas-contains");

    if (!search) {
      debugLog("error", "The parameter 'search' is required");
      return;
    }

    if (!canvasRules) {
      mark();
      const CanvasProto = CanvasRenderingContext2D$1.prototype;
      debugLog("info", "CanvasRenderingContext2D proxied");

      function overrideFunctionInCanvas(functionName){
        const originalFunction = CanvasProto[functionName];

        let wrappedFunction = proxy(originalFunction, function(text, ...args) {
          const canvas = this.canvas;

          if (matchedCanvases.has(canvas))
            return apply$2(originalFunction, this, [text, ...args]);
          const accumulated =
            ((canvasTextBuffers.get(canvas) || "") + text)
              .slice(-MAX_BUFFER);
          canvasTextBuffers.set(canvas, accumulated);
          for (const [searchRegex, rule] of canvasRules) {

            if (rule.mode === "data")
              continue;
            if (searchRegex.test(accumulated))
              processMatch(canvas, rule);
          }
          const result = apply$2(originalFunction, this, [text, ...args]);
          markDirtyForDataMode(canvas);
          return result;
        });
        proxyToStringCalls(wrappedFunction, originalFunction);

        Object$g.defineProperty(window.CanvasRenderingContext2D.prototype, functionName, {
          value: wrappedFunction
        });
      }

      overrideFunctionInCanvas("fillText");
      overrideFunctionInCanvas("strokeText");

      function overrideClearRect() {
        const originalClearRect = CanvasProto.clearRect;
        let wrappedClearRect = proxy(originalClearRect, function(...args) {

          let forceAlways = false;
          let forceNever = true;
          for (const {clearRectBehavior: crb} of canvasRules.values()) {
            if (crb === "always")
              forceAlways = true;
            if (crb !== "never")
              forceNever = false;
          }

          if (!forceNever) {
            const [x, y, w, h] = args;
            const fullCanvas = x <= 0 && y <= 0 &&
              w >= this.canvas.width && h >= this.canvas.height;
            if (forceAlways || fullCanvas)
              canvasTextBuffers.delete(this.canvas);
          }
          const result = apply$2(originalClearRect, this, args);
          markDirtyForDataMode(this.canvas);
          return result;
        });
        proxyToStringCalls(wrappedClearRect, originalClearRect);

        Object$g.defineProperty(window.CanvasRenderingContext2D.prototype, "clearRect", {
          value: wrappedClearRect
        });
      }

      overrideClearRect();

      function overrideDrawImage() {
        const originalDrawImage = CanvasProto.drawImage;

        let wrappedDrawImage = proxy(originalDrawImage, function(image, ...args) {
          debugLog("info", "drawImage called with arguments:", image, ...args);
          if (image && typeof image.src === "string" && image.src) {
            for (const [searchRegex, rule] of canvasRules) {

              if (rule.mode === "data")
                continue;
              if (searchRegex.test(image.src))
                processMatch(this.canvas, rule);
            }
          }
          const result = apply$2(originalDrawImage, this, [image, ...args]);
          markDirtyForDataMode(this.canvas);
          return result;
        });
        proxyToStringCalls(wrappedDrawImage, originalDrawImage);

        Object$g.defineProperty(window.CanvasRenderingContext2D.prototype, "drawImage", {
          value: wrappedDrawImage
        });
      }

      overrideDrawImage();
      canvasRules = new Map$c();

      const mo = new MutationObserver$h(mutationsList => {
        for (let mutation of $(mutationsList)) {
          if (mutation.type === "childList") {

            checkPendingElements();
          }
        }
      });

      mo.observe(document$4, {childList: true, subtree: true});
      end();
    }

    const searchRegex = toRegExp(search);

    canvasRules.set(searchRegex, {selector, formattedArguments: formattedArgsToLog, clearRectBehavior, mode});

    if (mode === "data") {
      dataModeActive = true;
      installDataModeHooks();

      for (const canvas of $$("canvas"))
        markDirtyForDataMode(canvas);
    }
  }

  function scheduleElementToHide(canvasElement, rule) {
    pendingHideCanvasElements.add({canvasElement, rule});
  }

  function checkPendingElements() {
    pendingHideCanvasElements.forEach(entry => {
      const elementToHide = $(entry.canvasElement).closest(entry.rule.selector);
      if (elementToHide && !hideCanvasSeenMap.has(elementToHide)) {
        hideElement(elementToHide);
        hideCanvasSeenMap.add(elementToHide);
        pendingHideCanvasElements.delete(entry);

        getDebugger("hide-if-canvas-contains")("success", "Matched: ", elementToHide, `\nFILTER: hide-if-canvas-contains ${entry.rule.formattedArguments}`);
        const fStr =
          "hide-if-canvas-contains " +
          entry.rule.formattedArguments;
        if (!hitFilters$m.has(fStr)) {
          hitFilters$m.add(fStr);
          sendSnippetHitEvent(fStr);
        }
      }
    });
  }

  let {Array: Array$8, Error: Error$h, Map: Map$b, parseInt: parseInt$7} = $(window);

  let stack = null;
  let won = null;

  function race(action, winners = "1") {
    switch (action) {
      case "start":
        stack = {
          winners: parseInt$7(winners, 10) || 1,
          participants: new Map$b()
        };
        won = new Array$8();
        break;
      case "end":
      case "finish":
      case "stop":
        stack = null;
        for (let win of won)
          win();
        won = null;
        break;
      default:
        throw new Error$h(`Invalid action: ${action}`);
    }
  }

  function raceWinner(name, lose) {

    if (stack === null)
      return noop;

    let current = stack;
    let {participants} = current;
    participants.set(win, lose);

    return win;

    function win() {

      if (current.winners < 1)
        return;

      let debugLog = getDebugger("race");
      debugLog("success", `${name} won the race`);

      if (current === stack) {
        won.push(win);
      }
      else {
        participants.delete(win);
        if (--current.winners < 1) {
          for (let looser of participants.values())
            looser();

          participants.clear();
        }
      }
    }
  }

  const {Map: Map$a, MutationObserver: MutationObserver$g, Object: Object$f, Set: Set$9, WeakSet: WeakSet$c} = $(window);

  let ElementProto = Element.prototype;
  let {attachShadow} = ElementProto;

  let hiddenShadowRoots = new WeakSet$c();
  let searches = new Map$a();
  const hitFilters$l = new Set$9();
  let observer = null;

  function hideIfShadowContains(search, selector = "*") {

    const formattedArgs = formatArguments(arguments);

    let key = `${search}\\${selector}`;
    if (!searches.has(key)) {
      searches.set(key, [
        toRegExp(search),
        selector,
        raceWinner(
          "hide-if-shadow-contains",
          () => {
            searches.delete(key);
          }
        ),
        formattedArgs
      ]);
    }

    const debugLog = getDebugger("hide-if-shadow-contains");
    const {mark, end} = profile("hide-if-shadow-contains");

    if (!observer) {
      observer = new MutationObserver$g(records => {
        mark();
        let visited = new Set$9();
        for (let {target} of $(records)) {

          let parent = $(target).parentNode;
          while (parent)
            [target, parent] = [parent, $(target).parentNode];

          if (hiddenShadowRoots.has(target))
            continue;

          if (visited.has(target))
            continue;

          visited.add(target);
          for (let [
            re, selfOrParent, win, searchFormattedArgs
          ] of searches.values()) {
            if (re.test($(target).textContent)) {
              let closest = $(target.host).closest(selfOrParent);
              if (closest) {
                win();

                $(target).appendChild(
                  document.createElement("style")
                ).textContent = ":host {display: none !important}";

                hideElement(closest);

                hiddenShadowRoots.add(target);
                debugLog("success",
                         "Hiding: ",
                         closest,
                         `\nFILTER: hide-if-shadow-contains ${searchFormattedArgs}`);
                const fStr =
                  "hide-if-shadow-contains " +
                  searchFormattedArgs;
                if (!hitFilters$l.has(fStr)) {
                  hitFilters$l.add(fStr);
                  sendSnippetHitEvent(fStr);
                }
              }
              end();
            }
          }
        }
      });

      let wrappedAttachShadow = proxy(attachShadow, function() {

        let root = apply$2(attachShadow, this, arguments);
        debugLog("info", "attachShadow is called for: ", root);

        observer.observe(root, {
          childList: true,
          characterData: true,
          subtree: true
        });

        return root;
      });
      proxyToStringCalls(wrappedAttachShadow, attachShadow);
      Object$f.defineProperty(ElementProto, "attachShadow", {
        value: wrappedAttachShadow
      });
    }
  }

  const {Error: Error$g, Object: Object$e, Array: Array$7, parseFloat: parseFloat$5, isNaN: isNaN$3} = $(window);
  class JSONPath {

    constructor(query) {
      if (typeof query !== "string")
        throw new Error$g("JSONPath: query must be a string");
      if (!query.length)
        throw new Error$g("JSONPath: query must be a non-empty string");
      this._steps = this._tokenize(query);
    }

    _tokenize(query) {
      query = $(query);
      const steps = new Array$7();
      let i = 0;

      if (query[0].toString() === "$")
        i = 1;

      while (i < query.length) {
        let isRecursive = false;

        if (query.startsWith("..", i)) {
          isRecursive = true;
          i += 2;
        }
        else if (query[i].toString() === ".") {
          i++;
        }

        if (query[i].toString() === "[") {
          const end = query.indexOf("]", i);
          if (end === -1)
            throw new Error$g(`JSONPath: unclosed bracket in query "${query}"`);
          const inner = query.slice(i + 1, end);

          if (!inner.length)
            throw new Error$g(`JSONPath: empty bracket notation in query "${query}"`);

          if (inner.startsWith("?(")) {
            steps.push({type: "filter",
                        key: "?",
                        filter: this._parseFilter(inner),
                        recursive: isRecursive});
          }
          else {
            steps.push({type: "direct",
                        key: inner.replace(/['"]/g, "").toString(),
                        recursive: isRecursive});
          }
          i = end + 1;
        }
        else {

          const nextBoundary = query.slice(i).search(/[.[]/);
          const key = nextBoundary === -1 ?
          query.slice(i).toString() : query.slice(i, i + nextBoundary).toString();

          if (!key && !isRecursive)
            throw new Error$g(`JSONPath: trailing dot with no property name in query "${query}"`);

          if (key || isRecursive) {

            steps.push({type: "direct", key: key || "*",
                        recursive: isRecursive});
          }
          i += key.length;
        }
      }
      return steps;
    }

    _parseFilter(str) {
      str = $(str);
      const match = str.match(
        /(?:[@.]?)([\w]+(?:\.[\w]+)*)\s*([!=^$*]=|[<>]=?)\s*(?:['"](.+?)['"]|([\w.+-]+))\)/
      );
      if (!match)
        throw new Error$g(`JSONPath: invalid filter expression "${str}"`);
      return {
        property: match[1],
        operator: match[2],
        target: match[3] != null ? match[3] : match[4]
      };
    }

    evaluate(obj) {
      if (!obj || typeof obj !== "object")
        throw new Error$g("JSONPath: evaluate() requires an object or array");

      let targets = $([{parent: {root: obj}, key: "root"}]);
      for (const step of this._steps) {
        const nextTargets = [];
        for (const {parent, key} of targets) {
          const current = parent[key];
          if (!current || typeof current !== "object")
            continue;
          if (step.recursive)

            this._deepSearch(current, step, nextTargets);
          else

            this._match(current, step, nextTargets);
        }

        targets = nextTargets;
      }
      return targets;
    }

    _match(obj, step, out) {
      const keys = (step.key === "*" || step.key === "?") ?
        Object$e.keys(obj) : [step.key];
      for (const k of keys) {
        if (hasOwnProperty(obj, k)) {
          if (step.key === "?" && !this._test(obj[k], step.filter))
            continue;
          out.push({parent: obj, key: k});
        }
      }
    }

    _deepSearch(obj, step, out, depth = 10000) {
      this._match(obj, step, out);
      if (depth <= 0)
        return;
      for (const k of Object$e.keys(obj)) {
        if (obj[k] && typeof obj[k] === "object")
          this._deepSearch(obj[k], step, out, depth - 1);
      }
    }

    _test(obj, filter) {
      if (!filter || !obj)
        return false;

      let val = obj;
      for (const seg of $(filter.property).split(".")) {
        if (val == null || typeof val !== "object")
          return false;
        val = val[seg];
      }

      const value = $(val);
      const target = $(filter.target);
      const valueStr = value.toString();
      const targetStr = target.toString();

      const nValue = parseFloat$5(value);
      const nTarget = parseFloat$5(target);
      const isNumeric = !isNaN$3(nValue) && !isNaN$3(nTarget);

      switch (filter.operator) {
        case "==": return isNumeric ? nValue === nTarget : valueStr === targetStr;
        case "!=": return isNumeric ? nValue !== nTarget : valueStr !== targetStr;
        case "<": return isNumeric ? nValue < nTarget : valueStr < targetStr;
        case "<=": return isNumeric ? nValue <= nTarget : valueStr <= targetStr;
        case ">": return isNumeric ? nValue > nTarget : valueStr > targetStr;
        case ">=": return isNumeric ? nValue >= nTarget : valueStr >= targetStr;
        case "^=": return value.startsWith(target);
        case "$=": return value.endsWith(target);
        case "*=": return value.includes(target);
        default: return false;
      }
    }
  }

  const {Array: Array$6, Error: Error$f, JSON: JSON$4, Map: Map$9, Object: Object$d, Response: Response$2} = $(window);

  let paths$1 = null;
  const hitFilters$k = new Set();
  function sendHitOnce$3(filter) {
    if (!hitFilters$k.has(filter)) {
      hitFilters$k.add(filter);
      sendSnippetHitEvent(filter);
    }
  }

  function jsonOverride(rawOverridePaths, value,
                               rawNeedlePaths = "", filter = "") {
    if (!rawOverridePaths)
      throw new Error$f("[json-override snippet]: Missing paths to override.");

    if (typeof value == "undefined")
      throw new Error$f("[json-override snippet]: No value to override with.");

    let debugLog = getDebugger("json-override");
    const {mark, end} = profile("json-override");

    if (!paths$1) {
      mark();
      function overrideObject(obj, str) {
        for (let {formattedArgs,
                  prune,
                  jsonPathObjects,
                  needle,
                  filter: flt,
                  value: val} of paths$1.values()) {
          if (flt && !flt.test(str))
            continue;

          if ($(needle).some(path => !findOwner(obj, path)))
            return obj;

          for (let path of prune) {
            if (path.startsWith("jsonpath(")) {
              try {
                const engine = jsonPathObjects.get(path);
                const matches = engine.evaluate(obj);
                matches.forEach(({parent, key}) => {
                  debugLog("success", `JSONPath match found at [${key}], replaced with ${val}`, `\nFILTER: json-override ${formattedArgs}`);
                  sendHitOnce$3("json-override " + formattedArgs);
                  parent[key] = overrideValue(val);
                });
              }
              catch (e) {
                debugLog("error", `JSONPath evaluation failed for: ${path}. Error: ${e.message}`);
              }
            }
            else if (path.includes("{}") || path.includes("[]")) {
              overridePathWithPlaceholders(obj, path, val, formattedArgs);
            }
            else {
              overridePathSimple(obj, path, val, formattedArgs);
            }
          }
        }
        return obj;
      }

      function overridePathWithPlaceholders(obj, path, newValue, formattedArgs) {
        let pathParts = $(path).split(".");
        let currentObj = obj;

        for (let i = 0; i < pathParts.length; i++) {
          let part = pathParts[i];

          if (part === "[]") {

            if (Array$6.isArray(currentObj)) {
              debugLog("info", `Iterating over array at: ${part}`);
              $(currentObj).forEach(item => {
                if (item !== null && typeof item !== "undefined") {
                  overridePathWithPlaceholders(item,
                                               pathParts.slice(i + 1).join("."),
                                               newValue,
                                               formattedArgs);
                }
              });
            }
            return;
          }
          else if (part === "{}") {

            if (currentObj && typeof currentObj === "object") {
              debugLog("info", `Iterating over object at: ${part}`);
              Object$d.keys(currentObj).forEach(key => {
                let nextItem = currentObj[key];
                if (nextItem !== null && typeof nextItem !== "undefined") {
                  overridePathWithPlaceholders(nextItem,
                                               pathParts.slice(i + 1).join("."),
                                               newValue,
                                               formattedArgs);
                }
              });
            }
            return;
          }
          else if (currentObj && typeof currentObj === "object" &&
            hasOwnProperty(currentObj, part)) {

            if (i === pathParts.length - 1) {
              debugLog("success", `Found ${path}, replaced it with ${newValue}`, `\nFILTER: json-override ${formattedArgs}`);
              sendHitOnce$3("json-override " + formattedArgs);
              currentObj[part] = overrideValue(newValue);
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

      function overridePathSimple(obj, path, newValue, formattedArgs) {
        let details = findOwner(obj, path);
        if (typeof details != "undefined") {
          debugLog("success", `Found ${path}, replaced it with ${newValue}`, `\nFILTER: json-override ${formattedArgs}`);
          sendHitOnce$3("json-override " + formattedArgs);
          details[0][details[1]] = overrideValue(newValue);
        }
      }

      let {parse} = JSON$4;
      paths$1 = new Map$9();

      let wrappedParse = proxy(parse, function(str) {
        let result = apply$2(parse, this, arguments);
        return overrideObject(result, str);
      });
      proxyToStringCalls(wrappedParse, parse);
      Object$d.defineProperty(window.JSON, "parse", {
        value: wrappedParse
      });
      debugLog("info", "Wrapped JSON.parse for override");

      let {json} = Response$2.prototype;
      Object$d.defineProperty(window.Response.prototype, "json", {
        value: proxy(json, function(str) {
          let resultPromise = apply$2(json, this, arguments);
          return resultPromise.then(obj => overrideObject(obj, str));
        })
      });
      debugLog("info", "Wrapped Response.json for override");
      end();
    }

    const formattedArgsToLog = formatArguments(arguments);

    const pruneList = $(rawOverridePaths).split(/ +/);
    const jsonPathObjects = new Map$9();
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

    paths$1.set(rawOverridePaths, {
      formattedArgs: formattedArgsToLog,
      prune: pruneList,
      jsonPathObjects,
      needle: rawNeedlePaths.length ? $(rawNeedlePaths).split(/ +/) : [],
      filter: filter ? toRegExp(filter) : null,
      value
    });
  }

  let {Array: Array$5, Error: Error$e, JSON: JSON$3, Map: Map$8, Object: Object$c, Response: Response$1} = $(window);

  let paths = null;
  const hitFilters$j = new Set();
  function sendHitOnce$2(filter) {
    if (!hitFilters$j.has(filter)) {
      hitFilters$j.add(filter);
      sendSnippetHitEvent(filter);
    }
  }

  function jsonPrune(rawPrunePaths,
                            rawNeedlePaths = "",
                            rawNeedleStack = "") {
    if (!rawPrunePaths)
      throw new Error$e("Missing paths to prune");

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

          if ($(needle).length > 0 &&
            $(needle).some(path => !findOwner(obj, path)))
            return obj;

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
                  sendHitOnce$2("json-prune " + formattedArgs);
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
            if (Array$5.isArray(currentObj)) {
              debugLog("info", `Iterating over array at: ${part}`);
              $(currentObj).forEach(item =>
                prunePathWithPlaceholders(item,
                                          pathParts.slice(i + 1).join("."),
                                          formattedArgs));
            }
            return;
          }
          else if (part === "[-]") {
            if (Array$5.isArray(currentObj)) {
              debugLog("info", `Iterating over array with element removal at: ${part}`);
              let remainingPath = pathParts.slice(i + 1).join(".");
              let indicesToRemove = [];

              $(currentObj).forEach((item, index) => {
                if (shouldRemoveElement(item, remainingPath))
                  indicesToRemove.push(index);
              });

              for (let j = indicesToRemove.length - 1; j >= 0; j--) {
                debugLog("success", `Found element at index ${indicesToRemove[j]} matching ${remainingPath} and removed entire element, \nFILTER: json-prune ${formattedArgs}`);
                sendHitOnce$2("json-prune " + formattedArgs);
                currentObj.splice(indicesToRemove[j], 1);
              }
            }
            return;
          }
          else if (part === "{}") {
            if (typeof currentObj === "object" && currentObj !== null) {
              debugLog("info", `Iterating over object at: ${part}`);
              Object$c.keys(currentObj).forEach(key =>
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

              Object$c.keys(currentObj).forEach(key => {
                if (shouldRemoveElement(currentObj[key], remainingPath))
                  keysToRemove.push(key);
              });

              keysToRemove.forEach(key => {
                debugLog("success", `Found object key ${key} matching ${remainingPath} and removed entire element, \nFILTER: json-prune ${formattedArgs}`);
                sendHitOnce$2("json-prune " + formattedArgs);
                delete currentObj[key];
              });
            }
            return;
          }
          else if (currentObj && typeof currentObj === "object" &&
            hasOwnProperty(currentObj, part)) {
            if (i === pathParts.length - 1) {
              debugLog("success", `Found ${path} and deleted, \nFILTER: json-prune ${formattedArgs}`);
              sendHitOnce$2("json-prune " + formattedArgs);
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
            if (Array$5.isArray(currentObj)) {
              return $(currentObj).some(item =>
                shouldRemoveElement(item, pathParts.slice(i + 1).join(".")));
            }
            return false;
          }
          else if (part === "{}") {
            if (typeof currentObj === "object" && currentObj !== null) {
              return Object$c.keys(currentObj).some(key =>
                shouldRemoveElement(currentObj[key],
                                    pathParts.slice(i + 1).join(".")));
            }
            return false;
          }
          else if (currentObj && typeof currentObj === "object" &&
            hasOwnProperty(currentObj, part)) {
            if (i === pathParts.length - 1)
              return true;
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
          sendHitOnce$2("json-prune " + formattedArgs);
          delete details[0][details[1]];
        }
      }

      let {parse} = JSON$3;
      paths = new Map$8();

      let wrappedParse = proxy(parse, function() {
        let result = apply$2(parse, this, arguments);
        return pruneObject(result);
      });
      proxyToStringCalls(wrappedParse, parse);
      Object$c.defineProperty(window.JSON, "parse", {
        value: wrappedParse
      });
      debugLog("info", "Wrapped JSON.parse for prune");

      let {json} = Response$1.prototype;
      let wrappedJson = proxy(json, function() {
        let resultPromise = apply$2(json, this, arguments);
        return resultPromise.then(obj => pruneObject(obj));
      });
      proxyToStringCalls(wrappedJson, json);
      Object$c.defineProperty(window.Response.prototype, "json", {
        value: wrappedJson
      });
      debugLog("info", "Wrapped Response.json for prune");
      end();
    }

    const formattedArgs = formatArguments(arguments);

    const pruneList = $(rawPrunePaths).split(/ +/);
    const jsonPathObjects = new Map$8();
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

  const {Error: Error$d, Object: Object$b, Map: Map$7} = $(window);

  let mapValues = null;
  const hitFilters$i = new Set();
  function sendHitOnce$1(filter) {
    if (!hitFilters$i.has(filter)) {
      hitFilters$i.add(filter);
      sendSnippetHitEvent(filter);
    }
  }

  function isMatchingValue(val, needle, pathSegments) {

    if (!pathSegments.length) {
      if (typeof val === "string" || typeof val === "number") {
        const valStr = val.toString();
        return needle.test(valStr);
      }
      return false;
    }

    let current = val;
    for (const segment of pathSegments) {

      if (!current || !hasOwnProperty(current, segment))
        return false;
      current = current[segment];
    }

    if (typeof current === "string" || typeof current === "number") {
      const currStr = current.toString();
      return needle.test(currStr);
    }

    return false;
  }

  function mapOverride(method, needle, returnValue = "", path,
                              stack) {
    if (!method)
      throw new Error$d("[map-override snippet]: Missing method to override.");

    if (!needle)
      throw new Error$d("[map-override snippet]: Missing needle.");

    if (!mapValues)
      mapValues = new Map$7();

    let debugLog = getDebugger("map-override");
    const {mark, end} = profile("map-override");
    const {set, get, has} = Map$7.prototype;
    const formattedArgsToLog = formatArguments(arguments);

    if (method === "set" && !mapValues.has("set")) {
      mark();
      call(set, mapValues, "set", $([]));

      let wrappedSet = proxy(set, function(key, val) {
        const overrideVals = call(get, mapValues, "set");
        for (const {needleRegex, pathSegments, stackNeedles} of overrideVals) {
          if (isMatchingValue(val, needleRegex, pathSegments) &&
              matchesStackTrace(stackNeedles, debugLog)) {
            debugLog("success", `Map.set is ignored for value matching needle: ${needleRegex}\nFILTER: map-override ${formattedArgsToLog}`);
            sendHitOnce$1("map-override " + formattedArgsToLog);
            return this;
          }
        }
        return apply$2(set, this, arguments);
      });
      proxyToStringCalls(wrappedSet, set);
      Object$b.defineProperty(window.Map.prototype, "set", {
        value: wrappedSet
      });
      debugLog("info", "Wrapped Map.prototype.set");
      end();
    }

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
              sendHitOnce$1("map-override " + formattedArgsToLog);
              return retVal;
            }
          }
        }
        return apply$2(get, this, arguments);
      });
      proxyToStringCalls(wrappedGet, get);
      Object$b.defineProperty(window.Map.prototype, "get", {
        value: wrappedGet
      });
      debugLog("info", "Wrapped Map.prototype.get");
      end();
    }

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
              sendHitOnce$1("map-override " + formattedArgsToLog);
              return retVal;
            }
          }
        }
        return apply$2(has, this, arguments);
      });
      proxyToStringCalls(wrappedHas, has);
      Object$b.defineProperty(window.Map.prototype, "has", {
        value: wrappedHas
      });
      debugLog("info", "Wrapped Map.prototype.has");
      end();
    }

    const needleRegex = toRegExp(needle);
    let pathSegments = [];
    if (path)
      pathSegments = path.split(".");

    let stackNeedles = [];
    if (stack)
      stackNeedles = stack.split(",").map(s => s.trim());

    const overrideVals = call(get, mapValues, method);

    let retVal;
    if (method === "get") {

      retVal = returnValue === "" ? void 0 : returnValue;
    }
    else if (method === "has") {

      retVal = returnValue === "true";
    }

    overrideVals.push({needleRegex, retVal, pathSegments, stackNeedles});
    call(set, mapValues, method, overrideVals);
  }

  let {Error: Error$c} = $(window);

  function overridePropertyRead(property, value, setConfigurable) {
    if (!property) {
      throw new Error$c("[override-property-read snippet]: " +
                       "No property to override.");
    }
    if (typeof value === "undefined") {
      throw new Error$c("[override-property-read snippet]: " +
                       "No value to override with.");
    }

    const formattedArguments = formatArguments(arguments);
    let debugLog = getDebugger("override-property-read");
    const {mark, end} = profile("override-property-read");

    let cValue = overrideValue(value);

    let hitEventSent = false;
    let newGetter = () => {
      debugLog("success", `${property} override done.`, "\nFILTER: override-property-read", formattedArguments);
      if (!hitEventSent) {
        hitEventSent = true;
        sendSnippetHitEvent("override-property-read " + formattedArguments);
      }
      return cValue;
    };

    debugLog("info", `Overriding ${property}.`);

    const configurableFlag = !(setConfigurable === "false");
    mark();
    wrapPropertyAccess(window,
                       property,
                       {get: newGetter, set() {}},
                       configurableFlag);
    end();
  }

  const {
    Array: Array$4,
    addEventListener: addEventListener$1,
    Error: Error$b,
    Object: Object$a,
    Reflect: Reflect$1,
    Set: Set$8,
    WeakSet: WeakSet$b
  } = $(window);

  const matchedElements = new WeakSet$b();
  const activeFilters$1 = new Array$4();
  const hitFilters$h = new Set$8();

  const patchedPrototypes = new Set$8();

  function preventElementSrcLoading(tagName, search) {
    if (!tagName || typeof tagName !== "string") {
      throw new Error$b(
        "[prevent-element-src-loading snippet]: tagName param must be a string."
      );
    }
    if (!search) {
      throw new Error$b(
        "[prevent-element-src-loading snippet]: Missing search parameter."
      );
    }
    tagName = $(tagName).toString().toLowerCase();
    if (!$(["script", "img", "iframe", "link"]).includes(tagName)) {
      throw new Error$b(
        "[prevent-element-src-loading snippet]: tagName parameter is incorrect."
      );
    }
    const srcMockData = {

      script: "data:text/javascript;base64,KCk9Pnt9",

      img: "data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==",

      iframe: "data:text/html;base64,PGRpdj48L2Rpdj4=",

      link: "data:text/plain;base64,"
    };

    const constructors = {
      script: window.HTMLScriptElement,
      img: window.HTMLImageElement,
      iframe: window.HTMLIFrameElement,
      link: window.HTMLLinkElement
    };
    const instance = constructors[tagName];

    const sourcePropertyName = tagName === "link" ? "href" : "src";
    const onerrorPropertyName = "onerror";
    const debugLog = getDebugger("[prevent-element-src-loading snippet]");
    const formattedArgsToLog = formatArguments(arguments);
    const filterStr =
      "prevent-element-src-loading " + formattedArgsToLog;
    const {mark, end} = profile("prevent-element-src-loading");
    mark();
    const searchRegex = toRegExp(search);
    activeFilters$1.push({tagName, searchRegex});
    debugLog("info", `Added filter rule\nFILTER: prevent-element-src-loading ${formattedArgsToLog}`);

    if (!patchedPrototypes.has(tagName)) {
      patchedPrototypes.add(tagName);
      let setAttributeWrapper = (target, thisArg, args) => {

        if (!args[0] || !args[1])
          return Reflect$1.apply(target, thisArg, args);

        const nodeName = thisArg.nodeName.toLowerCase();
        const attrName = args[0].toLowerCase();
        const attrValue = args[1];
        const isMatched = attrName === sourcePropertyName &&
        activeFilters$1.some(f =>
          nodeName === f.tagName &&
          f.searchRegex.test(attrValue)
        );
        if (!isMatched)
          return Reflect$1.apply(target, thisArg, args);
        matchedElements.add(thisArg);

        debugLog(
          "success",
          `Replaced setAttribute for ${attrName}: ${attrValue} → ${srcMockData[nodeName]}`);
        if (!hitFilters$h.has(filterStr)) {
          hitFilters$h.add(filterStr);
          sendSnippetHitEvent(filterStr);
        }
        return Reflect$1.apply(target, thisArg, [attrName, srcMockData[nodeName]]);
      };
      const setAttributeHandler = {
        apply: setAttributeWrapper
      };
      instance.prototype.setAttribute =
        new Proxy(instance.prototype.setAttribute, setAttributeHandler);
      debugLog("info", "Wrapped setAttribute function");

      const origSrcDescriptor =
        Object$a.getOwnPropertyDescriptor(instance.prototype, sourcePropertyName);
      if (!origSrcDescriptor)
        return;
      Object$a.defineProperty(instance.prototype, sourcePropertyName, {
        enumerable: true,
        configurable: true,
        get() {
          return origSrcDescriptor.get.call(this);
        },
        set(urlValue) {
          const nodeName = this.nodeName.toLowerCase();
          const isMatched = activeFilters$1.some(f =>
            nodeName === f.tagName &&
            f.searchRegex.test(urlValue)
          );
          if (!isMatched) {
            origSrcDescriptor.set.call(this, urlValue);
            return;
          }

          matchedElements.add(this);
          debugLog("success", `Replaced in src/href setter ${urlValue} → ${srcMockData[nodeName]}`);
          if (!hitFilters$h.has(filterStr)) {
            hitFilters$h.add(filterStr);
            sendSnippetHitEvent(filterStr);
          }
          origSrcDescriptor.set.call(this, srcMockData[nodeName]);
        }
      });
      debugLog("info", "Wrapped src/href property setter");
    }

    if (activeFilters$1.length === 1) {
      const origOnerrorDescriptor =
        Object$a.getOwnPropertyDescriptor(
          HTMLElement.prototype,
          onerrorPropertyName);
      if (!origOnerrorDescriptor)
        return;
      Object$a.defineProperty(HTMLElement.prototype, onerrorPropertyName, {
        enumerable: true,
        configurable: true,
        get() {
          return origOnerrorDescriptor.get.call(this);
        },
        set(cb) {
          const isMatched = matchedElements.has(this);

          if (!isMatched) {
            origOnerrorDescriptor.set.call(this, cb);
            return;
          }
          debugLog("success", `Replaced in onerror setter ${cb} → () => {}`);
          if (!hitFilters$h.has(filterStr)) {
            hitFilters$h.add(filterStr);
            sendSnippetHitEvent(filterStr);
          }
          origOnerrorDescriptor.set.call(this, () => {});
        }
      });
      debugLog("info", "Wrapped onerror property setter");

      const addEventListenerWrapper = (target, thisArg, args) => {

        if (!args[0] || !args[1] || !thisArg)
          return Reflect$1.apply(target, thisArg, args);

        const eventName = args[0];
        const isMatched = typeof thisArg.getAttribute === "function" &&
          matchedElements.has(thisArg) &&
          eventName === "error";

        if (isMatched) {
          debugLog("success", `Replaced error event handler on ${thisArg} with () => {}`);
          if (!hitFilters$h.has(filterStr)) {
            hitFilters$h.add(filterStr);
            sendSnippetHitEvent(filterStr);
          }
          return Reflect$1.apply(target, thisArg, [eventName, () => {}]);
        }
        return Reflect$1.apply(target, thisArg, args);
      };
      const addEventListenerHandler = {
        apply: addEventListenerWrapper
      };
      EventTarget.prototype.addEventListener =
        new Proxy(
          EventTarget.prototype.addEventListener,
          addEventListenerHandler);
      debugLog("info", "Wrapped addEventListener");

      const preventInlineOnerror = () => {
        addEventListener$1("error", event => {
          const target = event.target;
          if (!target || !target.nodeName)
            return;
          const url = target.src || target.href;
          const nodeName = target.nodeName.toLowerCase();
          const isMatched = activeFilters$1.some(f =>
            nodeName === f.tagName && url && f.searchRegex.test(url)
          );
          if (!isMatched)
            return;
          target.onerror = () => {};
        }, true);
        debugLog("info", "Added event listener to defuse global errors");
      };
      preventInlineOnerror();
    }
    end();
  }

  let {Error: Error$a, Map: Map$6, Object: Object$9, console: console$1} = $(window);

  let {toString: toString$1} = Function.prototype;
  let EventTargetProto = EventTarget.prototype;
  let {addEventListener} = EventTargetProto;

  let events = null;
  const hitFilters$g = new Set();

  function preventListener(event, eventHandler, selector) {
    if (!event)
      throw new Error$a("[prevent-listener snippet]: No event type.");

    if (!events) {
      events = new Map$6();

      let debugLog = getDebugger("[prevent]");
      const {mark, end} = profile("prevent-listener");

      let wrappedAddEventListener = proxy(
        addEventListener,
        function(type, listener) {
          mark();
          for (let {evt, handlers, selectors, formattedArgs} of events.values()) {

            if (!evt.test(type))
              continue;

            let isElement = this instanceof Element;

            for (let i = 0; i < handlers.length; i++) {
              const handler = handlers[i];
              const sel = selectors[i];

              if (sel && !(isElement && $(this).matches(sel)))
                continue;

              if (handler) {
                const proxiedHandlerMatch = function() {
                  try {
                    const proxiedHandlerString = call(
                      toString$1,
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

                if (!proxiedHandlerMatch() && !actualHandlerMatch())
                  continue;
              }

              const filter =
                "prevent-listener " + formattedArgs;
              if (!hitFilters$g.has(filter)) {
                hitFilters$g.add(filter);
                sendSnippetHitEvent(filter);
              }
              if (debug()) {
                console$1.groupCollapsed("DEBUG [prevent] was successful", `\nFILTER: prevent-listener ${formattedArgs}`);
                debugLog("success", `type: ${type} matching ${evt}`);
                debugLog("success", "handler:", listener);
                if (handler)
                  debugLog("success", `matching ${handler}`);
                if (sel)
                  debugLog("success", "on element: ", this, ` matching ${sel}`);
                debugLog("success", "was prevented from being added");
                console$1.groupEnd();
              }
              return;
            }
          }
          end();
          return apply$2(addEventListener, this, arguments);
        }
      );
      proxyToStringCalls(wrappedAddEventListener, addEventListener);
      Object$9.defineProperty(EventTargetProto, "addEventListener", {
        value: wrappedAddEventListener
      });

      debugLog("info", "Wrapped addEventListener");
    }

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

  const NativeProxy = Proxy;
  const {toStringTag} = Symbol;
  const {
    defineProperty: reflectDefineProperty,
    deleteProperty: reflectDeleteProperty,
    get: reflectGet,
    getOwnPropertyDescriptor: reflectGetOwnPropertyDescriptor,
    has: reflectHas,
    set: reflectSet
  } = bound(Reflect);
  const {Array: Array$3, Error: Error$9, Map: Map$5, Object: Object$8, Set: Set$7,
         document: document$3, parseFloat: parseFloat$4, setTimeout: setTimeout$3} = $(window);

  const activeFilters = new Array$3();
  const hitFilters$f = new Set$7();
  function sendHitOnce(filter) {
    if (!hitFilters$f.has(filter)) {
      hitFilters$f.add(filter);
      sendSnippetHitEvent(filter);
    }
  }

  const facadeNames = new Set$7([
    "closed", "close", "opener", "frameElement",
    "parent", "top", "self", "window", "globalThis", "frames",
    "location", "document", "history", toStringTag
  ]);

  function preventWindowOpen(pattern = "", delay = "", decoy = "iframe") {
    if (decoy === "")
      decoy = "iframe";
    if (decoy !== "iframe" && decoy !== "obj" && decoy !== "blank") {
      throw new Error$9(
        "[prevent-window-open snippet]: decoy must be iframe, obj or blank."
      );
    }

    let invert = false;
    if ($(pattern).startsWith("!")) {
      invert = true;
      pattern = $(pattern).slice(1);
    }

    activeFilters.push({
      regex: toRegExp(pattern),
      invert,
      hasDelay: delay !== "",
      autoRemoveAfter: parseFloat$4(delay) || 0,
      decoy,
      formattedArgs: formatArguments(arguments)
    });

    if (activeFilters.length > 1)
      return;

    const debugLog = getDebugger("[prevent-window-open]");
    const {mark, end} = profile("prevent-window-open");

    const openDescriptor = Object$8.getOwnPropertyDescriptor(window, "open");
    if (!openDescriptor || typeof openDescriptor.value !== "function" ||
        !openDescriptor.configurable) {
      debugLog("warn", "window.open not wrappable, bailing out");
      return;
    }
    const nativeOpen = openDescriptor.value;

    const fakePopup = (autoRemoveAfter = 0, cleanup = () => {}) => {
      let isClosed = false;

      const closePopup = () => {
        if (isClosed)
          return;
        isClosed = true;
        cleanup();
      };

      setTimeout$3(closePopup, autoRemoveAfter);

      const fakeLocation = {
        href: "about:blank",
        assign() {}, replace() {}, reload() {},
        toString() {
          return "about:blank";
        }
      };

      const fakeDocument = {
        location: fakeLocation,
        defaultView: null,
        cookie: "",
        open() {}, write() {}, writeln() {}, close() {}
      };
      const fakeHistory = {
        length: 0, state: null, scrollRestoration: "auto",
        back() {}, forward() {}, go() {}, pushState() {}, replaceState() {}
      };

      const noops = new Map$5();

      const popupTarget = Object$8.create(Object$8.create(null));

      const popup = new NativeProxy(popupTarget, {
        get(target, prop, receiver) {

          if (reflectGetOwnPropertyDescriptor(target, prop))
            return reflectGet(target, prop, receiver);

          if (prop === "closed")
            return isClosed;
          if (prop === "close")
            return closePopup;
          if (prop === "opener")
            return window;
          if (prop === "frameElement")
            return null;

          if (prop === toStringTag)
            return "Window";

          if (prop === "parent" || prop === "top" || prop === "self" ||
              prop === "window" || prop === "globalThis" || prop === "frames")
            return receiver;
          if (prop === "location")
            return fakeLocation;
          if (prop === "document")
            return fakeDocument;
          if (prop === "history")
            return fakeHistory;

          let value;
          try {
            value = reflectGet(window, prop);
          }
          catch (error) {
            return void 0;
          }
          if (typeof value === "function") {
            let noop = noops.get(prop);
            if (!noop) {
              noop = () => {};
              noops.set(prop, noop);
            }
            return noop;
          }

          if (value !== null && typeof value === "object")
            return void 0;
          return value;
        },
        set(target, prop, value) {

          if (prop === "location" || prop === "opener")
            return true;

          if (facadeNames.has(prop))
            return true;
          return reflectSet(target, prop, value);
        },
        defineProperty(target, prop, descriptor) {

          if (facadeNames.has(prop))
            return false;
          return reflectDefineProperty(target, prop, descriptor);
        },
        deleteProperty(target, prop) {

          return reflectDeleteProperty(target, prop);
        },
        has(target, prop) {

          return facadeNames.has(prop) ||
                 reflectHas(target, prop) || reflectHas(window, prop);
        },

        setPrototypeOf() {
          return false;
        },
        preventExtensions() {
          return false;
        }
      });
      fakeDocument.defaultView = popup;
      return popup;
    };

    const wrappedOpen = proxy(nativeOpen, function(url) {
      mark();

      const callArgs = new Array$3(arguments.length);
      for (let i = 0; i < arguments.length; i++)
        callArgs[i] = arguments[i];
      const haystack = callArgs.join(" ");

      for (let index = 0; index < activeFilters.length; index++) {
        const rule = activeFilters[index];
        if (rule.regex.test(haystack) === rule.invert)
          continue;

        sendHitOnce("prevent-window-open " + rule.formattedArgs);
        debugLog("success", `Prevented window.open(${haystack})`, `\nFILTER: prevent-window-open ${rule.formattedArgs}`);
        end();

        if (!rule.hasDelay)
          return null;

        if (rule.decoy === "blank") {
          callArgs[0] = "about:blank";
          const realPopup = apply$2(nativeOpen, this, callArgs);

          const closePopup = realPopup && realPopup.close;
          if (typeof closePopup === "function") {
            setTimeout$3(
              () => apply$2(closePopup, realPopup, []), rule.autoRemoveAfter
            );
          }
          return realPopup;
        }

        const tag = rule.decoy === "obj" ? "object" : "iframe";
        const urlProp = rule.decoy === "obj" ? "data" : "src";
        let decoyElem;
        try {
          decoyElem = $(document$3).createElement(tag);

          decoyElem[urlProp] =
            (url === void 0 || url === null) ? "about:blank" : url;

          const {style} = $(decoyElem, "HTMLElement");
          const $style = $(style, "CSSStyleDeclaration");
          $style.setProperty("height", "1px", "important");
          $style.setProperty("position", "fixed", "important");
          $style.setProperty("top", "-1px", "important");
          $style.setProperty("width", "1px", "important");
          const parent = $(document$3).body || $(document$3).documentElement;
          $(parent).appendChild(decoyElem);
        }
        catch (error) {

          if (decoyElem) {
            try {
              $(decoyElem).remove();
            }
            catch (cleanupError) {

            }
          }
          return fakePopup(rule.autoRemoveAfter);
        }

        return fakePopup(rule.autoRemoveAfter, () => $(decoyElem).remove());
      }
      debugLog("info", `Allowed window.open(${haystack})`);
      end();
      return apply$2(nativeOpen, this, arguments);
    });
    proxyToStringCalls(wrappedOpen, nativeOpen);
    Object$8.defineProperty(
      window, "open", {...openDescriptor, value: wrappedOpen}
    );
    debugLog("info", "Wrapped window.open");
  }

  let {Array: Array$2, Map: Map$4, Object: Object$7, parseInt: parseInt$6, RegExp: RegExp$4, Set: Set$6} = $(window);

  const rulesByMethod = new Map$4();

  const patchedMethods = new Set$6();

  const hitFilters$e = new Set$6();

  function toGlobalRegExp(pattern) {
    const base = toRegExp(pattern);
    return new RegExp$4(base.source, base.flags + "g");
  }

  function replaceArgument(methodPath, argPosition,
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
    const position = parseInt$6(posStr, 10);

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
      rules = new Array$2();
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

                if (original !== null && typeof original === "object")
                  continue;

                const originalAsStr = "" + original;
                replaced = $(originalAsStr)
                  .replace(thisRule.search, thisRule.replacement).toString();

                if (replaced === originalAsStr)
                  continue;
              }

              const newArgs = Array$2.from(arguments);
              newArgs[thisRule.argPosition] = replaced;
              if (!hitFilters$e.has(thisRule.filterStr)) {
                hitFilters$e.add(thisRule.filterStr);
                sendSnippetHitEvent(thisRule.filterStr);
              }
              debugLog(
                "success",
                `argument ${thisRule.argPosition} of ${methodPath} replaced` +
                `\nFILTER: ${thisRule.filterStr}`
              );
              applyArgs = newArgs;

              break;
            }
          }
        }
        catch (e) {

          applyArgs = arguments;
        }
        return apply$2(nativeMethod, this, applyArgs);
      });
      proxyToStringCalls(wrappedMethod, nativeMethod);
      Object$7.defineProperty(parent, method, {value: wrappedMethod});
      debugLog("info", `${methodPath} wrapped`);
      end();
    }
  }

  let {fetch} = $(window);

  let hasFetchBeenProxied = false;

  const preFetchCallbacks = [];

  const postFetchCallbacks = [];

  const proxyFetch = () => {

    if (!hasFetchBeenProxied) {
      let wrappedFetch = proxy(fetch, (...args) => {
        let [source] = args;

        let requestUrl =
          typeof source === "string" ? source :
            (source && typeof source.url === "string" ? source.url : "");
        if (preFetchCallbacks.length > 0 && typeof source === "string") {
          let url;
          try {
            url = new URL(source);
          }
          catch (e) {
            if (e instanceof TypeError)
              url = new URL(source, $(document).location);
            else
              throw e;
          }
          preFetchCallbacks.forEach(fn => fn(url));
          args[0] = url.href;
          requestUrl = url.href;
        }

        const promise = apply$2(fetch, self, args).then(origResponse => {
          let transformedResponse = origResponse;
          postFetchCallbacks.forEach(fn => {
            transformedResponse = fn(transformedResponse, {url: requestUrl});
          });
          return transformedResponse;
        });
        return promise;
      });
      proxyToStringCalls(wrappedFetch, window.fetch);
      window.fetch = wrappedFetch;
      hasFetchBeenProxied = true;
    }
  };

  const addPreFetchCallback = callback => {
    preFetchCallbacks.push(callback);
    proxyFetch();
  };

  const addPostFetchCallback = callback => {
    postFetchCallbacks.push(callback);
    proxyFetch();
  };

  let {Map: Map$3, Object: Object$6, RegExp: RegExp$3, Response} = $(window);
  let fetchRules;
  const hitFilters$d = new Set();

  function replaceFetchResponse(search, replacement = "", needle = null) {
    const formattedArgsToLog = formatArguments(arguments);
    const debugLog = getDebugger("replace-fetch-response");
    const {mark, end} = profile("replace-fetch-response");
    if (!search) {
      debugLog("error", "The parameter 'search' is required");
      return;
    }

    if (!fetchRules) {
      const mainLogic = origResponse => {
        mark();
        const clonedResponse = $(origResponse).clone();
        return clonedResponse.text().then(origText => {
          let replacedText = $(origText);

          for (const [thisSearch, {replacement: thisReplacement, needle: thisNeedle, formattedArgs}] of fetchRules) {
            if (thisNeedle) {
              const needleRegex = toRegExp(thisNeedle);

              if (needleRegex.test(replacedText)) {
                if (debug()) {
                  console.groupCollapsed(`DEBUG [replace-fetch-response] success: '${thisNeedle}' found in fetch response`);
                  debugLog("info", `${replacedText}`);
                  console.groupEnd();
                }
              }
              else {
                if (debug()) {
                  console.groupCollapsed(`DEBUG [replace-fetch-response] warn: '${thisNeedle}' not found in fetch response`);
                  debugLog("warn", `${replacedText}`);
                  console.groupEnd();
                }
                continue;
              }
            }
            const prevText = replacedText.toString();
            replacedText = replacedText.replace(thisSearch, thisReplacement);
            if (replacedText.toString() !== prevText) {
              const filter = "replace-fetch-response " + formattedArgs;
              if (!hitFilters$d.has(filter)) {
                hitFilters$d.add(filter);
                sendSnippetHitEvent(filter);
              }
              if (debug()) {
                console.groupCollapsed(`DEBUG [replace-fetch-response] success: '${thisSearch}' replaced with '${thisReplacement}' in fetch response`,
                  `\nFILTER: replace-fetch-response ${formattedArgs}`
                );
                debugLog("success", `${replacedText}`);
                console.groupEnd();
              }
            }
          }

          if (replacedText.toString() === origText.toString())
            return origResponse;

          const replacedResponse = new Response(replacedText.toString(), {
            status: origResponse.status,
            statusText: origResponse.statusText,
            headers: origResponse.headers
          });
          Object$6.defineProperties(replacedResponse, {
            ok: {value: origResponse.ok},
            redirected: {value: origResponse.redirected},
            type: {value: origResponse.type},
            url: {value: origResponse.url}
          });
          end();
          return replacedResponse;
        });
      };

      fetchRules = new Map$3();
      debugLog("info", "Network API proxied");
      addPostFetchCallback(mainLogic);
    }

    const regex = toRegExp(search);

    const globalisedRegEx = new RegExp$3(regex, "g");
    fetchRules.set(globalisedRegEx,
                   {replacement, needle, formattedArgs: formattedArgsToLog});
  }

  const {Error: Error$8, Object: Object$5, atob, btoa, RegExp: RegExp$2} = $(window);

  function replaceOutboundValue(methodPath, textToReplace = "",
                                       replacement = "", decodeMethod = "",
                                       path = "", stack = "") {
    if (!methodPath)
      throw new Error$8("[replace-outbound-value snippet]: Missing method path.");

    let debugLog = getDebugger("replace-outbound-value");
    const {mark, end} = profile("replace-outbound-value");
    const formattedArgsToLog = formatArguments(arguments);
    let hitEventSent = false;
    function sendHitOnce() {
      if (!hitEventSent) {
        hitEventSent = true;
        sendSnippetHitEvent("replace-outbound-value " + formattedArgsToLog);
      }
    }

    function getPropertyInChain(base, propertyPath) {
      let object = base;
      let chain = $(propertyPath).split(".");

      for (let i = 0; i < chain.length - 1; i++) {
        let prop = chain[i];
        if (!object || (typeof object !== "object" &&
                        typeof object !== "function")) {
          return {
            base: object,
            prop,
            remainingPath: chain.slice(i).join("."),
            success: false
          };
        }
        object = object[prop];
      }

      let prop = chain[chain.length - 1];
      return {
        base: object,
        prop,
        success: true
      };
    }

    function isValidBase64(str) {
      try {
        if (str === "")
          return false;

        const decodedString = atob(str);
        const encodedString = btoa(decodedString);

        const stringWithoutPadding = $(str).replace(/=+$/, "").toString();
        const encodedStringWithoutPadding = $(encodedString).replace(/=+$/, "").toString();
        return encodedStringWithoutPadding === stringWithoutPadding;
      }
      catch (_e) {
        return false;
      }
    }

    function decodeAndReplaceContent(content, pattern, textReplacement, decode) {
      switch (decode) {
        case "base64":
          try {
            const isBase64Encoded = isValidBase64(content);

            if (isBase64Encoded) {

              const decodedContent = atob(content);
              debugLog("info", `Decoded base64 content: ${decodedContent}`);

              const modifiedContent = pattern ?
                $(decodedContent).replace(pattern, textReplacement).toString() :
                decodedContent;

              const message = modifiedContent !== decodedContent ?
                `Modified decoded content: ${modifiedContent}` :
                "Decoded content was not modified";

              debugLog("info", message);

              const encodedContent = btoa(modifiedContent);
              debugLog("info", `Re-encoded to base64: ${encodedContent}`);
              return encodedContent;
            }

            debugLog("info", `Content is plain text: ${content}`);

            const modifiedContent = pattern ?
              $(content).replace(pattern, textReplacement).toString() :
              content;

            const message = modifiedContent !== content ?
              `Modified plain text content: ${modifiedContent}` :
              "Plain text content was not modified";

            debugLog("info", message);

            const encodedContent = btoa(modifiedContent);
            debugLog("info", `Encoded to base64: ${encodedContent}`);
            return encodedContent;
          }
          catch (e) {
            debugLog("info", `Error processing base64 content: ${e.message}`);
            return content;
          }
        default:
          return pattern ?
            $(content).replace(pattern, textReplacement).toString() :
            content;
      }
    }

    function replaceValueAtPath(obj, pathSegments, pattern, textReplacement,
                                decode) {
      if (!pathSegments.length)
        return obj;

      let current = obj;

      for (let i = 0; i < pathSegments.length - 1; i++) {
        if (!current || typeof current !== "object") {
          debugLog("info", `Cannot navigate to path: property '${pathSegments[i]}' not found`);
          return obj;
        }
        current = current[pathSegments[i]];
      }

      const finalProp = pathSegments[pathSegments.length - 1];
      if (!current || typeof current !== "object" || !(finalProp in current)) {
        debugLog("info", `Target property '${finalProp}' not found at path`);
        return obj;
      }

      const originalValue = current[finalProp];
      if (typeof originalValue !== "string") {
        debugLog("info", `Property at path is not a string: ${typeof originalValue}`);
        return obj;
      }

      const modifiedValue = decodeAndReplaceContent(
        originalValue, pattern, textReplacement, decode);

      if (modifiedValue !== originalValue) {

        const result = JSON.parse(JSON.stringify(obj));
        let resultCurrent = result;

        for (let i = 0; i < pathSegments.length - 1; i++)
          resultCurrent = resultCurrent[pathSegments[i]];
        resultCurrent[finalProp] = modifiedValue;

        debugLog("info", `Replaced value at path '${pathSegments.join(".")}': '${originalValue}' -> '${modifiedValue}'`);
        return result;
      }

      return obj;
    }

    function processReturnValue(returnValue, pathParts, textPattern, replaceWith,
                                decode, formattedArgs) {

      const patternRegexp = textPattern ? new RegExp$2(toRegExp(textPattern), "g") :
       null;
      if (pathParts.length && typeof returnValue === "object" &&
          returnValue !== null) {
        const modifiedObject = textPattern ?
          replaceValueAtPath(
            returnValue,
            pathParts,
            patternRegexp,
            replaceWith,
            decode
          ) :
          returnValue;

        if (modifiedObject !== returnValue) {
          debugLog("success",
                 `Replaced outbound value\nFILTER: replace-outbound-value ${formattedArgs}`);
          sendHitOnce();
        }

        return modifiedObject;
      }

      else if (typeof returnValue === "string") {
        if (!textPattern)
          debugLog("info", `Original text content: ${returnValue}`);

        const modifiedContent = textPattern ?
          decodeAndReplaceContent(
            returnValue,
            patternRegexp,
            replaceWith,
            decode
          ) :
          returnValue;

        if (modifiedContent !== returnValue) {
          debugLog("success",
                 `Replaced outbound value: ${modifiedContent} \nFILTER: replace-outbound-value ${formattedArgs}`);
          sendHitOnce();
        }

        return modifiedContent;
      }

      return returnValue;
    }

    mark();

    const result = getPropertyInChain(window, methodPath);

    if (!result.success) {
      debugLog("error", `Could not reach the end of the prop chain: ${methodPath}. Remaining path: ${result.remainingPath}`);
      end();
      return;
    }

    const {base, prop} = result;
    const nativeMethod = base[prop];
    if (!nativeMethod || typeof nativeMethod !== "function") {
      debugLog("error", `Could not retrieve the method: ${methodPath}`);
      end();
      return;
    }

    let pathSegments = [];
    if (path)
      pathSegments = $(path).split(".");

    let stackNeedles = [];
    if (stack)
      stackNeedles = $(stack).split(",").map(s => s.trim());

    let isMatchingSuspended = false;

    let wrappedMethod = proxy(nativeMethod, function() {
      if (isMatchingSuspended)
        return apply$2(nativeMethod, this, arguments);

      isMatchingSuspended = true;
      const methodResult = apply$2(nativeMethod, this, arguments);

      if (stackNeedles.length && !matchesStackTrace(stackNeedles, debugLog)) {
        isMatchingSuspended = false;
        return methodResult;
      }

      if (methodResult && typeof methodResult.then === "function") {
        debugLog("info", "Method returned a Promise, modifying resolved value");

        isMatchingSuspended = false;
        return methodResult.then(resolvedValue => {
          const valueType = typeof resolvedValue === "object" ?
            JSON.stringify(resolvedValue) : resolvedValue;
          debugLog("info", `Promise resolved with value: ${valueType}`);

          return processReturnValue(
            resolvedValue,
            pathSegments,
            textToReplace,
            replacement,
            decodeMethod,
            path);
        }).catch(error => {
          debugLog("info", `Promise rejected: ${error.message}`);
          throw error;
        });
      }

      const processedResult = processReturnValue(
        methodResult,
        pathSegments,
        textToReplace,
        replacement,
        decodeMethod,
        path);
      isMatchingSuspended = false;
      return processedResult;
    });
    proxyToStringCalls(wrappedMethod, nativeMethod);
    Object$5.defineProperty(base, prop, {
      value: wrappedMethod
    });

    debugLog("info", `Wrapped ${methodPath}`);
    end();
  }

  let {XMLHttpRequest, WeakMap: WeakMap$1, Object: Object$4} = $(window);

  let hasXhrBeenProxied = false;

  const preSendCallbacks = [];

  const postResponseCallbacks = [];

  const xhrData = new WeakMap$1();

  const proxyXhr = () => {
    if (hasXhrBeenProxied)
      return;

    const XMLHttpRequestWrapper = class extends XMLHttpRequest {
      open(method, url, ...args) {
        xhrData.set(this, {method, url});
        return super.open(method, url, ...args);
      }
      send(body) {
        let modifiedBody = body;
        if (
          typeof body === "string" &&
          preSendCallbacks.length > 0
        ) {
          for (const fn of preSendCallbacks)
            modifiedBody = fn(modifiedBody);
        }
        return super.send(modifiedBody);
      }
      get response() {
        const innerResponse = super.response;
        if (postResponseCallbacks.length === 0)
          return innerResponse;

        const data = xhrData.get(this);
        if (typeof data === "undefined")
          return innerResponse;

        const responseLength =
          typeof innerResponse === "string" ?
            innerResponse.length : void 0;
        if (
          data.lastResponseLength !== responseLength
        ) {
          data.cachedResponse = void 0;
          data.lastResponseLength = responseLength;
        }

        if (typeof data.cachedResponse !== "undefined")
          return data.cachedResponse;

        if (typeof innerResponse !== "string")
          return (data.cachedResponse = innerResponse);

        let transformed = innerResponse;
        for (const fn of postResponseCallbacks)
          transformed = fn(transformed, {url: data.url});

        return (data.cachedResponse = transformed);
      }
      get responseText() {
        const response = this.response;
        if (typeof response !== "string")
          return super.responseText;

        return response;
      }
    };
    proxyToStringCalls(XMLHttpRequestWrapper, window.XMLHttpRequest);
    proxyToStringCalls(
      XMLHttpRequestWrapper.prototype.open,
      window.XMLHttpRequest.prototype.open
    );
    proxyToStringCalls(
      XMLHttpRequestWrapper.prototype.send,
      window.XMLHttpRequest.prototype.send
    );
    proxyToStringCalls(
      Object$4.getOwnPropertyDescriptor(
        XMLHttpRequestWrapper.prototype, "response"
      ).get,
      Object$4.getOwnPropertyDescriptor(
        window.XMLHttpRequest.prototype, "response"
      ).get
    );
    proxyToStringCalls(
      Object$4.getOwnPropertyDescriptor(
        XMLHttpRequestWrapper.prototype, "responseText"
      ).get,
      Object$4.getOwnPropertyDescriptor(
        window.XMLHttpRequest.prototype, "responseText"
      ).get
    );

    window.XMLHttpRequest = XMLHttpRequestWrapper;

    hasXhrBeenProxied = true;
  };

  const addPreSendCallback = callback => {
    preSendCallbacks.push(callback);
    proxyXhr();
  };

  const addPostResponseCallback = callback => {
    postResponseCallbacks.push(callback);
    proxyXhr();
  };

  let {Array: Array$1, Error: Error$7, JSON: JSON$2, Object: Object$3, RegExp: RegExp$1} = $(window);
  let xhrRequestRules;
  const hitFilters$c = new Set();

  function replaceXhrRequest(
    search, replacement = "", needle = null,
    mode = "replace"
  ) {
    const formattedArgsToLog = formatArguments(arguments);
    const debugLog = getDebugger("replace-xhr-request");
    const {mark, end} = profile("replace-xhr-request");

    if (!search)
      throw new Error$7("[replace-xhr-request]: Missing 'search' parameter");

    function parseJSON(str) {
      try {
        return JSON$2.parse(str);
      }
      catch (_e) {
        return str;
      }
    }

    function appendValue(parent, key, parsed) {
      let existing = parent[key];
      if (Array$1.isArray(existing)) {
        if (Array$1.isArray(parsed))
          parent[key] = $(existing).concat(parsed);
        else
          $(existing).push(parsed);
      }
      else if (
        typeof existing === "object" &&
        existing !== null &&
        typeof parsed === "object" &&
        parsed !== null &&
        !Array$1.isArray(parsed)
      ) {
        Object$3.assign(existing, parsed);
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
              let obj = JSON$2.parse(modifiedBody);
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
                if (!hitFilters$c.has(filter)) {
                  hitFilters$c.add(filter);
                  sendSnippetHitEvent(filter);
                }
              });
              modifiedBody = JSON$2.stringify(obj);
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
              if (!hitFilters$c.has(filter)) {
                hitFilters$c.add(filter);
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

      const globalisedRegEx = new RegExp$1(regex, "g");
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

  let {JSON: JSON$1, RegExp} = $(window);
  let xhrRules;
  const hitFilters$b = new Set();

  function replaceXhrResponse(search, replacement = "", needle = null) {
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
              let obj = JSON$1.parse(replacedText);
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
                if (!hitFilters$b.has(filter)) {
                  hitFilters$b.add(filter);
                  sendSnippetHitEvent(filter);
                }
              });
              replacedText = JSON$1.stringify(obj);
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
              if (!hitFilters$b.has(filter)) {
                hitFilters$b.add(filter);
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

  let {delete: deleteParam, has: hasParam} = caller(URLSearchParams.prototype);

  let parameters;
  const hitFilters$a = new Set();

  function stripFetchQueryParameter(name, urlPattern = null) {
    const formattedArgs = formatArguments(arguments);
    const debugLog = getDebugger("strip-fetch-query-parameter");
    const {mark, end} = profile("strip-fetch-query-parameter");

    const stripFunction = url => {
      mark();
      for (let [key, value] of parameters.entries()) {
        const {reg, args} = value;
        if (!reg || reg.test(url)) {
          if (hasParam(url.searchParams, key)) {
            debugLog("success", `${key} has been stripped from url ${url}`, `\nFILTER: strip-fetch-query-parameter ${args}`);
            const filter = "strip-fetch-query-parameter " + args;
            if (!hitFilters$a.has(filter)) {
              hitFilters$a.add(filter);
              sendSnippetHitEvent(filter);
            }
            deleteParam(url.searchParams, key);
          }
        }
      }
      end();
    };

    if (!parameters) {
      parameters = new Map();
      addPreFetchCallback(stripFunction);
    }

    parameters.set(name,
                   {reg: urlPattern && toRegExp(urlPattern),
                    args: formattedArgs});
  }

  const {Error: Error$6, Object: Object$2, parseInt: parseInt$5, isNaN: isNaN$2} = $(window);

  const {toString} = Function.prototype;

  const origSetTimeout = window.setTimeout;
  const origSetInterval = window.setInterval;

  const MODES = {
    TIMEOUT: "timeout",
    INTERVAL: "interval",
    BOTH: "both"
  };

  let timerOverrides = null;
  const hitFilters$9 = new Set();

  function timerOverride(timerValue,
                                needle = "",
                                callbackFunc = "",
                                mode = MODES.BOTH,
                                stackNeedle = "") {
    if (!timerValue) {
      throw new Error$6(
        "[timer-override snippet]: " +
        "Missing required parameter timerValue."
      );
    }

    if (!Object$2.values(MODES).includes(mode)) {
      throw new Error$6(
        "[timer-override snippet]: " +
        "Invalid mode. Acceptable values are: " +
        Object$2.values(MODES).join(", ")
      );
    }

    const newDelay = parseInt$5(timerValue, 10);
    if (isNaN$2(newDelay)) {
      throw new Error$6(
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
          if (!hitFilters$9.has(filter)) {
            hitFilters$9.add(filter);
            sendSnippetHitEvent(filter);
          }

          const newArgs = $([finalCb, finalDelay]);
          for (let i = 2; i < args.length; i++)
            newArgs.push(args[i]);
          return apply$2(origFn, ctx, newArgs);
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
        return apply$2(origSetTimeout, this, arguments);
      });
      proxyToStringCalls(wrappedSetTimeout, origSetTimeout);
      Object$2.defineProperty(window, "setTimeout", {
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
        return apply$2(origSetInterval, this, arguments);
      });
      proxyToStringCalls(wrappedSetInterval, origSetInterval);
      Object$2.defineProperty(window, "setInterval", {
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

  function trace(...args) {

    apply$2(log, null, args);
  }

  const {
    Array,
    Date: Date$1,
    Object: Object$1,
    Set: Set$5,
    WeakSet: WeakSet$a,
    document: document$2,
    parseInt: parseInt$4,
    window: w$1
  } = $(window);

  let installed$1 = false;

  const S_FIRST = "param_first";
  const S_SECOND = "param_second";
  const S_PYV = "pyv";
  const S_CLIENT_SCREEN = "client_screen";
  const S_AD_TYPE = "ad_type";
  const S_NONE = "none";

  const PARAMS_FIRST = "eAFgAQ";
  const PARAMS_SECOND = "8AUB";
  const CLIENT_SCREEN_CHANNEL = "CHANNEL";

  const ERROR_MARKERS = ["playerErrorMessageRenderer", "UNPLAYABLE"];

  function installHook(installFn, name, debugLog) {
    try {
      installFn();
    }
    catch (e) {
      debugLog("error", `Failed to install ${name}: ${e}`);
    }
  }

  function tmpYtBufferingSpoof(
    disabledHooks, lateMuteWindowMs, userGestureWindowMs, paths
  ) {
    if (installed$1)
      return;
    installed$1 = true;

    const {
      Document,
      HTMLIFrameElement,
      Response
    } = $(window);

    const debugLog = getDebugger("tmp-yt-buffering-spoof");
    const {mark, end} = profile("tmp-yt-buffering-spoof");
    mark();

    let currentState = S_FIRST;
    let lastVideoId = null;
    let mutationCount = 0;
    let responseCount = 0;

    const nativeStringify = window.JSON.stringify;
    const nativeParse = window.JSON.parse;

    const origVisibilityDescriptor = Object$1.getOwnPropertyDescriptor(
      Document.prototype, "visibilityState"
    );

    const forceVisible = () => {
      try {
        Object$1.defineProperty(document$2, "visibilityState", {
          get() {
            return "visible";
          },
          configurable: true
        });
      }
      catch (e) {

      }
    };

    const restoreVisibility = () => {
      try {
        if (origVisibilityDescriptor) {
          Object$1.defineProperty(
            document$2, "visibilityState", origVisibilityDescriptor
          );
        }
      }
      catch (e) {

      }
    };

    const dig = function(obj) {
      for (let i = 1; i < arguments.length; i++) {
        if (obj === null || typeof obj === "undefined")
          return void 0;
        obj = obj[arguments[i]];
      }
      return obj;
    };

    const DEFAULT_PATHS = "!homepage !shorts watch";
    const effectivePaths =
      typeof paths === "string" && paths.replace(/\s+/g, "").length > 0 ?
        paths : DEFAULT_PATHS;
    const pathRules = parsePathRules$1(effectivePaths);

    const disabledHookSet = new Set$5();
    if (typeof disabledHooks === "string") {
      const tokens = disabledHooks.split(/\s+/);
      for (let i = 0; i < tokens.length; i++) {
        const n = parseInt$4(tokens[i], 10);
        if (n >= 1)
          disabledHookSet.add(n);
      }
    }
    const hookEnabled = n => !disabledHookSet.has(n);

    const installHookIf = (n, installFn, name) => {
      if (hookEnabled(n))
        installHook(installFn, name, debugLog);
    };

    const hardExcluded = () => {
      const href = w$1.location.href;
      return href.indexOf("/shorts/") !== -1 ||
        href.indexOf("youtube.com/tv") !== -1 ||
        href.indexOf("youtube.com/embed/") !== -1;
    };

    const isExcluded = () =>
      hardExcluded() || !pathAllowed$1(w$1.location.href, pathRules);

    const getPlayabilityStatus = () => {
      try {
        const player = document$2.getElementById("movie_player");
        if (!player || typeof player.getPlayerResponse !== "function")
          return null;
        const pr = player.getPlayerResponse();
        return dig(pr, "playabilityStatus", "status");
      }
      catch (e) {
        return null;
      }
    };

    const deleteFingerprint = body => {
      if (!body.playbackContext && !body.playerRequest)
        return;
      const configInfo = dig(body, "context", "client", "configInfo");
      if (configInfo && configInfo.appInstallData)
        delete configInfo.appInstallData;
    };

    const trackVideoId = body => {
      const vid = body.videoId;
      if (typeof vid !== "string" || vid.length === 0)
        return;
      if (lastVideoId !== null && lastVideoId !== vid) {
        debugLog("info",
                 `New video ${vid} (was ${lastVideoId}) — ` +
                 `reset to ${S_FIRST}`);
        currentState = S_FIRST;
      }
      lastVideoId = vid;
    };

    const advanceState = reason => {
      let next;
      if (currentState === S_FIRST)
        next = S_SECOND;
      else if (currentState === S_SECOND)
        next = S_PYV;
      else if (currentState === S_PYV)
        next = S_CLIENT_SCREEN;
      else if (currentState === S_CLIENT_SCREEN)
        next = S_AD_TYPE;
      else
        next = S_NONE;
      debugLog("info", `State: ${currentState} → ${next} (${reason})`);
      currentState = next;
    };

    const mutateBody = (body, pbCtx) => {
      try {
        if (!body || !pbCtx)
          return;
        trackVideoId(body);

        let effective = currentState;
        const status = getPlayabilityStatus();
        if (status === "LOGIN_REQUIRED" ||
            status === "CONTENT_CHECK_REQUIRED")
          effective = S_NONE;

        const csCurrent =
          dig(body, "context", "client", "clientScreen");

        const refreshLact = () => {
          if (pbCtx.contentPlaybackContext) {

            pbCtx.contentPlaybackContext.lactMilliseconds =
              `${Date$1.now()}`;
          }
        };

        if (effective === S_FIRST &&
            csCurrent !== CLIENT_SCREEN_CHANNEL) {
          body.params = PARAMS_FIRST;
          if (body.playerRequest &&
              body.playerRequest.params !== PARAMS_FIRST)
            body.playerRequest.params = PARAMS_FIRST;
          if (body.playbackContext &&
              body.playbackContext.params !== PARAMS_FIRST)
            body.playbackContext.params = PARAMS_FIRST;
          refreshLact();
          forceVisible();
          deleteFingerprint(body);
          mutationCount++;
          return;
        }

        if (effective === S_SECOND &&
            csCurrent !== CLIENT_SCREEN_CHANNEL) {
          if (body.params !== PARAMS_SECOND)
            body.params = PARAMS_SECOND;
          if (body.playerRequest &&
              body.playerRequest.params !== PARAMS_SECOND)
            body.playerRequest.params = PARAMS_SECOND;
          if (body.playbackContext &&
              body.playbackContext.params !== PARAMS_SECOND)
            body.playbackContext.params = PARAMS_SECOND;
          if (!body.playlistId && body.context && body.context.client)
            body.context.client.clientScreen = CLIENT_SCREEN_CHANNEL;
          refreshLact();
          forceVisible();
          deleteFingerprint(body);
          mutationCount++;
          return;
        }

        if (effective === S_PYV &&
            csCurrent !== CLIENT_SCREEN_CHANNEL) {
          const pcParams = pbCtx.params;
          const startsWithSpoofedParams =
            typeof pcParams === "string" &&
            (pcParams.indexOf(PARAMS_FIRST) === 0 ||
             pcParams.indexOf(PARAMS_SECOND) === 0);
          if (startsWithSpoofedParams)
            return;
          pbCtx.adPlaybackContext = {pyv: true};
          refreshLact();
          deleteFingerprint(body);
          mutationCount++;
          return;
        }

        if (effective === S_CLIENT_SCREEN) {
          const clientName =
            dig(body, "context", "client", "clientName");
          if (clientName !== "WEB")
            return;
          body.context.client.clientScreen = CLIENT_SCREEN_CHANNEL;
          refreshLact();
          forceVisible();
          deleteFingerprint(body);
          mutationCount++;
          return;
        }

        if (effective === S_AD_TYPE) {
          pbCtx.adPlaybackContext = {adType: "AD_TYPE_INSTREAM"};
          refreshLact();
          forceVisible();
          deleteFingerprint(body);
          mutationCount++;
          return;
        }

        if (effective === S_NONE) {
          if (pbCtx.adPlaybackContext)
            delete pbCtx.adPlaybackContext;
          restoreVisibility();
        }
      }
      catch (e) {

      }
    };

    const applyToBody = body => {
      if (!body || !body.context || !body.context.client)
        return;
      if (body.playbackContext &&
          typeof body.playbackContext.adPlaybackContext === "undefined")
        mutateBody(body, body.playbackContext);
      if (body.playerRequest &&
          body.playerRequest.playbackContext &&
          typeof body.playerRequest.playbackContext.adPlaybackContext ===
            "undefined")
        mutateBody(body, body.playerRequest.playbackContext);
    };

    const wrappedStringify = proxy(nativeStringify, function() {
      if (hardExcluded())
        return apply$2(nativeStringify, this, arguments);
      try {
        const arg = arguments[0];
        if (arg && typeof arg === "object")
          applyToBody(arg);
      }
      catch (e) {

      }
      return apply$2(nativeStringify, this, arguments);
    });
    proxyToStringCalls(wrappedStringify, nativeStringify);

    installHookIf(1, () => {
      Object$1.defineProperty(window.JSON, "stringify", {
        value: wrappedStringify,
        writable: true,
        configurable: true
      });
    }, "JSON.stringify");

    const wrappedParse = proxy(nativeParse, function() {
      if (isExcluded() || currentState === S_NONE)
        return apply$2(nativeParse, this, arguments);
      let result;
      try {
        result = apply$2(nativeParse, this, arguments);
      }
      catch (e) {

        return apply$2(nativeParse, this, arguments);
      }
      try {
        if (!result || typeof result !== "object")
          return result;
        if (!result.responseContext && !result.playabilityStatus)
          return result;
        responseCount++;
        const stringified = nativeStringify(result);
        let hasError = false;
        for (const m of ERROR_MARKERS) {
          if (stringified.indexOf(m) !== -1) {
            hasError = true;
            break;
          }
        }
        const hasContentCheck =
          stringified.indexOf("CONTENT_CHECK_REQUIRED") !== -1;
        if (hasError && !hasContentCheck) {
          advanceState("response had error marker");
          return result;
        }

        if (currentState === S_FIRST) {
          const audioConfig =
            dig(result, "playerConfig", "audioConfig");
          if (audioConfig && audioConfig.muteOnStart) {
            const onWatch = w$1.location.href.indexOf("/watch") !== -1;
            const isMiniplayer =
              dig(result, "playabilityStatus", "miniplayer");
            if (onWatch || (result.cards && !isMiniplayer)) {
              delete audioConfig.muteOnStart;
              const messages = result.messages;
              if (messages && messages[0] && messages[0].youThereRenderer)
                delete messages[0].youThereRenderer;
            }
          }
        }
        if (currentState === S_AD_TYPE) {

          const gvsc =
            dig(result, "playerConfig", "granularVariableSpeedConfig");
          if (gvsc) {
            gvsc.maximumPlaybackRate = 200;
            gvsc.minimumPlaybackRate = 25;
          }
        }
      }
      catch (e) {

      }
      return result;
    });
    proxyToStringCalls(wrappedParse, nativeParse);
    installHookIf(2, () => {
      Object$1.defineProperty(window.JSON, "parse", {
        value: wrappedParse,
        writable: true,
        configurable: true
      });
    }, "JSON.parse");

    const nativeEncode = window.TextEncoder.prototype.encode;
    const wrappedEncode = proxy(nativeEncode, function() {
      if (hardExcluded())
        return apply$2(nativeEncode, this, arguments);
      try {
        const text = arguments[0];
        if (typeof text === "string" &&
            (text.indexOf("\"contentPlaybackContext\"") !== -1 ||
             text.indexOf("\"adSignalsInfo\"") !== -1)) {
          const parsed = nativeParse(text);
          if (parsed && parsed.context && parsed.context.client) {
            applyToBody(parsed);
            arguments[0] = nativeStringify(parsed);
          }
        }
      }
      catch (e) {

      }
      return apply$2(nativeEncode, this, arguments);
    });
    proxyToStringCalls(wrappedEncode, nativeEncode);
    installHookIf(3, () => {
      Object$1.defineProperty(window.TextEncoder.prototype, "encode", {
        value: wrappedEncode,
        writable: true,
        configurable: true
      });
    }, "TextEncoder.prototype.encode");

    const wrappedRequest = new Proxy(window.Request, {
      construct(target, args, newTarget) {
        try {
          if (hardExcluded())
            return Reflect.construct(target, args, newTarget);
          const url = args[0];
          const init = args[1];
          const urlStr =
            typeof url === "string" ? url :
              (url && typeof url.url === "string" ? url.url : "");
          const body = init && init.body;
          if (urlStr.indexOf("youtubei") !== -1 &&
              typeof body === "string" &&
              (body.indexOf("\"contentPlaybackContext\"") !== -1 ||
               body.indexOf("\"adSignalsInfo\"") !== -1)) {
            const parsed = nativeParse(body);
            if (parsed && parsed.context && parsed.context.client) {
              applyToBody(parsed);
              init.body = nativeStringify(parsed);
            }
          }
        }
        catch (e) {

        }
        return Reflect.construct(target, args, newTarget);
      }
    });
    installHookIf(4, () => {
      Object$1.defineProperty(window, "Request", {
        value: wrappedRequest,
        writable: true,
        configurable: true
      });
    }, "Request");

    const nativeSend = window.XMLHttpRequest.prototype.send;
    const wrappedSend = proxy(nativeSend, function() {
      if (hardExcluded())
        return apply$2(nativeSend, this, arguments);
      try {
        const first = arguments[0];
        const wasArray = Array.isArray(first);
        const text = wasArray ? first[0] : first;
        if (typeof text === "string" &&
            (text.indexOf("\"contentPlaybackContext\"") !== -1 ||
             text.indexOf("\"adSignalsInfo\"") !== -1)) {
          const parsed = nativeParse(text);
          if (parsed && parsed.context && parsed.context.client) {
            applyToBody(parsed);
            const newText = nativeStringify(parsed);
            if (wasArray)
              arguments[0][0] = newText;
            else
              arguments[0] = newText;
          }
        }
      }
      catch (e) {

      }
      return apply$2(nativeSend, this, arguments);
    });
    proxyToStringCalls(wrappedSend, nativeSend);
    installHookIf(5, () => {
      Object$1.defineProperty(window.XMLHttpRequest.prototype, "send", {
        value: wrappedSend,
        writable: true,
        configurable: true
      });
    }, "XMLHttpRequest.prototype.send");

    const jspbResponseHandler = {
      apply(target, thisArg, args) {
        const result = Reflect.apply(target, thisArg, args);
        try {
          if (result && result.responseContext) {
            delete result.adSlots;
            delete result.playerAds;
            const audioConfig = dig(result, "playerConfig", "audioConfig");
            if (audioConfig && audioConfig.muteOnStart) {
              const onWatch = w$1.location.href.indexOf("/watch") !== -1;
              const isMiniplayer =
                dig(result, "playabilityStatus", "miniplayer");
              if (onWatch || (result.cards && !isMiniplayer)) {
                delete audioConfig.muteOnStart;
                const messages = result.messages;
                if (messages && messages[0] &&
                    messages[0].youThereRenderer)
                  delete messages[0].youThereRenderer;
              }
            }
          }
        }
        catch (e) {

        }
        return result;
      }
    };
    const stringValueHandler = {
      apply(target, thisArg, args) {
        try {
          const arg = args[0];
          if (arg && typeof arg.value === "string" &&
              arg.value.indexOf("playerResponse") !== -1) {
            let s = arg.value;
            const onWatch = w$1.location.href.indexOf("/watch") !== -1;
            const cardsButNotMiniplayer =
              s.indexOf("cards") !== -1 &&
              s.indexOf("\"miniplayer\"") === -1;
            if ((onWatch || cardsButNotMiniplayer) &&
                s.indexOf("\"muteOnStart\":true") !== -1) {
              s = s.replace(
                "\"muteOnStart\":true", "\"muteOnStart\":false"
              );
              if (s.indexOf("\"youThereRenderer\":") !== -1) {
                s = s.replace(
                  "\"youThereRenderer\":", "\"no_youThereRenderer\":"
                );
              }
            }
            s = s.replace(
              /"(adSlots|playerAds)":/g, "\"no_ads\":"
            );
            arg.value = s;
            args[0] = arg;
          }
        }
        catch (e) {

        }
        return Reflect.apply(target, thisArg, args);
      }
    };

    const nativeThen = window.Promise.prototype.then;
    const wrappedThen = proxy(nativeThen, function() {

      if (isExcluded())
        return apply$2(nativeThen, this, arguments);
      try {
        const cb = arguments[0];
        if (typeof cb === "function") {
          const src = cb.toString();
          if (src.indexOf("jspbResponseCtor") !== -1)
            arguments[0] = new Proxy(cb, jspbResponseHandler);
          else if (src.indexOf(".next(") !== -1)
            arguments[0] = new Proxy(cb, stringValueHandler);
        }
      }
      catch (e) {

      }
      return apply$2(nativeThen, this, arguments);
    });
    proxyToStringCalls(wrappedThen, nativeThen);
    installHookIf(6, () => {
      Object$1.defineProperty(window.Promise.prototype, "then", {
        value: wrappedThen,
        writable: true,
        configurable: true
      });
    }, "Promise.prototype.then");

    const nativeAppend = window.Node.prototype.appendChild;
    const wrappedAppend = proxy(nativeAppend, function() {
      const result = apply$2(nativeAppend, this, arguments);

      if (hardExcluded())
        return result;
      try {
        if (result instanceof HTMLIFrameElement &&
            result.src === "about:blank" &&
            result.contentWindow) {
          result.contentWindow.fetch = w$1.fetch;
          result.contentWindow.Request = w$1.Request;
        }
      }
      catch (e) {

      }
      return result;
    });
    proxyToStringCalls(wrappedAppend, nativeAppend);
    installHookIf(7, () => {
      Object$1.defineProperty(window.Node.prototype, "appendChild", {
        value: wrappedAppend,
        writable: true,
        configurable: true
      });
    }, "Node.prototype.appendChild");

    const PLAYER_ENDPOINTS = [
      "/youtubei/v1/player",
      "/get_watch",
      "/get_video_info"
    ];
    let muteCleanupCount = 0;
    let startSecondsInjectCount = 0;
    let honeypotBypassCount = 0;
    addPostFetchCallback((response, reqInfo) => {

      if (!hookEnabled(8) || !reqInfo ||
          typeof reqInfo.url !== "string" || isExcluded())
        return response;
      let matched = false;
      for (const ep of PLAYER_ENDPOINTS) {
        if (reqInfo.url.indexOf(ep) !== -1) {
          matched = true;
          break;
        }
      }
      if (!matched)
        return response;
      if (typeof response.url === "string" &&
          response.url.indexOf("data:") === 0) {
        honeypotBypassCount++;
        return response;
      }
      const ctype =
        (response.headers.get("content-type") || "").toLowerCase();
      if (ctype.indexOf("json") === -1)
        return response;

      const desiredStart = parseStartSecondsFromHref(w$1.location.href);
      return response.clone().json().then(obj => {
        let touched = false;

        const targets = [];
        if (obj && obj.playabilityStatus)
          targets.push(obj);
        if (Array.isArray(obj)) {
          for (const entry of obj) {
            if (entry && entry.playerResponse &&
                entry.playerResponse.playabilityStatus)
              targets.push(entry.playerResponse);
          }
        }
        for (const t of targets) {
          const cleaned = cleanPlayerResponse(t);
          const seekInjected = injectStartSeconds(t, desiredStart);
          if (cleaned)
            touched = true;
          if (seekInjected) {
            touched = true;
            startSecondsInjectCount++;
          }
        }
        if (!touched)
          return response;
        muteCleanupCount++;
        const reconstructed = new Response(nativeStringify(obj), {
          status: response.status,
          statusText: response.statusText,
          headers: response.headers
        });

        Object$1.defineProperties(reconstructed, {
          ok: {value: response.ok},
          redirected: {value: response.redirected},
          type: {value: response.type},
          url: {value: response.url}
        });
        return reconstructed;
      }).catch(() => response);
    });

    addPostResponseCallback((responseText, reqInfo) => {

      if (!hookEnabled(9) || !reqInfo ||
          typeof reqInfo.url !== "string" || isExcluded())
        return responseText;
      let matched = false;
      for (const ep of PLAYER_ENDPOINTS) {
        if (reqInfo.url.indexOf(ep) !== -1) {
          matched = true;
          break;
        }
      }
      if (!matched)
        return responseText;
      if (reqInfo.url.indexOf("data:") === 0) {
        honeypotBypassCount++;
        return responseText;
      }
      if (typeof responseText !== "string" ||
          responseText.length === 0)
        return responseText;

      if (responseText.indexOf("playerResponse") === -1 &&
          responseText.indexOf("playabilityStatus") === -1)
        return responseText;
      const desiredStart = parseStartSecondsFromHref(w$1.location.href);
      try {
        const obj = nativeParse(responseText);
        let touched = false;
        const targets = [];
        if (obj && obj.playabilityStatus)
          targets.push(obj);
        if (Array.isArray(obj)) {
          for (const entry of obj) {
            if (entry && entry.playerResponse &&
                entry.playerResponse.playabilityStatus)
              targets.push(entry.playerResponse);
          }
        }
        for (const t of targets) {
          const cleaned = cleanPlayerResponse(t);
          const seekInjected = injectStartSeconds(t, desiredStart);
          if (cleaned)
            touched = true;
          if (seekInjected) {
            touched = true;
            startSecondsInjectCount++;
          }
        }
        if (!touched)
          return responseText;
        muteCleanupCount++;
        return nativeStringify(obj);
      }
      catch (e) {
        return responseText;
      }
    });

    const parseWindowMs = (raw, fallback) => {
      if (typeof raw === "undefined" || raw === null)
        return fallback;

      const n = parseInt$4(`${raw}`, 10);
      return n >= 0 ? n : fallback;
    };
    const LATE_MUTE_WINDOW_MS = parseWindowMs(lateMuteWindowMs, 5000);
    const USER_GESTURE_WINDOW_MS = parseWindowMs(userGestureWindowMs, 600);
    const unmutedVideoIds = new Set$5();

    const userMutedVideoIds = new Set$5();
    const watchedVideos = new WeakSet$a();
    let unmuteCount = 0;

    let lastUserGestureAt = 0;

    const MAX_LATE_REVERTS = 5;

    const currentVideoId = () => {
      try {
        const player = document$2.getElementById("movie_player");
        const pr =
          player && typeof player.getPlayerResponse === "function" ?
            player.getPlayerResponse() : null;
        return (pr && pr.videoDetails && pr.videoDetails.videoId) || "";
      }
      catch (e) {
        return "";
      }
    };

    const unmuteInScope = () => !isExcluded();

    const unmuteNow = video => {
      let usedApi = false;
      try {
        const player = document$2.getElementById("movie_player");
        if (player && typeof player.unMute === "function") {
          player.unMute();
          usedApi = true;

          if (typeof player.getVolume === "function" &&
              typeof player.setVolume === "function" &&
              player.getVolume() === 0)
            player.setVolume(100);
        }
      }
      catch (e) {

      }
      try {
        if (video && video.muted)
          video.muted = false;
      }
      catch (e) {

      }
      return usedApi;
    };

    const armVideoUnmuteWatcher = () => {
      if (isExcluded())
        return;
      const video =
        document$2.querySelector("video.html5-main-video") ||
        document$2.querySelector("video.video-stream");
      if (!video || watchedVideos.has(video))
        return;
      watchedVideos.add(video);

      let playingAt = 0;
      let lateReverts = 0;
      video.addEventListener("playing", () => {
        try {
          playingAt = Date$1.now();
          lateReverts = 0;
          if (!unmuteInScope())
            return;
          if (!video.muted)
            return;
          const vid = currentVideoId();

          if (vid && userMutedVideoIds.has(vid))
            return;
          if (vid && unmutedVideoIds.has(vid))
            return;
          if (vid)
            unmutedVideoIds.add(vid);
          unmuteCount++;
          const usedApi = unmuteNow(video);
          debugLog("info",
                   "[video.playing] muted at first playing for " +
                   `videoId=${vid || "?"} — unmuted (via ` +
                   `${usedApi ? "player.unMute()" : "element"}).`);
        }
        catch (e) {

        }
      });
      video.addEventListener("volumechange", () => {
        try {

          if (!unmuteInScope())
            return;
          const vid = currentVideoId();
          const recentGesture =
            lastUserGestureAt !== 0 &&
            (Date$1.now() - lastUserGestureAt) < USER_GESTURE_WINDOW_MS;

          if (!video.muted) {

            if (recentGesture && vid)
              userMutedVideoIds.delete(vid);
            return;
          }

          if (recentGesture) {
            if (vid)
              userMutedVideoIds.add(vid);
            debugLog("info",
                     "[video.volumechange] mute within user-gesture " +
                     "window — remembering + respecting user mute " +
                     `(videoId=${vid || "?"}).`);
            return;
          }

          if (vid && userMutedVideoIds.has(vid)) {
            debugLog("info",
                     "[video.volumechange] mute on user-muted video " +
                     `— respecting (videoId=${vid}).`);
            return;
          }

          if (playingAt === 0)
            return;
          const sincePlaying = Date$1.now() - playingAt;
          if (sincePlaying >= LATE_MUTE_WINDOW_MS)
            return;
          if (lateReverts >= MAX_LATE_REVERTS)
            return;
          lateReverts++;
          unmuteCount++;
          const usedApi = unmuteNow(video);
          debugLog("info",
                   "[video.volumechange] late mute at " +
                   `+${sincePlaying}ms after playing for videoId=` +
                   `${vid || "?"} — unmuted (via ` +
                   `${usedApi ? "player.unMute()" : "element"}).`);
        }
        catch (e) {

        }
      });
      debugLog("info",
               "[video-watcher] attached to <video> element " +
               `(late-mute window=${LATE_MUTE_WINDOW_MS}ms).`);
    };

    if (hookEnabled(10)) {

      armVideoUnmuteWatcher();
      const videoWaitObs = new MutationObserver(() => {
        armVideoUnmuteWatcher();
      });
      videoWaitObs.observe(document$2, {childList: true, subtree: true});

      document$2.addEventListener("yt-navigate-finish", () => {
        armVideoUnmuteWatcher();
      });

      const markUserGesture = () => {
        lastUserGestureAt = Date$1.now();
      };
      document$2.addEventListener("click", evt => {
        try {
          const target = evt.target;
          if (target && typeof target.closest === "function" &&
              target.closest(".ytp-mute-button"))
            markUserGesture();
        }
        catch (e) {

        }
      }, true);
      document$2.addEventListener("keydown", evt => {
        try {
          const key = evt.key;
          if (key !== "m" && key !== "M")
            return;

          const active = document$2.activeElement;
          const tag = active && active.tagName ? active.tagName : "";
          if (tag === "INPUT" || tag === "TEXTAREA" ||
              (active && active.isContentEditable))
            return;
          markUserGesture();
        }
        catch (e) {

        }
      }, true);
    }

    debugLog("info",
             `Installed. Starting state: ${currentState}. Hooks: ` +
             "JSON.{stringify,parse}, TextEncoder.encode, Request, " +
             "XMLHttpRequest.send, Promise.then, Node.appendChild, " +
             "fetch-postFetch, xhr-postResponse, video-unmute. " +
             `Counters: ${mutationCount} mutations, ` +
             `${responseCount} responses inspected, ` +
             `${muteCleanupCount} response-rewrites, ` +
             `${startSecondsInjectCount} startSeconds-injects, ` +
             `${honeypotBypassCount} honeypot bypasses, ` +
             `${unmuteCount} video-element unmutes. ` +
             `Windows: late-mute=${LATE_MUTE_WINDOW_MS}ms, ` +
             `user-gesture=${USER_GESTURE_WINDOW_MS}ms.` +
             (disabledHookSet.size > 0 ?
               ` Disabled hooks: ${[...disabledHookSet].join(",")}.` :
               "") +
             describePathRules$1(pathRules));
    end();
  }

  function parsePathRules$1(paths) {
    const allow = [];
    const deny = [];
    if (typeof paths !== "string" || paths.length === 0)
      return {allow, deny};
    const tokens = paths.split(/\s+/);
    for (let i = 0; i < tokens.length; i++) {
      const t = tokens[i];
      if (!t)
        continue;
      if (t.charAt(0) === "!" && t.length > 1)
        deny.push(t.slice(1).toLowerCase());
      else
        allow.push(t.toLowerCase());
    }
    return {allow, deny};
  }

  function firstPathSegment$1(href) {
    if (typeof href !== "string" || href.length === 0)
      return "homepage";
    let p = href;
    const q = p.indexOf("?");
    if (q !== -1)
      p = p.slice(0, q);
    const h = p.indexOf("#");
    if (h !== -1)
      p = p.slice(0, h);
    const ss = p.indexOf("://");
    if (ss !== -1)
      p = p.slice(ss + 3);
    const slash = p.indexOf("/");
    if (slash === -1)
      return "homepage";
    const path = p.slice(slash);
    const m = /^\/([^/]+)/.exec(path);
    return m ? m[1].toLowerCase() : "homepage";
  }

  function pathAllowed$1(href, rules) {
    const segment = firstPathSegment$1(href);
    for (let i = 0; i < rules.deny.length; i++) {
      if (rules.deny[i] === segment)
        return false;
    }
    if (rules.allow.length === 0)
      return true;
    for (let i = 0; i < rules.allow.length; i++) {
      if (rules.allow[i] === segment)
        return true;
    }
    return false;
  }

  function describePathRules$1(rules) {
    if (rules.allow.length === 0 && rules.deny.length === 0)
      return "";
    const parts = [];
    if (rules.allow.length > 0)
      parts.push("allow=[" + rules.allow.join(",") + "]");
    if (rules.deny.length > 0)
      parts.push("deny=[" + rules.deny.join(",") + "]");
    return " Path filter: " + parts.join(" ") + ".";
  }

  function cleanPlayerResponse(pr) {
    if (!pr || typeof pr !== "object")
      return false;
    let touched = false;
    if (pr.adSlots) {
      delete pr.adSlots;
      touched = true;
    }
    if (pr.playerAds) {
      delete pr.playerAds;
      touched = true;
    }
    const audioConfig =
      pr.playerConfig && pr.playerConfig.audioConfig;
    if (audioConfig && audioConfig.muteOnStart) {
      delete audioConfig.muteOnStart;
      touched = true;
    }
    const messages = pr.messages;
    if (messages && messages[0] && messages[0].youThereRenderer) {
      delete messages[0].youThereRenderer;
      touched = true;
    }
    return touched;
  }

  function injectStartSeconds(pr, startSeconds) {
    if (!pr || typeof pr !== "object")
      return false;
    if (startSeconds === null || !(startSeconds > 0))
      return false;
    if (!pr.playerConfig)
      pr.playerConfig = {};
    if (!pr.playerConfig.playbackStartConfig)
      pr.playerConfig.playbackStartConfig = {};
    const cfg = pr.playerConfig.playbackStartConfig;
    if (cfg.startSeconds === startSeconds)
      return false;
    cfg.startSeconds = startSeconds;
    return true;
  }

  function parseStartSecondsFromHref(href) {
    if (typeof href !== "string" || href.length === 0)
      return null;
    const m = /[?&]t=([^&#]+)/.exec(href);
    if (!m)
      return null;
    let raw = m[1];
    try {
      raw = decodeURIComponent(raw);
    }
    catch (e) {

    }

    if (/^\d+$/.test(raw))
      return parseInt$4(raw, 10);

    const hms = /^(?:(\d+)h)?(?:(\d+)m)?(?:(\d+)s)?$/i.exec(raw);
    if (!hms || (!hms[1] && !hms[2] && !hms[3]))
      return null;
    const h = parseInt$4(hms[1] || "0", 10);
    const min = parseInt$4(hms[2] || "0", 10);
    const s = parseInt$4(hms[3] || "0", 10);
    return h * 3600 + min * 60 + s;
  }

  const {
    Date,
    MutationObserver: MutationObserver$f,
    Set: Set$4,
    document: document$1,
    parseInt: parseInt$3,
    setTimeout: setTimeout$2,
    window: w
  } = $(window);

  let installed = false;

  const COLD_GIVE_UP_MS = 10000;

  const NAV_RETRY_MAX = 30;
  const NAV_RETRY_INTERVAL_MS = 100;

  function tmpYtForceReload(mode, errorMode, delayMs, paths) {
    if (installed)
      return;
    installed = true;

    const debugLog = getDebugger("tmp-yt-force-reload");
    const {mark, end} = profile("tmp-yt-force-reload");
    mark();

    const extraDelay = (() => {
      const raw = typeof delayMs === "string" ? delayMs.toString() : "0";
      const n = parseInt$3(raw, 10);
      return isNaN(n) || n < 0 ? 0 : n;
    })();

    const normalizedMode = (() => {
      const raw = typeof mode === "string" ? mode.toString() : "";
      const lower = raw.toLowerCase();
      if (lower === "every")
        return "every";
      return "first";
    })();

    const normalizedErrorMode = (() => {
      const raw =
        typeof errorMode === "string" ? errorMode.toString() : "";
      const lower = raw.toLowerCase();
      if (lower === "dom" || lower === "player" || lower === "both")
        return lower;
      return "none";
    })();

    const pathRules = parsePathRules(paths);

    const installedAt = Date.now();
    let lastFiredVideoId = "";
    let fireCount = 0;

    let disabled = false;

    const errorRetried = new Set$4();
    let errorObserverInstalled = false;
    let errorFireCount = 0;

    const checkErrorState = player => {
      if (normalizedErrorMode === "none" || !player)
        return false;
      let domErr = false;
      let playerErr = false;
      if (normalizedErrorMode === "dom" ||
          normalizedErrorMode === "both") {
        try {
          domErr =
            player.classList.contains("ytp-error") ||
            player.querySelector(".ytp-error") !== null;
        }
        catch (e) {

        }
      }
      if (normalizedErrorMode === "player" ||
          normalizedErrorMode === "both") {
        try {
          const pr = typeof player.getPlayerResponse === "function" ?
            player.getPlayerResponse() : null;
          const status =
            pr && pr.playabilityStatus && pr.playabilityStatus.status;
          playerErr =
            typeof status === "string" &&
            status !== "OK" && status !== "OK_LIMITED";
        }
        catch (e) {

        }
      }
      if (normalizedErrorMode === "both")
        return domErr && playerErr;
      if (normalizedErrorMode === "player")
        return playerErr;
      return domErr;
    };

    const tryErrorFire = () => {
      if (normalizedErrorMode === "none")
        return;
      if (w.location.href.indexOf("/watch?") === -1)
        return;
      if (!pathAllowed(w.location.href, pathRules))
        return;
      const player = document$1.getElementById("movie_player");
      if (!player || typeof player.loadVideoById !== "function")
        return;
      if (!checkErrorState(player))
        return;
      let pr;
      try {
        pr = typeof player.getPlayerResponse === "function" ?
          player.getPlayerResponse() : null;
      }
      catch (e) {
        pr = null;
      }
      const videoId = pr && pr.videoDetails && pr.videoDetails.videoId;
      if (typeof videoId !== "string" || videoId === "")
        return;
      if (errorRetried.has(videoId)) {

        return;
      }
      errorRetried.add(videoId);
      const startSeconds =
        (pr.playerConfig && pr.playerConfig.playbackStartConfig &&
         pr.playerConfig.playbackStartConfig.startSeconds) || 0;
      errorFireCount++;
      const seq = errorFireCount;
      const elapsed = Date.now() - installedAt;
      const playabilityStatus =
        pr && pr.playabilityStatus && pr.playabilityStatus.status;
      debugLog("info",
               `error#${seq} [+${elapsed}ms] Error detected for ` +
               `"${videoId}" (signal=${normalizedErrorMode}, ` +
               `playabilityStatus=${playabilityStatus}). Firing ` +
               `loadVideoById("${videoId}", ${startSeconds}).`);
      try {
        player.loadVideoById(videoId, startSeconds);
      }
      catch (e) {
        debugLog("error", `error#${seq} loadVideoById threw: ${e}`);
      }
    };

    const ensureErrorObserver = player => {
      if (normalizedErrorMode === "none")
        return;
      if (errorObserverInstalled || !player)
        return;
      errorObserverInstalled = true;
      const obs = new MutationObserver$f(() => {
        tryErrorFire();
      });
      obs.observe(player, {
        attributes: true,
        attributeFilter: ["class"],
        childList: true,
        subtree: true
      });
      debugLog("info",
               "Error arm attached to movie_player " +
               `(signal=${normalizedErrorMode}).`);

      tryErrorFire();
    };

    const tryFire = () => {

      if (disabled)
        return true;
      if (w.location.href.indexOf("/watch?") === -1)
        return false;

      if (!pathAllowed(w.location.href, pathRules))
        return false;
      const player = document$1.getElementById("movie_player");
      if (!player || typeof player.loadVideoById !== "function")
        return false;

      ensureErrorObserver(player);
      let pr;
      try {
        pr = typeof player.getPlayerResponse === "function" ?
          player.getPlayerResponse() : null;
      }
      catch (e) {
        pr = null;
      }
      const videoId = pr && pr.videoDetails && pr.videoDetails.videoId;
      if (typeof videoId !== "string" || videoId === "")
        return false;
      if (videoId === lastFiredVideoId)
        return false;

      const state = safeCall(player, "getPlayerState");
      const currentTime = safeCall(player, "getCurrentTime");
      const loadedFraction = safeCall(player, "getVideoLoadedFraction");
      const duration = safeCall(player, "getDuration");
      const stateObj = safeCall(player, "getPlayerStateObject");
      const stateSnapshot =
        `state=${state}, current=${currentTime}, ` +
        `loadedFraction=${loadedFraction}, duration=${duration}, ` +
        `isBuffering=${stateObj && stateObj.isBuffering}`;

      const MIN_PLAYBACK_LOADED_FRACTION = 0.05;
      const hasMeaningfulBuffer =
        typeof loadedFraction === "number" &&
        loadedFraction >= MIN_PLAYBACK_LOADED_FRACTION;
      let shouldFire;
      let reason;
      if (state === 1 || state === 2 || state === 0) {
        shouldFire = false;
        reason = "already playing/paused/ended";
      }
      else if (state === 3 && typeof currentTime === "number" &&
               currentTime >= 1 && hasMeaningfulBuffer) {
        shouldFire = false;
        reason = "mid-playback buffer";
      }
      else {
        shouldFire = true;
        reason = "fresh / pre-playback";
      }
      if (!shouldFire) {
        lastFiredVideoId = videoId;
        debugLog("info",
                 `Skipping reload for "${videoId}": ` +
                 `${reason}. ${stateSnapshot}`);
        return true;
      }
      const startSeconds =
        (pr.playerConfig && pr.playerConfig.playbackStartConfig &&
         pr.playerConfig.playbackStartConfig.startSeconds) || 0;

      lastFiredVideoId = videoId;
      fireCount++;
      const seq = fireCount;
      const idForLog = videoId;
      const startForLog = startSeconds;
      const fire = () => {
        try {
          const elapsed = Date.now() - installedAt;
          debugLog("info",
                   `#${seq} [+${elapsed}ms] Firing ` +
                   `loadVideoById("${idForLog}", ${startForLog}). ` +
                   `${stateSnapshot}`);
          player.loadVideoById(idForLog, startForLog);
        }
        catch (e) {
          debugLog("error", `#${seq} loadVideoById threw: ${e}`);
        }
      };
      if (extraDelay > 0)
        setTimeout$2(fire, extraDelay);
      else
        fire();
      if (normalizedMode === "first") {
        disabled = true;
        debugLog("info",
                 "first-mode: disabling further reloads after this fire.");
      }
      return true;
    };

    const coldArm = () => {
      if (tryFire())
        return;
      let observer = new MutationObserver$f(() => {
        if (tryFire() && observer) {
          observer.disconnect();
          observer = null;
        }
      });
      observer.observe(document$1, {childList: true, subtree: true});
      setTimeout$2(() => {
        if (observer) {
          observer.disconnect();
          observer = null;
        }
      }, COLD_GIVE_UP_MS);
    };

    if (document$1.readyState === "loading")
      document$1.addEventListener("DOMContentLoaded", coldArm);
    else
      coldArm();

    document$1.addEventListener("yt-navigate-finish", () => {
      let attempts = NAV_RETRY_MAX;
      const tick = () => {
        if (tryFire())
          return;
        attempts--;
        if (attempts <= 0)
          return;
        setTimeout$2(tick, NAV_RETRY_INTERVAL_MS);
      };
      setTimeout$2(tick, NAV_RETRY_INTERVAL_MS);
    });

    debugLog("info",
             "Installed. Mode=" + normalizedMode + ". " +
             (normalizedMode === "first" ?
               "Fires once on the first video, then disables." :
               "Fires on every new video (cold load + SPA nav).") +
             (extraDelay > 0 ? ` +${extraDelay}ms delay.` : "") +
             (normalizedErrorMode === "none" ?
               " Error arm disabled." :
               ` Error arm via ${normalizedErrorMode} signal ` +
               "(1 reload/video).") +
             describePathRules(pathRules));
    end();
  }

  function safeCall(obj, name) {
    if (obj === null || typeof obj === "undefined")
      return void 0;
    const fn = obj[name];
    if (typeof fn !== "function")
      return void 0;
    try {
      return fn.call(obj);
    }
    catch (e) {
      return void 0;
    }
  }

  function parsePathRules(paths) {
    const allow = [];
    const deny = [];
    if (typeof paths !== "string" || paths.length === 0)
      return {allow, deny};
    const tokens = paths.split(/\s+/);
    for (let i = 0; i < tokens.length; i++) {
      const t = tokens[i];
      if (!t)
        continue;
      if (t.charAt(0) === "!" && t.length > 1)
        deny.push(t.slice(1).toLowerCase());
      else
        allow.push(t.toLowerCase());
    }
    return {allow, deny};
  }

  function firstPathSegment(href) {
    if (typeof href !== "string" || href.length === 0)
      return "";
    let p = href;

    const q = p.indexOf("?");
    if (q !== -1)
      p = p.slice(0, q);
    const h = p.indexOf("#");
    if (h !== -1)
      p = p.slice(0, h);

    const ss = p.indexOf("://");
    if (ss !== -1)
      p = p.slice(ss + 3);
    const slash = p.indexOf("/");
    if (slash === -1)
      return "";
    const path = p.slice(slash);
    const m = /^\/([^/]+)/.exec(path);
    return m ? m[1].toLowerCase() : "";
  }

  function pathAllowed(href, rules) {
    const segment = firstPathSegment(href);
    for (let i = 0; i < rules.deny.length; i++) {
      if (rules.deny[i] === segment)
        return false;
    }
    if (rules.allow.length === 0)
      return true;
    for (let i = 0; i < rules.allow.length; i++) {
      if (rules.allow[i] === segment)
        return true;
    }
    return false;
  }

  function describePathRules(rules) {
    if (rules.allow.length === 0 && rules.deny.length === 0)
      return "";
    const parts = [];
    if (rules.allow.length > 0)
      parts.push("allow=[" + rules.allow.join(",") + "]");
    if (rules.deny.length > 0)
      parts.push("deny=[" + rules.deny.join(",") + "]");
    return " Path filter: " + parts.join(" ") + ".";
  }

  const snippets$2 = {
    "abort-current-inline-script": abortCurrentInlineScript,
    "abort-on-iframe-property-read": abortOnIframePropertyRead,
    "abort-on-iframe-property-write": abortOnIframePropertyWrite,
    "abort-on-property-read": abortOnPropertyRead,
    "abort-on-property-write": abortOnPropertyWrite,
    "array-override": arrayOverride,
    "blob-override": blobOverride,
    "cookie-remover": cookieRemover,
    "debug": setDebug,
    "event-override": eventOverride,
    "freeze-element": freezeElement,
    "hide-if-canvas-contains": hideIfCanvasContains,
    "hide-if-shadow-contains": hideIfShadowContains,
    "json-override": jsonOverride,
    "json-prune": jsonPrune,
    "map-override": mapOverride,
    "override-property-read": overridePropertyRead,
    "prevent-element-src-loading": preventElementSrcLoading,
    "prevent-listener": preventListener,
    "prevent-window-open": preventWindowOpen,
    "profile": setProfile,
    "replace-argument": replaceArgument,
    "replace-fetch-response": replaceFetchResponse,
    "replace-outbound-value": replaceOutboundValue,
    "replace-xhr-request": replaceXhrRequest,
    "replace-xhr-response": replaceXhrResponse,
    "strip-fetch-query-parameter": stripFetchQueryParameter,
    "timer-override": timerOverride,
    "trace": trace,
    "tmp-yt-buffering-spoof": tmpYtBufferingSpoof,
    "tmp-yt-force-reload": tmpYtForceReload
  };

  let {Error: Error$5, MutationObserver: MutationObserver$e, Set: Set$3} = $(window);

  const elementQSA$1 = Element.prototype.querySelectorAll;

  const elementLoadHandlers = new Set$3();
  let sharedElementMo = null;

  const candidatesBuffer = $([]);

  function fillCandidates(records) {
    candidatesBuffer.length = 0;
    for (let r = 0; r < records.length; r++) {
      const record = records[r];
      const type = record.type;
      if (type === "attributes") {
        const el = record.target;
        if (el.src || el.href)
          candidatesBuffer.push(el);
      }
      else {
        const nodes = record.addedNodes;
        const len = nodes.length;
        for (let i = 0; i < len; i++) {
          const node = nodes[i];
          if (node.nodeType !== 1)
            continue;

          if (node.src || node.href)
            candidatesBuffer.push(node);
          if (node.childElementCount > 0) {
            for (const el of call(elementQSA$1, node, "[src],[href]"))
              candidatesBuffer.push(el);
          }
        }
      }
    }
  }

  function addElementHandler(handler) {
    elementLoadHandlers.add(handler);
    if (!sharedElementMo) {
      sharedElementMo = new MutationObserver$e(records => {
        fillCandidates(records);
        for (const h of elementLoadHandlers)
          h(candidatesBuffer);
      });
      sharedElementMo.observe(document, {
        childList: true,
        subtree: true,
        attributes: true,
        attributeFilter: ["src", "href"]
      });
    }
  }

  function removeElementHandler(handler) {
    elementLoadHandlers.delete(handler);
    if (elementLoadHandlers.size === 0 && sharedElementMo) {
      sharedElementMo.disconnect();
      sharedElementMo = null;
    }
  }

  function logIfElementLoads(urlPattern, type,
                                    tag = null, specifier = null) {
    if (!urlPattern)
      throw new Error$5("[log-if-element-loads snippet]: Missing URL pattern.");
    if (!type)
      throw new Error$5("[log-if-element-loads snippet]: Missing type.");

    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("log-if-element-loads");
    const {mark, end} = profile("log-if-element-loads");

    const re = toRegExp(urlPattern);
    const tagUpper = tag ? tag.toUpperCase() : null;

    const selector = tag ? `${tag}[src],${tag}[href]` : "[src],[href]";

    const urlOf = el => el.src || el.href || null;
    const matchesTag = el => tagUpper === null || el.nodeName === tagUpper;

    let callback;
    const win = raceWinner("log-if-element-loads", () => {
      removeElementHandler(callback);
    });

    let matched = false;
    callback = elements => {
      mark();
      const toCheck = elements !== null ? elements : $$(selector);

      let matchedEl = null;
      for (let i = 0; i < toCheck.length; i++) {
        const el = toCheck[i];
        if (!matchesTag(el))
          continue;
        const url = urlOf(el);
        if (url && re.test(url)) {
          matchedEl = el;
          matched = true;
          break;
        }
      }

      if (matched) {
        const url = specifier !== null ? specifier : urlOf(matchedEl);
        sendDetectionEvent(type, url);
        debugLog("success", "Matched element:", matchedEl, formattedArguments);
        win();
        removeElementHandler(callback);
      }
      end();
    };

    callback(null);
    if (matched)
      return;

    addElementHandler(callback);
  }

  function logIfAnchorHrefMatches(urlPattern, type, specifier = null) {
    logIfElementLoads(urlPattern, type, "a", specifier);
  }

  let {Error: Error$4, MutationObserver: MutationObserver$d} = $(window);

  function logIfSelectorExists(selector, type, specifier = null) {
    if (!selector)
      throw new Error$4("[log-if-selector-exists snippet]: Missing selector.");
    if (!type)
      throw new Error$4("[log-if-selector-exists snippet]: Missing type.");

    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("log-if-selector-exists");
    const {mark, end} = profile("log-if-selector-exists");
    let exists;
    if (selector.startsWith("xpath(") && selector.endsWith(")")) {
      let queryAll;
      try {
        queryAll = initQueryAll(selector);
      }
      catch (error) {
        throw new Error$4("[log-if-selector-exists snippet]: " +
                        "Invalid XPath selector: " + error.message);
      }
      exists = () => queryAll().length > 0;
    }
    else {
      exists = () => $$(selector).length > 0;
    }

    let mo;
    const win = raceWinner("log-if-selector-exists", () => {
      mo.disconnect();
    });

    let matched = false;
    const callback = () => {
      mark();
      if (exists()) {
        sendDetectionEvent(type, specifier);
        debugLog("success", "Matched selector:", selector, formattedArguments);
        win();
        mo.disconnect();
        matched = true;
      }
      end();
    };

    mo = new MutationObserver$d(callback);

    callback();
    if (matched)
      return;

    mo.observe(document, {childList: true, subtree: true});
  }

  function logIfScriptLoads(urlPattern, type, specifier = null) {
    logIfElementLoads(urlPattern, type, "script", specifier);
  }

  let {Error: Error$3, MutationObserver: MutationObserver$c, getComputedStyle: getComputedStyle$6, Set: Set$2} = $(window);

  const computedStyleHandlers = new Set$2();
  let sharedComputedStyleMo = null;

  const addedElementsBuffer = $([]);

  function fillAddedElements(records) {
    addedElementsBuffer.length = 0;
    for (let r = 0; r < records.length; r++) {
      const record = records[r];
      const nodes = record.addedNodes;
      const len = nodes.length;
      for (let i = 0; i < len; i++) {
        const node = nodes[i];
        if (node.nodeType === 1)
          addedElementsBuffer.push(node);
      }
    }
  }

  function addComputedStyleHandler(handler) {
    computedStyleHandlers.add(handler);
    if (!sharedComputedStyleMo) {
      sharedComputedStyleMo = new MutationObserver$c(records => {
        fillAddedElements(records);
        for (const h of computedStyleHandlers)
          h(addedElementsBuffer);
      });
      sharedComputedStyleMo.observe(document, {
        childList: true,
        subtree: true
      });
    }
  }

  function removeComputedStyleHandler(handler) {
    computedStyleHandlers.delete(handler);
    if (computedStyleHandlers.size === 0 && sharedComputedStyleMo) {
      sharedComputedStyleMo.disconnect();
      sharedComputedStyleMo = null;
    }
  }

  function logIfComputedStyleMatches(type, specifier, ...pairs) {
    if (!type)
      throw new Error$3("[log-if-computed-style-matches snippet]: Missing type.");
    if (pairs.length === 0 || pairs.length % 2 !== 0)
      throw new Error$3("[log-if-computed-style-matches snippet]: Uneven pairs.");

    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("log-if-computed-style-matches");
    const {mark, end} = profile("log-if-computed-style-matches");

    const conditions = $([]);
    for (let i = 0; i < pairs.length; i += 2)
      conditions.push({property: pairs[i], value: pairs[i + 1]});

    const resolvedSpecifier = specifier === "null" ? null : specifier;

    const matchesAllConditions = el => {
      try {
        const style = getComputedStyle$6(el);
        return conditions.every(({property, value}) =>
          style[property] === value);
      }
      catch (e) {
        return false;
      }
    };

    let callback;
    const win = raceWinner("log-if-computed-style-matches", () => {
      removeComputedStyleHandler(callback);
    });

    let matched = false;
    callback = elements => {
      mark();
      const toCheck = elements !== null ? elements : $$("*");
      let matchedEl = null;
      for (let i = 0; i < toCheck.length; i++) {
        const el = toCheck[i];
        if (matchesAllConditions(el)) {
          matchedEl = el;
          matched = true;
          break;
        }
      }
      if (matched) {
        sendDetectionEvent(type, resolvedSpecifier);
        debugLog(
          "success",
          "Matched computed style:",
          matchedEl,
          formattedArguments
        );
        win();
        removeComputedStyleHandler(callback);
      }
      end();
    };

    callback(null);
    if (matched)
      return;

    addComputedStyleHandler(callback);
  }

  function logIfIframeLoads(urlPattern, type, specifier = null) {
    logIfElementLoads(urlPattern, type, "iframe", specifier);
  }

  let {Error: Error$2, MutationObserver: MutationObserver$b, Set: Set$1} = $(window);

  const elementQSA = Element.prototype.querySelectorAll;

  const inlineFingerprintHandlers = new Set$1();
  let sharedInlineMo = null;

  const inlineScriptsBuffer = $([]);

  function fillInlineScripts(records) {
    inlineScriptsBuffer.length = 0;
    for (let r = 0; r < records.length; r++) {
      const record = records[r];
      const nodes = record.addedNodes;
      const len = nodes.length;
      for (let i = 0; i < len; i++) {
        const node = nodes[i];
        const nodeName = node.nodeName;
        if (nodeName === "SCRIPT") {
          if (!node.src)
            inlineScriptsBuffer.push(node);
        }
        else if (node.nodeType === 1 && node.childElementCount > 0) {
          const found = call(elementQSA, node, "script:not([src])");
          for (let j = 0; j < found.length; j++)
            inlineScriptsBuffer.push(found[j]);
        }
      }
    }
  }

  function addInlineHandler(handler) {
    inlineFingerprintHandlers.add(handler);
    if (!sharedInlineMo) {
      sharedInlineMo = new MutationObserver$b(records => {
        fillInlineScripts(records);
        for (const h of inlineFingerprintHandlers)
          h(inlineScriptsBuffer);
      });
      sharedInlineMo.observe(document, {childList: true, subtree: true});
    }
  }

  function removeInlineHandler(handler) {
    inlineFingerprintHandlers.delete(handler);
    if (inlineFingerprintHandlers.size === 0 && sharedInlineMo) {
      sharedInlineMo.disconnect();
      sharedInlineMo = null;
    }
  }

  function logIfInlineScriptContainsFingerprint(
    textPattern, type, specifier = null
  ) {
    if (!textPattern) {
      throw new Error$2(
        "[log-if-inline-script-contains-fingerprint snippet]: " +
        "Missing text pattern."
      );
    }
    if (!type) {
      throw new Error$2(
        "[log-if-inline-script-contains-fingerprint snippet]: " +
        "Missing type."
      );
    }

    if (textPattern.length < 8)
      return;

    const formattedArguments = formatArguments(arguments);
    const debugLog =
      getDebugger("log-if-inline-script-contains-fingerprint");
    const {mark, end} =
      profile("log-if-inline-script-contains-fingerprint");

    const spec =
      specifier !== null ? specifier : textPattern.slice(0, 5);

    let callback;
    const win = raceWinner(
      "log-if-inline-script-contains-fingerprint",
      () => {
        removeInlineHandler(callback);
      }
    );

    let matched = false;
    callback = scripts => {
      mark();
      const toCheck =
        scripts !== null ? scripts : $$("script:not([src])");
      for (let i = 0; i < toCheck.length; i++) {
        if (toCheck[i].textContent.includes(textPattern)) {
          matched = true;
          break;
        }
      }
      if (matched) {
        sendDetectionEvent(type, spec);
        debugLog(
          "success",
          "Matched inline script content",
          formattedArguments
        );
        win();
        removeInlineHandler(callback);
      }
      end();
    };

    callback(null);
    if (matched)
      return;

    addInlineHandler(callback);
  }

  const hitFilters$8 = new Set();

  function hideIfContains(search, selector = "*", searchSelector = null) {
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("hide-if-contains");
    const {mark, end} = profile("hide-if-contains");
    const onHideCallback = node => {
      mark();
      debugLog("success",
               "Matched: ",
               node,
               "\nFILTER: hide-if-contains",
               formattedArguments);
      const filter =
        "hide-if-contains " + formattedArguments;
      if (!hitFilters$8.has(filter)) {
        hitFilters$8.add(filter);
        sendSnippetHitEvent(filter);
      }
      end();
    };
    let re = toRegExp(search);

    const mo = hideIfMatches(element => re.test($(element).textContent),
                             selector,
                             searchSelector,
                             onHideCallback);
    mo.race(raceWinner(
      "hide-if-contains",
      () => {
        mo.disconnect();
      }
    ));
  }

  let {MutationObserver: MutationObserver$a, WeakSet: WeakSet$9, getComputedStyle: getComputedStyle$5} = $(window);
  const hitFilters$7 = new Set();

  function hideIfContainsAndMatchesStyle(search,
                                                selector = "*",
                                                searchSelector = null,
                                                style = null,
                                                searchStyle = null,
                                                waitUntil,
                                                windowWidthMin = null,
                                                windowWidthMax = null
  ) {
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("hide-if-contains-and-matches-style");
    const {mark, end} = profile("hide-if-contains-and-matches-style");
    const hiddenMap = new WeakSet$9();
    const logMap = debug() && new WeakSet$9();
    if (searchSelector == null)
      searchSelector = selector;

    const searchRegExp = toRegExp(search);

    const styleRegExp = style ? toRegExp(style) : null;
    const searchStyleRegExp = searchStyle ? toRegExp(searchStyle) : null;
    const mainLogic = () => {
      const callback = () => {
        mark();
        if ((windowWidthMin && window.innerWidth < windowWidthMin) ||
           (windowWidthMax && window.innerWidth > windowWidthMax)
        )
          return;
        for (const {element, rootParents} of $$(searchSelector, true)) {
          if (hiddenMap.has(element))
            continue;
          if (searchRegExp.test($(element).textContent)) {
            if (!searchStyleRegExp ||
              searchStyleRegExp.test(getComputedCSSText(element))) {
              const closest = $closest($(element), selector, rootParents);
              if (!closest)
                continue;
              if (!styleRegExp || styleRegExp.test(getComputedCSSText(closest))) {
                win();
                hideElement(closest);
                hiddenMap.add(element);
                debugLog("success",
                         "Matched: ",
                         closest,
                         "which contains: ",
                         element,
                         "\nFILTER: hide-if-contains-and-matches-style",
                         formattedArguments);
                const filter =
                  "hide-if-contains-and-matches-style " +
                  formattedArguments;
                if (!hitFilters$7.has(filter)) {
                  hitFilters$7.add(filter);
                  sendSnippetHitEvent(filter);
                }
              }
              else {
                if (!logMap || logMap.has(closest))
                  continue;
                debugLog("info",
                         "In this element the searchStyle matched " +
                         "but style didn't:\n",
                         closest,
                         getComputedStyle$5(closest),
                         formattedArguments);
                logMap.add(closest);
              }
            }
            else {
              if (!logMap || logMap.has(element))
                continue;
              debugLog("info",
                       "In this element the searchStyle didn't match:\n",
                       element,
                       getComputedStyle$5(element),
                       formattedArguments);
              logMap.add(element);
            }
          }
        }
        end();
      };

      const mo = new MutationObserver$a(callback);
      const win = raceWinner(
        "hide-if-contains-and-matches-style",
        () => mo.disconnect()
      );
      mo.observe(document, {childList: true, characterData: true, subtree: true});
      callback();
    };
    waitUntilEvent(debugLog, mainLogic, waitUntil);
  }

  let {
    getComputedStyle: getComputedStyle$4,
    MutationObserver: MutationObserver$9,
    Uint8Array
  } = $(window);
  const hitFilters$6 = new Set();

  function hideIfContainsImage(search, selector, searchSelector) {
    if (searchSelector == null)
      searchSelector = selector;

    let searchRegExp = toRegExp(search);

    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("hide-if-contains-image");
    const {mark, end} = profile("hide-if-contains-image");

    let callback = () => {
      mark();
      for (const {element, rootParents} of $$(searchSelector, true)) {
        let style = getComputedStyle$4(element);
        let match = $(style["background-image"]).match(/^url\("(.*)"\)$/);
        if (match) {
          fetchContent(match[1]).then(content => {
            if (searchRegExp.test(uint8ArrayToHex(new Uint8Array(content)))) {
              let closest = $closest($(element), selector, rootParents);
              if (closest) {
                win();
                hideElement(closest);
                debugLog("success",
                         "Matched: ",
                         closest,
                         "\nFILTER: hide-if-contains-image",
                         formattedArguments);
                const filter =
                  "hide-if-contains-image " +
                  formattedArguments;
                if (!hitFilters$6.has(filter)) {
                  hitFilters$6.add(filter);
                  sendSnippetHitEvent(filter);
                }
              }
            }
          });
        }
      }
      end();
    };

    let mo = new MutationObserver$9(callback);
    let win = raceWinner(
      "hide-if-contains-image",
      () => mo.disconnect()
    );
    mo.observe(document, {childList: true, subtree: true});
    callback();
  }

  let {
    getComputedStyle: getComputedStyle$3,
    MutationObserver: MutationObserver$8,
    WeakSet: WeakSet$8,
    DOMParser,
    Math: Math$3,
    Map: Map$2
  } = $(window);
  const hitFilters$5 = new Set();

  function hideIfSvgContains(
    search,
    selector,
    searchSelector,
    ...attributes
  ) {
    if (searchSelector == null)
      searchSelector = selector;

    const textSearchRegExp = toRegExp(search);
    const formattedArguments = formatArguments(arguments);
    const hiddenMap = new WeakSet$8();
    const debugLog = getDebugger("hide-if-svg-contains");
    const {mark, end} = profile("hide-if-svg-contains");
    const defaultOptionalParameters = new Map$2([
      ["-position-threshold", "500"],
      ["-disable-contained-check", "false"],
      ["-wait-until", ""],
      ["-opacity-alpha-threshold", "0.1"],
      ["-font-size-threshold", "1"]
    ]);
    let entries = $([]);
    for (let attr of attributes) {
      attr = $(attr);
      let markerIndex = attr.indexOf(":");
      if (markerIndex < 0)
        continue;

      let key = attr.slice(0, markerIndex).trim();
      let value = attr.slice(markerIndex + 1).trim();

      if (key && value) {
        if (defaultOptionalParameters.has(key))
          defaultOptionalParameters.set(key, value);
        else
          entries.push([key, value]);
      }
    }

    let defaultCSSEntries = $([
      ["display", "none"],
      ["visibility", "hidden"],
      ["opacity", "0"],
      ["fill", "none"],
      ["font-size", "0"]
    ]);
    let attributesMap = new Map$2(defaultCSSEntries.concat(entries));

    const positionThresh =
      parseFloat(defaultOptionalParameters.get("-position-threshold")) || 0;
    const disableContainedCheck =
      (defaultOptionalParameters.get("-disable-contained-check") === "true");
    const opacityAlphaThreshold =
      parseFloat(defaultOptionalParameters.get("-opacity-alpha-threshold")) || 0;
    const fontSizeThreshold =
      parseFloat(defaultOptionalParameters.get("-font-size-threshold")) || 0;

    const svgFetchCache = new Map$2();

    const mainLogic = async() => {

      let host;
      let callback = async() => {
        mark();
        for (const {element, rootParents} of $$(searchSelector, true)) {
          if (!host)
            host = createIsolatedHost();
          if (hiddenMap.has(element))
            continue;
          let isMatchAndVisible = false;
          try {
            const backgroundImage = $(getComputedStyle$3(element).backgroundImage);
            const urlMatch = backgroundImage.match(/url\("?(.+?)"?\)/);
            if (!urlMatch)
              continue;
            const url = urlMatch[1];

            let svgFetchPromise = svgFetchCache.get(url);
            if (!svgFetchPromise) {
              svgFetchPromise = fetchContent(url, {as: "text"});
              svgFetchCache.set(url, svgFetchPromise);
            }
            const svgContent = await svgFetchPromise;
            if (hiddenMap.has(element))
              continue;

            const parser = new DOMParser();
            const svgDoc = parser.parseFromString(svgContent, "image/svg+xml");
            if (svgDoc.querySelector("parsererror")) {
              debugLog(
                "warn",
                "Skipping malformed SVG:",
                url,
                "for element:",
                element);
              continue;
            }

            host.svgContainer.innerHTML = "";
            host.svgContainer.appendChild(svgDoc.documentElement);

            const textElements =
              host.svgContainer.querySelectorAll("text, tspan");
            for (const textEl of textElements) {
              if (
                isElementVisibleAndTextMatchesInSvg(
                  textEl,
                  textSearchRegExp
                )) {
                isMatchAndVisible = true;
                debugLog(
                  "info",
                  "Condition met: Text found visible in SVG of element",
                  element);
                break;
              }
            }
          }
          catch (error) {
            debugLog(
              "warn",
              "An error occurred while processing element:",
              element,
              error);
            continue;
          }
          if (isMatchAndVisible && !hiddenMap.has(element)) {
            const closestToHide = $closest($(element), selector, rootParents);
            if (closestToHide) {
              win();
              hideElement(closestToHide);
              hiddenMap.add(element);
              debugLog("success",
                       "Matched: ",
                       closestToHide,
                       "\nFILTER: hide-if-svg-contains",
                       formattedArguments);
              const filter =
                "hide-if-svg-contains " +
                formattedArguments;
              if (!hitFilters$5.has(filter)) {
                hitFilters$5.add(filter);
                sendSnippetHitEvent(filter);
              }
            }
          }
        }
        end();
      };

      let mo = new MutationObserver$8(callback);
      let win = raceWinner(
        "hide-if-svg-contains",
        () => {
          mo.disconnect();

          host.cleanup();
        }
      );
      mo.observe(document, {childList: true, subtree: true});
      callback();
    };
    const waitUntil = defaultOptionalParameters.get("-wait-until");
    waitUntilEvent(debugLog, mainLogic, waitUntil);

    function isElementVisibleAndTextMatchesInSvg(
      element,
      searchRegExp
    ) {
      if (!searchRegExp.test(element.textContent))
        return false;
      const styles = getComputedStyle$3(element);
      const elementCoordinates = element.getBoundingClientRect();
      const parentCoordinates = element.ownerSVGElement.getBoundingClientRect();
      const rgbaRegex = /\b(rgba?|hsla?|hwb)\b/;

      for (const [key, value] of attributesMap) {
        const styleValue = styles[key];
        const floatStyleValue = parseFloat(styleValue);
        switch (key) {
          case "color":
          case "fill": {
            if (styleValue && rgbaRegex.test(styleValue) &&
            opacityAlphaThreshold != 0) {
              const alphaStr = styleValue.split(",")[3] ||
              styleValue.split("/")[1];
              const alphaValue = alphaStr ? parseFloat(alphaStr) : 1.0;
              if (!isNaN(alphaValue) && alphaValue <= opacityAlphaThreshold)
                return false;
            }
            else if (styleValue && toRegExp(value).test(styleValue)) {
              return false;
            }
            break;
          }
          case "opacity": {
            if (!isNaN(floatStyleValue) && opacityAlphaThreshold > 0) {
              if (floatStyleValue <= opacityAlphaThreshold)
                return false;
            }
            break;
          }
          case "font-size": {
            if (!isNaN(floatStyleValue) && fontSizeThreshold > 0) {
              if (floatStyleValue <= fontSizeThreshold)
                return false;
            }
            break;
          }

          default: {
            if (styleValue && toRegExp(value).test(styleValue))
              return false;
            break;
          }
        }
      }
      if (!disableContainedCheck && positionThresh > 0) {
        const finalX = elementCoordinates.x - parentCoordinates.x;
        const finalY = elementCoordinates.y - parentCoordinates.y;

        if (Math$3.abs(finalX) > positionThresh ||
        Math$3.abs(finalY) > positionThresh)
          return false;
      }
      return true;
    }

    function createIsolatedHost() {
      debugLog(
        "info",
        "Creating Isolated element to host SVGs"
      );
      const shadowRootHost = document.createElement("div");
      shadowRootHost.style.cssText = "position: absolute; " +
      "top: -9999px; left: -9999px; " +
      "border: none; " +
      "pointer-events: none;";
      document.body.appendChild(shadowRootHost);
      const attachedShadowRoot = shadowRootHost.attachShadow({mode: "closed"});
      attachedShadowRoot.innerHTML = `
        <div id="container" style="{ all: initial; }">
        </div>
      `;
      let svgContainer = attachedShadowRoot.querySelector("#container");
      return {
        svgContainer,
        cleanup: () => shadowRootHost.remove()
      };
    }
  }

  const {parseFloat: parseFloat$3, Math: Math$2, MutationObserver: MutationObserver$7, WeakSet: WeakSet$7} = $(window);
  const {min} = Math$2;
  const hitFilters$4 = new Set();

  const ld = (a, b) => {
    const len1 = a.length + 1;
    const len2 = b.length + 1;
    const d = [[0]];
    let i = 0;
    let I = 0;

    while (++i < len2)
      d[0][i] = i;

    i = 0;
    while (++i < len1) {
      const c = a[I];
      let j = 0;
      let J = 0;
      d[i] = [i];
      while (++j < len2) {
        d[i][j] = min(d[I][j] + 1, d[i][J] + 1, d[I][J] + (c != b[J]));
        ++J;
      }
      ++I;
    }
    return d[len1 - 1][len2 - 1];
  };

  function hideIfContainsSimilarText(
    search, selector,
    searchSelector = null,
    ignoreChars = 0,
    maxSearches = 0
  ) {
    const visitedNodes = new WeakSet$7();
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("hide-if-contains-similar-text");
    const {mark, end} = profile("hide-if-contains-similar-text");
    const $search = $(search);
    const {length} = $search;
    const chars = length + parseFloat$3(ignoreChars) || 0;
    const find = $([...$search]).sort();
    const guard = parseFloat$3(maxSearches) || Infinity;

    if (searchSelector == null)
      searchSelector = selector;

    debugLog("info", "Looking for similar text: " + $search);

    const callback = () => {
      mark();
      for (const {element, rootParents} of $$(searchSelector, true)) {
        if (visitedNodes.has(element))
          continue;

        visitedNodes.add(element);
        const {innerText} = $(element);
        const loop = min(guard, innerText.length - chars + 1);
        for (let i = 0; i < loop; i++) {
          const str = $(innerText).substr(i, chars);
          const distance = ld(find, $([...str]).sort()) - ignoreChars;
          if (distance <= 0) {
            const closest = $closest($(element), selector, rootParents);
            if (closest) {
              win();
              hideElement(closest);
              debugLog("success",
                       "Found similar text: " + $search,
                       closest,
                       "\nFILTER: hide-if-contains-similar-text",
                       formattedArguments);
              const filter =
                "hide-if-contains-similar-text " +
                formattedArguments;
              if (!hitFilters$4.has(filter)) {
                hitFilters$4.add(filter);
                sendSnippetHitEvent(filter);
              }
              break;
            }
          }
        }
      }
      end();
    };

    let mo = new MutationObserver$7(callback);
    let win = raceWinner(
      "hide-if-contains-similar-text",
      () => mo.disconnect()
    );
    mo.observe(document, {childList: true, characterData: true, subtree: true});
    callback();
  }

  let {getComputedStyle: getComputedStyle$2, Map: Map$1, WeakSet: WeakSet$6, parseFloat: parseFloat$2, Math: Math$1} = $(window);
  const hitFilters$3 = new Set();

  const {ELEMENT_NODE: ELEMENT_NODE$2, TEXT_NODE} = Node;

  function hideIfContainsVisibleText(search, selector,
                                            searchSelector = null,
                                            ...attributes) {
    const {mark, end} = profile("hide-if-contains-visible-text");
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("hide-if-contains-visible-text");
    let entries = $([]);
    const optionalParams = new Map$1([
      ["-snippet-box-margin", "2"],
      ["-disable-bg-color-check", "false"],
      ["-check-is-contained", "false"],
      ["-pseudo-box-margin", "2"],
      ["-ignore-padding", "false"],
      ["-disable-font-check", "false"],
      ["-wait-until", ""]
    ]);
    let defaultEntries = $([
      ["opacity", "0"],
      ["font-size", "0px"],

      ["color", "rgba(0, 0, 0, 0)"]
    ]);
    for (let attr of attributes) {
      attr = $(attr);
      let markerIndex = attr.indexOf(":");
      if (markerIndex < 0)
        continue;
      let key = attr.slice(0, markerIndex).trim().toString();
      let value = attr.slice(markerIndex + 1).trim().toString();
      if (key && value) {
        if (optionalParams.has(key))
          optionalParams.set(key, value);
        else
          entries.push([key, value]);
      }
    }
    let attributesMap = new Map$1(defaultEntries.concat(entries));

    function getPseudoContent(element, pseudo, parentMatrix,
                              {bgColorCheck = true,
                               transThresh = 2,
                               fontCheck = true} = {}) {
      let style = getComputedStyle$2(element, pseudo);

      if (!isVisible(element, style) ||
       !isTextVisible(element,
                      style,
                      attributesMap,
                      {bgColorCheck, fontCheck: false}))
        return "";

      let {content} = $(style);
      if (content && content !== "none") {
        let strings = $([]);

        const domMatrix = getTransformMatrix(element, pseudo);
        const resultMatrix = parentMatrix.multiply(domMatrix);

        const angle = Math$1.atan2(resultMatrix.b, resultMatrix.a);
        const angleDegrees = angle * (180 / Math$1.PI);
        const rotated = Math$1.abs(angleDegrees) > 5;

        if (rotated)
          return "";

        const translated = Math$1.abs(resultMatrix.e) > transThresh ||
                           Math$1.abs(resultMatrix.f) > transThresh;
        if (translated)
          return "";

        content = $(content).trim().replace(
          /(["'])(?:(?=(\\?))\2.)*?\1/g,
          value => `\x01${strings.push($(value).slice(1, -1)) - 1}`
        );

        content = content.replace(
          /\s*attr\(\s*([^\s,)]+)[^)]*?\)\s*/g,
          (_, name) => $(element).getAttribute(name) || ""
        );

        const finalText = content.replace(
          /\x01(\d+)/g,
          (_, index) => strings[index]);

        if (fontCheck && finalText && !isFontVisible(element, style, finalText))
          return "";

        return finalText;
      }
      return "";
    }

    function getVisibleContent(element,
                               closest,
                               style,
                               parentOverflowNode,
                               originalElement,
                               shadowRootParents,
                               domMatrix,
                               {
                                 boxMargin = 2,
                                 bgColorCheck,
                                 checkIsContained,
                                 fontCheck,
                                 transThresh
                               } = {}) {
      let checkClosest = !style;
      if (checkClosest)
        style = getComputedStyle$2(element);

      if (!isVisible(element, style, checkClosest && closest, shadowRootParents))
        return "";

      if (!parentOverflowNode &&
        (
          $(style).getPropertyValue("overflow-x") === "hidden" ||
          $(style).getPropertyValue("overflow-y") === "hidden"
        )
      )
        parentOverflowNode = element;

      if (!domMatrix)
        domMatrix = getTransformMatrix(element);

      else
        domMatrix = domMatrix.multiply(getTransformMatrix(element));

      let text = getPseudoContent(element,
                                  ":before",
                                  domMatrix,
                                  {bgColorCheck,
                                   transThresh,
                                   fontCheck});
      for (let node of $childNodes($(element))) {
        switch ($(node).nodeType) {
          case ELEMENT_NODE$2:
            text += getVisibleContent(node,
                                      element,
                                      getComputedStyle$2(node),
                                      parentOverflowNode,
                                      originalElement,
                                      shadowRootParents,
                                      domMatrix,
                                      {
                                        boxMargin,
                                        bgColorCheck,
                                        checkIsContained,
                                        transThresh,
                                        fontCheck
                                      }
            );
            break;
          case TEXT_NODE:

            if (parentOverflowNode) {
              if (isContained(element, parentOverflowNode, {
                boxMargin,
                ignorePadding
              }) && isTextVisible(element,
                                  style,
                                  attributesMap,
                                  {bgColorCheck, fontCheck}))
                text += $(node).nodeValue;
            }
            else if (isTextVisible(element,
                                   style,
                                   attributesMap,
                                   {bgColorCheck, fontCheck})) {
              if (checkIsContained && !isContained(element, originalElement, {
                boxMargin,
                ignorePadding
              }))
                continue;
              text += $(node).nodeValue;
            }
            break;
        }
      }
      text += getPseudoContent(element,
                               ":after",
                               domMatrix,
                               {bgColorCheck,
                                transThresh,
                                fontCheck});
      return text;
    }

    const boxMargin = parseFloat$2(optionalParams.get("-snippet-box-margin")) || 0;
    const bgColorCheck = optionalParams.get("-disable-bg-color-check") !== "true";
    const fontCheck = optionalParams.get("-disable-font-check") !== "true";
    const checkIsContained = optionalParams.get("-check-is-contained") === "true";
    const ignorePadding = optionalParams.get("-ignore-padding") === "true";
    const transThresh = parseFloat$2(optionalParams.get("-pseudo-box-margin")) || 0;

    let searchRegex = toRegExp(search);
    let seen = new WeakSet$6();

    const mainLogic = async() => {
      const mo = hideIfMatches(
        (element, closest, rootParents) => {
          mark();
          if (seen.has(element))
            return false;

          seen.add(element);
          let text = getVisibleContent(
            element, closest, null, null, element, rootParents, null, {
              boxMargin,
              bgColorCheck,
              checkIsContained,
              transThresh,
              fontCheck
            }
          );
          let result = searchRegex.test(text);
          if (text.length) {
            if (result) {
              debugLog("success", result, searchRegex, text, "\nFILTER: hide-if-contains-visible-text", formattedArguments);
              const filter =
                "hide-if-contains-visible-text " +
                formattedArguments;
              if (!hitFilters$3.has(filter)) {
                hitFilters$3.add(filter);
                sendSnippetHitEvent(filter);
              }
            }
            else {
              debugLog("info", result, searchRegex, text);
            }
          }
          end();
          return result;
        },
        selector,
        searchSelector
      );
      mo.race(raceWinner(
        "hide-if-contains-visible-text",
        () => {
          mo.disconnect();
        }
      ));
    };
    const waitUntil = optionalParams.get("-wait-until");
    waitUntilEvent(debugLog, mainLogic, waitUntil);
  }

  let {MutationObserver: MutationObserver$6, WeakSet: WeakSet$5, getComputedStyle: getComputedStyle$1} = $(window);
  const hitFilters$2 = new Set();

  function hideIfHasAndMatchesStyle(search,
                                           selector = "*",
                                           searchSelector = null,
                                           style = null,
                                           searchStyle = null,
                                           waitUntil = null,
                                           windowWidthMin = null,
                                           windowWidthMax = null
  ) {
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("hide-if-has-and-matches-style");
    const {mark, end} = profile("hide-if-has-and-matches-style");
    const hiddenMap = new WeakSet$5();
    const logMap = debug() && new WeakSet$5();
    if (searchSelector == null)
      searchSelector = selector;

    const styleRegExp = style ? toRegExp(style) : null;
    const searchStyleRegExp = searchStyle ? toRegExp(searchStyle) : null;
    const mainLogic = () => {
      const callback = () => {
        mark();
        if ((windowWidthMin && window.innerWidth < windowWidthMin) ||
           (windowWidthMax && window.innerWidth > windowWidthMax)
        )
          return;
        for (const {element, rootParents} of $$(searchSelector, true)) {
          if (hiddenMap.has(element))
            continue;
          if ($(element).querySelector(search) &&
              (!searchStyleRegExp ||
              searchStyleRegExp.test(getComputedCSSText(element)))) {
            const closest = $closest($(element), selector, rootParents);
            if (closest && (!styleRegExp ||
                            styleRegExp.test(getComputedCSSText(closest)))) {
              win();
              hideElement(closest);
              hiddenMap.add(element);
              debugLog("success",
                       "Matched: ",
                       closest,
                       "which contains: ",
                       element,
                       "\nFILTER: hide-if-has-and-matches-style",
                       formattedArguments);
              const filter =
                "hide-if-has-and-matches-style " +
                formattedArguments;
              if (!hitFilters$2.has(filter)) {
                hitFilters$2.add(filter);
                sendSnippetHitEvent(filter);
              }
            }
            else {
              if (!logMap || logMap.has(closest))
                continue;
              debugLog("info",
                       "In this element the searchStyle matched" +
                       "but style didn't:\n",
                       closest,
                       getComputedStyle$1(closest),
                       ...arguments);
              logMap.add(closest);
            }
          }
          else {
            if (!logMap || logMap.has(element))
              continue;
            debugLog("info",
                     "In this element the searchStyle didn't match:\n",
                     element,
                     getComputedStyle$1(element),
                     ...arguments);
            logMap.add(element);
          }
        }
        end();
      };

      const mo = new MutationObserver$6(callback);
      const win = raceWinner(
        "hide-if-has-and-matches-style",
        () => mo.disconnect()
      );
      mo.observe(document, {childList: true, subtree: true});
      callback();
    };
    waitUntilEvent(debugLog, mainLogic, waitUntil);
  }

  let {getComputedStyle, MutationObserver: MutationObserver$5, WeakSet: WeakSet$4} = $(window);

  function hideIfLabelledBy(search, selector, searchSelector = null) {
    const {mark, end} = profile("hide-if-labelled-by");
    let sameSelector = searchSelector == null;

    let searchRegExp = toRegExp(search);

    let matched = new WeakSet$4();

    let callback = () => {
      mark();
      for (const {element, rootParents} of $$(selector, true)) {
        let closest = sameSelector ?
                      element :
                      $closest($(element), searchSelector, rootParents);
        if (!closest ||
            !isVisible(element, getComputedStyle(element), closest))
          continue;

        let attr = $(element).getAttribute("aria-labelledby");
        let fallback = () => {
          if (matched.has(closest))
            return;

          if (searchRegExp.test(
            $(element).getAttribute("aria-label") || ""
          )) {
            win();
            matched.add(closest);
            hideElement(closest);
          }
        };

        if (attr) {
          for (let label of $(attr).split(/\s+/)) {
            let target = $(document).getElementById(label);
            if (target) {
              if (!matched.has(target) && searchRegExp.test(target.innerText)) {
                win();
                matched.add(target);
                hideElement(closest);
              }
            }
            else {
              fallback();
            }
          }
        }
        else {
          fallback();
        }
      }
      end();
    };

    let mo = new MutationObserver$5(callback);
    let win = raceWinner(
      "hide-if-labelled-by",
      () => mo.disconnect()
    );
    mo.observe(document, {characterData: true, childList: true, subtree: true});
    callback();
  }

  let {MutationObserver: MutationObserver$4, WeakSet: WeakSet$3} = $(window);
  const hitFilters$1 = new Set();

  const {ELEMENT_NODE: ELEMENT_NODE$1} = Node;

  function hideIfMatchesXPath(query, scopeQuery, waitUntil) {
    const {mark, end} = profile("hide-if-matches-xpath");
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("hide-if-matches-xpath");

    const mainLogic = () => {
      const startHidingMutationObserver = scopeNode => {
        const queryAndApply = initQueryAndApply(`xpath(${query})`);
        const seenMap = new WeakSet$3();
        const hideNode = node => {
          seenMap.add(node);
          win();

          if ($(node).nodeType === ELEMENT_NODE$1)
            hideElement(node);
          else
            $(node).textContent = "";
          debugLog("success",
                   "Matched: ",
                   node,
                   "\nFILTER: hide-if-matches-xpath",
                   formattedArguments);
          const filter =
            "hide-if-matches-xpath " +
            formattedArguments;
          if (!hitFilters$1.has(filter)) {
            hitFilters$1.add(filter);
            sendSnippetHitEvent(filter);
          }
        };

        const callback = () => {
          mark();
          queryAndApply(node => {
            if (seenMap.has(node))
              return false;

            if (scopeQuery) {
              const scopeQueryAndApply = initQueryAndApply(`xpath(${scopeQuery})`);
              scopeQueryAndApply(matchingScopeNode => {
                if (matchingScopeNode.contains(node)) {

                  hideNode(node);
                }
                else {

                  return false;
                }
              });
            }
            else {
              hideNode(node);
            }
          });
          end();
        };
        const mo = new MutationObserver$4(callback);
        const win = raceWinner(
          "hide-if-matches-xpath",
          () => mo.disconnect()
        );
        mo.observe(
          scopeNode, {characterData: true, childList: true, subtree: true});
        callback();
      };

      if (scopeQuery) {

        let count = 0;
        let scopeMutationObserver;
        const scopeQueryAndApply = initQueryAndApply(`xpath(${scopeQuery})`);
        const findMutationScopeNodes = () => {
          scopeQueryAndApply(scopeNode => {

            startHidingMutationObserver(scopeNode);
            count++;
          });
          if (count > 0)
            scopeMutationObserver.disconnect();
        };
        scopeMutationObserver = new MutationObserver$4(findMutationScopeNodes);
        scopeMutationObserver.observe(
          document, {characterData: true, childList: true, subtree: true}
        );
        findMutationScopeNodes();
      }
      else {

        startHidingMutationObserver(document);
      }
    };

    waitUntilEvent(debugLog, mainLogic, waitUntil);
  }

  let {MutationObserver: MutationObserver$3, WeakSet: WeakSet$2} = $(window);
  const hitFilters = new Set();

  const {ELEMENT_NODE} = Node;

  function hideIfMatchesComputedXPath(query, searchQuery, searchRegex,
                                             waitUntil) {
    const {mark, end} = profile("hide-if-matches-computed-xpath");
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("hide-if-matches-computed-xpath");

    if (!searchQuery || !query) {
      debugLog("error", "No query or searchQuery provided.");
      return;
    }

    const computeQuery = foundText => query.replace("{{}}", foundText);

    const startHidingMutationObserver = foundText => {
      const computedQuery = computeQuery(foundText);
      debugLog("info",
               "Starting hiding elements that match query: ",
               computedQuery);
      const queryAndApply = initQueryAndApply(`xpath(${computedQuery})`);
      const seenMap = new WeakSet$2();
      const callback = () => {
        mark();
        queryAndApply(node => {
          if (seenMap.has(node))
            return false;
          seenMap.add(node);
          win();
          if ($(node).nodeType === ELEMENT_NODE)
            hideElement(node);
          else
            $(node).textContent = "";
          debugLog("success",
                   "Matched: ",
                   node,
                   "\nFILTER: hide-if-matches-computed-xpath",
                   formattedArguments);
          const filter =
            "hide-if-matches-computed-xpath " +
            formattedArguments;
          if (!hitFilters.has(filter)) {
            hitFilters.add(filter);
            sendSnippetHitEvent(filter);
          }
        });
        end();
      };
      const mo = new MutationObserver$3(callback);
      const win = raceWinner(
        "hide-if-matches-computed-xpath",
        () => mo.disconnect()
      );
      mo.observe(
        document, {characterData: true, childList: true, subtree: true});
      callback();
    };

    const re = toRegExp(searchRegex);

    const mainLogic = () => {
      if (searchQuery) {
        debugLog("info", "Started searching for: ", searchQuery);
        const seenMap = new WeakSet$2();
        let searchMO;
        const searchQueryAndApply = initQueryAndApply(`xpath(${searchQuery})`);
        const findMutationSearchNodes = () => {
          searchQueryAndApply(searchNode => {
            if (seenMap.has(searchNode))
              return false;
            seenMap.add(searchNode);
            debugLog("info", "Found node: ", searchNode);
            if (searchNode.innerHTML) {
              debugLog("info", "Searching in: ", searchNode.innerHTML);
              const foundTextArr = searchNode.innerHTML.match(re);
              if (foundTextArr && foundTextArr.length) {
                let foundText = "";

                foundTextArr[1] ? foundText = foundTextArr[1] :
                  foundText = foundTextArr[0];
                debugLog("info", "Matched search query: ", foundText);
                startHidingMutationObserver(foundText);
              }
            }
          });
        };

        searchMO = new MutationObserver$3(findMutationSearchNodes);
        searchMO.observe(
          document, {characterData: true, childList: true, subtree: true}
        );
        findMutationSearchNodes();
      }
    };

    waitUntilEvent(debugLog, mainLogic, waitUntil);
  }

  let {
    parseInt: parseInt$2,
    setTimeout: setTimeout$1,
    Error: Error$1,
    MouseEvent: MouseEvent$1,
    MutationObserver: MutationObserver$2,
    WeakSet: WeakSet$1
  } = $(window);

  const VALID_TYPES = ["auxclick", "click", "dblclick",	"gotpointercapture",
                       "lostpointercapture", "mouseenter", "mousedown",
                       "mouseleave", "mousemove", "mouseout", "mouseover",
                       "mouseup",	"pointerdown", "pointerenter",
                       "pointermove", "pointerover", "pointerout",
                       "pointerup", "pointercancel", "pointerleave"];

  function simulateMouseEvent(...selectors) {
    const formattedArguments = formatArguments(arguments);
    const debugLog = getDebugger("simulate-mouse-event");
    const {mark, end} = profile("simulate-mouse-event");
    const MAX_ARGS = 7;
    if (selectors.length < 1)
      throw new Error$1("[simulate-mouse-event snippet]: No selector provided.");

    if (selectors.length > MAX_ARGS) {

      selectors = selectors.slice(0, MAX_ARGS);
    }
    function parseArg(theRule) {
      if (!theRule)
        return null;

      const result = {
        selector: "",
        continue: false,
        trigger: false,
        event: "click",
        delay: "500",
        clicked: false,
        found: false
      };
      const textArr = theRule.split("$");
      let options = [];
      if (textArr.length >= 2)
        options = textArr[1].toLowerCase().split(",");

      [result.selector] = textArr;

      for (const option of options) {
        if (option === "trigger") {
          result.trigger = true;
        }
        else if (option === "continue") {
          result.continue = true;
        }
        else if (option.startsWith("event")) {
          const event = option.toLowerCase().split("=");
          event[1] ? result.event = event[1] : result.event = "click";
        }
        else if (option.startsWith("delay")) {
          const delay = option.toLowerCase().split("=");
          delay[1] ? result.delay = delay[1] : result.delay = "500";
        }
      }
      if (!VALID_TYPES.includes(result.event)) {
        debugLog("warn",
                 result.event,
                 " might be misspelled, check for typos.\n",
                 "These are the supported events:",
                 VALID_TYPES);
      }
      return result;
    }

    const parsedArgs = $([]);

    $(selectors).forEach(rule => {
      const parsedRule = parseArg(rule);
      parsedArgs.push(parsedRule);
    });

    function checkIfAllSelectorsFound() {
      parsedArgs.forEach(arg => {
        if (!arg.found) {
          const queryAll = initQueryAll(arg.selector);
          const elems = queryAll();
          if (elems.length > 0)
            arg.found = true;
        }
      });
      return parsedArgs.every(arg => arg.found);
    }

    function resetAttributes() {
      parsedArgs.forEach(arg => {
        arg.found = false;
        arg.clicked = false;
      });
    }

    function triggerEvent(node, event, delay) {

      if (!node || !event)
        return;

      if (event === "click" && node.click) {
        node.click();
        debugLog("success",
                 "Clicked on this node:\n",
                 node,
                 "\nwith a delay of",
                 delay,
                 "ms",
                 `n\nFILTER: simulate-mouse-event ${formattedArguments}`
        );
      }
      else {
        node.dispatchEvent(
          new MouseEvent$1(event, {bubbles: true, cancelable: true})
        );
        debugLog("success",
                 "A",
                 event,
                 "event was dispatched with a delay of",
                 delay,
                 "ms on this node:\n",
                 node,
                 `n\nFILTER: simulate-mouse-event ${formattedArguments}`
        );
      }

      if (!hitEventSent.has(node)) {
        hitEventSent.add(node);
        sendSnippetHitEvent("simulate-mouse-event " + formattedArguments);
      }
    }
    let allFound = false;

    const [last] = parsedArgs.slice(-1);
    last.trigger = true;

    let dispatchedNodes = new WeakSet$1();
    let hitEventSent = new WeakSet$1();

    let observer = new MutationObserver$2(findNodesAndDispatchEvents);
    observer.observe(document, {childList: true, subtree: true});
    findNodesAndDispatchEvents();

    function findNodesAndDispatchEvents() {
      mark();

      if (!allFound)
        allFound = checkIfAllSelectorsFound();
      if (allFound) {
        for (const parsedRule of parsedArgs) {
          const queryAndApply = initQueryAndApply(parsedRule.selector);
          const delayInMiliseconds = parseInt$2(parsedRule.delay, 10);
          if (parsedRule.trigger) {
            queryAndApply(node => {
              if (!dispatchedNodes.has(node)) {
                dispatchedNodes.add(node);
                if (parsedRule.continue) {
                  setInterval(() => {
                    resetAttributes();
                    const allIsStillFound = checkIfAllSelectorsFound();
                    if (allIsStillFound)
                      triggerEvent(node, parsedRule.event, parsedRule.delay);
                  }, delayInMiliseconds);
                }
                else {
                  setTimeout$1(() => {
                    triggerEvent(node, parsedRule.event, parsedRule.delay);
                  }, delayInMiliseconds);
                }
              }
            });
          }
        }
      }
      end();
    }
  }

  let {isNaN: isNaN$1, MutationObserver: MutationObserver$1, parseInt: parseInt$1, parseFloat: parseFloat$1, setTimeout} = $(window);

  function skipVideo(playerSelector, xpathCondition, ...attributes) {
    const formattedArguments = formatArguments(arguments);
    const optionalParameters = new Map([
      ["-max-attempts", "10"],
      ["-retry-ms", "10"],
      ["-run-once", "false"],
      ["-wait-until", ""],
      ["-skip-to", "-0.1"],
      ["-stop-on-video-end", "false"],
      ["-start-from", "0"],
      ["-mute-video-when-skipping", "true"]
    ]);

    for (let attr of attributes) {
      attr = $(attr);
      let markerIndex = attr.indexOf(":");
      if (markerIndex < 0)
        continue;

      let key = attr.slice(0, markerIndex).trim().toString();
      let value = attr.slice(markerIndex + 1).trim().toString();

      if (key && value && optionalParameters.has(key))
        optionalParameters.set(key, value);
    }

    const maxAttemptsStr = optionalParameters.get("-max-attempts");
    const maxAttemptsNum = parseInt$1(maxAttemptsStr || 10, 10);

    const retryMsStr = optionalParameters.get("-retry-ms");
    const retryMsNum = parseInt$1(retryMsStr || 10, 10);

    const runOnceStr = optionalParameters.get("-run-once");
    const runOnceFlag = (runOnceStr === "true");

    const skipToStr = optionalParameters.get("-skip-to");
    const skipToNum = parseFloat$1(skipToStr || -0.1);

    const startFromStr = optionalParameters.get("-start-from");
    const startFrom = parseInt$1(startFromStr || 0, 10);

    const waitUntil = optionalParameters.get("-wait-until");

    const stopOnVideoEndStr = optionalParameters.get("-stop-on-video-end");
    const stopOnVideoEndFlag = (stopOnVideoEndStr === "true");

    const muteVideoStr = optionalParameters.get("-mute-video-when-skipping");
    const muteVideo = !(muteVideoStr === "false");

    const debugLog = getDebugger("skip-video");
    const {mark, end} = profile("skip-video");
    const queryAndApply = initQueryAndApply(`xpath(${xpathCondition})`);
    let skippedOnce = false;

    const mainLogic = () => {
      mark();
      const seenMap = new WeakSet();
      const callback = (retryCounter = 0) => {
        if (skippedOnce && runOnceFlag) {
          if (mo)
            mo.disconnect();
          return;
        }
        queryAndApply(node => {
          let nodeAlreadySeen = seenMap.has(node);
          let lastSkippedVideoDuration;
          if (!nodeAlreadySeen) {
            debugLog("info", "Matched:", node, " for selector: ", xpathCondition);
            debugLog("info", "Running video skipping logic.");
          }
          const videos = $$(playerSelector);
          let foundValidVideo = false;
          for (const video of videos) {
            if (!video || isNaN$1(video.duration) || isNaN$1(video.currentTime))
              continue;
            foundValidVideo = true;
            const videoNearEnd = (video.duration - video.currentTime) < 0.5;
            if ((video.duration > 0) && (video.currentTime < video.duration) &&
                !(stopOnVideoEndFlag && videoNearEnd)) {
              if (muteVideo) {
                video.muted = true;
                if (!nodeAlreadySeen) {
                  debugLog("success", "Muted video...");
                  sendSnippetHitEvent("skip-video " + formattedArguments);
                }
              }
              if (startFrom <= video.currentTime * 1000) {

                skipToNum <= 0 ?
                  video.currentTime = video.duration + skipToNum :
                  video.currentTime += skipToNum;
                if (lastSkippedVideoDuration !== video.duration) {
                  debugLog("success",
                           "Skipped video, currentTime: ",
                           video.currentTime,
                           "s.",
                           "\nFILTER: skip-video",
                           formattedArguments);
                  sendSnippetHitEvent("skip-video " + formattedArguments);
                  seenMap.add(node);
                  lastSkippedVideoDuration = video.duration;
                }
                video.paused && video.play();
                skippedOnce = true;
                win();
              }
            }
          }
          if (!foundValidVideo && retryCounter < maxAttemptsNum) {
            setTimeout(() => {
              const attempt = retryCounter + 1;
              debugLog("info",
                       "Running video skipping logic. Attempt: ",
                       attempt);
              callback(attempt);
            }, retryMsNum);
          }
        });
      };
      const mo = new MutationObserver$1(callback);
      const win = raceWinner(
        "skip-video",
        () => mo.disconnect()
      );
      mo.observe(
        document, {characterData: true, childList: true, subtree: true});
      callback();
      end();
    };

    waitUntilEvent(debugLog, mainLogic, waitUntil);
  }

  const snippets$1 = {
    "debug": setDebug,
    "hide-if-contains": hideIfContains,
    "hide-if-contains-and-matches-style": hideIfContainsAndMatchesStyle,
    "hide-if-contains-image": hideIfContainsImage,
    "hide-if-contains-similar-text": hideIfContainsSimilarText,
    "hide-if-contains-visible-text": hideIfContainsVisibleText,
    "hide-if-has-and-matches-style": hideIfHasAndMatchesStyle,
    "hide-if-labelled-by": hideIfLabelledBy,
    "hide-if-matches-computed-xpath": hideIfMatchesComputedXPath,
    "hide-if-matches-xpath": hideIfMatchesXPath,
    "hide-if-svg-contains": hideIfSvgContains,
    log,
    "log-if-anchor-href-matches": logIfAnchorHrefMatches,
    "log-if-computed-style-matches": logIfComputedStyleMatches,
    "log-if-element-loads": logIfElementLoads,
    "log-if-iframe-loads": logIfIframeLoads,
    "log-if-inline-script-contains-fingerprint":
      logIfInlineScriptContainsFingerprint,
    "log-if-script-loads": logIfScriptLoads,
    "log-if-selector-exists": logIfSelectorExists,
    "profile": setProfile,
    race,
    "simulate-mouse-event": simulateMouseEvent,
    "skip-video": skipVideo
  };

  const snippets = {
    ...snippets$2,
    ...snippets$1
  };
  let context;
  for (const [name, ...args] of filters) {
    if (snippets.hasOwnProperty(name)) {
      try { context = snippets[name].apply(context, args); }
      catch (error) { console.error(error); }
    }
  }
  context = void 0;
}