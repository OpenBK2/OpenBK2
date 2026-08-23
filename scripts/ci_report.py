#!/usr/bin/env python3
"""Render ctest JUnit XML and Google Benchmark JSON into one markdown report.

Google Benchmark writes console, JSON or CSV and nothing else, so a PR comment needs
a converter. ctest --output-junit gives the test side. Both are read here and printed
as markdown on stdout, which the workflow posts as a single comment.

  python scripts/ci_report.py --junit x86.xml=x86 x64.xml=x64 \
                              --benchmark bench.json=x64 --title "..."

Each argument is path=label so several architectures can share one report.
"""

import argparse
import json
import os
import xml.etree.ElementTree as ET


def parse_junit(path):
    """Return (label -> list of (name, status, seconds))."""
    tree = ET.parse(path)
    cases = []
    for case in tree.iter("testcase"):
        name = case.get("name") or "?"
        classname = case.get("classname") or ""
        full = f"{classname}.{name}" if classname and classname != name else name
        if case.find("failure") is not None or case.find("error") is not None:
            status = "fail"
        elif case.find("skipped") is not None:
            status = "skip"
        else:
            status = "pass"
        try:
            seconds = float(case.get("time") or 0.0)
        except ValueError:
            seconds = 0.0
        cases.append((full, status, seconds))
    return cases


def parse_benchmarks(path):
    """Return list of (name, label, ns_per_op)."""
    with open(path, encoding="utf-8") as handle:
        data = json.load(handle)
    rows = []
    for entry in data.get("benchmarks", []):
        if entry.get("run_type") == "aggregate" and entry.get("aggregate_name") != "median":
            continue
        name = entry.get("name", "?")
        # The label carries the kernel set for the lighting benchmarks.
        variant = entry.get("label") or ""
        real = entry.get("real_time")
        unit = entry.get("time_unit", "ns")
        if real is None:
            continue
        factor = {"ns": 1.0, "us": 1e3, "ms": 1e6, "s": 1e9}.get(unit, 1.0)
        rows.append((name, variant, real * factor))
    return rows


ICON = {"pass": "ok", "fail": "**FAIL**", "skip": "skipped"}


def render_tests(sources):
    lines = ["## Unit tests", ""]
    any_rows = False
    for path, label in sources:
        if not os.path.exists(path):
            lines.append(f"- `{label}`: no results file produced")
            continue
        cases = parse_junit(path)
        if not cases:
            lines.append(f"- `{label}`: no test cases recorded")
            continue
        any_rows = True
        failed = sum(1 for _, s, _ in cases if s == "fail")
        skipped = sum(1 for _, s, _ in cases if s == "skip")
        passed = len(cases) - failed - skipped
        lines.append(f"### {label} - {passed} passed, {failed} failed, {skipped} skipped")
        lines.append("")
        lines.append("| test | result | time |")
        lines.append("|---|---|---|")
        for name, status, seconds in cases:
            lines.append(f"| `{name}` | {ICON[status]} | {seconds:.2f}s |")
        lines.append("")
    if not any_rows:
        lines.append("No test results were produced.")
        lines.append("")
    return lines


def render_benchmarks(sources):
    lines = ["## Benchmarks", ""]
    for path, label in sources:
        if not os.path.exists(path):
            lines.append(f"- `{label}`: no benchmark output produced")
            continue
        rows = parse_benchmarks(path)
        if not rows:
            lines.append(f"- `{label}`: no benchmarks recorded")
            continue
        # Group by benchmark name with the variant label as the column, which is how
        # the lighting benchmark reports ref / sse2 / avx2 for the same kernel.
        variants = []
        for _, variant, _ in rows:
            if variant and variant not in variants:
                variants.append(variant)
        lines.append(f"### {label}")
        lines.append("")
        if variants:
            grouped = {}
            for name, variant, ns in rows:
                base = name.split("/")[0]
                args = "/".join(name.split("/")[2:]) or "-"
                grouped.setdefault((base, args), {})[variant] = ns
            lines.append("| benchmark | n | " + " | ".join(variants) + " |")
            lines.append("|---" * (len(variants) + 2) + "|")
            for (base, args), by_variant in grouped.items():
                cells = []
                for v in variants:
                    ns = by_variant.get(v)
                    cells.append(f"{ns:,.0f} ns" if ns is not None else "-")
                lines.append(f"| `{base}` | {args} | " + " | ".join(cells) + " |")
        else:
            lines.append("| benchmark | time |")
            lines.append("|---|---|")
            for name, _, ns in rows:
                lines.append(f"| `{name}` | {ns:,.0f} ns |")
        lines.append("")
    lines.append("Times are real time per operation, lower is better.")
    lines.append("")
    return lines


def pairs(values):
    out = []
    for value in values or []:
        path, _, label = value.partition("=")
        out.append((path, label or os.path.basename(path)))
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--junit", nargs="*", help="path=label")
    parser.add_argument("--benchmark", nargs="*", help="path=label")
    parser.add_argument("--benchmark-dir", nargs="*",
                        help="dir=label; every .json in the directory, so CI does not "
                             "have to know what benchmarks exist")
    parser.add_argument("--title", default="CI results")
    args = parser.parse_args()

    benchmarks = pairs(args.benchmark)
    for directory, label in pairs(args.benchmark_dir):
        if not os.path.isdir(directory):
            continue
        for name in sorted(os.listdir(directory)):
            if name.endswith(".json"):
                stem = os.path.splitext(name)[0]
                benchmarks.append((os.path.join(directory, name), f"{label}: {stem}"))

    lines = [f"# {args.title}", ""]
    lines += render_tests(pairs(args.junit))
    lines += render_benchmarks(benchmarks)
    lines.append("<sub>Benchmarks run on a shared CI runner; treat small differences "
                 "as noise and compare orders of magnitude, not percentages.</sub>")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
