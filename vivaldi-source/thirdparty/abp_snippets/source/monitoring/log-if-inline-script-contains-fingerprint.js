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
import {$$} from "../utils/dom.js";
import {call} from "proxy-pants/function";
import {getDebugger} from "../introspection/log.js";
import {profile} from "../introspection/profile.js";
import {raceWinner} from "../introspection/race.js";
import {formatArguments, sendDetectionEvent} from "../utils/general.js";

let {Error, MutationObserver, Set} = $(window);

// Captured before page JS can poison Element.prototype.
// In the bundle, this runs inside the callback when the extension
// content script loads (document_start), before any page JS executes.
const elementQSA = Element.prototype.querySelectorAll;

const inlineFingerprintHandlers = new Set();
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
    sharedInlineMo = new MutationObserver(records => {
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

/**
 * @description Detects when an inline `<script>` element (no src attribute)
 * contains a fingerprint substring, then fires a telemetry event via
 * env.sendDetectionEvent. Uses literal `String.includes()` matching
 * (never regex) so any character is safe in the pattern.
 * @memberof module:snippets/monitoring
 *
 * @param {string} textPattern Fingerprint substring to match against
 *   inline script textContent. Any characters accepted (matched
 *   literally via `String.includes()`).
 *   Must be at least 8 characters to prevent overly broad matches.
 * @param {string} type Detection type (e.g. "freestar-recovered").
 * @param {string} [specifier] Optional override for the event `specifier`
 *   field. When omitted, the first 5 characters of textPattern are used.
 *
 * @example
 * example.com#$#log-if-inline-script-contains-fingerprint
 *   aHR0cHM6Ly9mcmVl freestar
 *
 * @since Adblock Plus 4.28.0
 */
export function logIfInlineScriptContainsFingerprint(
  textPattern, type, specifier = null
) {
  if (!textPattern) {
    throw new Error(
      "[log-if-inline-script-contains-fingerprint snippet]: " +
      "Missing text pattern."
    );
  }
  if (!type) {
    throw new Error(
      "[log-if-inline-script-contains-fingerprint snippet]: " +
      "Missing type."
    );
  }

  // Minimum 8 chars prevents overly broad matches (e.g. "a"
  // would match nearly every inline script). No character
  // restriction: matching is always literal via
  // String.includes(), so regex injection is not possible.
  if (textPattern.length < 8)
    return;

  const formattedArguments = formatArguments(arguments);
  const debugLog =
    getDebugger("log-if-inline-script-contains-fingerprint");
  const {mark, end} =
    profile("log-if-inline-script-contains-fingerprint");

  const spec =
    specifier !== null ? specifier : textPattern.slice(0, 5);

  // Declared before win so the raceWinner closure can reference it.
  let callback;
  const win = raceWinner(
    "log-if-inline-script-contains-fingerprint",
    () => {
      removeInlineHandler(callback);
    }
  );

  // Receives the shared inlineScriptsBuffer from the MO (or null for the
  // initial sync scan).
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

  // Sync check after handler setup so nothing is missed
  // between scan and observe()
  callback(null);
  if (matched)
    return;

  addInlineHandler(callback);
}
