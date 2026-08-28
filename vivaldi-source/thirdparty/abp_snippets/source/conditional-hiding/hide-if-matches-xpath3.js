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

import {formatArguments, sendSnippetHitEvent} from "../utils/general.js";
import {hideElement} from "../utils/dom.js";
import {profile} from "../introspection/profile.js";
import {raceWinner} from "../introspection/race.js";
import {getDebugger} from "../introspection/log.js";

let {MutationObserver} = $(window);
const hitFilters = new Set();

const {ELEMENT_NODE} = Node;
/* global fontoxpath */

/**
 * @description Hide a specific element through a XPath 3.1 query string.
 * See {@tutorial xpath-filters} to know more.
 * @memberof module:snippets/conditional-hiding
 *
 * @param {string} query The XPath query that targets the element to hide.
 * @param {string} scopeQuery CSS or XPath selector that the filter devs can
 * use to restrict the scope of the MO.
 * It is important that the selector is as specific as possible to avoid to
 * match too many nodes.
 * @example
 * hide-if-matches-xpath3 '//{@literal *}[lower-case(text())="sponsored"]' =>
 * Hides any element that contains the text "sponsored" in its text content,
 * regardless of the element type or the case (upper or lower) of
 * the text (e.g.: "sPoNSoReD").
 *
 * @see {@link https://eyeo.atlassian.net/wiki/spaces/CV/pages/69960763/hide-if-matches-xpath3} for internal documentation.
 * @see {@link https://developers.eyeo.com/snippets/conditional-hiding-snippets/hide-if-matches-xpath3} for external documentation.
 * @since Adblock Plus 3.19
 */
export function hideIfMatchesXPath3(query, scopeQuery) {
  let {mark, end} = profile("hide-if-matches-xpath3");

  const namespaceResolver = prefix => {
    switch (prefix) {
      case "": return "http://www.w3.org/1999/xhtml";
      default: return false;
    }
  };
  function queryNodes(nodeQuery) {
    return fontoxpath.evaluateXPathToNodes(nodeQuery, document, null, null, {
      language: fontoxpath.evaluateXPath.XQUERY_3_1_LANGUAGE,
      namespaceResolver
    });
  }

  const formattedArguments = formatArguments(arguments);
  let debugLog = getDebugger("hide-if-matches-xpath3");

  const startHidingMutationObserver = scopeNode => {
    const seenMap = new WeakSet();
    const callback = () => {
      mark();

      const nodes = queryNodes(query);
      for (const node of $(nodes)) {
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
                 "\nFILTER: hide-if-matches-xpath3",
                 formattedArguments);
        const filter =
          "hide-if-matches-xpath3 " +
          formattedArguments;
        if (!hitFilters.has(filter)) {
          hitFilters.add(filter);
          sendSnippetHitEvent(filter);
        }
      }
      end();
    };

    const mo = new MutationObserver(callback);
    const win = raceWinner(
      "hide-if-matches-xpath3",
      () => mo.disconnect()
    );
    mo.observe(
      scopeNode, {characterData: true, childList: true, subtree: true});
    callback();
  };

  if (scopeQuery) {
    // This is a performance optimization: we only observe DOM subtrees
    // instead of the whole document, if a scope query is given.
    let count = 0;
    let scopeMutationObserver;
    const scopeNodes = queryNodes(scopeQuery);
    const findMutationScopeNodes = () => {
      for (const scopeNode of $(scopeNodes)) {
        // Start a Mutation Observer for each found node
        startHidingMutationObserver(scopeNode);
        count++;
      }
      if (count > 0)
        scopeMutationObserver.disconnect();
    };

    scopeMutationObserver = new MutationObserver(findMutationScopeNodes);
    scopeMutationObserver.observe(
      document, {characterData: true, childList: true, subtree: true}
    );
    findMutationScopeNodes();
  }
  else {
    // If no scope query has been specified, observe the document
    startHidingMutationObserver(document);
  }
}
