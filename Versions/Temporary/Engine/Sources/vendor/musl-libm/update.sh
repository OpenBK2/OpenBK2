#!/bin/sh
# Refresh the copy of musl's libm from a musl checkout:
#
#   sh update.sh /path/to/musl
#
# Copies every path in MANIFEST verbatim, then records the upstream commit in
# UPSTREAM. Nothing is patched; see README.md for what to check afterwards.
set -eu
musl=${1:?usage: update.sh <musl checkout>}
here=$(cd "$(dirname "$0")" && pwd)

grep -v '^#' "$here/MANIFEST" | grep -v '^$' | while read -r path; do
	if [ "$path" = COPYRIGHT ]; then
		dest="$here/COPYRIGHT"
	else
		dest="$here/$path"
	fi
	mkdir -p "$(dirname "$dest")"
	cp "$musl/$path" "$dest"
done

{
	echo "https://git.musl-libc.org/git/musl"
	git -C "$musl" log -1 --format='%H %cs'
} > "$here/UPSTREAM"
echo "copied $(grep -cv '^#\|^$' "$here/MANIFEST") files from $(tail -1 "$here/UPSTREAM")"
