#!/usr/bin/env python3
"""Run clang-format over whole source files, skipping the ones nothing should.

The whole tree is formatted, so there is nothing clever to do here: every source
is checked in full. What this adds over calling clang-format directly is the
skip list, which cannot be written as a path pattern because most of it is
decided by what is written at the top of the file.

    clang-format-sources.py [files...]      format in place, exit 1 if anything changed
    clang-format-sources.py --check [files...]   report only, change nothing

With no files, every source git knows about. pre-commit passes the staged ones.

Skipped, because a generator decides their layout and would revert the edit on
its next run, which is also how CI would find out:

    the dbcodegen output, by the banner it writes
    3Dmotor/GfxShaders.*, from ShaderCompiler
    Parser/lang.tab.* and lex.yy.c, from bison and flex
    the editor resource tables, from scripts/port/rc2binary.py and friends

Skipped because they are not ours: the vendored Lua in Script (but not Nival's
own lsaver.cpp beside it), Sources/vendor, third_party, and the 1990s STL
documentation under Soft.

Parser/test/cases is skipped because its bytes are the input to a parser test.
3Dmotor/test/original needs no entry here: it carries a .clang-format of its own
with DisableFormat, so clang-format returns those files unchanged.
"""
import argparse
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

SOURCE_EXT = ('.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.hxx', '.inl')

EXCLUDE_DIRS = (
    'Versions/Temporary/Engine/Sources/Parser/test/cases/',
    'Versions/Temporary/Engine/Sources/vendor/',
    'third_party/',
    'Soft/SDK/STL docs/',
)

GENERATED_NAMES = ('GfxShaders.h', 'GfxShaders.cpp', 'lang.tab.c', 'lang.tab.h', 'lex.yy.c')

# What dbcodegen writes at the top of each of its 101 outputs.
GENERATED_BANNER = b"automatically generated file, don't change manually"

# What this repository's other generators write: scripts/port/rc2binary.py,
# rc2menus.py, rc2strings.py, gen-d3d9-stub.py, gen-granny-shim.py. Requiring
# "from" or "by" keeps it off the many hand-written comments that merely begin
# "Generated constructors do not initialize ...".
GENERATED_FROM = re.compile(rb'^//\s*Generated (from|by)\b', re.I | re.M)

# Upstream Lua, which keeps upstream's layout. lsaver.cpp is Nival's addition
# and sorts under the same pattern, so it is named out of it.
VENDORED_LUA = re.compile(
    r'^Versions/Temporary/Engine/Sources/Script/(?!lsaver)(l[a-z]+|lua[a-z]*)\.(c|cpp|h)$')


def formattable(path):
    if not path.lower().endswith(SOURCE_EXT):
        return False
    if path.startswith(EXCLUDE_DIRS):
        return False
    if os.path.basename(path) in GENERATED_NAMES:
        return False
    if VENDORED_LUA.match(path):
        return False
    try:
        with open(path, 'rb') as fh:
            head = fh.read(4096)
    except OSError:
        return False
    if GENERATED_BANNER in head:
        return False
    return not GENERATED_FROM.search(b'\n'.join(head.split(b'\n')[:6]))


def clang_format():
    # The hook environment puts the pinned wheel's binary first on PATH; a
    # manual run uses whatever is installed. CLANG_FORMAT overrides both.
    return os.environ.get('CLANG_FORMAT', 'clang-format')


def run(path, check):
    out = subprocess.run([clang_format(), '--style=file', path], capture_output=True)
    if out.returncode:
        return (path, 'error', out.stderr.decode('utf-8', 'replace')[:400])
    with open(path, 'rb') as fh:
        before = fh.read()
    if out.stdout == before:
        return (path, 'clean', '')
    if not check:
        with open(path, 'wb') as fh:
            fh.write(out.stdout)
    return (path, 'changed', '')


def all_sources():
    out = subprocess.run(['git', 'ls-files', '-z'], capture_output=True, check=True).stdout
    return [f.decode('utf-8', 'surrogateescape') for f in out.split(b'\0') if f]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--check', action='store_true', help='report without writing')
    ap.add_argument('files', nargs='*')
    args = ap.parse_args()

    root = subprocess.run(['git', 'rev-parse', '--show-toplevel'],
                          capture_output=True, check=True).stdout.decode().strip()
    os.chdir(root)

    candidates = args.files or all_sources()
    files = [f for f in candidates if formattable(f)]

    changed, failed = [], []
    # Threads, not processes: each one spends its life waiting on a clang-format
    # that is doing the work in another process anyway.
    with ThreadPoolExecutor(max_workers=min(16, (os.cpu_count() or 4))) as pool:
        for path, verdict, detail in pool.map(lambda f: run(f, args.check), files):
            if verdict == 'changed':
                changed.append(path)
            elif verdict == 'error':
                failed.append((path, detail))

    for path, detail in failed:
        sys.stderr.write(f'clang-format failed on {path}:\n{detail}\n')
    if changed:
        verb = 'not formatted' if args.check else 'reformatted'
        noun = 'file' if len(changed) == 1 else 'files'
        print(f'{len(changed)} {noun} {verb}:')
        for path in changed[:40]:
            print(f'    {path}')
        if len(changed) > 40:
            print(f'    ... and {len(changed) - 40} more')
        if args.check:
            print('\nRun: python scripts/hygiene/clang-format-sources.py')
    return 1 if (changed or failed) else 0


if __name__ == '__main__':
    sys.exit(main())
