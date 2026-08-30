#!/usr/bin/env python3
"""Diff two granny call logs: one from libgr2, one from the shim over the real DLL.

Record the same play twice, once with each DLL in place, then compare. What
survives normalisation is a real difference in what the two implementations were
asked to do or told the engine.

    LIBGR2_LOG_FILE=vanilla.log   ...run with the shim...
    LIBGR2_LOG_FILE=libgr2.log    ...run with libgr2...
    python gr2logdiff.py vanilla.log libgr2.log

Three things are normalised away, none of which mean anything:

  * timestamps, which differ by the speed of the machine that day;
  * addresses, which differ every run. Trace.h already gives every object a
    stable number beside its address, which is the part that carries meaning;
  * the shim's own startup line saying which DLL it forwards to.

Floats are compared with a tolerance rather than as text, because a clock printed
as 0.30000001 against 0.3 is not a divergence. --tolerance sets it; 0 compares
the text exactly.

Both logs have to be recorded at the same LIBGR2_LOG_LEVEL, and the libgr2 one
needs a build configured with -DLIBGR2_TRACE=ON, or its per-call lines are
compiled out and every call reads as missing.
"""
import argparse
import difflib
import re
import sys

TIMESTAMP_RE = re.compile(r'^\[[0-9:.]+\]\s*')
ADDRESS_RE = re.compile(r'0x[0-9a-fA-F]+')
NUMBER_RE = re.compile(r'-?\d+\.\d+(?:e[-+]?\d+)?', re.I)
SHIM_NOISE_RE = re.compile(r'\[info\] shim:')


def normalise(line):
    line = TIMESTAMP_RE.sub('', line.rstrip('\n'))
    return ADDRESS_RE.sub('0xADDR', line)


def read(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        return [normalise(line) for line in f if not SHIM_NOISE_RE.search(line)]


def same(a, b, tolerance):
    """Equal, or equal once the floats in them are compared with a tolerance."""
    if a == b:
        return True
    if tolerance <= 0:
        return False
    if NUMBER_RE.sub('#', a) != NUMBER_RE.sub('#', b):
        return False
    for x, y in zip(NUMBER_RE.findall(a), NUMBER_RE.findall(b)):
        fx, fy = float(x), float(y)
        if abs(fx - fy) > tolerance * max(1.0, abs(fx), abs(fy)):
            return False
    return True


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('reference', help='the log the real granny2 produced, through the shim')
    parser.add_argument('candidate', help='the log libgr2 produced')
    parser.add_argument('--tolerance', type=float, default=1e-6,
                        help='relative tolerance for floats in a line (0 compares text)')
    parser.add_argument('--show', type=int, default=40, help='differing lines to print')
    args = parser.parse_args(argv[1:])

    a, b = read(args.reference), read(args.candidate)
    print('%s: %d lines' % (args.reference, len(a)))
    print('%s: %d lines' % (args.candidate, len(b)))

    # Line by line first, since two recordings of the same play line up until
    # they do not, and the first place they stop is the interesting one.
    shown = 0
    differing = 0
    first = None
    matcher = difflib.SequenceMatcher(
        None, a, b, autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == 'equal':
            continue
        # A one-for-one replacement whose lines only differ in float formatting
        # is not a difference.
        if tag == 'replace' and (i2 - i1) == (j2 - j1):
            pairs = list(zip(a[i1:i2], b[j1:j2]))
            if all(same(x, y, args.tolerance) for x, y in pairs):
                continue
        for offset in range(max(i2 - i1, j2 - j1)):
            x = a[i1 + offset] if i1 + offset < i2 else None
            y = b[j1 + offset] if j1 + offset < j2 else None
            if x is not None and y is not None and same(x, y, args.tolerance):
                continue
            differing += 1
            if shown < args.show:
                shown += 1
                if first is None:
                    first = i1 + offset + 1
                if x is not None:
                    print('  -%s' % x)
                if y is not None:
                    print('  +%s' % y)

    if differing == 0:
        print('\nidentical')
        return 0
    print('\n%d differing lines, first around line %d of %s'
          % (differing, first or 0, args.reference))
    return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
