#!/usr/bin/env python3
"""Run clang-format over the lines a change touches, and no others.

A whole-tree pass with .clang-format would rewrite about a third of every
source file, because the tree was never formatted mechanically. That diff is
not reviewable, and it would bury every real change made in the same release.
So the rule is narrower and holds from here on: whatever lines a commit writes
come out formatted, and the lines around them are left exactly as they were.

clang-format's own --lines does the work. All this does is work out which lines
to pass it, from git rather than from a guess.

Two modes:

    clang-format-changed.py --staged [files...]
        Format the lines that differ between the index and HEAD, in place. This
        is the pre-commit hook: pre-commit stashes unstaged changes first, so
        the file on disk is the file being committed.

    clang-format-changed.py --base <ref> [--check]
        Format, or with --check only report, the lines that differ from <ref>.
        This is the CI mode.

Exits non-zero when a file needed formatting, so either mode fails the gate.
"""
import argparse
import os
import re
import subprocess
import sys

# Formatted by their generator, not by hand, and the generator is the thing that
# decides their layout. Formatting one means the next regenerate-db or
# regenerate-gfxshaders run reverts it, and CI fails on the difference. The 101
# dbcodegen outputs carry the banner; the rest have no marker worth matching.
GENERATED_BANNER = b"automatically generated file, don't change manually"
GENERATED_NAMES = (
    'GfxShaders.h', 'GfxShaders.cpp',       # ShaderCompiler, from GfxShaders.txt
    'lang.tab.c', 'lang.tab.h', 'lex.yy.c',  # bison 1.24 and flex, from lang.y and lang.l
)

SOURCE_EXT = ('.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.hxx', '.inl')

# Never formatted. Mirrors the exclude list in .pre-commit-config.yaml; the
# fixture corpus is input to a parser test and the rest is third-party.
EXCLUDE_DIRS = (
    'Versions/Temporary/Engine/Sources/Parser/test/cases/',
    'Versions/Temporary/Engine/Sources/vendor/',
    'third_party/',
)

# The vendored Lua inside Script/, which keeps upstream's layout. Nival's own
# files there (Script.cpp, CommonFunctions.cpp, lsaver.cpp) are not Lua's and
# are formatted like the rest of the tree, so this matches names rather than the
# directory. lsaver is Nival's addition and sorts under the same pattern, so it
# is named out.
VENDORED_LUA = re.compile(
    r'^Versions/Temporary/Engine/Sources/Script/(?!lsaver)(l[a-z]+|lua[a-z]*)\.(c|cpp|h)$')

HUNK = re.compile(rb'^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@')
NEW_FILE = re.compile(rb'^\+\+\+ (?:b/(.*)|/dev/null)$')


def repo_root():
    out = subprocess.run(['git', 'rev-parse', '--show-toplevel'],
                         capture_output=True, check=True)
    return out.stdout.decode().strip()


def clang_format_binary():
    # The hook environment puts the pinned wheel's binary first on PATH; a
    # manual run uses whatever is installed. CLANG_FORMAT overrides both.
    return os.environ.get('CLANG_FORMAT', 'clang-format')


def is_formattable(path):
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
            if GENERATED_BANNER in fh.read(4096):
                return False
    except OSError:
        return False
    return True


def changed_lines(diff_args):
    """Per path, the line ranges it now has whose code the change wrote.

    One git diff for the whole change rather than one per file: a commit here
    can touch thousands of files, and a process each would dominate the run.
    quotePath off so a path with a non-ASCII byte arrives as those bytes rather
    than as an escape sequence.

    --ignore-all-space is what makes the rule "lines this change wrote" instead
    of "lines this change touched". A line whose whitespace alone moved was not
    written here, and its formatting is as old as the rest of the file, so
    reformatting it would pull in exactly the churn this hook exists to avoid.
    Stripping a trailing space off a thousand files would otherwise queue a
    thousand files up for reformatting.
    """
    out = subprocess.run(
        ['git', '-c', 'core.quotePath=false', 'diff', '-U0', '--no-color',
         '--no-ext-diff', '--no-renames', '--diff-filter=d',
         '--ignore-all-space'] + diff_args,
        capture_output=True, check=True)
    ranges = {}
    path = None
    for line in out.stdout.split(b'\n'):
        if line.startswith(b'+++ '):
            m = NEW_FILE.match(line)
            path = m.group(1).decode('utf-8', 'surrogateescape') if m and m.group(1) else None
            continue
        if path is None or not line.startswith(b'@@ '):
            continue
        m = HUNK.match(line)
        if not m:
            continue
        start = int(m.group(1))
        count = 1 if m.group(2) is None else int(m.group(2))
        if count == 0:
            # A deletion only. Nothing of ours is on that line any more, and
            # clang-format does not take a zero-length --lines range.
            continue
        ranges.setdefault(path, []).append((start, start + count - 1))
    return ranges


def format_file(path, ranges, check):
    args = [clang_format_binary(), '--style=file']
    for start, end in ranges:
        args.append(f'--lines={start}:{end}')
    out = subprocess.run(args + [path], capture_output=True)
    if out.returncode:
        sys.stderr.write(out.stderr.decode('utf-8', 'replace'))
        return 'error'
    with open(path, 'rb') as fh:
        before = fh.read()
    if out.stdout == before:
        return 'clean'
    if not check:
        with open(path, 'wb') as fh:
            fh.write(out.stdout)
    return 'reformatted'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--staged', action='store_true',
                        help='diff the index against HEAD (the pre-commit hook)')
    parser.add_argument('--base', metavar='REF',
                        help='diff the working tree against REF (CI)')
    parser.add_argument('--check', action='store_true',
                        help='report what would change without writing it')
    parser.add_argument('files', nargs='*',
                        help='candidates; without any, every file the diff touches')
    args = parser.parse_args()

    if args.staged == bool(args.base):
        parser.error('pass exactly one of --staged and --base')

    diff_args = ['--cached'] if args.staged else [args.base]

    os.chdir(repo_root())
    by_path = changed_lines(diff_args)

    # pre-commit passes the staged files; without a list, everything the diff
    # touched. Either way only the paths the diff actually touched are formatted.
    if args.files:
        wanted = {os.path.relpath(os.path.abspath(f)) for f in args.files}
        by_path = {p: r for p, r in by_path.items() if p in wanted}

    touched, failed = [], False
    for path, ranges in sorted(by_path.items()):
        if not is_formattable(path):
            continue
        result = format_file(path, ranges, args.check)
        if result == 'error':
            failed = True
        elif result == 'reformatted':
            touched.append(path)

    if touched:
        verb = 'needs formatting' if args.check else 'reformatted'
        print(f'clang-format {verb} the lines this change touches, in:')
        for path in touched:
            print(f'    {path}')
        if args.check:
            print('\nRun: python scripts/hygiene/clang-format-changed.py '
                  f'--base {args.base}')
        else:
            print('\nReview the result and stage it.')

    return 1 if (touched or failed) else 0


if __name__ == '__main__':
    sys.exit(main())
