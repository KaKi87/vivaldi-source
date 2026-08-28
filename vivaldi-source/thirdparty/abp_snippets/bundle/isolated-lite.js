/**
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

import {logIfAnchorHrefMatches} from
  "../source/monitoring/log-if-anchor-href-matches.js";
import {logIfElementLoads} from
  "../source/monitoring/log-if-element-loads.js";
import {logIfSelectorExists} from
  "../source/monitoring/log-if-selector-exists.js";
import {logIfScriptLoads} from
  "../source/monitoring/log-if-script-loads.js";
import {logIfComputedStyleMatches} from
  "../source/monitoring/log-if-computed-style-matches.js";
import {logIfIframeLoads} from
  "../source/monitoring/log-if-iframe-loads.js";
import {logIfInlineScriptContainsFingerprint} from
  "../source/monitoring/log-if-inline-script-contains-fingerprint.js";
import {hideIfContains} from
  "../source/conditional-hiding/hide-if-contains.js";
import {hideIfContainsAndMatchesStyle} from
  "../source/conditional-hiding/hide-if-contains-and-matches-style.js";
import {hideIfContainsImage} from
  "../source/conditional-hiding/hide-if-contains-image.js";
import {hideIfSvgContains} from
  "../source/conditional-hiding/hide-if-svg-contains.js";
import {hideIfContainsSimilarText} from
  "../source/conditional-hiding/hide-if-contains-similar-text.js";
import {hideIfContainsVisibleText} from
  "../source/conditional-hiding/hide-if-contains-visible-text.js";
import {hideIfHasAndMatchesStyle} from
  "../source/conditional-hiding/hide-if-has-and-matches-style.js";
import {hideIfLabelledBy} from
  "../source/conditional-hiding/hide-if-labelled-by.js";
import {hideIfMatchesXPath} from
  "../source/conditional-hiding/hide-if-matches-xpath.js";
import {hideIfMatchesComputedXPath} from
  "../source/conditional-hiding/hide-if-matches-computed-xpath.js";
import {log} from "../source/introspection/log.js";
import {race} from "../source/introspection/race.js";
import {setDebug} from "../source/introspection/debug.js";
import {setProfile} from "../source/introspection/profile.js";
import {simulateMouseEvent} from
  "../source/behavioral/simulate-mouse-event.js";
import {skipVideo} from "../source/behavioral/skip-video.js";

export const snippets = {
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

