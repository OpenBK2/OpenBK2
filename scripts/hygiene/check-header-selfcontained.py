#!/usr/bin/env python3
"""Compile each engine header on its own and report the ones that need help.

A header that compiles on its own can be included in any order. One that does
not is relying on whoever includes it to have included something else first,
which is why `SortIncludes` is off in .clang-format: turning it on reorders the
includes of a .cpp and the build stops. The rule the tree actually follows is
not "stdafx first, then alphabetical", it is "every header's unstated
dependencies are already in scope", and no sort order can express that.

This measures the gap, so the rule can eventually be written down as a sort
order instead of as a convention nobody can check.

    check-header-selfcontained.py                   # report
    check-header-selfcontained.py --baseline FILE    # fail only on new breakage
    check-header-selfcontained.py --write-baseline FILE
    check-header-selfcontained.py --strict           # drop the precompiled prelude

Each header becomes a translation unit containing only `#include "that/header.h"`
and is compiled with the real flags of a .cpp in the same directory, read from
the build directory's compile_commands.json. Configure a build first.

By default the module's precompiled prelude is kept, because that is what a
.cpp in that module gets. --strict drops it and asks whether the header stands
up with nothing but what it includes itself; that is the standard that would
let `stdafx.h` stop being load-bearing, and far more headers fail it.

Not measured: `include_*.h`. Those are fragments that dbcodegen pastes inside a
generated struct, not headers, and they cannot compile alone by construction.
"""
import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SOURCES = os.path.join(ROOT, 'Versions/Temporary/Engine/Sources')
HEADER_EXT = ('.h', '.hpp', '.hxx', '.inl')
SKIP_DIRS = ('/vendor/', '/Parser/test/cases', '/3Dmotor/test/original')

MISSING = re.compile(
    r"[‘'\"]([A-Za-z_][A-Za-z0-9_:]*)[’'\"] "
    r"(?:has not been declared|was not declared|does not name a type)")


def find_build_dir(explicit):
    if explicit:
        return explicit
    candidates = [
        os.path.join(ROOT, 'out/build/Linux-x64-Release'),
        os.path.join(ROOT, 'out/build/Linux-x64-Debug'),
        os.path.join(ROOT, 'linux-build-ubsan'),
    ]
    for c in candidates:
        if os.path.exists(os.path.join(c, 'compile_commands.json')):
            return c
    return None


def commands_by_dir(build):
    with open(os.path.join(build, 'compile_commands.json')) as fh:
        entries = json.load(fh)
    best = {}
    for e in entries:
        f = os.path.realpath(e['file'])
        if f.startswith(SOURCES) and f.endswith(('.cpp', '.cc', '.cxx')):
            best.setdefault(os.path.dirname(f), e)
    return best


def flags_for(entry, strict):
    """The compile command reduced to flags, with the source and output dropped."""
    argv = shlex.split(entry['command'])
    out, skip = [], False
    for a in argv:
        if skip:
            skip = False
            continue
        if a in ('-o', '-MT', '-MF'):
            skip = True
            continue
        if a in ('-c', '-MD', 'cmake', '-E', 'env'):
            continue
        if a.endswith(('.cpp', '.cc', '.cxx')) or a.endswith('c++') or a.endswith('ccache'):
            continue
        if a.startswith('CCACHE_'):
            continue
        if strict and (a == '-Winvalid-pch' or a.endswith('cmake_pch.hxx')):
            continue
        if strict and a == '-include':
            skip = True
            continue
        out.append(a)
    return out


def check(job):
    header, entry, build, strict = job
    rel = os.path.relpath(header, SOURCES)
    fd, name = tempfile.mkstemp(suffix='.cpp', dir='/tmp')
    try:
        with os.fdopen(fd, 'w') as tu:
            tu.write(f'#include "{header}"\n')
        proc = subprocess.run(['g++', '-fsyntax-only', *flags_for(entry, strict), name],
                              capture_output=True, cwd=build)
    finally:
        os.unlink(name)
    if proc.returncode == 0:
        return (rel, None, None)
    first = ''
    for line in proc.stderr.decode('utf-8', 'replace').split('\n'):
        if ' error: ' in line:
            first = line.split(' error: ', 1)[1].strip()
            break
    m = MISSING.search(first)
    return (rel, first[:140], m.group(1) if m else None)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build-dir')
    ap.add_argument('--baseline', help='fail only on headers not already listed here')
    ap.add_argument('--write-baseline', help='write the current failures to this file')
    ap.add_argument('--strict', action='store_true', help='compile without the prelude')
    ap.add_argument('--jobs', type=int, default=min(16, (os.cpu_count() or 4)))
    args = ap.parse_args()

    build = find_build_dir(args.build_dir)
    if build is None:
        sys.stderr.write('no compile_commands.json found; configure a build first\n')
        return 2
    per_dir = commands_by_dir(build)

    jobs = []
    for dirpath, _, names in os.walk(SOURCES):
        if any(s in dirpath for s in SKIP_DIRS):
            continue
        entry = per_dir.get(os.path.realpath(dirpath))
        if entry is None:
            continue
        for n in sorted(names):
            if not n.endswith(HEADER_EXT):
                continue
            if n.lower() == 'stdafx.h' or n.startswith('include_'):
                continue
            jobs.append((os.path.join(dirpath, n), entry, build, args.strict))

    ok, failures, names = 0, {}, defaultdict(int)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for rel, first, missing in pool.map(check, jobs):
            if first is None:
                ok += 1
            else:
                failures[rel] = first
                if missing:
                    names[missing] += 1

    total = ok + len(failures)
    mode = 'without the prelude' if args.strict else 'with the module prelude'
    print(f'{total} headers compiled on their own, {mode}')
    print(f'  self-contained:     {ok}')
    print(f'  not self-contained: {len(failures)}  ({100.0 * len(failures) / max(total, 1):.1f}%)')
    if names:
        print('\nmost often missing:')
        for n, c in sorted(names.items(), key=lambda x: -x[1])[:12]:
            print(f'   {c:4d}  {n}')

    if args.write_baseline:
        with open(args.write_baseline, 'w') as fh:
            fh.write('# Headers that do not compile on their own, and the first thing they\n'
                     '# are missing. Generated by scripts/hygiene/check-header-selfcontained.py.\n'
                     '# This list is a ratchet: it may shrink, and a header not on it that\n'
                     '# stops compiling alone is a regression. See docs/CleanupRoadmap.md.\n')
            for rel in sorted(failures):
                fh.write(f'{rel}\t{failures[rel]}\n')
        print(f'\nwrote {len(failures)} entries to {args.write_baseline}')
        return 0

    if args.baseline:
        known = set()
        if os.path.exists(args.baseline):
            with open(args.baseline) as fh:
                known = {l.split('\t')[0] for l in fh if l.strip() and not l.startswith('#')}
        new = sorted(set(failures) - known)
        fixed = sorted(known - set(failures))
        if fixed:
            print(f'\n{len(fixed)} header(s) became self-contained; '
                  f'regenerate the baseline with --write-baseline')
        if new:
            print(f'\n{len(new)} header(s) newly fail to compile on their own:')
            for rel in new:
                print(f'   {rel}\n       {failures[rel]}')
            return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
