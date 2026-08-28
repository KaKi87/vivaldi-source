# @eyeo/snippets

This module goal is to provide a pre-bundled, pre-tested, and pre-optimized version of all *snippets* used in various mobile, or desktop, Ads blockers.


### Snippets in a nutshell

Each snippet is represented as a filter text, defined through `#$#` syntax, followed by the snippet *name* and zero, one, or more arguments separated by a space or, when spaces are part of the argument, included in quotes.

```
! logs 1 2 3 in the console
evilsite.com#$#log 1 2 3
```

Multiple snippets can be executed per single filter by using a semicolon `;` in between calls.

```
! logs 1 2 then 3 4 5 in the console
evilsite.com#$#log 1 2; log 3 4 5
```

#### The two kind of snippets

Snippets can run either as [content script](https://developer.chrome.com/docs/extensions/mv3/content_scripts/#isolated_world), which is a separate environment that replicates the *DOM* but never interferes with the *JS* that is running on the user page (and vice-versa), or as [injected script](https://developer.chrome.com/docs/extensions/mv3/content_scripts/#programmatic) that runs on the page among 3rd party and regular site's *JS* code.

The former kind is called **isolated**, because it runs sandboxed through the Web extension API, while the latter kind is called **main**, or **injected**, because it requires evaluation within the rest of the code that runs in the page, hence it's literally injected into such page.

Some snippet might be just an utility and, as such, be available in both the *main* and the *isolated* worlds. The way this snippet gets executed, and its possible side effects, remain confined within the isolated or main "*world*" it runs into.


## How to use this module

This module currently provides the following exports:

  * `@eyeo/snippets` which includes both `isolated` and `main`, aliased also as `injected`, export names
  * `@eyeo/snippets/isolated` which includes only the `isolated` export
  * `@eyeo/snippets/main`, aliased as `@eyeo/snippets/injected` too, which includes only the `main` export
  * `@eyeo/all` which exports all snippets, as single callback for those environments where the *isolated* world is *not supported*, but standard *ESM* is used

Accordingly to the project needs, this module can be used in various ways.

## How to reach our files directly after releases

Please always check [our releases page](https://gitlab.com/eyeo/snippets/-/releases) and feel free to use URLs that point at our released branches, via `https://gitlab.com/eyeo/snippets/-/raw/RELEASE_VERSION/dist/YOUR_FILE_HERE.js`, where `RELEASE_VERSION` can be `v0.5.0` or others, and `YOUR_FILE_HERE` is:

  * for *Chromium* based products, we have an *ISOLATED* world first artifact, reachable through this module's [isolated-first.jst](./dist/isolated-first.jst) file
  * for *WebKit* based products, we a *MAIN* it all artifact, reachable through this module's [snippets.js](./dist/snippets.js) file

Both `main.js` and `isolated.js` are also there for possible testing purpose, containing the same code other files have.

Their source code is the exact same available in the [webext folder](./webext/) and its source files.

## XPath 3.1 Support

With the release of snippets module version 0.8.0, we introduced a new snippet called `hide-if-matches-xpath3` that allows using XPath 3.1 functions to target ads. As XPath 3.1 is not supported natively by the browsers, the snippet requires injecting a dependency into the isolated context to work properly. This dependency is automatically injected using existing mechanisms for web extensions, if you are using [eyeo's Web Extension Ad Blocking Toolkit](https://gitlab.com/eyeo/adblockplus/abc/webext-sdk). For Chromium based products, the dependency injection logic will be supported by [eyeo Browser Ad-Filtering Solution ](https://gitlab.com/eyeo/adblockplus/chromium-sdk).

As a Chromium-based product, if you are using the `@eyeo/snippets` module as a standalone library you would need to actively opt-in to be able to use the `hide-if-matches-xpath3` snippet. You can keep using the `isolated-first.jst` artifact without any changes to your existing build steps. `isolated-first.jst` does not include `hide-if-matches-xpath3` snippet but it will keep being supported. If you want to include the `hide-if-matches-xpath3` snippet you would need to do two steps. First you would need to use [isolated-first-all.jst](./dist/isolated-first-all.jst) artifact instead. Secondly you would need to implement the dependency injection logic so that the required dependencies are included before the snippet runs. You can do this by injecting the [dependencies.jst](./dist/dependencies.jst) artifact into the page and calling the dependency function: `hideIfMatchesXPath3Dependency()`. The snippet should work as expected after that. 

### Used Third-Party Libraries

* [fontoxpath](https://github.com/FontoXML/fontoxpath) - [License](https://github.com/FontoXML/fontoxpath/blob/master/LICENSE.md)
* [prsc.js](https://github.com/bwrrp/prsc.js) - [License](https://github.com/bwrrp/prsc.js/blob/main/LICENSE)
* [xspattern.js](https://github.com/bwrrp/xspattern.js) - [License](https://github.com/bwrrp/xspattern.js/blob/main/LICENSE)
* [whynot.js](https://github.com/bwrrp/whynot.js) - [License](https://github.com/bwrrp/whynot.js/blob/main/LICENSE)

## Module Structure

The [source folder](./source) contains all the files used to create artifacts in both [dist folder](./dist) and [webext folder](./webext).

The former contains files used by 3rd party, non Web Extension related, projects, while the *webext* folder contains both optimized and source version of the files, so that it is possible to eventually offer a *debugging* mode instead of a *production* one.


### Architectural Overview

This module is used by both Web Extensions and 3rd party partners and, even if the source code is always the same, these are different targets.

This section would like to explain, on a high level, the difference among these folders.

#### Webext

This folder contains all exports defined as *ECMAScript* standard module (aka *ESM*) within the `package.json` file.

Both *main* and *isolated* exports are *pure functions* that can get passed to the Manifest v3 [scripting API](https://developer.chrome.com/docs/extensions/reference/scripting/).

These functions have been augmented like a read-only *Map* via a `.has(snippetName)` and `.get(snipetName)` where:

  * `.has(...)` returns *true* if the snippets is known for that specific world (either *MAIN* or *ISOLATED*)
  * `.get(...)` returns either *null* or a *callback* that is used as *dependency* for that very specific script

This approach gives us the ability to execute the minimum amount of code whenever such snippet is not needed for that specific page, as dependencies can indeed be quite big (in this case a stripped version of the TensorFlow Library + a binary model).

These callbacks are usually not used, nor needed, by other 3rd party snippets' consumers.

#### Dist

This folder contains files that can be *embedded* directly as part of any *HTML* page and executed right away through surrounding text:

```js
// example of distFileContent source code
// distFileContent = readFileSync("dist/snippets.js")
// distFileContent = readFileSync("dist/isolated-first.jst")
page.evaluate(`(${distFileContent})(...${JSON.stringify([env, ...snippets])})`);
```

In the `snippets.js` case we have all snippets except the *ML* one embedded into a single callback, and all snippets run in the *MAIN* world.

The `isolated-first.jst` file instead executes right away any *ISOLATED* snippet and, if there's anything in the *MAIN* world to execute too, it contains itself some logic to append a script on the user page that runs *MAIN* world snippets right after, still passing along the very same arguments it received through `[env, ...snippets]` serialization.

The `.jst` extension means *JS Template* and it's been chosen because otherwise some toolchain might try to automatically minify such file and, because its content is not referenced anyway as that's just a callback that's not even executed right away, the minifier could decide to entirely drop its content.

We came up with a template extension to avoid minifiers embedded in toolchains (i.e. Android) to actually break our code once injected.

Other files in the *dist* folder offer, like it is for the *webext* one, full source code alternatives or any other possible alternative as just *isolated.js* or *main.js* could be.

**Plase note** these files in the *dist* folder are not exported as module *but* are used through release tags as explained in the previous session.


## About Releases

We try to follow official guidance from [GitLab](https://docs.gitlab.com/ee/user/project/releases/) through the following steps:

  1. be sure the internal source code and repository points at the release code we'd like to ship as module
  2. from `main`, run the release preparation script:
     ```
     ./prepare-release.sh [major|minor|patch] [path-to-abp-snippets/source]
     ```
     The script automates the following steps (and pauses at step 5 for manual intervention):
     - creates branch `vX.X.X`
     - bumps the version in `package.json` without creating a tag
     - imports source code files from the provided `abp-snippets/source` path (optional; can be run manually afterwards with `npm run build.assets <path>`)
     - **pauses** so you can make any manual changes if needed, such as:
       * registering new snippets in `bundle/main.js` (main context) or `bundle/isolated.js` (isolated context)
       * updating files outside of `source`
     - runs `npm run build` to create the artifacts
     - commits everything with message **@eyeo/snippets vX.X.X**
     - pushes the branch with `git push --set-upstream origin vX.X.X`
  3. when reviewing the MR, ignore `dist`, `webext`, and `source` — these will always differ
  4. in GitLab create a descriptive release MR using the release template
  5. once reviewed and approved, merge the release branch into main squashing commits **without deleting the branch**
  6. after merge, trigger manually the release job in the pipeline: it will take care of publishing the tags and create the gitLab release
  7. if the publishing command fails in the CI: checkout `main`, pull and if you are sure you have access rights and a *2FA* protected account run:
      * `npm login`
      * `npm publish`
