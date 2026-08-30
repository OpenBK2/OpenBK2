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

`--only <regex>` compares just the lines that match, and narrows a trace to one
entry point when chasing something specific.

`--manifest` is the mode to reach for after two recordings of a *played* session,
and it does not compare positions at all. A trace of real play only lines up
while the work is deterministic, which is startup and mission load; once frames
begin the model clock follows wall-clock time and the two recordings separate
however well either implementation behaves. Worse, two sessions load the same
resources in a slightly different order, so a positional diff of the file
manifest reports the permutation and buries the question worth asking.

    python gr2logdiff.py vanilla.log libgr2.log --manifest

matches files by their content hash instead, and asks: of the files both runs
loaded, did the two implementations parse each into the same skeletons, bones,
models, meshes, animations and track groups? That answer holds no matter how
differently the two sessions were played, and over a mission it is a cross-check
of the loader against several hundred real resources.

Both logs have to be recorded at the same LIBGR2_LOG_LEVEL, and the libgr2 one
needs a build configured with -DLIBGR2_TRACE=ON, or its per-call lines are
compiled out and every call reads as missing.

Streaming, with a bounded resynchronisation window, because these logs run to
gigabytes: a minute of play is sixteen million lines, and holding two of those in
memory to hand to difflib is several times what the machine has. The window is
what lets one side skip a run of extra lines and the comparison pick up again
afterwards; a divergence wider than the window ends the comparison rather than
pretending to align across it.
"""
import argparse
import re
import sys

TIMESTAMP_RE = re.compile(r'^\[[0-9:.]+\]\s*')
ADDRESS_RE = re.compile(r'0x[0-9a-fA-F]+')
NUMBER_RE = re.compile(r'-?\d+\.\d+(?:e[-+]?\d+)?', re.I)
SHIM_NOISE_RE = re.compile(r'\[info\] shim:')


def normalise(line):
    line = TIMESTAMP_RE.sub('', line.rstrip('\n'))
    return ADDRESS_RE.sub('0xADDR', line)


def lines_of(path, only):
    """Normalised lines, lazily, skipping the shim's own startup chatter."""
    keep = re.compile(only) if only else None
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if SHIM_NOISE_RE.search(line):
                continue
            if keep is not None and not keep.search(line):
                continue
            yield normalise(line)


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


class Side(object):
    """One log, read lazily, with a lookahead buffer and a line counter."""

    def __init__(self, path, only):
        self.path = path
        self.source = lines_of(path, only)
        self.buffer = []
        self.consumed = 0
        self.exhausted = False

    def fill(self, n):
        while len(self.buffer) < n and not self.exhausted:
            try:
                self.buffer.append(next(self.source))
            except StopIteration:
                self.exhausted = True

    def peek(self, i=0):
        self.fill(i + 1)
        return self.buffer[i] if i < len(self.buffer) else None

    def take(self):
        self.fill(1)
        if not self.buffer:
            return None
        self.consumed += 1
        return self.buffer.pop(0)

    def drain(self):
        """Count whatever is left, for the tail report."""
        n = 0
        while self.take() is not None:
            n += 1
        return n


def find_ahead(side, target, window, tolerance):
    """How far ahead in side the target line appears, or None."""
    for i in range(1, window + 1):
        line = side.peek(i)
        if line is None:
            return None
        if same(line, target, tolerance):
            return i
    return None


def compare(a, b, tolerance, window, show, limit):
    differing = 0
    shown = 0
    first = None
    matched = 0
    gave_up = False

    while True:
        # Past a real fork there is nothing more to learn, and every mismatched
        # line costs a full resynchronisation window of comparisons. Measured the
        # hard way: two recordings of separately played sessions diverge early,
        # and without this the run spent twenty-five minutes on a pair of traces
        # and would not have finished. The number that matters is where they
        # first stopped agreeing, and that is already known by this point.
        if differing >= limit:
            gave_up = True
            break
        x, y = a.peek(), b.peek()
        if x is None or y is None:
            break
        if same(x, y, tolerance):
            a.take()
            b.take()
            matched += 1
            continue

        if first is None:
            first = (a.consumed + 1, b.consumed + 1)

        # One side has extra lines. Whichever resynchronises sooner is the one
        # that gained them, which is the reading that keeps the comparison going
        # instead of declaring everything after the first hiccup different.
        ahead_in_b = find_ahead(b, x, window, tolerance)
        ahead_in_a = find_ahead(a, y, window, tolerance)

        if ahead_in_b is not None and (ahead_in_a is None or ahead_in_b <= ahead_in_a):
            for _ in range(ahead_in_b):
                line = b.take()
                differing += 1
                if shown < show:
                    shown += 1
                    print('  +%s' % line)
        elif ahead_in_a is not None:
            for _ in range(ahead_in_a):
                line = a.take()
                differing += 1
                if shown < show:
                    shown += 1
                    print('  -%s' % line)
        else:
            # Neither side comes back within the window. Report the pair and
            # step both, which is right for a substitution and gives up
            # gracefully on a genuine fork.
            differing += 1
            if shown < show:
                shown += 1
                print('  -%s' % a.take())
                print('  +%s' % b.take())
            else:
                a.take()
                b.take()

    if gave_up:
        # Not drained. Counting the rest of two gigabyte logs says nothing once
        # the comparison has given up on them.
        return differing, first, matched, None, None
    return differing, first, matched, a.drain(), b.drain()


MANIFEST_RE = re.compile(
    r'\[info\] file#(\d+) ([0-9a-f]{16}) "([^"]*)" (\d+) bytes: '
    r'skeletons=(\d+) bones=\[([^\]]*)\] models=(\d+) meshes=(\d+) '
    r'animations=(\d+) trackgroups=(\d+)')

#! What a manifest line says a file turned into, in the order it is printed.
SHAPE_FIELDS = ('size', 'skeletons', 'bones', 'models', 'meshes', 'animations',
                'trackgroups')


def read_manifest(path):
    """Every file a run loaded, keyed by content hash rather than by position."""
    out = {}
    order = []
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '[info] file#' not in line:
                continue
            m = MANIFEST_RE.search(line)
            if m is None:
                continue
            digest, source = m.group(2), m.group(3)
            shape = (m.group(4), m.group(5), m.group(6), m.group(7), m.group(8),
                     m.group(9), m.group(10))
            if digest in out:
                # The same resource is loaded more than once in a run, and every
                # load of the same bytes has to produce the same answer. If one
                # does not, that is worth more than anything the comparison
                # against the other implementation will say.
                if out[digest][0] != shape:
                    print('  %s parsed two different ways within %s' % (digest, path))
                continue
            out[digest] = (shape, source)
            order.append(digest)
    return out, order


def describe(shape):
    return ' '.join('%s=%s' % (name, value) for name, value in zip(SHAPE_FIELDS, shape))


def compare_manifests(reference, candidate, show):
    ref, ref_order = read_manifest(reference)
    cand, _ = read_manifest(candidate)
    both = [d for d in ref_order if d in cand]

    print('%s: %d distinct files' % (reference, len(ref)))
    print('%s: %d distinct files' % (candidate, len(cand)))
    print('loaded by both: %d' % len(both))

    disagree = [d for d in both if ref[d][0] != cand[d][0]]
    print('\nparsed differently by the two implementations: %d' % len(disagree))
    for d in disagree[:show]:
        print('  %s  %s' % (d, ref[d][1]))
        print('    reference %s' % describe(ref[d][0]))
        print('    candidate %s' % describe(cand[d][0]))

    # Not a fault. Two played sessions see different units and stream different
    # resources, so this is a measure of how alike the two sessions were, not of
    # how alike the implementations are.
    only_ref = [d for d in ref_order if d not in cand]
    only_cand = [d for d in cand if d not in ref]
    print('\nloaded only by the reference run: %d' % len(only_ref))
    for d in only_ref[:show]:
        print('  %s  %s' % (d, ref[d][1]))
    print('loaded only by the candidate run: %d' % len(only_cand))
    for d in only_cand[:show]:
        print('  %s  %s' % (d, cand[d][1]))

    return 1 if disagree else 0


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('reference', help='the log the real granny2 produced, through the shim')
    parser.add_argument('candidate', help='the log libgr2 produced')
    parser.add_argument('--tolerance', type=float, default=1e-6,
                        help='relative tolerance for floats in a line (0 compares text)')
    parser.add_argument('--only', default=None,
                        help='compare only lines matching this regex, e.g. "GrannyBuildWorldPose"')
    parser.add_argument('--manifest', action='store_true',
                        help='compare the loaded files by content hash instead of by position')
    parser.add_argument('--window', type=int, default=200,
                        help='lines to look ahead when resynchronising (default 200)')
    parser.add_argument('--show', type=int, default=40, help='differing lines to print')
    parser.add_argument('--max-diff', type=int, default=1000, dest='limit',
                        help='stop after this many differing lines (default 1000)')
    args = parser.parse_args(argv[1:])

    if args.manifest:
        return compare_manifests(args.reference, args.candidate, args.show)

    a = Side(args.reference, args.only)
    b = Side(args.candidate, args.only)
    differing, first, matched, tail_a, tail_b = compare(a, b, args.tolerance,
                                                        args.window, args.show,
                                                        args.limit)

    print('\n%s: %d lines compared' % (args.reference, a.consumed))
    print('%s: %d lines compared' % (args.candidate, b.consumed))
    print('%d lines matched' % matched)
    if tail_a is None:
        print('gave up after %d differing lines, the rest of both logs unread'
              % differing)
    elif tail_a or tail_b:
        print('%d lines left over in %s, %d in %s, after the comparison stopped'
              % (tail_a, args.reference, tail_b, args.candidate))
    if differing == 0 and not tail_a and not tail_b:
        print('identical')
        return 0
    print('%d differing lines' % differing)
    if first is not None:
        print('first difference at line %d of %s, line %d of %s'
              % (first[0], args.reference, first[1], args.candidate))
    return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
