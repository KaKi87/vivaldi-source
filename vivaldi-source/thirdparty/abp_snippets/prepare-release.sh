#!/usr/bin/env bash
# Automates the release branch creation:
# creates a release branch, bumps the version in package.json, 
# imports assets, builds artifacts, and pushes the release branch
# Usage: ./prepare-release.sh <major|minor|patch> [source-path]
# Example: ./prepare-release.sh minor ~/gitlab/abp-snippets/source

set -euo pipefail

# ── helpers ──────────────────────────────────────────────────────────────────

abort() { echo "ERROR: $*" >&2; exit 1; }

# ── argument validation ───────────────────────────────────────────────────────

BUMP="${1-}"
SOURCE_PATH="${2-}"

[[ "$BUMP" =~ ^(major|minor|patch)$ ]] \
  || abort "First argument must be major, minor, or patch. Got: '$BUMP'"

# ── sanity checks ─────────────────────────────────────────────────────────────

CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
[[ "$CURRENT_BRANCH" == "main" ]] \
  || abort "Must be on 'main' branch. Currently on: '$CURRENT_BRANCH'"

# Warn if working tree is dirty (uncommitted changes)
if ! git diff --quiet || ! git diff --cached --quiet; then
  echo "WARNING: Working tree has uncommitted changes."
  read -rp "Continue anyway? [y/N] " yn
  [[ "$yn" =~ ^[Yy]$ ]] || abort "Aborted."
fi

# ── step 2: compute next version and create branch ───────────────────────────

CURRENT_VERSION=$(node -p "require('./package.json').version")
NEXT_VERSION=$(node -e "
  const [major, minor, patch] = '$CURRENT_VERSION'.split('.').map(Number);
  const bump = '$BUMP';
  if (bump === 'major') console.log((major + 1) + '.0.0');
  else if (bump === 'minor') console.log(major + '.' + (minor + 1) + '.0');
  else console.log(major + '.' + minor + '.' + (patch + 1));
")
BRANCH="v${NEXT_VERSION}"

echo ""
echo "Current version : $CURRENT_VERSION"
echo "Next version    : $NEXT_VERSION  (branch: $BRANCH)"
echo ""
read -rp "Proceed with release $BRANCH? [y/N] " yn
[[ "$yn" =~ ^[Yy]$ ]] || abort "Aborted."

echo ""
echo "── Step 2: creating branch $BRANCH ──"
git checkout -b "$BRANCH"

# ── step 3: bump version in package.json ─────────────────────────────────────

echo "── Step 3: bumping $BUMP version ──"
npm version "$BUMP" --no-git-tag-version

# ── step 4: import source assets ─────────────────────────────────────────────

if [[ -n "$SOURCE_PATH" ]]; then
  echo "── Step 4: importing source assets from $SOURCE_PATH ──"
  npm run build.assets "$SOURCE_PATH"
else
  echo "── Step 4: SKIPPED (no source path provided) ──"
  echo "   Run manually: npm run build.assets <path-to-abp-snippets/source>"
fi

# ── step 5: manual changes ───────────────────────────────────────────────────

echo ""
echo "── Step 5: manual changes ──"
echo "If needed, make any of the following changes now:"
echo "  • Register new snippets in bundle/main.js (main context)"
echo "  • Register new snippets in bundle/isolated.js (isolated context)"
echo "  • Update any other files outside of source/"
echo ""
read -rp "Press Enter when done with manual changes (or to skip)..."

# ── step 6: build artifacts ───────────────────────────────────────────────────

echo "── Step 6: building artifacts ──"
npm run build

echo ""
echo "Artifacts built. Review dist/ and webext/ before committing."
read -rp "Press Enter to commit and push (or Ctrl+C to abort)..."

# ── step 7: commit everything ────────────────────────────────────────────────

echo "── Step 7: committing ──"
git add .
git commit -m "@eyeo/snippets v${NEXT_VERSION}"

# ── step 9: push branch ──────────────────────────────────────────────────────

echo "── Step 9: pushing branch $BRANCH ──"
git push --set-upstream origin "$BRANCH"

echo ""
echo "Release branch $BRANCH is ready."
echo "Next: open a GitLab MR using the release template (README step 4)."
