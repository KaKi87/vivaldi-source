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
import {hasOwnProperty} from "proxy-pants/object";
/**
 * A lightweight JSONPath processor for querying and
 * filtering JavaScript objects.
 * Supports dot notation, bracket notation, recursive descent,
 * and basic filter expressions.
 * @example
 * const engine = new JSONPath("$.store.books[?(@.price < 20)]");
 * const results = engine.evaluate(data);
 */
const {Error, Object, Array, parseFloat, isNaN} = $(window);
export class JSONPath {
  /**
   * Creates a new JSONPath instance and tokenizes the provided query string.
   * @param {string} query - The JSONPath query string (e.g. "$.items[*].name").
   */
  constructor(query) {
    if (typeof query !== "string")
      throw new Error("JSONPath: query must be a string");
    if (!query.length)
      throw new Error("JSONPath: query must be a non-empty string");
    this._steps = this._tokenize(query);
  }

  /**
   * Analyzes the query string and breaks it into executable steps.
   * Handles recursive descent (..), dot access (.), and bracket notation ([]).
   * @param {string} query - The raw query string.
   * @returns {Array<Object>} An array of step objects for the evaluator.
   */
  _tokenize(query) {
    query = $(query);
    const steps = new Array();
    let i = 0;

    if (query[0].toString() === "$")
      i = 1;

    while (i < query.length) {
      let isRecursive = false;

      // 1. Detect Recursive Modifier
      if (query.startsWith("..", i)) {
        isRecursive = true;
        i += 2;
      }
      else if (query[i].toString() === ".") {
        i++;
      }

      // 2. Determine the "Action" (Bracket or Dot Access)
      if (query[i].toString() === "[") {
        const end = query.indexOf("]", i);
        if (end === -1)
          throw new Error(`JSONPath: unclosed bracket in query "${query}"`);
        const inner = query.slice(i + 1, end);

        if (!inner.length)
          throw new Error(`JSONPath: empty bracket notation in query "${query}"`);

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
        // Standard Dot Access
        const nextBoundary = query.slice(i).search(/[.[]/);
        const key = nextBoundary === -1 ?
        query.slice(i).toString() : query.slice(i, i + nextBoundary).toString();

        if (!key && !isRecursive)
          throw new Error(`JSONPath: trailing dot with no property name in query "${query}"`);

        if (key || isRecursive) {
          // If query is "..*", key might be empty but isRecursive is true
          steps.push({type: "direct", key: key || "*",
                      recursive: isRecursive});
        }
        i += key.length;
      }
    }
    return steps;
  }
  /**
   * Parses the interior of a filter expression `[?(...)]`.
   * Extracts the property name, comparison operator, and target value.
   * @param {string} str - The string inside the filter brackets.
   * @returns {Object} An object containing {property, operator, target}.
   * @throws {Error} If the filter expression cannot be parsed.
   */
  _parseFilter(str) {
    str = $(str);
    const match = str.match(
      /(?:[@.]?)([\w]+(?:\.[\w]+)*)\s*([!=^$*]=|[<>]=?)\s*(?:['"](.+?)['"]|([\w.+-]+))\)/
    );
    if (!match)
      throw new Error(`JSONPath: invalid filter expression "${str}"`);
    return {
      property: match[1],
      operator: match[2],
      target: match[3] != null ? match[3] : match[4]
    };
  }

  /**
   * Evaluates the compiled query against a source object.
   * Returns a list of "target" objects containing the parent
   * and the specific key found.
   * @param {Object|Array} obj - The JSON-like structure to query.
   * @returns {Array<{parent: Object, key: string}>} A list of matches.
   */
  evaluate(obj) {
    if (!obj || typeof obj !== "object")
      throw new Error("JSONPath: evaluate() requires an object or array");
    // We start with a virtual root to ensure we can access top-level keys
    let targets = $([{parent: {root: obj}, key: "root"}]);
    for (const step of this._steps) {
      const nextTargets = [];
      for (const {parent, key} of targets) {
        const current = parent[key];
        if (!current || typeof current !== "object")
          continue;
        if (step.recursive)
          // Find the key anywhere inside current(all nested levels)
          this._deepSearch(current, step, nextTargets);
        else
          // Find the key only as a direct child
          this._match(current, step, nextTargets);
      }
      // Matches of this step are starting points of next step
      targets = nextTargets;
    }
    return targets;
  }

  /**
   * Attempts to match a specific step against the direct children of an object.
   * @param {Object|Array} obj - The current object being inspected.
   * @param {Object} step - The current query step (direct or filter).
   * @param {Array} out - The accumulator array for matches.
   */
  _match(obj, step, out) {
    const keys = (step.key === "*" || step.key === "?") ?
      Object.keys(obj) : [step.key];
    for (const k of keys) {
      if (hasOwnProperty(obj, k)) {
        if (step.key === "?" && !this._test(obj[k], step.filter))
          continue;
        out.push({parent: obj, key: k});
      }
    }
  }

  /**
   * Performs a depth-first search to find matches at
   * any level of the object hierarchy.
   * Triggered by the `..` operator.
   * @param {Object|Array} obj - The current object to search within.
   * @param {Object} step - The current query step to apply.
   * @param {Array} out - The accumulator array for matches.
   * @param {number} [depth=10000] - Maximum recursion depth.
   */
  _deepSearch(obj, step, out, depth = 10000) {
    this._match(obj, step, out);
    if (depth <= 0)
      return;
    for (const k of Object.keys(obj)) {
      if (obj[k] && typeof obj[k] === "object")
        this._deepSearch(obj[k], step, out, depth - 1);
    }
  }

  /**
   * Tests a specific value against a filter criteria.
   * Supports equality, inequality, numeric comparisons,
   * and string pattern matching.
   * @param {*} obj - The value/object to test.
   * @param {Object} filter - The filter criteria (property, operator, target).
   * @returns {boolean} True if the object matches the filter.
   */
  _test(obj, filter) {
    if (!filter || !obj)
      return false;

    // Walk dotted property path: "info.type" → obj.info.type
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

    // Numeric support for <, >, <=, >=
    const nValue = parseFloat(value);
    const nTarget = parseFloat(target);
    const isNumeric = !isNaN(nValue) && !isNaN(nTarget);

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
