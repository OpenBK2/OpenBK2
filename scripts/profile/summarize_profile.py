#!/usr/bin/env python3
"""Reduce an xperf butterfly report to a plain text hot list.

`xperf -a stack -butterfly` writes XHTML: a couple of megabytes of tables meant for a
browser. The numbers in it are exactly what a hot list needs, so this pulls them out
and writes something small enough to paste into a conversation.

The alternative, `-a dumper`, gives one row per frame per sample and would turn a
780 MB trace into many gigabytes of CSV. The butterfly report already aggregates, and
carries callers and callees for the top functions, which answers more than a flat list
would.

Percentages are reported twice, because the raw ones mislead. A sampling profile of a
game counts every sample the process was on a CPU, including time in kernel mode, and
kernel frames are unresolved without Microsoft's symbols. The "user" column
renormalises over the modules that are not the kernel, which is the number to reason
about when asking where the engine spends its time.
"""

import argparse
import html
import io
import re
import sys

KERNEL_MODULES = ("ntkrnlmp.exe", "ntoskrnl.exe", "hal.dll")


def strip_tags(text):
    return html.unescape(re.sub("<[^>]+>", "", text)).strip()


def sections(source):
    out = {}
    for part in re.split(r"<h2>", source)[1:]:
        name = strip_tags(part.split("</h2>")[0])
        out[name] = part
    return out


def rows(section, limit=None):
    found = []
    for tr in re.findall(r"<tr[^>]*>(.*?)</tr>", section, re.S):
        cells = [strip_tags(c) for c in re.findall(r"<t[dh][^>]*>(.*?)</t[dh]>", tr, re.S)]
        if not cells or cells[0] in ("module name", "function name"):
            continue
        found.append(cells)
        if limit and len(found) >= limit:
            break
    return found


def as_int(text):
    try:
        return int(re.sub(r"[^0-9]", "", text) or 0)
    except ValueError:
        return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", required=True, help="the .html xperf wrote")
    parser.add_argument("--out", required=True)
    parser.add_argument("--top", type=int, default=30)
    args = parser.parse_args()

    with io.open(args.report, encoding="utf-8", errors="replace") as handle:
        source = handle.read()

    found = sections(source)
    modules = rows(found.get("Modules by Exclusive Hits", ""))
    functions = rows(found.get("Functions by Exclusive Hits", ""))
    inclusive = rows(found.get("Functions by UniInclusive Hits", ""))

    if not modules and not functions:
        print("no tables found; is this an xperf -a stack -butterfly report?",
              file=sys.stderr)
        return 1

    total = sum(as_int(r[1]) for r in modules)
    kernel = sum(as_int(r[1]) for r in modules
                 if any(k in r[0].lower() for k in KERNEL_MODULES))
    user = total - kernel

    def pct(hits, base):
        return (100.0 * hits / base) if base else 0.0

    with io.open(args.out, "w", encoding="utf-8") as out:
        out.write(f"samples          : {total}\n")
        out.write(f"kernel mode      : {kernel} ({pct(kernel, total):.1f}%)"
                  f"  unresolved without Microsoft symbols\n")
        out.write(f"user mode        : {user} ({pct(user, total):.1f}%)"
                  f"  the column below renormalises over this\n\n")

        out.write("modules, by time in the module itself\n")
        out.write(f"{'total%':>8} {'user%':>8} {'hits':>9}  module\n")
        for r in modules[:args.top]:
            hits = as_int(r[1])
            is_kernel = any(k in r[0].lower() for k in KERNEL_MODULES)
            u = "-" if is_kernel else f"{pct(hits, user):8.2f}"
            out.write(f"{pct(hits, total):8.2f} {u:>8} {hits:9d}  {r[0]}\n")

        out.write("\nfunctions, by time in the function itself\n")
        out.write(f"{'total%':>8} {'user%':>8} {'hits':>9}  function\n")
        for r in functions[:args.top]:
            hits = as_int(r[1])
            is_kernel = any(k in r[0].lower() for k in KERNEL_MODULES)
            u = "-" if is_kernel else f"{pct(hits, user):8.2f}"
            out.write(f"{pct(hits, total):8.2f} {u:>8} {hits:9d}  {r[0]}\n")

        if inclusive:
            out.write("\nfunctions, by time anywhere beneath them\n")
            out.write(f"{'total%':>8} {'hits':>9}  function\n")
            for r in inclusive[:args.top]:
                hits = as_int(r[1])
                out.write(f"{pct(hits, total):8.2f} {hits:9d}  {r[0]}\n")

    print(f"{total} samples, {pct(kernel, total):.1f}% kernel, wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
