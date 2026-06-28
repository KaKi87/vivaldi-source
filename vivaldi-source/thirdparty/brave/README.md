# Brave Query Filter (Upstream Mirror)

This repository contains a small, verbatim subset of upstream files from [brave-core](https://github.com/brave/brave-core), specifically the query filter component used by Vivaldi.

## What's here

| File | Purpose |
|------|---------|
| `components/query_filter/browser/utils.{cc,h}` | The query filter utilities (search & replace patterns, URL parsing) that power the **Brave Query Filter** feature — auto-removing tracking/ads query parameters from URLs |
| `components/query_filter/BUILD.gn` | Build target for integrating these files into Vivaldi's Chromium build (`//vivaldi/thirdparty/brave/components/query_filter`) |
| `download.sh` | Script to fetch & verify upstream sources |
| `license.txt` | Mozilla Public License v2.0 (upstream license) |

## Why this repo

The query filter files are kept **verbatim** from Brave's upstream — no modifications, no patches. This mirror makes it easy to:

- Track upstream changes via Git diffs
- Re-sync cleanly when Brave updates the component
- Integrate into the Vivaldi build without maintaining separate forks

## Usage

### Updating from upstream

```bash
./download.sh
```

The script:

1. Downloads `utils.cc` and `utils.h` from `brave-core/master/components/query_filter/browser/`
2. Verifies each file's SHA-256 hash against the hardcoded expected values
3. Copies verified files into `components/query_filter/browser/` (overwriting only if changed)

### When hashes change

If Brave updates upstream and your `download.sh` reports a hash mismatch:

1. Check [the Brave commit history](https://github.com/brave/brave-core/commits/master/components/query_filter/browser/) for recent changes
2. Update the expected hashes in `download.sh` (line 12–13, the `EXPECTED_FILES` array)
3. Commit the updated script + files together

```bash
# Example: update utils.cc hash
"utils.cc <new_sha256>"
```

## Build integration

The build is configured via `components/query_filter/BUILD.gn`:

- **`source_set("query_filter")`** — compiles `utils.cc` + `utils.h` as a `vivaldi/thirdparty` target with Brave's original include path (`//vivaldi/thirdparty`)
- **`config("query_filter_includes")`** — provides friendlier includes (`"components/query_filter/browser/utils.h"`) for downstream consumers

Consume from your Vivaldi BUILD.gn:

```gn
deps = [ "//vivaldi/thirdparty/brave/components/query_filter:query_filter" ]
```

## License

All files in this repository are derived from Brave Software, Inc. and distributed under the **Mozilla Public License v2.0** (MPL-2.0). See `license.txt` for the full text.

The upstream source is marked "Incompatible With Secondary Licenses" per Exhibit B of MPL-2.0.
