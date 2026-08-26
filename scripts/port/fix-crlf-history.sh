#!/usr/bin/env bash
#
# Restore CRLF on the CMake files that a session on 2026-08-26 flipped to LF,
# across every commit in a range, so those commits stop reading as whole file
# rewrites.
#
# The flip came from an editing script that read with universal newlines, so
# CRLF became LF in memory, and wrote back without translating, so LF went to
# disk. Nothing about the content changed. This repository is mixed on purpose:
# C++ sources are LF and CMake files are CRLF, and nothing enforces either,
# since .gitattributes is empty and core.autocrlf is false.
#
# THIS REWRITES HISTORY. Every commit in the range gets a new hash. Only run it
# on commits that have not been pushed, and re-point any other clone afterwards
# with `git reset --hard`, which the script prints instructions for.
#
# Usage:
#   scripts/port/fix-crlf-history.sh [<base>]
#
#   <base>  the commit before the first one to rewrite. Everything after it, up
#           to HEAD, is rewritten. Defaults to 11a8b8557, the commit before the
#           session that caused this.
#
# Why --index-filter and not --tree-filter: a tree filter checks out the whole
# working tree for every commit in the range before running anything. This
# repository carries the game's data directories, so that is a very large copy
# fifteen times over, which is minutes of disk and CPU for six files. An index
# filter never touches the working tree at all: it rewrites the six blobs
# straight into the object database and updates the index entry, so the cost per
# commit is six lookups rather than a full checkout.
#
# If `git filter-repo` is installed it is faster still and is what the git
# project recommends over filter-branch. This uses filter-branch because it is
# always present.

set -euo pipefail

BASE="${1:-11a8b8557}"

FILES=(
	"CMakeLists.txt"
	"Versions/Temporary/Engine/Sources/3Dmotor/CMakeLists.txt"
	"Versions/Temporary/Engine/Sources/Game/CMakeLists.txt"
	"Versions/Temporary/Engine/Sources/Input/CMakeLists.txt"
	"cmake/ffmpeg.cmake"
	"cmake/granny.cmake"
)

cd "$(git rev-parse --show-toplevel)"

# A dirty tree would be carried into every rewritten commit, or lost.
# Submodules are excluded: this repository carries a modified third_party
# submodule pointer as a matter of course, and it is not part of any rewrite.
if ! git diff --quiet --ignore-submodules=all || ! git diff --cached --quiet --ignore-submodules=all; then
	echo "error: working tree has changes. Commit or stash them first." >&2
	git status --short --ignore-submodules=all | grep -v '^??' >&2
	exit 1
fi

if ! git rev-parse --verify --quiet "${BASE}^{commit}" >/dev/null; then
	echo "error: ${BASE} is not a commit" >&2
	exit 1
fi

# perl rather than sed for the conversion. sed works line by line and appends a
# trailing newline to a file that had none, which would be a real change to the
# content on top of the endings. Slurping the whole blob leaves that alone.
if ! command -v perl >/dev/null; then
	echo "error: perl is needed for the conversion" >&2
	exit 1
fi

COUNT="$(git rev-list --count "${BASE}..HEAD")"
if [ "${COUNT}" -eq 0 ]; then
	echo "nothing to do: no commits between ${BASE} and HEAD"
	exit 0
fi

BRANCH="$(git rev-parse --abbrev-ref HEAD)"
BACKUP="backup/pre-crlf-$(git rev-parse --short HEAD)"

echo "branch:   ${BRANCH}"
echo "range:    ${BASE}..HEAD  (${COUNT} commits)"
echo "backup:   ${BACKUP}"
echo "files:    ${#FILES[@]}"
echo
read -r -p "rewrite these ${COUNT} commits? [y/N] " REPLY
case "${REPLY}" in
	y | Y) ;;
	*)
		echo "aborted"
		exit 1
		;;
esac

git branch -f "${BACKUP}" HEAD
echo "backup branch ${BACKUP} points at $(git rev-parse --short HEAD)"

# The filter is written out rather than inlined, so that quoting it through
# filter-branch's eval does not decide how readable it is.
HELPER="$(mktemp)"
trap 'rm -f "${HELPER}"' EXIT

{
	echo '#!/usr/bin/env bash'
	echo 'set -euo pipefail'
	printf 'FILES=('
	printf '"%s" ' "${FILES[@]}"
	printf ')\n'
	cat <<'EOS'
for f in "${FILES[@]}"; do
	entry="$(git ls-files --stage -- "${f}")"
	[ -n "${entry}" ] || continue
	mode="$(printf '%s' "${entry}" | awk '{print $1}')"
	sha="$(printf '%s' "${entry}" | awk '{print $2}')"
	new="$(git cat-file blob "${sha}" | perl -0777 -pe 's/\r\n/\n/g; s/\n/\r\n/g' | git hash-object -w --stdin)"
	if [ "${new}" != "${sha}" ]; then
		git update-index --cacheinfo "${mode},${new},${f}"
	fi
done
EOS
} >"${HELPER}"
chmod +x "${HELPER}"

FILTER_BRANCH_SQUELCH_WARNING=1 git filter-branch -f --index-filter "${HELPER}" -- "${BASE}..HEAD"

echo
echo "done. ${BRANCH} is now $(git rev-parse --short HEAD), was $(git rev-parse --short "${BACKUP}")."
echo
echo "check a commit that touched one of these files, for example:"
echo "  git show --stat HEAD"
echo
echo "if it looks right, drop the backup and re-point any other clone:"
echo "  git branch -D ${BACKUP}"
echo "  git update-ref -d refs/original/refs/heads/${BRANCH}"
echo "  # in the WSL clone, or anywhere else that fast-forwarded this branch:"
echo "  git fetch windows && git reset --hard windows/${BRANCH}"
echo
echo "if it looks wrong:"
echo "  git reset --hard ${BACKUP}"
