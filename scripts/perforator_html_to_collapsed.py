#!/usr/bin/env python3
"""Turn a perforator flame graph HTML into collapsed stacks.

perforator's `record -o x.html` writes a self-contained page with the whole
profile inside it, so the file holds everything a tool would want and offers
none of it: there is no text to grep, and the numbers only exist once a browser
has run the script. Its pprof output is a separate recording decision rather
than a view of the same data, so "open the HTML" and "analyse the profile" had
no common ground.

They do now. The page carries `window.__data__`, which is base64 over gzip over
JSON, and that JSON is the flame graph as a list of depth levels:

    {"rows": [[{"parentIndex": -1, "textId": 0, "sampleCount": 335,
                "eventCount": 21146184770, ...}], ...],
     "stringTable": ["all", "", ...],
     "meta": {"eventType": 969, "frameType": 970, "version": 2}}

rows[d] holds the frames at depth d, and a frame's parentIndex is an index into
rows[d-1], so a stack is read by walking that chain back to the root. textId
indexes stringTable.

Collapsed stacks are the output because that is the format everything else
reads: FlameGraph's flamegraph.pl, speedscope, pprof converters, and `sort |
uniq -c` for the questions that need nothing more.

    scripts/perforator_html_to_collapsed.py profile.html > profile.folded
    scripts/perforator_html_to_collapsed.py profile.html --self | head -20

Weights are eventCount, which for a cycles profile is cycles and is what the
flame graph widths show. --samples weighs by sampleCount instead, which counts
each sample once however expensive it was.
"""

import argparse
import base64
import gzip
import json
import re
import sys

# The assignment the page makes before the bundle runs. Non-greedy and anchored
# on the quotes because the blob is one long base64 string with no escapes.
DATA_RE = re.compile(r'window\.__data__\s*=\s*"([A-Za-z0-9+/=]+)"')


def load_profile(path):
    """The decoded JSON out of a perforator flame graph page."""
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        html = handle.read()
    match = DATA_RE.search(html)
    if match is None:
        raise SystemExit(
            f"{path}: no window.__data__ blob; is this a perforator flame graph?"
        )
    try:
        raw = gzip.decompress(base64.b64decode(match.group(1)))
    except (ValueError, OSError) as exc:
        raise SystemExit(f"{path}: could not decode the profile blob: {exc}")
    return json.loads(raw)


def collapse(profile, weigh_by_samples=False):
    """Collapsed stacks, as (stack list root first, weight) pairs.

    Only the weight a frame does not pass on to a child belongs to that frame,
    so each frame's own weight is its total minus the sum of its children's.
    Emitting every frame's total instead would count a stack once per level.
    """
    rows = profile["rows"]
    strings = profile["stringTable"]
    field = "sampleCount" if weigh_by_samples else "eventCount"

    # Sum of each frame's children, keyed by (depth, index within that depth).
    child_total = {}
    for depth in range(1, len(rows)):
        for frame in rows[depth]:
            key = (depth - 1, frame["parentIndex"])
            child_total[key] = child_total.get(key, 0) + frame[field]

    def name(frame):
        text = strings[frame["textId"]]
        # Unnamed frames are real: an address with no symbol still carries time,
        # and dropping the frame would reparent its callees onto its caller.
        return text if text else "[unknown]"

    out = []
    for depth, row in enumerate(rows):
        for index, frame in enumerate(row):
            own = frame[field] - child_total.get((depth, index), 0)
            if own <= 0:
                continue
            stack, d, i = [], depth, index
            while i >= 0 and d >= 0:
                stack.append(name(rows[d][i]))
                i = rows[d][i]["parentIndex"]
                d -= 1
            stack.reverse()
            out.append((stack, own))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("html", help="perforator flame graph HTML")
    parser.add_argument("--samples", action="store_true",
                        help="weigh by sample count rather than by event count")
    parser.add_argument("--self", dest="self_report", action="store_true",
                        help="print a self-time ranking instead of collapsed stacks")
    parser.add_argument("--filter", default=None,
                        help="keep only stacks containing this substring")
    args = parser.parse_args()

    profile = load_profile(args.html)
    stacks = collapse(profile, args.samples)
    if args.filter:
        stacks = [(s, w) for s, w in stacks if any(args.filter in f for f in s)]

    total = sum(w for _, w in stacks)
    if total == 0:
        raise SystemExit("no weight in the profile after filtering")

    if args.self_report:
        per_leaf = {}
        for stack, weight in stacks:
            per_leaf[stack[-1]] = per_leaf.get(stack[-1], 0) + weight
        print(f"# {total} total, {len(per_leaf)} distinct leaves", file=sys.stderr)
        for leaf, weight in sorted(per_leaf.items(), key=lambda kv: -kv[1]):
            print(f"{100.0 * weight / total:6.2f}%  {weight:>14}  {leaf}")
        return

    # Collapsed stacks are conventionally one line per distinct stack, so equal
    # stacks reached at different points in the walk are summed rather than
    # printed twice.
    merged = {}
    for stack, weight in stacks:
        key = ";".join(stack)
        merged[key] = merged.get(key, 0) + weight
    for key, weight in sorted(merged.items(), key=lambda kv: -kv[1]):
        print(f"{key} {weight}")


if __name__ == "__main__":
    main()
