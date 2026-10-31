#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"
# Run this in a terminal with GitHub network access. Never changes visibility.
fail() {
    echo "$1" >&2
    exit 1
}
owner=$(gh api user --jq .login)
case "$owner" in
    ''|*[!A-Za-z0-9-]*) fail "GitHub returned an invalid account name." ;;
esac
repository="$owner/godfield-rl"
if ! test -e .git; then
    git init -b main
fi
branch=$(git symbolic-ref --quiet --short HEAD) || fail "Detached HEAD; check out a branch before publishing."
test -n "$branch" || fail "No current branch; refusing to publish."

check_origin() {
    # Validate effective fetch and push URLs. Multiple URLs and a separately
    # configured push destination must not redirect publication elsewhere.
    origin_fetch=$(git remote get-url --all origin)
    origin_push=$(git remote get-url --push --all origin)
    for origin_url in "$origin_fetch" "$origin_push"; do
        case "$origin_url" in
            "https://github.com/$repository"|"https://github.com/$repository.git"|\
            "git@github.com:$repository"|"git@github.com:$repository.git"|\
            "ssh://git@github.com/$repository"|"ssh://git@github.com/$repository.git") ;;
            *) fail "origin does not match the authenticated account's $repository GitHub repository; refusing to publish." ;;
        esac
    done
}

check_private() {
    visibility=$(gh repo view "$repository" --json visibility --jq .visibility)
    test "$visibility" = PRIVATE || fail "$repository is not confirmed PRIVATE; refusing to publish."
}

if git remote get-url origin >/dev/null 2>&1; then
    check_origin
else
    # Keep repository creation separate from pushing so visibility is checked
    # before any content is staged, committed or uploaded.
    gh repo create "$repository" --private --source=. --remote=origin
    check_origin
fi
check_private
git add CMakeLists.txt .gitignore README.md include src tests python scripts docs data
if ! git diff --cached --quiet; then
    git commit -m "Add C++20 duel environment and batched RL interface"
fi
current_branch=$(git symbolic-ref --quiet --short HEAD) || fail "Detached HEAD; refusing to push."
test "$current_branch" = "$branch" || fail "Current branch changed; refusing to push."
check_origin
check_private
git -c remote.origin.mirror=false push --no-force --set-upstream origin "HEAD:refs/heads/$branch"
gh repo view "$repository" --json url,visibility
