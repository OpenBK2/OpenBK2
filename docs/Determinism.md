# Determinism hazards and the sites that still carry them

Multiplayer and replays are lockstep: only commands cross the wire, every client
runs the same simulation, and any divergence is an ASYNC. The goal this file
serves is cross-platform multiplayer, which means a Windows client and a Linux
client have to compute bit-identical simulation state from identical input.

This is a record of what threatens that, which of it is detectable and by what,
and the concrete sites found so far.

Most of the fixes want the replay harness first, because they change simulation
output on at least one platform by definition and cannot be verified without it.
The exceptions are the ones that touch no simulation state, and those are marked
**fixed** below as they land.

## What the language guarantees, and where the guarantee stops

The as-if rule lets a compiler perform any transformation that preserves
observable behaviour. That makes a whole category of worry unfounded: dead store
elimination, `memset` removal, and shortening a `std::transform` whose results
are never read cannot change simulation results, because the transformation is
legal only when nothing can observe the difference. Reading a value into a
checksum is an observation, so anything that feeds a checksum is by definition
preserved.

Two things break that reassurance.

**Copy elision is an explicit exception.** `[class.copy.elision]` permits eliding
a copy or move constructor "even if the constructor selected for the copy/move
operation and/or the destructor for the object have side effects". So a
side-effecting copy constructor may or may not run, and which happens varies by
compiler, by optimisation level, and by standard version, since C++17 made the
prvalue cases mandatory. This is benign here today only because `CPtr` and `CObj`
copy constructors just move refcounts around, and refcounts are not simulation
state.

**Undefined behaviour voids the whole model.** The compiler preserves observable
behaviour only under the assumption that the program has no UB. Where it does,
the optimiser's assumptions are unsound, there is nothing it is obliged to
preserve, and the result can be anything. Two compilers will disagree, and one
compiler at two optimisation levels will disagree with itself. For C++03-era code
with heavy casting this is the single largest threat to cross-platform agreement,
larger than floating point.

There is no authoritative enumeration of UB in C++. The standard scatters it
across hundreds of clauses and the community catalogues, `ubbook` among them, are
incomplete by their own admission. So "we checked the list" is not a strategy
available to us, and the plan has to assume unknown UB remains.

## Hazard classes, in descending order of risk here

These are categories of defect. The detection tiers further down are a separate
axis: a hazard class can sit in any of them.

**1. Undefined behaviour.** Uninitialised reads, signed overflow, strict aliasing
violations, misaligned loads, out-of-range float to int conversions, library
precondition violations. Partly detectable, see below.

**2. Unspecified behaviour.** The standard lists the permitted outcomes but not
which one you get. Evaluation order of function arguments and of most operator
operands is the case that bites: two side-effecting calls in one expression may
run in either order. MSVC has historically gone right to left, GCC and clang left
to right. **No sanitizer detects this.**

**3. Implementation-defined behaviour.** `sizeof`, alignment, whether `char` is
signed, the layout of bitfields. Documented per implementation and therefore
different between them. No sanitizer detects it either.

**4. Licensed floating-point deviation.** FP contraction and excess intermediate
precision are explicitly allowed to change results. This is the one tier that is
fully fixable by flags.

**5. Timing.** None of the above, and worse than all of them: the simulation
reads something whose value is a question about when, not about what. Resource
loading runs on a background thread (`System/GResource.cpp:150`) and readiness
is polled rather than waited on (`GResource.h:200`, `NeedUpdate` returns true
only once `pRequest->IsReady()`), so "has this loaded yet" has a different
answer on a fast machine and a slow one. Nothing detects this, no flag fixes it,
and unlike every other class on this list it does not reproduce: the same binary
on the same input gives a different answer twice.

The one instance chased to the end turned out not to be this class at all: an
uninitialised hash seed was dropping events, and it was the stack contents
rather than the clock that varied. The class is still real, but a run that does
not reproduce is not on its own evidence of it, and reading it that way cost
several hours. Check for an unwritten variable first.

## Detection, in four tiers

Detection capability is not uniform, and knowing which tier a hazard falls into
decides whether the answer is "run a tool", "write a tool", or "make the
construct unrepresentable".

### Tier 1: off-the-shelf dynamic analysis

Catches a lot, for the cost of a build and a slow run. Needs the code path to
actually execute, which is why it is worth pointing at the replay corpus rather
than only at unit tests.

| instrument | finds | status here |
| --- | --- | --- |
| ASan | heap and stack overflow, use after free, use after scope | `Linux-x64-ASan+UBSan` |
| UBSan | signed overflow, misaligned access, bad casts, out-of-range float to int | same preset |
| UBSan `integer` and `implicit-conversion` groups | implicit truncation and sign change. Not UB, but behaviour hazards, and this code converts between `int`, `short` and `float` constantly. Verified available | not enabled yet |
| TSan | data races | `Linux-x64-TSan` |
| TypeSanitizer | strict aliasing violations | works via `clang-21`, see below |
| MemorySanitizer | uninitialised reads | runtime exists for clang-19 to clang-21, but needs every dependency instrumented including the standard library, DXVK and SDL3. Impractical for the game; usable for unit tests |
| Valgrind, heaptrack | leaks, footprint | lower priority, ASan is a faster superset for memory errors; heaptrack is worth one footprint audit |
| HWASan, tagged pointers | memory errors via pointer tagging | **not available on this target.** It links only with `-Wl,--no-relax` or lld and then aborts at runtime: "HWAddressSanitizer requires a kernel with tagged address ABI". That is an AArch64 TBI and MTE feature; x86-64 Linux has no tagged address ABI. Do not retry |

**TypeSanitizer specifics**, measured on this machine rather than quoted:

- It uses shadow memory, one pointer-sized slot per application byte, so peak RSS
  measured 8.35x baseline against ASan's 1.23x. Time was 6.3x on a
  memory-traffic-dominated loop, which is close to worst case.
- It needs its own tree. The driver rejects `-fsanitize=type,address` and
  `type,thread` outright. `type,undefined` links only to fail on duplicate
  symbols between `libclang_rt.tysan` and `libclang_rt.ubsan_standalone`. The one
  working fusion is `-fsanitize=type,undefined -fsanitize-trap=undefined`, which
  costs UBSan's diagnostic text, since trap mode reports as bare SIGILL.
- The runtime is installed for clang-20 and clang-21 only; the clang-22 snapshot
  has no `libclang_rt.tysan`, so a TySan build must name `clang-21`.
- It wants the whole program instrumented. Uninstrumented writes leave the shadow
  stale, giving both false positives and missed detections. Milder than MSan's
  version of this problem, because TySan's "unknown type" state is not an error
  where MSan's "uninitialised" state is, so it still returns value with only our
  own translation units instrumented.
- It matters here because **the build sets no `-fno-strict-aliasing`**, so both
  compilers are actively exploiting strict aliasing over code written before
  anyone cared. Static detection of this class is poor: GCC's own documentation
  concedes `-Wstrict-aliasing=3` misses a great deal. That gap is the reason
  TySan exists.

### Tier 2: off-the-shelf static analysis

Cheap, runs on code that never executes, and catches things dynamic analysis
needs coverage to reach. Also weaker on anything flow-sensitive.

- Compiler warnings from **both** compilers. The sets differ, which is a
  standing argument for the clang secondary build.
- clang-tidy. No `.clang-tidy` exists in the tree yet, so the configuration is
  greenfield.
- The clang static analyzer, and MSVC `/analyze`.

### Tier 3: no instrument exists, so build one

This is where the `isalnum` class lives. `std::isalnum(ch)` is UB unless the
argument is representable as `unsigned char` or equals EOF, and a negative
`char` sign-extends. No sanitizer sees it, because it is a library precondition
violation rather than an instrumented language construct. The options, roughly in
increasing cost:

1. **Make it unrepresentable.** Usually the cheapest and the only *complete*
   answer. For this case: a project-local `NStr::IsAlnum( char )` that casts
   through `unsigned char`, then ban the raw `<cctype>` functions. The bug stops
   existing rather than being detected.
2. **Hardened standard library.** `_GLIBCXX_ASSERTIONS` for libstdc++,
   `_LIBCPP_HARDENING_MODE` for libc++, `_ITERATOR_DEBUG_LEVEL` for MSVC. These
   catch a large slice of library precondition UB: `operator[]` out of range,
   invalid iterators, `front()` on an empty container. **None of these is enabled
   today**, and `_GLIBCXX_ASSERTIONS` is cheap enough to leave on in debug
   builds. It does not cover `isalnum`, but it covers the container half of the
   same category for one compile definition.
3. **A custom clang-tidy check using AST matchers.** Exact, because it has the
   full type information. The right tool for type-dependent rules.
4. **semgrep or a grep guard in CI.** Crude but immediate. The honest limitation
   is that it matches syntax, not types, so it cannot reliably tell
   `isalnum( someChar )` from `isalnum( someInt )`. Good for banning identifiers
   outright, bad for conditional rules.

Custom rules worth writing for this tree specifically:

- Two or more `NRandom::Random` calls inside one full-expression. A regex already
  found six sites, so even the crude version works; an AST matcher would be
  exact.
- Raw `<cctype>` functions applied to a `char`.
- The forbidden-construct guard: `#include <execution>`, `std::execution`,
  `std::reduce`, `std::transform_reduce`, `tbb::`, `rxcpp` under the simulation
  directories.

### Tier 4: nothing can detect it, so pin it or diff it

Two things survive all of the above.

**Implementation-defined choices** are legal everywhere and therefore never
diagnosed: whether `char` is signed, `sizeof` and alignment, bitfield layout. The
answer is not detection but `static_assert` on the assumptions the code actually
relies on, so a platform that differs fails to build instead of failing to agree.

**Unknown UB and unspecified behaviour we have not thought of.** There is no
authoritative list to work through, so some residue is guaranteed. The only
instrument that covers the unknown is differential execution: run the same
recorded game under deliberately different conditions and compare per-segment
checksums.

- Windows against Linux.
- libstdc++ against libc++, which changes `std::hash`, container internals and
  `std::sort` tie-breaking all at once.
- gcc against clang, and one compiler at two optimisation levels, which is the
  sharpest test for UB specifically: where the program is well defined the
  optimiser must agree with itself.
- The same build twice, which catches dependence on address values.

No unit test substitutes for this, because the defect is not a wrong result from
a function but which of several legal behaviours the program happened to get.

#### Deliberately hostile but conforming configurations

The strongest form of tier 4 is to run against an implementation that makes
unusual choices which the standard permits but no mainstream platform exhibits:
unusual type sizes, unusual `char` signedness, unusual limits. The canonical name
for that thought experiment is the **DeathStation 9000**.

As a literal toolchain it is not available. The real tools that execute the
standard's semantics and report undefined behaviour directly, RV-Match and
`kcc`, and Cerberus, are **C only**, so a C++03-era C++ codebase is out of
scope for them. Writing a hostile C++ implementation is not a project anyone
should start. What is achievable is varying the choices real toolchains already
let you vary, and the useful distinction is whether an axis preserves memory
layout, because `CCheckSumSaver` hashes raw bytes and therefore only
layout-preserving axes stay checksum-comparable.

**Layout-preserving, checksums compare directly. Cheap, and worth doing.**

- `-funsigned-char`, which is what ARM does by default. Verified to work on gcc
  here. Directly targets the `isalnum` family.
- libstdc++ against libc++. Safe for checksums because `CCheckSumSaver` walks
  fields through `IBinSaver::operator&` rather than hashing container internals.
- `-O0` against `-O2` against `-O3` with one compiler. The sharpest UB test
  available, because where the program is well defined the optimiser is obliged
  to agree with itself.
- `USE_MIMALLOC` on and off, which moves every address and so catches dependence
  on address values.
- gcc against clang.

**Layout-changing. Compare gameplay outcomes, not checksums.**

- `-fshort-enums`, the ARM EABI default, and `-fshort-wchar`, which makes Linux
  match Windows' 2-byte `wchar_t`. Both verified available on gcc here. Both
  change struct sizes, so they also break ABI against prebuilt dependencies.
- x86 ILP32 against x86-64 LP64, which the build already produces and which
  `ESaverMode` already acknowledges with its 32- and 64-bit saver variants.

**Cross-architecture under qemu-user, which is the real prize.**

`aarch64` is the sweet spot. It is LP64 and little-endian like x86-64, so layout
is largely preserved and checksums have a real chance of being directly
comparable, while it simultaneously varies `char` signedness, alignment
strictness and libm. No simulation module uses `long double` anywhere, so the
x86 80-bit against aarch64 128-bit quad difference does not arise. That makes it
an excellent acceptance test for the vendored libm work specifically: once libm
is vendored, aarch64 and x86-64 should agree bit for bit, and if they do not, the
vendoring is incomplete.

It is feasible only because `AILogic` does not link the renderer. `ReplayRunner`
can be cross-compiled where `Game` cannot, since there is no D3D9, DXVK or
windowing in its dependency set. `qemu-user-static` and `gcc-aarch64-linux-gnu`
are both one `apt install` away and `binfmt_misc` is already mounted, so the
cost is not qemu: it is cross-building the FetchContent dependency graph, which
is days of work and gated on `ReplayRunner` existing.

**Big-endian is the most revealing axis and the one to skip.** `s390x` would
expose every serialisation and punning assumption at once, and
`gcc-s390x-linux-gnu` is available. But `IBinSaver` writes raw memory, so saves
and replays would not load at all without making the format endian-aware. That
is a large project with no player-facing payoff, and it would have to be done
before the test could run rather than as a result of it.

## Sites found so far

Not fixed. Each needs a replay comparison before and after, because every one of
them changes simulation output on at least one platform by definition.

### Unspecified evaluation order with RNG side effects

`NRandom` is one sequential stream, so the order and the count of draws are both
part of simulation state.

- **`AILogic/Scripts.cpp:1597`** and **`AILogic/Scripts.cpp:1628`** - live, in
  scenario command dispersion feeding `GroupCommand`:
  `tmpCmd.vPos = command.vPos + NRandom::Random( fDispersion ) * GetVectorByDirection( NRandom::Random( 0, 65535 ) );`
  Two draws in one expression, order unspecified. Fix by hoisting both into named
  locals so the sequence is pinned.
- `AILogic/AILogicInternal.cpp:1783`, `1784`, `1800`, `1801` - the same pattern in
  `static void PathfinderTest(...)`, a console debug command. Harmless in play,
  worth fixing for consistency.

### Floating point

- **`AILogic/CMakeLists.txt:4`** - `add_compile_options("/fp:strict")` is inside
  `if(MSVC)`, so the GCC and clang builds of the simulation have no
  float-determinism flag. The counterpart is `-ffp-contract=off`. GCC defaults to
  `-ffp-contract=fast` and clang to `=on`, so this matters as soon as FMA is
  reachable.
- **`cmake/arch.cmake`** - MSVC-only as well, so `ARCHITECTURE` is inert on GCC
  and clang. Wiring it up without the flag above would introduce FMA and with it
  divergence.
- Open question: 32-bit x87 computes float expressions at 80-bit internal
  precision and then rounds, which differs from SSE2 for identical source.
  `AILogic` is float-dominated, 2666 uses of `float` against 10 of `double`.
  MSVC has defaulted to `/arch:SSE2` on x86 since VS2012 so this is probably
  already safe, but it is unverified and `arch.cmake` passes nothing by default.
  Moot if x86 and x64 clients never need to play each other.

### libm

`sqrt` and `fmod` are exactly specified by IEEE-754 and are already
bit-identical. They need no replacement. The rest are not identical between the
MSVC CRT and glibc:

- `AILogic`: `exp` 9 sites, `tan` 6, `sin` 3, `log`, `acos`, `atan2f`.
- `B2_M1_World` and `Common_RTS_AI`: `atan2f` 4, `exp` 1.

Two strategies. Vendor one portable implementation and compile it identically on
both platforms, openlibm or musl's libm, which requires holding `-ffp-contract=off`
on those translation units. Or take a correctly-rounded implementation,
CORE-MATH, where the result is uniquely defined by the standard and therefore
identical by construction rather than by compilation discipline.

### Unstable sorting

22 `std::sort` calls in simulation modules and zero `std::stable_sort`:
`AILogic` 17, `Common_RTS_AI` 3, `B2_M1_World` 1, `B2_M1_Terrain` 1. `std::sort`
is not stable, so wherever a comparator admits ties the relative order of equal
elements is unspecified and differs between libstdc++, libc++ and MSVC. This is
22 comparators to read rather than 22 bugs, since it only bites where ties are
reachable. Fix by making the order total, tie-breaking on a unique id, or by
using `stable_sort` where the partial order is deliberate.

### Container iteration

`AILogic` is already clean: 121 `det_map`, 35 `det_set`, no unordered containers
at all, and its 7 `std::map` plus 1 `std::set` are keyed on `int`, which iterates
in key order and is portable. Two of them carry comments saying so.

`CCheckSumSaver` was checked and is safe. It holds a pointer-keyed
`std::unordered_map` but touches it only through `find`; the checksum walk
iterates `addOrder`, a `std::list` in insertion order.

The residue is elsewhere, and only *iterated* sites matter, since a hash
container used purely for lookup is deterministic: `B2_M1_World` 26 plus 1,
`libdb` 21 plus 1, `Stats_B2_M1` 18, `Common_RTS_AI` 9, `System` 37 plus 9. The
libstdc++ against libc++ replay comparison is a cheaper way to find the ones that
matter than reading all 123 declarations.

### Type punning

The punning surface in the simulation is small: `AILogic` has 1
`reinterpret_cast` and no unions, `B2_M1_World` none. The weight is in `System`
(12 casts, 4 unions) and `libdb` (5 casts, 2 unions).

One site was worth naming because of where it sat, and it turned out to be dead
code. `Common_RTS_AI/Checksums.h`, in the two-argument
`NCheckSums::GetCRC( newCheckSum, oldCheckSum )`:

```cpp
static union { uLong checkSum; uint8_t array[ sizeof(uLong) ]; };
checkSum = oldCheckSum;
return crc32( newCheckSum, array, sizeof(uLong) );
```

Writing `checkSum` and reading `array` is reading a non-active union member and
therefore UB, the `static` made it non-reentrant, and `uLong` is `unsigned long`,
so `sizeof(uLong)` is 8 on Linux against 4 on Windows and it fed `crc32` a
different byte count on each platform. **Fixed by deleting it**: nothing called
it, so removal was safer than repair. The rest of the namespace is live and
stays, since `SCheckSumBufferStorage`, `CopyToBuf` and the one-argument `GetCRC`
are used by the explosion checksums in `AILogic/Shell.cpp`.

That `uLong` detail generalises into an open audit item. `CCheckSumSaver::DataChunk`
hashes `nSize` raw bytes where `nSize` comes from the field's `sizeof`, so **any
`long` or `unsigned long` member reached by the checksum walk contributes 8 bytes
on Linux and 4 on Windows.** That has to be swept before cross-platform
agreement is possible, and it is not a trivial fix.

The other places TySan is likely to earn its keep are `IBinSaver`'s
`DataChunk( void *pData, int nSize )` raw-memory serialisation, the cross-DLL
`CastToObjectBase` and `CastToUserObject` machinery, and `3Dmotor`'s MMX and SIMD
reinterpretation, though the last is presentation and outside lockstep.

### A per-process RNG in AI air scouting (fixed)

`AILogic/GeneralAirForce.cpp`, `CGeneralAirForce::LaunchScoutFree`, built the
list of unseen tiles to scout and shuffled it with

```cpp
std::random_device rd;
std::mt19937 g(rd());
std::shuffle( points.begin(), points.end(), g );
```

then issued a `GroupCommand` per point. The order of those commands is
simulation state, and `std::random_device` makes it per-process, so two clients
sent their scouts to the same tiles in a different order: an ASYNC on any map
where the AI calls air reconnaissance, in multiplayer and in replays alike.

This is a **port regression**, not an original defect. `515090eb6` ("get rid of
vendored copy of Hewlett-Packard C++ standard library from 1999") replaced
`random_shuffle( points.begin(), points.end(), pr )` over
`SGeneralHelper::SRandomFunctor` -- which draws from the shared seeded generator
and calls `RecordRandomCall()` -- with the standard engine, because C++17
removed `std::random_shuffle` and `std::shuffle` takes a URBG rather than the
old `operator()( int N )` functor. The functor was left in place at
`AILogic/GeneralHelper.h:21` and is what the fix uses again.

Fixed by writing out the shuffle the vendored `random_shuffle` performed, so the
draw sequence is the one the original engine made:

```cpp
SGeneralHelper::SRandomFunctor pr;
for ( size_t i = 1; i < points.size(); ++i )
    std::iter_swap( points.begin() + i, points.begin() + pr( static_cast<int>( i ) + 1 ) );
```

Two things generalise from it.

**The whole tree was swept for the same substitution and this was the only
simulation site.** The other `std::random_device` uses are seed sources and
legitimate: `System/RandomGenInternal.cpp:273` seeds ISAAC at the start of a
game, which is recorded in the replay, and `3Dmotor/RandomGen.cpp:162` is the
presentation generator.

**No instrument from "Detection" would have caught it.** It is not a
divergence between two builds of the same binary, so the replay harness only
sees it if the two runs are different *processes* and the AI actually calls a
scout. The detector is a grep, and the rule it enforces -- `<random>` has no
place in a simulation translation unit -- is now a comment at the top of that
file. It belongs in "Rules for new and modernised code" and in whatever lint
eventually encodes them: **C++17 removing an algorithm is a determinism event**,
because the mechanical replacement is nondeterministic by default.

### The RNG debug checksums cannot match across platforms (fixed)

`System/RandomGenInternal.cpp`, `CalcDebugChecksum`, is FNV-1a with an
`unsigned long` accumulator:

```cpp
static unsigned long CalcDebugChecksum( const uint32_t *pValues, const int nCount )
{
    unsigned long nChecksum = 2166136261u;
    for ( int i = 0; i < nCount; ++i ) { nChecksum ^= pValues[i]; nChecksum *= 16777619u; }
    return nChecksum;
}
```

`unsigned long` is 32 bits on Windows and 64 on Linux. On Windows the multiply
wraps modulo 2^32 and produces canonical FNV-1a-32; on Linux the carries
propagate into the upper 32 bits and the value diverges after the first
iteration. `SDebugState::randrslChecksum` and `randmemChecksum` are
`unsigned long` too.

So those two fields can **never** agree between a Windows and a Linux client,
and they are printed by the RNG state dump in `AILogic/AILogicInternal.cpp`,
whose adjacent comment states that the file's "entire purpose is being compared
against another machine's copy". The same comment records an earlier bug of
exactly this class, a `%I64u` format that printed rubbish on Linux.

Fixed by using `uint32_t` for the accumulator, the return type and both
`SDebugState` fields, which makes it canonical FNV-1a-32 everywhere. Safe
without the harness because nothing in the simulation reads these values, so no
save, replay or segment checksum changed. The ISAAC core itself was already
clean, using `uint32_t` for `randrsl`, `randmem` and `randa/b/c`, so this was
the only width problem left in the generator.

### An uninitialised hash seed was dropping executor events (fixed)

The most serious defect found so far, and the one that says most about how to
look for the rest.

`boost::hash_combine` reads the seed as well as writing it. All three seeds in
the tree were uninitialised, so the hash of a key depended on whatever occupied
that stack slot. The same key hashed one way when an executor registered for an
event and another way when the event was raised, `events.find` missed an entry
that was present, and the event was dropped silently.

An undelivered event there is an executor that never gets scheduled, so its
queue is never drained, so a draw from the shared generator never happens, and
the random stream parts company from that point on. Two clients whose stacks
differ at that call drop different events. This is an ASYNC in ordinary
multiplayer, not a sanitizer artefact.

**Why it hid for so long.** A run that reaches the hash through the same call
path every time sees the same slot contents and works. A release build delivered
all thirty events it raised. Under ASan, which moves the stack around, the same
replay raised thirty two and delivered eighteen, and differed from itself on
every run.

**One cause, four symptoms.** All of these turned out to be the same bug, and
each was chased separately first: a full state checksum differing two tenths of
a second in; the random draw count differing by one a second and a half later;
the executor scheduling wheel holding different entries; and the walk of the
state visiting sixteen more fields. With the seed initialised, a full state
checksum every segment agrees between a release build and an ASan build over a
whole replay, and an ASan build agrees with itself. Neither was true before.

**What detects it.** Nothing that was running. A garbage `size_t` has no invalid
values for UBSan to report and the arithmetic is well defined; only the read of
an unwritten variable is undefined, and no sanitizer in use here reports that.
Measured afterwards on the exact pattern:

| tool | reports it |
| --- | --- |
| `g++ -Wall -Wextra -O0` | no |
| `g++ -Wall -Wextra -O1` or better | **yes**, `-Wuninitialized` |
| `clang++ -Wall -Wextra`, any `-O` | no |
| clang static analyzer | **yes**, with the full value trace |

So it was a free find for a flag this build does not pass, and the lesson is an
ordering one: static analysis before sanitizers, and sanitizers before anything
exotic. It also inverts a habit, since GCC needs optimisation to see through the
inlined `hash_combine` and a Debug-only warning build reports nothing. And a
second compiler would not have helped: on the clang side the value is the
analyzer, not the warnings.

**Wrong turns, recorded because they were expensive.** A particle resource load
race, which `resource_sync_load` disproved and which the simulation cannot
observe anyway. Allocator garbage, which `MALLOC_PERTURB_` appeared to rule out
except that ASan replaces the allocator, so the test said nothing about the
configuration that was failing. Padding in the checksum walk, which there never
was. And the verification stopping at the first differing checksum, which hid
the draw count, the one oracle that distinguishes a simulation that diverged
from bytes that merely differ.

### A blind spot in the detector itself

Both string overloads in `Common_RTS_AI/ChecksumSaver.cpp` are commented out and
do nothing:

```cpp
void CCheckSumSaver::DataChunkString( std::string &data )  {}   // body commented out
void CCheckSumSaver::DataChunkString( std::wstring &data ) {}   // body commented out
```

So **string state is excluded from the per-segment checksum entirely.** Two
consequences, pointing in opposite directions.

It removes what looked like a serious obstacle to cross-platform multiplayer.
`wchar_t` is 2 bytes on Windows and 4 on Linux, and simulation-reachable code
uses wide strings heavily, 52 occurrences in `libdb` alone, 10 in `AILogic`.
Had the commented-out code been live it would have hashed
`data.size() * sizeof( value_type )` raw bytes, and Windows and Linux could never
have agreed on a segment containing a single wide string, whatever was done about
libm or containers.

It also means any divergence that manifests only in string state goes
undetected, so "the checksums matched" is a weaker statement than it appears.
For a lockstep RTS that is probably a small hole, since simulation-relevant
strings are mostly identifiers rather than computed values, but it is a known
limit on how much confidence a passing comparison earns. Enabling it would be a
cross-platform problem to solve rather than a free win: it needs a width-stable
encoding, hashing UTF-8 bytes rather than `wchar_t` units.

Note also that the disabled code used `adler32` where the rest of the file uses
`crc32`, which is moot while it stays off.

### Cheap hardening available now

**Put `NRandom::GetRandomCallsCounter()` into the per-segment checksum.** The
counter exists and is exported from `System/RandomGen.h`, but today it only
appears in a debug dump at `AILogic/AILogicInternal.cpp:1206` and a query at
`AILogic/Shell.cpp:132`. It is not in the checksum path. Adding it makes any
change in the number of draws diverge on the very next segment instead of only
when a drawn value happens to matter, which turns a class of silent refactoring
hazard into an immediate and localised failure. It would have caught the
`Scripts.cpp` sites above the first time a Windows and a Linux replay were
compared.

## Hardening initiatives, ordered by how invasive they are

The underlying problem is one sentence: C++ permits undefined behaviour for
performance and then gives no reliable way to find it. Every proposal in this
area is an attempt to extend the language, the toolchain or the runtime so that
some class of defect becomes findable, and they differ mainly in **how invasive
the extension is**. Invasiveness buys completeness of guarantee, and the
exchange rate is worth knowing before picking one.

**1. Annotations the language itself ignores.** Microsoft's SAL (`_In_`,
`_Out_writes_bytes_(cb)`, `_Success_`), GSL's `Expects` and `gsl::not_null`,
`__attribute__((nonnull))`, clang nullability. Macros or attributes that expand
to nothing for compilers that do not participate, so adoption is incremental and
costs non-participating builds nothing. They catch only what was annotated,
where it was annotated, so effort scales with the surface area covered.

**2. Standard-adjacent contract syntax.** C++26 contracts (`pre`, `post`,
`contract_assert`) and `[[assume]]`. Same shape as tier 1 but in standard
syntax rather than a vendor's, so it is not a private extension. Checking
remains implementation-defined, and `[[assume]]` is a promise to the optimiser
rather than a check, so it *adds* UB if wrong.

**3. External tooling over unmodified source.** clang-tidy, the clang and MSVC
analyzers, semgrep. Zero source change, which is their great advantage. Limited
by being flow-insensitive in practice, and by false positives on old code.

**4. Compile-time and library hardening.** `-fhardened`, `/sdl`,
`_FORTIFY_SOURCE`, `_GLIBCXX_ASSERTIONS`, `_LIBCPP_HARDENING_MODE`, and the
Safe Buffers model (`-Wunsafe-buffer-usage` plus `std::span`). Small or no
source change, modest runtime cost, bounded but real guarantee. The best
effort-to-benefit ratio available to this project.

**5. Runtime instrumentation with a metadata or shadow runtime.** ASan, UBSan,
TSan, MSan, TySan, Valgrind, Fil-C. No source change, heavy runtime cost, and
coverage limited to code paths that actually execute. This is where this project
mostly lives.

**6. Hardware-assisted tagging.** ARM MTE and HWASan, CHERI and CHERIoT,
ChkTag. Cheap at runtime where the hardware exists, and unavailable here: x86-64
Linux has no tagged address ABI, which is why HWASan aborts at startup on this
machine.

**7. A dialect.** Safe C++ and P3390R0, Cpp2 and cppfront, TrapC. New syntax
inside a C++-shaped language, requiring a specific compiler and a rewrite of
whatever is to be protected. In exchange the guarantee is by construction rather
than by detection. The committee's direction has moved toward Profiles instead,
which matters here because Profiles are designed to be applied incrementally to
existing code, which a dialect is not.

**8. A different language with an interop boundary.** Rust, Carbon. Strongest
guarantee, highest migration cost, and it requires an ABI surface that the thing
being replaced can actually be expressed through.

### What that means for this tree

The constraints select hard for the low-invasiveness end. 1.4M lines, three
compilers that all have to keep working, 25 shared libraries, prebuilt DXVK,
SDL3, wx and crashpad, cross-DLL `CastToObjectBase` casting, inline asm and
MASM, and raw-memory serialisation. Anything requiring a specific compiler or
whole-program recompilation is effectively ruled out.

Concretely, tiers 1 to 5 are reachable and tiers 6 to 8 are not, with one
exception worth evaluating: **Fil-C** is the only item at the heavy end that
compiles real C and C++ rather than requiring a dialect, and `ReplayRunner` is
the one binary whose dependency set has no DXVK, no SDL3 and no asm. Using it as
a bug-finding oracle on the headless simulation, never as a shipping
configuration, is plausible once that binary exists.

Two tier-1 and tier-4 items are cheap enough to take without further analysis:

- **`-fhardened` on the gcc presets.** Verified on gcc 14.2 here: it defines
  `_FORTIFY_SOURCE=3` and `_GLIBCXX_ASSERTIONS=1` and adds stack protector,
  stack clash protection, CF protection, RELRO and PIE. `_GLIBCXX_ASSERTIONS` is
  the container-precondition win described under tier 3 of the detection matrix,
  and it is ABI-compatible, unlike `_GLIBCXX_DEBUG`, so it is safe against the
  prebuilt dependencies. It changes no arithmetic, so it is determinism-neutral.
  **clang does not accept `-fhardened`** and needs the components spelled out.
  `/sdl` is the MSVC counterpart.
- **SAL and `/analyze`, for the Windows build only.** No SAL exists in these
  sources and neither `/analyze` nor `/sdl` is enabled. The leverage is that the
  Windows SDK and CRT headers are *already* annotated, so `/analyze` checks this
  code's calls into Win32 against contracts that already exist, with nothing to
  annotate first: buffer and size arguments agreeing, null returns, handle
  lifetimes. It is slow enough to want its own CI job rather than the default
  build. Annotating the project's own 1965 headers is the expensive half and is
  not worth doing wholesale; module boundaries at most.

### Profiles, and the shipping subset to use instead

Profiles are the direction the committee drifted toward in preference to Safe
C++, and they are **a direction rather than a feature**. Not in C++26, C++29 at
the earliest, the proposed syntax has moved between paper revisions, and the
lifetime profile is contested on the grounds that it cannot give real guarantees
without borrow-checker-grade annotations. The concept is also not new: the C++
Core Guidelines already defined `Pro.type`, `Pro.bounds` and `Pro.lifetime`, and
MSVC shipped checkers for them which then largely stagnated. That history is
evidence about feasibility.

They are **not a replacement for contracts**; the two are orthogonal. Contracts
(C++26 `pre`, `post`, `contract_assert`) are per-function obligations that
someone writes, checked at runtime: "this function requires n > 0". A profile is
a categorical guarantee the compiler enforces across a region with nothing
written per function: "in here, no out-of-bounds indexing can occur". Contracts
are assertions you author, profiles are dialect-lite you switch on.

The shape, indicatively, since the spelling is unsettled:

```cpp
[[profiles::enforce(bounds, type)]]
namespace sim {
    [[profiles::suppress(bounds)]] uint32_t fast_path( ... );   // audited exception
}
```

Opt-in per region is the whole point against a dialect: existing code keeps
compiling untouched. Named profiles under discussion are type, bounds, lifetime,
initialization, arithmetic and invalidation, enforced by a mixture of static
rejection, inserted runtime checks and annotation-dependent analysis.

**What is already shipping under other names, and is what to actually use:**

| profile | shipping today |
| --- | --- |
| bounds | clang Safe Buffers: `-Wunsafe-buffer-usage`, `#pragma clang unsafe_buffer_usage begin/end` for audited regions, hardened libc++ for the runtime half |
| type, bounds, lifetime, partially | MSVC `/analyze` with the Core Guidelines checker |
| lifetime, fragments | `[[clang::lifetimebound]]`, `-Wdangling`, `-Wreturn-stack-address` |
| bounds, runtime half | `_GLIBCXX_ASSERTIONS` and `_LIBCPP_HARDENING_MODE`, already free through `-fhardened` |

**A worked example from this tree.** `System/RandomGenInternal.cpp:138`:

```c
#define ind(mm,x)  (*(uint32_t *)(( uint8_t *)(mm) + ((x) & ((RANDSIZ-1)<<2))))
```

Three profile targets in one line: a `uint32_t*` to `uint8_t*` to `uint32_t*`
cast round trip for the type profile, raw pointer arithmetic for the bounds
profile, and a hand-rolled mask standing in for a bounds check. Clang's Safe
Buffers already reports it, through the macro expansion:
`warning: unsafe pointer arithmetic [-Wunsafe-buffer-usage]`.

The nuance matters more than the catch: **this code is correct.** The
`& ((RANDSIZ-1)<<2)` mask guarantees both in-range and 4-byte-aligned access.
The profile rejects it because it cannot prove the invariant, not because the
invariant is broken. That is the true cost of profiles on legacy code, and why
suppression escapes are mandatory rather than decorative. The remedy in
profile-world is `std::span<uint32_t, RANDSIZ>` with index rather than byte
arithmetic, which is clearer code, but **`std::span` is C++20 and this project
is C++17 with no GSL dependency**, so the remedy needs a standard bump or a
vendored GSL first.

**Measured cost of adopting the bounds profile here.** Run against real
translation units through `compile_commands.json`, two `AILogic` files produced
102 `-Wunsafe-buffer-usage` diagnostics each, and the distribution is the
finding:

| location | count |
| --- | --- |
| `Stats_B2_M1/Commands_Actions.h` | 33 |
| boost::uuid headers, third party | 45 |
| `Misc/2Darray.h` | 10 |
| `Misc/BitData.h` | 9 |
| `Stats_B2_M1/AITypes.h` | 3 |
| `System/FilePath.h` | 2 |
| the `.cpp` itself | 0 |

All of them are in headers and none in the translation unit. Forty-four per cent
are third-party and get suppressed rather than fixed. The project's own share
sits in five headers, two of which, `2Darray.h` and `BitData.h`, are hand-rolled
containers and so exactly what `span` and `mdspan` exist to replace. So the cost
is not 102 multiplied by 1393 translation units; it is a handful of headers
amplified across the tree, which is a bounded project rather than a sweep.

**What it would catch of the hazards recorded above:** the union punning in
`Checksums.h`, by the type profile. Not the `unsigned long` FNV width, not the
two RNG draws in one expression, not the `std::sort` tie instability, not FP
contraction or libm. One of five, and it is the one shaped like a memory-safety
bug.

### Order of adoption

Readily available tooling comes before anything requiring a toolchain of our
own. A compiler built from an experimental branch is not just a build step: it
is a second toolchain to keep green, a reproducibility problem for CI and for
anyone else building the tree, and a dependency on a moving target. That price
is worth paying only once the cheaper tiers are exhausted and the findings that
remain justify it.

1. Flags that cost nothing and need no source change: `-fhardened` on gcc, the
   UBSan `integer` and `implicit-conversion` groups, `/sdl` and `/analyze` in
   their own Windows job.
2. Warnings enabled for new code only, so the backlog does not block adoption:
   `-Wunsafe-buffer-usage`, the clang-tidy configuration that does not exist
   yet.
3. Separate sanitizer trees, which need a build but no new toolchain: TySan on
   clang-21, MSan over the unit tests.
4. Bounded source projects with a measured scope: the five Safe Buffers headers,
   the `det_map` rebuild, the modular extractions below.
5. Only then anything exotic: Fil-C as an oracle over `ReplayRunner`, an
   `aarch64` cross-build under qemu, a compiler from an experimental branch for
   profiles or contracts.

### The conclusion that matters most

**None of tiers 1 to 8 delivers determinism.** Every one of them targets memory
safety. A borrow checker, a capability architecture or a shadow runtime says
nothing about FP contraction, about which of two legal evaluation orders ran,
about unordered iteration, or about libm disagreeing between platforms. Even the
most invasive option on the list, rewriting in another language, would not give
cross-platform bit-exactness for free; it would simply move the arithmetic into a
different implementation whose own choices would have to be pinned down.

So for the goal this file serves there is no "safe by design" option at any
price. Determinism has to come from discipline about a small number of named
hazards plus differential execution to catch the residue. The hardening work is
worth doing for its own sake, and it should not be allowed to displace the
harness, because it does not substitute for it.

## Modular extraction: candidates in order

Applying anything invasive to 1.4M lines at once is not viable. Extracting a
small module, giving it tests, and then hardening or rewriting it is. The module
chosen matters more than the technique, and the right choice depends on which
property is being bought.

For **determinism**, the risk in a small arithmetic module is not memory safety
at all: it is integer width, overflow semantics and evaluation order. The remedy
is fixed-width types, `static_assert` on the widths relied upon, pinned
evaluation order and exhaustive golden-vector tests. That is careful plain
C++17, and no dialect on the list above would help. The `unsigned long` FNV
accumulator recorded earlier is the proof: `uint32_t` and one golden test would
have caught it, a borrow checker never would.

For **memory safety**, the payoff is where untrusted input arrives: packet
parsing in `Net` and `Server_Client_Common`, and the XML and database parsers.
That is the remote attack surface, and it is also the only place a component in
another language behind a C ABI is realistic, because the interface is bytes in
and structs out rather than `CObjectBase` with cross-DLL casts.

Candidates, in the order worth attempting:

1. **`System/RandomGen`.** The best first target. A tiny interface, seed in and
   `uint32_t` out, ISAAC, no object model in the core, and exhaustively testable:
   a fixed seed yields a fixed output vector, so a golden test proves
   bit-exactness outright rather than probabilistically. It is simultaneously the
   most determinism-critical component in the game and small enough to verify
   completely. The core is already `uint32_t`-clean; only the debug checksum
   above is wrong.
2. **`AILogic/AIGeometry` and `PlanePathMath`.** Bounded, pure float, no object
   model, testable by value.
3. **The libm layer.** Vendoring openlibm or CORE-MATH is this same strategy
   applied at a natural pure-function boundary.
4. **`det_map` and `det_set`.** Already isolated templates. Rebuilding them on a
   dense vector with a flat index is self-contained, independently testable, and
   collects a locality win alongside the determinism one.

**Not `AILogic` as a whole.** It is the largest module in the tree and the thing
the extracted pieces serve, not a starting point.

## Rules for new and modernised code

Mechanical-looking modernisations are not uniformly safe. The dividing line is
whether the transformation can change the number, order or occurrence of side
effects.

**Safe, provably order-preserving.** `for (auto i = c.cbegin(); i != c.cend(); ++i)`
to a range-for: specified in terms of the same `begin` and `end`, same sequence.
Two caveats. The hand-written form re-evaluates `c.cend()` every iteration while
range-for evaluates `end()` once, so the conversion is only safe when the body
does not mutate the container. And prefer `const auto&`, because `auto` over a
container of `CPtr` copies and churns refcounts.

**Safe.** Replacing `std::algorithm(c.begin(), c.end(), ...)` with
`std::ranges::algorithm(c, ...)` once the project moves to C++20. Same algorithm,
same traversal. Note `ranges::sort` is unstable too and inherits the problem
above.

**Unsafe in simulation code.**

- Anything with an execution policy, `std::execution::par` or `par_unseq`, and
  anything from TBB. Beyond ordering, `CObjectBase`'s refcount is a plain
  non-atomic `++nRefData`, so parallel iteration over `CPtr` is a data race
  regardless.
- `std::reduce` and `std::transform_reduce`. These look like drop-in
  modernisations of `std::accumulate` and are not: they permit arbitrary
  association and order even under the sequential policy, so for floats they
  change results. There is currently no `accumulate`, `reduce` or `for_each`
  anywhere in the simulation modules, so this is a rule to hold rather than a
  migration to do.
- SIMD horizontal reductions, which is what a library like EVE would introduce.
  Reassociation changes float results by construction.
- rxcpp. It is not a modernisation of STL loops but ReactiveX, push-based with
  schedulers, and `observe_on` or `subscribe_on` introduce threads. Ruled out by
  the non-atomic refcount before determinism is even considered.
- Lazy range views whose predicate or transform is impure. Laziness does not
  change iteration order, which is the common misconception; it changes how many
  times a callable runs and whether it runs at all. `views::take(n)` over a
  `filter` stops invoking the predicate once n elements pass. If that predicate
  draws from the RNG, the draw count changes and the simulation diverges. The
  general rule, which is stronger and easier to check than a ban on views, is
  that **callables in simulation code must be pure**.
- `views::filter` combined with mutating elements through it is undefined, not
  merely non-deterministic: `filter_view` caches `begin()`, and changing an
  element so that the predicate result changes is UB.
- Copy and move constructors of simulation types must have no side effect
  observable to the simulation, because elision may remove them.
- Changing an accumulator's initial value type, `0` against `0.0f` against
  `0.0`, changes precision without touching a loop.
- `std::random_device`, `std::mt19937` and the `<random>` distributions, in any
  simulation translation unit. Simulation randomness has exactly one source,
  `NRandom` through `System/RandomGen.h`. This is not hypothetical: it is how the
  air-scouting ASYNC above was introduced, by the obvious replacement for an
  algorithm C++17 removed. **A removed or deprecated standard facility is a
  determinism event**, because the mechanical substitute is seeded from entropy
  by default -- so re-check the replacement for `std::random_shuffle`,
  `std::bind1st`/`bind2nd` wrapping an impure functor, and anything whose
  replacement takes a callable the old form did not.

Enforcement should be mechanism rather than discipline. There is no `.clang-tidy`
in the tree yet, so it is greenfield: a permissive root config enabling the safe
modernisations, a stricter one in the simulation directories, and a CI grep over
those directories for `#include <execution>`, `std::execution`, `std::reduce`,
`std::transform_reduce`, `tbb::`, `rxcpp` and `<random>`, none of which have a
legitimate use there. That last one would have caught a live ASYNC, which makes
the grep worth writing before the clang-tidy configs.
