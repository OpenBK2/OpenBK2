#!/usr/bin/env bash
# Compare two builds of oodle1-corpus-bench and say whether the difference is
# real. Pins, interleaves, and reports the minimum.
#
# Usage:
#   scripts/port/oodle1-measure.sh <bench_a> <bench_b> <gr2-dir> [pairs]
#   scripts/port/oodle1-measure.sh <bench_a> <bench_a> <gr2-dir> [pairs]
#
# The second form, the same binary against itself, measures this machine's noise
# floor. Do that before believing any small delta; on the machine this was
# written on it came out at 0.19 per cent on the minimum, so the threshold
# adopted for a claim was one per cent with the minimum and the median agreeing
# on the sign.
#
# WHY NOT JUST RUN EACH ONE N TIMES. Three variance sources, in the order they
# bite:
#
# 1. HYBRID CPUS. On a 12th generation Core and later, an unpinned run migrates
#    between P-cores and E-cores mid-measurement and the result is a blend of two
#    different machines. perf confirmed it happening: an unpinned run of this
#    benchmark attributed 1.67 per cent of its cycles to cpu_atom. The standard
#    benchmarking guides predate hybrid x86 and do not mention this, and it is
#    the single highest-value control here, ahead of the governor and turbo.
#    /sys/devices/cpu_core/cpus lists the P-cores; do not pin to an E-core.
#
# 2. DRIFT. Thermals, and anything else running. Running all of A and then all of
#    B measures A's weather and B's weather. So this interleaves A B A B, which is
#    the single-machine form of running a reference and a test simultaneously.
#    This is not ceremony. In the run that calibrated the floor, the first four
#    pairs came in at 1144-1327 ms and the last five at 1124-1150 ms because the
#    load changed partway through; both binaries saw the same weather so the
#    comparison survived, where A-then-B would have reported a seven per cent
#    improvement that did not exist.
#
# 3. ASLR, which moves code and data layout run to run. setarch -R pins it and
#    needs no root, unlike writing /proc/sys/kernel/randomize_va_space.
#
# Worth doing as well, all needing root, none of them done here:
#   cpupower frequency-set --governor performance
#   echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo
#   cset shield -c <cores> -k on

set -u

A="${1:-}"
B="${2:-}"
DIR="${3:-}"
PAIRS="${4:-9}"

# Override when anyone else is measuring: two sessions pinned to the same core
# produced a *noise floor* of minus 15 per cent, with runtimes of 2.2-4.3 s
# against the usual 1.15. The check below catches the common case only.
PIN="${PIN:-6}"

if [ -z "$A" ] || [ -z "$B" ] || [ -z "$DIR" ] || [ ! -x "$A" ] || [ ! -x "$B" ]; then
	echo "usage: oodle1-measure.sh <bench_a> <bench_b> <gr2-dir> [pairs]" >&2
	exit 2
fi

# Both the target core AND ITS SMT SIBLING have to be idle. Two benchmarks on one
# physical core share its execution units, which is not a small effect: one such
# collision produced a measured *noise floor* of minus 15 per cent, and a second
# made a real 11 per cent win read as a 184 per cent regression. This check
# originally compared the processor id exactly and so was blind to the sibling.
SIBLINGS="$(tr ',-' ' ' < /sys/devices/system/cpu/cpu$PIN/topology/thread_siblings_list 2>/dev/null)"
SIBLINGS="${SIBLINGS:-$PIN}"
BUSY=""
for cpu in $SIBLINGS; do
	found="$(ps -eo pid=,psr=,comm= 2>/dev/null \
		| awk -v p="$cpu" -v me="$$" '$2 == p && $1 != me && $3 ~ /oodle|bench/ {print $3}' \
		| sort -u | tr '\n' ' ')"
	[ -n "$found" ] && BUSY="$BUSY cpu$cpu:$found"
done
if [ -n "$BUSY" ]; then
	echo "REFUSING:$BUSY" >&2
	echo "  cpu$PIN shares a physical core with:$SIBLINGS" >&2
	echo "  pick a P-core whose sibling is also idle, with PIN=n $0 ..." >&2
	exit 3
fi

run_one() {
	setarch "$(uname -m)" -R taskset -c "$PIN" "$1" "$DIR" 1 2>/dev/null \
		| awk '/^best/ {print $2}'
}

echo "pinned to cpu$PIN, ASLR off, $PAIRS interleaved pairs"
echo "A = $A"
echo "B = $B"
echo

: > /tmp/.oodle_a
: > /tmp/.oodle_b
for i in $(seq 1 "$PAIRS"); do
	a="$(run_one "$A")"
	b="$(run_one "$B")"
	echo "$a" >> /tmp/.oodle_a
	echo "$b" >> /tmp/.oodle_b
	printf '  pair %-2d  A %9s ms   B %9s ms\n' "$i" "$a" "$b"
done

python3 - "$A" "$B" <<'PY'
import statistics
import sys

a = [float(x) for x in open('/tmp/.oodle_a') if x.strip()]
b = [float(x) for x in open('/tmp/.oodle_b') if x.strip()]
if not a or not b:
    print("no timings captured; did the benchmark print a 'best' line?")
    raise SystemExit(1)

def describe(name, xs):
    return "%-30s min %8.1f  median %8.1f  spread %5.2f%%" % (
        name[-30:], min(xs), statistics.median(xs),
        100.0 * (max(xs) - min(xs)) / min(xs))

print()
print(describe(sys.argv[1], a))
print(describe(sys.argv[2], b))

d_min = 100.0 * (min(b) - min(a)) / min(a)
d_med = 100.0 * (statistics.median(b) - statistics.median(a)) / statistics.median(a)
print()
print("delta on min     %+6.2f%%   (negative means B is faster)" % d_min)
print("delta on median  %+6.2f%%" % d_med)
print()
if len(a) < 5:
    print("TOO FEW PAIRS to verdict: %d. A pass of this corpus takes seconds, so" % len(a))
    print("  a two or three pair run has a noise floor of several per cent. Use 5 or")
    print("  more, and measure the floor first by passing the same binary twice.")
elif (d_min < 0) != (d_med < 0):
    print("VERDICT: min and median disagree on the sign. This is noise.")
elif max(abs(d_min), abs(d_med)) < 1.0:
    print("VERDICT: under one per cent. At the usual noise floor, call it no change.")
else:
    print("VERDICT: consistent direction, above the one per cent threshold.")
PY
