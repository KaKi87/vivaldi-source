# Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

"""
vivaldi_generate_policy_templates.py
=====================================
Wraps around Chromium chromium/components/policy/resources/policy_templates.py
and ads additional args, to generate a policy_templates.json, with all
Chromium + Vivaldi policies, excluding policies not supported by Vivaldi.
It adds additional arguments:
* --extra-sources - points to the chromium/components/policy/resources/templates/policy_definitions/
                    like structure.
* --policies-file - points to Vivaldi policies.yaml file
* --suppression-list - YAML list of policies not supported by Vivaldi


It imports the policy_templates.py script directly and patches needed functions,
then delages back to the original script.
"""

import argparse
import glob
import importlib.util
import os
import re
import sys


_CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
_CHROMIUM_SRC = os.path.abspath(
  os.path.join(_CURRENT_DIR, '..', '..', 'chromium'))

# We're sure pyyaml is there, so no need for chromeos checks.
sys.path.append(os.path.join(_CHROMIUM_SRC, 'third_party'))
import pyyaml

_MODULE_PATH = os.path.join(
  _CHROMIUM_SRC, 'components', 'policy', 'resources', 'policy_templates.py')

if not os.path.exists(_MODULE_PATH):
  print(f'ERROR: Could not find policy_templates.py at {_MODULE_PATH}',
        file=sys.stderr)
  sys.exit(1)

spec = importlib.util.spec_from_file_location('policy_templates', _MODULE_PATH)
_pt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(_pt)

# Save original handlers, those are needed by our patched versions.
# Keep it in global scope, because
# chromium/components/policy/resources/policy_templates.build.grd
# calls GetPolicyTemplates() directly.
_pt._original_GetMetadata = _pt._GetMetadata
_pt._original_GetPoliciesAndGroups = _pt._GetPoliciesAndGroups
_pt._original_GetPolicyTemplates = _pt.GetPolicyTemplates
_pt._original_WriteDepFile = _pt._WriteDepFile


def _GetMetadata(policies_file):
  result = _pt._original_GetMetadata()

  with open(policies_file, encoding='utf-8') as f:
    vivaldi_policies = pyyaml.safe_load(f)

  # Merge Vivaldi policies.yaml with chromium
  result['policies']['policies'].update(
      vivaldi_policies.get('policies') or {})
  result['policies']['atomic_groups'].update(
      vivaldi_policies.get('atomic_groups') or {})

  return result


def _GetPoliciesAndGroups(extra_sources):
  result = _pt._original_GetPoliciesAndGroups()

  for source_dir in extra_sources:
    for group_name in _pt._SafeListDir(source_dir):
      group_path = os.path.join(source_dir, group_name)
      if not os.path.isdir(group_path):
        continue

      is_existing_group = group_name in result
      has_group_details = os.path.exists(
          os.path.join(group_path, '.group.details.yaml'))

      # Existing chromium groups can be extended without .group.details.yaml.
      if not is_existing_group and not has_group_details:
        continue

      if not is_existing_group:
        result[group_name] = {'policies': {}, 'policy_atomic_groups': {}}

      for file in _pt._SafeListDir(group_path):
        filename = os.fsdecode(file)
        file_basename, file_extension = os.path.splitext(filename)
        file_path = os.path.join(group_path, filename)

        if file_extension != '.yaml':
          continue

        with open(file_path, encoding='utf-8') as f:
          data = pyyaml.safe_load(f)

        if file_basename == '.group.details':
          # Chromium's definition takes precedence for existing groups.
          if 'caption' not in result[group_name]:
            result[group_name].update(data)
        elif file_basename == 'policy_atomic_groups':
          result[group_name]['policy_atomic_groups'].update(data)
        else:
          result[group_name]['policies'][file_basename] = data

  return result


_DESKTOP_PLATFORM_RE = re.compile(r'^chrome\.')

def _is_desktop_platform(p):
  plat = re.sub(r':\S*$', '', p).strip().lower()
  return bool(_DESKTOP_PLATFORM_RE.match(plat))


def _is_desktop(policy):
  supported_on = policy.get('supported_on', [])
  future_on = policy.get('future_on', [])
  return any(_is_desktop_platform(p) for p in supported_on + future_on)


def GetPolicyTemplates(suppression_list_path=None):
  if suppression_list_path is None:
    suppression_list_path = os.path.join(_CURRENT_DIR, 'suppression_list.yaml')
  result = _pt._original_GetPolicyTemplates()

  with open(suppression_list_path, encoding='utf-8') as f:
    suppressed = pyyaml.safe_load(f)

  suppressed_set = set(suppressed or [])

  for policy in result[_pt.POLICY_DEFINITIONS_KEY]:
    if policy.get('type') == 'group':
      continue
    if policy.get('name') in suppressed_set or not _is_desktop(policy):
      policy['deprecated'] = True

  return result


def _WriteDepFile(extra_sources, dep_file, target, source_files):
  extra_files = sorted(set(
    f.replace('\\', '/')
    for source_dir in extra_sources
    for f in glob.glob(source_dir + '/**/*.yaml', recursive=True)
  ))
  _pt._original_WriteDepFile(dep_file, target, source_files + extra_files)


def main():
  parser = argparse.ArgumentParser(add_help=False)
  parser.add_argument('--extra-sources', dest='extra_sources',
                      action='append', required=True)
  parser.add_argument('--policies-file', dest='policies_file', required=True)
  parser.add_argument('--suppression-list', dest='suppression_list',
                      required=True)
  args, remaining_argv = parser.parse_known_args()

  _pt._GetMetadata = lambda: _GetMetadata(args.policies_file)
  _pt._GetPoliciesAndGroups = lambda: _GetPoliciesAndGroups(args.extra_sources)
  _pt.GetPolicyTemplates = lambda: GetPolicyTemplates(args.suppression_list)
  _pt._WriteDepFile = lambda dep_file, target, source_files: _WriteDepFile(
      args.extra_sources, dep_file, target, source_files)

  # Handle the rest, directly in policy_templates.py
  sys.argv = [sys.argv[0]] + remaining_argv
  _pt.main()


if __name__ == '__main__':
  main()
