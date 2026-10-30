#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"
# Run this in a terminal with GitHub network access. Never creates a public repo.
owner=$(gh api user --jq .login)
repository="$owner/godfield-rl"
if ! test -d .git; then
    git init -b main
fi
if git remote get-url origin >/dev/null 2>&1; then
    echo "origin is already configured; refusing to overwrite it." >&2
    exit 1
fi
git add CMakeLists.txt .gitignore README.md include src tests python scripts
if ! git diff --cached --quiet; then
    git commit -m "Add C++20 duel environment and batched RL interface"
fi
gh repo create "$repository" --private --source=. --remote=origin --push
visibility=$(gh repo view "$repository" --json visibility --jq .visibility)
test "$visibility" = PRIVATE
gh repo view "$repository" --json url,visibility
