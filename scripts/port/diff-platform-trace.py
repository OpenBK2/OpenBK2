#!/usr/bin/env python3
"""Compare a Windows and a Linux file trace and report the first divergence.

Paths are normalised first: separators folded to '/', case lowered, and the
'../' or './' noise dropped, because the two platforms legitimately spell the
same file differently and the question is which file, not how it was written.
"""
import io
import re
import sys


def load(path):
    out = []
    with io.open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            m = re.search(r'file: (opened|FAILED)\s+(\S+)\s+(.*)$', line)
            if not m:
                continue
            how, name = m.group(1), m.group(3)
            # strip a trailing "(reason)" from a failure
            name = re.sub(r'\s*\([^)]*\)\s*$', '', name).strip()
            name = name.replace('\\', '/').lower()
            name = re.sub(r'^(\.\./)+', '', name)
            name = re.sub(r'^\./', '', name)
            name = re.sub(r'^/.*?/(profiles|data)/', r'\1/', name)
            out.append((how, name))
    return out


def main():
    win = load(sys.argv[1])
    lin = load(sys.argv[2])
    print('windows: %d file events' % len(win))
    print('linux:   %d file events' % len(lin))
    print()

    n = min(len(win), len(lin))
    first = None
    for i in range(n):
        if win[i] != lin[i]:
            first = i
            break

    if first is None:
        print('identical for the first %d events; the shorter one just stops' % n)
        rest = win[n:] or lin[n:]
        who = 'windows' if len(win) > len(lin) else 'linux'
        print('%s continues with:' % who)
        for how, name in rest[:12]:
            print('   %-7s %s' % (how, name))
        return

    print('first divergence at event %d' % (first + 1))
    print()
    print('  ...context, both agree up to here:')
    for how, name in win[max(0, first - 5):first]:
        print('     %-7s %s' % (how, name))
    print()
    print('  windows: %-7s %s' % (win[first][0], win[first][1]))
    print('  linux:   %-7s %s' % (lin[first][0], lin[first][1]))
    print()
    print('  windows continues:')
    for how, name in win[first:first + 8]:
        print('     %-7s %s' % (how, name))
    print('  linux continues:')
    for how, name in lin[first:first + 8]:
        print('     %-7s %s' % (how, name))


if __name__ == '__main__':
    main()
