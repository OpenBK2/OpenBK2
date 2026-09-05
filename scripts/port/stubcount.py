"""What is still a stub in vendor/stingray, and which of those the editor calls.

Two questions that get answered together, because either one alone misleads. A
count of stubs says how much of the toolkit is unwritten, which is not the same
as how much is missing: most of it has no caller in this editor and never will.
A trace of what ran says what is reached, but not whether what it reached did
anything.

A stub here is a function whose body does nothing but log and hand back a
constant -- the shape every unwritten toolkit method was left in. The trace logs
the editor writes name every call with its full signature, so intersecting the
two gives the list that matters: functions the editor called that answered
nothing.

    python stubcount.py                          # counts, by file
    python stubcount.py --called                 # only the ones the editor ran
    python stubcount.py --called --logs "C:/Games/bk2/bin"

The logs default to the install the test recipe uses. Without them the "called"
column is empty rather than wrong.
"""
import argparse
import glob
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.normpath(os.path.join(HERE, '..', '..', 'Versions', 'Temporary',
                                    'Engine', 'Sources', 'vendor', 'stingray'))

# A definition line: something ending in ") {" or ") const {" at column zero,
# whose name has a :: in it. Anything indented is inside a class or a lambda.
DEFN = re.compile(r'^[A-Za-z_].*?\b(?P<cls>\w+)::(?P<fn>~?\w+)\s*\((?P<args>[^;]*?)\)'
                  r'(?:\s*const)?(?:\s*noexcept)?\s*\{\s*$')

# What a body may contain and still be a stub: nothing, a comment, a trace line,
# or a return of a literal. `return CToolBar::Something( x );` is a forward, not
# a stub, and counting it as one was the first version's mistake -- it put the
# working tree control's InsertItem on the list of things that answer nothing.
CONST = r'(?:TRUE|FALSE|0|1|-1|NULL|nullptr|\{\s*\}|""|_T\(""\)|CString\(\)|CSize\(\s*0\s*,\s*0\s*\))'
NOISE = re.compile(r'^\s*(?://.*|spdlog::\w+\(.*|return\s*' + CONST + r'\s*;|)$')
RETURNS_VALUE = re.compile(r'^\s*return\s+\S')


def arity(args):
    """How many parameters a parameter list has.

    The overload is the whole point: SECShortcutBar has three AddBar's and only
    one of them is written, so matching a trace by name alone reports the
    working one as a stub the editor called. The trace records the full
    signature, but its spelling is the compiler's -- LPCTSTR comes back as
    `const char *`, BOOL as `int` -- so the count of parameters is what the two
    sides can agree on without a type table.
    """
    args = args.strip()
    if not args or args == 'void':
        return 0
    depth, n = 0, 1
    for c in args:
        if c in '(<[':
            depth += 1
        elif c in ')>]':
            depth -= 1
        elif c == ',' and depth == 0:
            n += 1
    return n


def stubs_in(path):
    """(class, function, line, returns) for every stub defined in one file."""
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.read().splitlines()
    out = []
    i = 0
    while i < len(lines):
        m = DEFN.match(lines[i])
        if not m:
            i += 1
            continue
        # Collect the body up to the closing brace at column zero.
        body, j = [], i + 1
        while j < len(lines) and lines[j] != '}':
            body.append(lines[j])
            j += 1
        joined = [l for l in body if l.strip()]
        if joined and all(NOISE.match(l) for l in body):
            returns = any(RETURNS_VALUE.match(l) for l in body)
            out.append((m.group('cls'), m.group('fn'), arity(m.group('args')),
                        i + 1, returns))
        i = j + 1
    return out


def called_names(log_dir):
    """Every Class::Function(arity) the trace logs say ran."""
    names = set()
    for path in glob.glob(os.path.join(log_dir, 'stingray_*.log')):
        with open(path, 'r', encoding='utf-8', errors='replace') as f:
            for line in f:
                for cls, fn, args in re.findall(r'\b(\w+)::(~?\w+)\s*\(([^)]*)\)', line):
                    names.add((cls, fn, arity(args)))
    return names


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--logs', default=r'C:\Games\bk2\bin',
                    help='directory holding stingray_*.log from a run')
    ap.add_argument('--called', action='store_true',
                    help='list only stubs the trace says were called')
    args = ap.parse_args()

    ran = called_names(args.logs)
    if not ran:
        print('no stingray_*.log in %s; the called column will be empty\n' % args.logs)

    total = called = 0
    for path in sorted(glob.glob(os.path.join(SRC, '*.cpp'))):
        found = stubs_in(path)
        hit = [s for s in found if (s[0], s[1], s[2]) in ran]
        total += len(found)
        called += len(hit)
        if args.called:
            for cls, fn, nargs, line, returns in hit:
                print('  %s:%d  %s::%s/%d%s' % (os.path.basename(path), line, cls, fn,
                                                nargs,
                                                '  (returns a constant)' if returns else ''))
        elif found:
            print('  %-16s %3d stubs, %2d of them called' %
                  (os.path.basename(path), len(found), len(hit)))
    print('\n%d stubs, %d called by the editor in the traced run' % (total, called))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
