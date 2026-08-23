#!/usr/bin/env python3
"""Put several captures side by side.

One profile of one scene is not a characterisation of the game. A map of open ground
covered in tanks is bound by skinning and lighting; a wooded map, a siege, or the
moment a hundred units are given a move order will not be. The only way to tell which
findings are about the engine and which are about the scene is to capture a few and
compare them.

  python compare_profiles.py --out compare.txt \
      --report plains-butterfly.html=plains plains2-butterfly.html=forest

With no --report it takes every *-butterfly.html in the captures folder and names each
column after the file.

Percentages are of user mode, not of all samples: kernel frames are unresolved without
Microsoft symbols and swamp everything otherwise. That also makes the columns
comparable when one capture spent more time waiting on the GPU than another.
"""

import argparse
import glob
import html
import io
import os
import re
import sys

KERNEL_MODULES = ("ntkrnlmp.exe", "ntoskrnl.exe", "hal.dll")


def strip_tags(text):
    return html.unescape(re.sub("<[^>]+>", "", text)).strip()


def parse(path):
    """Return (label -> hits) for exclusive function time, plus the user-mode total."""
    with io.open(path, encoding="utf-8", errors="replace") as handle:
        source = handle.read()
    chunks = {}
    for part in re.split(r"<h2>", source)[1:]:
        chunks[strip_tags(part.split("</h2>")[0])] = part

    def table(name):
        found = []
        for tr in re.findall(r"<tr[^>]*>(.*?)</tr>", chunks.get(name, ""), re.S):
            cells = [strip_tags(c)
                     for c in re.findall(r"<t[dh][^>]*>(.*?)</t[dh]>", tr, re.S)]
            if cells and cells[0] not in ("module name", "function name"):
                found.append(cells)
        return found

    def hits(cell):
        return int(re.sub(r"[^0-9]", "", cell) or 0)

    modules = table("Modules by Exclusive Hits")
    total = sum(hits(r[1]) for r in modules)
    kernel = sum(hits(r[1]) for r in modules
                 if any(k in r[0].lower() for k in KERNEL_MODULES))
    user = total - kernel

    functions = {}
    for r in table("Functions by Exclusive Hits"):
        if any(k in r[0].lower() for k in KERNEL_MODULES):
            continue
        functions[r[0]] = functions.get(r[0], 0) + hits(r[1])
    return functions, user, total, kernel


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", nargs="*", help="path=label")
    parser.add_argument("--dir", default=os.path.join(os.path.dirname(__file__), "captures"))
    parser.add_argument("--out", required=True)
    parser.add_argument("--top", type=int, default=25)
    args = parser.parse_args()

    pairs = []
    for value in args.report or []:
        path, _, label = value.partition("=")
        pairs.append((path, label or os.path.splitext(os.path.basename(path))[0]))
    if not pairs:
        for path in sorted(glob.glob(os.path.join(args.dir, "*-butterfly.html"))):
            name = os.path.basename(path).replace("-butterfly.html", "")
            pairs.append((path, name))

    if not pairs:
        print("no butterfly reports found; run stop-profile.cmd first", file=sys.stderr)
        return 1

    captures = []
    for path, label in pairs:
        if not os.path.exists(path):
            print(f"missing: {path}", file=sys.stderr)
            continue
        functions, user, total, kernel = parse(path)
        captures.append((label, functions, user, total, kernel))

    if not captures:
        return 1

    # Rank by the highest share any single capture gives a function, so something that
    # dominates one scene and is absent from the others still surfaces.
    share = {}
    for _, functions, user, _, _ in captures:
        for name, hits in functions.items():
            value = (100.0 * hits / user) if user else 0.0
            share[name] = max(share.get(name, 0.0), value)
    ranked = sorted(share, key=share.get, reverse=True)[:args.top]

    width = max(12, max(len(label) for label, *_ in captures) + 2)
    with io.open(args.out, "w", encoding="utf-8") as out:
        out.write("percent of user-mode samples, by time in the function itself\n\n")
        for label, _, user, total, kernel in captures:
            out.write(f"  {label:<{width}} {user:>8} user of {total:>8} samples"
                      f"   ({100.0 * kernel / total:.0f}% kernel)\n" if total else "")
        out.write("\n")
        out.write(" " * 4 + "".join(f"{label:>{width}}" for label, *_ in captures)
                  + "  function\n")
        for name in ranked:
            cells = ""
            for _, functions, user, _, _ in captures:
                hits = functions.get(name, 0)
                value = (100.0 * hits / user) if user else 0.0
                cells += f"{value:>{width}.2f}" if hits else f"{'-':>{width}}"
            out.write(" " * 4 + cells + f"  {name}\n")

    print(f"compared {len(captures)} captures, wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
