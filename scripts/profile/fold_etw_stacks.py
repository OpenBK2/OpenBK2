#!/usr/bin/env python3
"""Reduce an xperf stack dump to collapsed stacks and a ranked hot list.

`xperf -a stack` writes one row per frame per sample, which is both enormous and
unreadable. This groups the rows back into stacks, keeps one process, and writes:

  --folded   one line per unique stack, "root;...;leaf count", the FlameGraph format
  --hot      functions ranked by self and inclusive samples, small enough to paste

The column layout of the dump is not fixed across Windows Kit versions, so the header
is read rather than assumed, and the frame order is detected rather than guessed: a
thread's outermost frame is one of a handful of known thread-start functions, so
whichever end of the stack they turn up on is the root.
"""

import argparse
import collections
import csv
import io
import re
import sys

# Frames that only ever appear at the bottom of a stack. Used to work out whether the
# dump lists frames root first or leaf first.
ROOT_MARKERS = (
    "BaseThreadInitThunk",
    "RtlUserThreadStart",
    "wWinMainCRTStartup",
    "mainCRTStartup",
    "__scrt_common_main",
)


def find_column(header, *candidates):
    """Index of the first column whose name contains one of the candidates."""
    lowered = [h.strip().strip('"').lower() for h in header]
    for candidate in candidates:
        for i, name in enumerate(lowered):
            if candidate in name:
                return i
    return None


def read_rows(path):
    """Yield rows from the stack section of an xperf csv dump.

    The file has a preamble of other sections; the stack section starts at a header
    row that mentions both a stack-ish column and a process column.
    """
    with io.open(path, "r", encoding="utf-8", errors="replace", newline="") as handle:
        header = None
        idx = {}
        for row in csv.reader(handle):
            if not row:
                continue
            first = row[0].strip().strip('"').lower()
            if header is None:
                if first in ("stack", "stackwalk") or (
                    find_column(row, "process") is not None
                    and find_column(row, "image!function", "symbol") is not None
                ):
                    header = row
                    idx = {
                        "process": find_column(row, "process name", "process"),
                        "thread": find_column(row, "threadid", "thread id", "tid"),
                        "time": find_column(row, "timestamp", "time"),
                        "order": find_column(row, "no.", "depth", "frame"),
                        "symbol": find_column(row, "image!function", "symbol", "function"),
                    }
                continue
            if idx.get("symbol") is None or len(row) <= idx["symbol"]:
                continue
            yield row, idx


PROCESS_RE = re.compile(r"^\s*(?P<name>[^(]+?)\s*\(\s*(?P<pid>\d+)\s*\)\s*$")


def process_name(cell):
    """xperf writes "Game.exe (1234)"; keep just the executable name."""
    cell = cell.strip().strip('"')
    match = PROCESS_RE.match(cell)
    return (match.group("name") if match else cell).strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv", required=True)
    parser.add_argument("--process", default="Game.exe")
    parser.add_argument("--folded", required=True)
    parser.add_argument("--hot", required=True)
    parser.add_argument("--top", type=int, default=40)
    args = parser.parse_args()

    # Group frames into stacks, keyed by whatever identifies one sample.
    samples = collections.OrderedDict()
    seen_process = collections.Counter()
    for row, idx in read_rows(args.csv):
        proc = process_name(row[idx["process"]]) if idx["process"] is not None else "?"
        seen_process[proc] += 1
        if args.process and proc.lower() != args.process.lower():
            continue
        key = (
            row[idx["time"]].strip() if idx["time"] is not None else "",
            row[idx["thread"]].strip() if idx["thread"] is not None else "",
        )
        order = 0
        if idx["order"] is not None and idx["order"] < len(row):
            try:
                order = int(row[idx["order"]].strip().strip('"'))
            except ValueError:
                order = len(samples.get(key, ()))
        symbol = row[idx["symbol"]].strip().strip('"')
        samples.setdefault(key, []).append((order, symbol))

    if not samples:
        print(f"no samples for {args.process!r}.", file=sys.stderr)
        if seen_process:
            print("processes present in the trace:", file=sys.stderr)
            for name, n in seen_process.most_common(15):
                print(f"  {n:9d}  {name}", file=sys.stderr)
        return 1

    # Decide the frame order once, from the whole population rather than one stack.
    first_is_root = 0
    last_is_root = 0
    for frames in samples.values():
        frames.sort(key=lambda f: f[0])
        names = [name for _, name in frames]
        if any(m in names[0] for m in ROOT_MARKERS):
            first_is_root += 1
        if any(m in names[-1] for m in ROOT_MARKERS):
            last_is_root += 1
    leaf_first = last_is_root > first_is_root

    folded = collections.Counter()
    self_samples = collections.Counter()
    inclusive = collections.Counter()
    for frames in samples.values():
        names = [name for _, name in frames]
        if leaf_first:
            names = list(reversed(names))
        folded[";".join(names)] += 1
        self_samples[names[-1]] += 1
        for name in set(names):
            inclusive[name] += 1

    total = sum(folded.values())
    with io.open(args.folded, "w", encoding="utf-8") as handle:
        for stack, count in folded.most_common():
            handle.write(f"{stack} {count}\n")

    with io.open(args.hot, "w", encoding="utf-8") as handle:
        handle.write(f"process        : {args.process}\n")
        handle.write(f"samples        : {total}\n")
        handle.write(f"unique stacks  : {len(folded)}\n")
        handle.write(f"frame order    : {'leaf first' if leaf_first else 'root first'}"
                     f" (detected)\n\n")
        handle.write("self time: the sample landed in this function itself\n")
        handle.write(f"{'self%':>7} {'self':>9}  function\n")
        for name, count in self_samples.most_common(args.top):
            handle.write(f"{100.0 * count / total:7.2f} {count:9d}  {name}\n")
        handle.write("\ninclusive: this function was somewhere on the stack\n")
        handle.write(f"{'incl%':>7} {'incl':>9}  function\n")
        for name, count in inclusive.most_common(args.top):
            handle.write(f"{100.0 * count / total:7.2f} {count:9d}  {name}\n")

    print(f"{total} samples, {len(folded)} unique stacks, "
          f"{'leaf-first' if leaf_first else 'root-first'} dump")
    return 0


if __name__ == "__main__":
    sys.exit(main())
