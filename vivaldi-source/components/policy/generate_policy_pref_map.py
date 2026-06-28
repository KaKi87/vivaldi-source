# Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

"""
generate_policy_pref_map.py
============================
Generates a JSON mapping of Chromium policy names to pref paths and value types.

Sources
-------
1. Generated policy_templates.json file (e.g. out/Debug/gen/chrome/app/policy/policy_templates.json)
2. Test data pref mappings files (e.g. components/policy/test/data/pref_mapping/<PolicyName>.json)
   (JSONC — C-style comments are stripped before parsing)

Skipped policies:
 - deprecated: true in YAML (includes Vivaldi removed policies, set by vivaldi_generate_policy_templates.py)
 - Pref mapping contains no pref mapping

Policies that cannot be handled automatically must be listed in one of:
 - STATIC_PREF_MAP - pref mapping cannot be derived automatically, also Vivaldi policies
 - KNOWN_NO_PREF_POLICIES - policy is known to have no associated pref
 - PRIMARY_PREF_OVERRIDE - pref mapping references multiple prefs

Consult chrome/browser/policy/configuration_policy_handler_list_factory.cc
when adding entries to any of these.

Usage
-----
    python3 generate_policy_pref_map.py \\
        --policy-templates-file out/Debug/gen/chrome/app/policy/policy_templates.json \\
        --pref-mappings-dir chromium/components/policy/test/data/pref_mapping \\
        --output policy_prefs.json [--verbose]
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Optional

# Policies with no test pref mapping — pref paths must be specified manually.
STATIC_PREF_MAP: dict = {
  "NTPShortcuts": "enterprise_shortcuts.policy_list",
  # components/site_isolation
  "IsolateOrigins": "site_isolation.isolate_origins",
  "SitePerProcess": "site_isolation.site_per_process",
  # All DefaultSearchProvider* policies are funnelled through
  # DefaultSearchPolicyHandler into a single dict pref.
  "DefaultSearchProviderEnabled": "default_search_provider.enabled",
  # components/search_engines/enterprise/
  "SiteSearchSettings": "site_search_settings.policy_site_search_settings",
  "EnterpriseSearchAggregatorSettings": "enterprise_search_aggregator.policy_settings",
  # Put Vivaldi policy -> pref mapping here:
}

# Policies known to have no pref mapping.
KNOWN_NO_PREF_POLICIES: frozenset = frozenset(
  {
    # DefaultSearchProvider policies are read directly
    # by components/search_engines/enterprise/default_search_policy_handler.cc
    "DefaultSearchProviderName",
    "DefaultSearchProviderKeyword",
    "DefaultSearchProviderSearchURL",
    "DefaultSearchProviderSuggestURL",
    "DefaultSearchProviderInstantURL",
    "DefaultSearchProviderIconURL",
    "DefaultSearchProviderEncodings",
    "DefaultSearchProviderAlternateURLs",
    "DefaultSearchProviderImageURL",
    "DefaultSearchProviderNewTabURL",
    "DefaultSearchProviderSearchURLPostParams",
    "DefaultSearchProviderSuggestURLPostParams",
    "DefaultSearchProviderImageURLPostParams",
    "DefaultSearchProviderContextMenuAccessAllowed",
    # Policy-system meta-policies (read directly by policy stack, no pref)
    "PolicyListMultipleSourceMergeList",
    "PolicyDictionaryMultipleSourceMergeList",
    "PolicyAtomicGroupsEnabled",
    "MaxInvalidationFetchDelay",
    "EnableExperimentalPolicies",
    # RemoteAccessHost policies are read by the remote access component directly
    "RemoteAccessHostAllowClientPairing",
    "RemoteAccessHostAllowFileTransfer",
    "RemoteAccessHostAllowGnubbyAuth",
    "RemoteAccessHostAllowPinAuthentication",
    "RemoteAccessHostAllowRelayedConnection",
    "RemoteAccessHostAllowRemoteAccessConnections",
    "RemoteAccessHostAllowRemoteSupportConnections",
    "RemoteAccessHostAllowUiAccessForRemoteAssistance",
    "RemoteAccessHostAllowUrlForwarding",
    "RemoteAccessHostClientDomainList",
    "RemoteAccessHostClipboardSizeBytes",
    "RemoteAccessHostDomainList",
    "RemoteAccessHostEnableUserInterface",
    "RemoteAccessHostFirewallTraversal",
    "RemoteAccessHostMatchUsername",
    "RemoteAccessHostMaximumSessionDurationMinutes",
    "RemoteAccessHostRequireCurtain",
    "RemoteAccessHostUdpPortRange",
    # Not mapping to a pref
    "AudioProcessHighPriorityEnabled",
    "AudioSandboxEnabled",
    "UserDataDir",
    "ReportExtensionsAndPluginsData",
    "ReportMachineIDData",
    "ReportPolicyData",
    "ReportUserIDData",
    "ReportVersionData",
    # This policy affects multiple separate prefs
    # TODO: We need a way to expose multiple prefs for one policy
    "SyncTypesListDisabled",
    "WatermarkStyle",
    "SafeBrowsingProtectionLevel",
    "HttpsOnlyMode",
    # This is already mapped by Vivaldi as kMemorySaverEnabled
    # (also type mismatch, bool/int - legacy issue).
    "HighEfficiencyModeEnabled",
  }
)

# Test pref mappings may reference multiple prefs; the correct one must be specified manually.
# Do not put the policies that map to multiple prefs here, put in KNOWN_NO_PREF_POLICIES instead.
PRIMARY_PREF_OVERRIDE: dict = {
  "AllowFileSelectionDialogs": "select_file_dialogs.allowed",
  "BatterySaverModeAvailability": "performance_tuning.battery_saver_mode.state",
  "BrowserSignin": "signin.allowed_on_next_startup",
  "BrowsingDataLifetime": "browser.clear_data.browsing_data_lifetime",
  "ClearBrowsingDataOnExitList": "browser.clear_data.clear_on_exit",
  "DefaultDownloadDirectory": "savefile.default_directory",
  "DownloadDirectory": "download.default_directory",
  "EnterpriseLogoUrl": "enterprise_logo.url.for_profile",
  "ExtensibleEnterpriseSSOBlocklist": "extensible_enterprise_sso.enabled",
  "SaasUsageReportingDomainUrlsForBrowsers": "enterprise_reporting.saas_usage.domain_urls_for_browser",
  "SaasUsageReportingDomainUrlsForProfiles": "enterprise_reporting.saas_usage.domain_urls_for_profile",
  "SpellcheckLanguage": "browser.enable_spellchecking",
  "WebAppInstallByUserEnabled": "profile.web_app.install_by_user_enabled",
  "ManagedBookmarks": "bookmarks.managed_bookmarks",
  "ProfileSeparationSettings": "profile_separation.settings",
  "BookmarkBarEnabled": "bookmark_bar.show_on_all_tabs",
}

# policy_templates.json may use schema: $ref instead of a known type; resolve it from the root schema.
_TOPLEVEL_TYPE_MAP: dict = {
  "main": "boolean",
  "int": "integer",
  "string": "string",
  "int-enum": "integer",
  "string-enum": "string",
  "string-enum-list": "list",
  "list": "list",
  "dict": "dictionary",
}

_SCHEMA_TYPE_MAP: dict = {
  "boolean": "boolean",
  "integer": "integer",
  "number": "number",
  "string": "string",
  "array": "list",
  "object": "dictionary",
}

# Strip '//' comments and parse JSON.
def _load_jsonc(text: str):
  result = []
  in_string = False
  i = 0
  while i < len(text):
    c = text[i]
    if in_string:
      if c == "\\":
        result.append(c)
        result.append(text[i + 1])
        i += 2
        continue
      if c == '"':
        in_string = False
      result.append(c)
    else:
      if c == '"':
        in_string = True
        result.append(c)
      elif c == "/" and i + 1 < len(text) and text[i + 1] == "/":
        while i < len(text) and text[i] != "\n":
          i += 1
        continue
      else:
        result.append(c)
    i += 1
  return json.loads("".join(result))


def parse_pref_mapping(data: list) -> tuple:
  entries = []
  for block in data:
    if not isinstance(block, dict):
      continue

    # simple_policy_pref_mapping_test → single pref_name with test values
    simple = block.get("simple_policy_pref_mapping_test")
    if isinstance(simple, dict) and simple.get("pref_name"):
      entries.append({"pref_name": simple["pref_name"]})

    # policy_pref_mapping_tests → prefs dict keys are pref names
    for test in block.get("policy_pref_mapping_tests", []):
      for pref_name in test.get("prefs", {}).keys():
        entries.append({"pref_name": pref_name})

  return entries


# Returns a tuple of:
# * a pref path for policy or None
# * boolean True, when multiple prefs
#   were found (caller should mark it as a warning).
def _resolve_pref(policy_name: str, entries: list) -> tuple:
  if not entries:
    return None, False

  distinct = list(dict.fromkeys(e["pref_name"] for e in entries))

  if policy_name in PRIMARY_PREF_OVERRIDE:
    target = PRIMARY_PREF_OVERRIDE[policy_name]
    if target in distinct:
      return target, False

  return distinct[0], len(distinct) > 1


def _resolve_type(policy: dict) -> Optional[str]:
  schema = policy.get("schema", {})
  schema_type = schema.get("type") if isinstance(schema, dict) else None
  if schema_type:
    return _SCHEMA_TYPE_MAP.get(schema_type)
  return _TOPLEVEL_TYPE_MAP.get(policy.get("type", ""))


def generate(templates_file: Path, pref_mappings_dir: Path,
             verbose: bool = False) -> dict:

  def log(msg: str, warning: bool = False) -> None:
    if warning or verbose:
      print(msg, file=sys.stderr)

  raw = json.loads(templates_file.read_text(encoding="utf-8"))
  policies = [p for p in raw["policy_definitions"] if p.get("type") != "group"]
  log(f"[policy_templates] {len(policies)} policies loaded")

  output: dict = {}
  skipped = 0
  warnings = 0

  for policy in policies:
    policy_name = policy["name"]

    if policy.get("deprecated"):
      log(f"  SKIP [deprecated]: {policy_name}")
      skipped += 1
      continue

    if policy_name in KNOWN_NO_PREF_POLICIES:
      log(f"  SKIP [no-pref, known]: {policy_name}")
      skipped += 1
      continue

    value_type = _resolve_type(policy)
    if not value_type:
      log(f"  [no-type]: {policy_name}", True)
      warnings += 1
      continue

    if policy_name in STATIC_PREF_MAP:
      pref_path = STATIC_PREF_MAP[policy_name]
    else:
      pref_mapping_list = []
      pref_mapping_file = pref_mappings_dir / f"{policy_name}.json"
      if pref_mapping_file.exists():
        try:
          pref_mapping_list = parse_pref_mapping(
            _load_jsonc(pref_mapping_file.read_text(encoding="utf-8"))
          )
        except Exception as exc:
          log(f"  [WARN] {pref_mapping_file.name}: {exc}", True)

      pref_path, multi_warn = _resolve_pref(policy_name, pref_mapping_list)
      if multi_warn:
        log(
          f"  [multi-pref]: {policy_name} — "
          f"{list(dict.fromkeys(e['pref_name'] for e in pref_mapping_list))}",
          True,
        )
        warnings += 1
        continue

    if pref_path is None:
      log(f"  [no-pref-path]: {policy_name}", True)
      warnings += 1
      continue

    output[f"kPolicy{policy_name}"] = {
      "path": pref_path,
      "type": value_type,
    }

  _YELLOW = "\033[33m"
  _RESET = "\033[0m"
  warn_str = f", {_YELLOW}{warnings} warnings (see logs){_RESET}" if warnings else ""
  print(
    f"[result] {len(output)} policies written, {skipped} skipped{warn_str}")
  return {"policies": output}


def main() -> None:
  parser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter,
  )
  parser.add_argument(
      "--policy-templates-file", metavar="FILE", required=True,
    )
  parser.add_argument(
      "--pref-mappings-dir", metavar="DIR", required=True,
    )
  parser.add_argument(
    "--output",
    metavar="FILE",
    required=True,
    help="Output JSON file path.",
  )
  parser.add_argument(
    "--verbose",
    "-v",
    action="store_true",
    help="Print per-policy skip reasons to stderr.",
  )
  args = parser.parse_args()

  templates_file = Path(args.policy_templates_file).resolve()
  pref_mappings_dir = Path(args.pref_mappings_dir).resolve()

  if not templates_file.is_file():
    sys.exit(f"ERROR: policy_templates.json not found: {templates_file}")
  if not pref_mappings_dir.is_dir():
    sys.exit(f"ERROR: pref_mappings directory not found: {pref_mappings_dir}")

  result = generate(
      templates_file,
      pref_mappings_dir,
      args.verbose,
  )

  with open(args.output, "w", encoding="utf-8") as fh:
    json.dump(result, fh, indent=2, ensure_ascii=False)
    fh.write("\n")


if __name__ == "__main__":
  main()
