# Sanitizer and coverage builds

What each sanitizer build is for, what it will report first, and which runtime
options it needs.

The configurations are presets in `CMakePresets.json` - `Linux-x64-ASan+UBSan`,
`Linux-x64-TSan` and `Linux-x64-TySan`, which `asan+ubsan`, `tsan` and `tysan`
are short for - so the IDE, ctest and the scripts all read one definition.

```
./build.sh asan+ubsan        # configure, build, install to ~/bk2-asan
./test.sh  asan+ubsan        # build the unittests target, then ctest
./launch.sh asan+ubsan       # run Game out of ~/bk2-asan
./launch.sh asan+ubsan --data retail    # ... against the retail data tree
```

## Why they are separate trees

ASan and TSan cannot share a binary: both map shadow memory over the whole
address space and want the same regions. `CMakeLists.txt` refuses the
combination outright. TySan has shadow memory of its own and clang's driver
refuses to pair it with either of them, so it is a third tree for the same
reason, with the refusal moved to configure time rather than discovered once
per translation unit.

UBSan is the one that composes. It has no shadow memory and no allocator of its
own, which is why `asan+ubsan` is a single preset and a single tree rather than
two: a separate UBSan build buys nothing but another 12 GB.

Each tree is 11-14 GB, lives in `out/build/<preset name>`, and none of them is
shared. `--no-editor` roughly halves one.

ccache is wired by `cmake/ccache.cmake`, which finds it on `PATH` at configure
time and wraps it with the `CCACHE_SLOPPINESS` the precompiled headers require.
Do not set `CMAKE_CXX_COMPILER_LAUNCHER` from a preset or the command line: it
shadows that launcher and silently makes every PCH compilation uncacheable.
`./build.sh --no-ccache` passes `-DUSE_CCACHE=OFF`, which is the supported way
off and what an object-file comparison needs.

## ASan + UBSan

`RelWithDebInfo`, not `Debug`: the sanitizer carries its own `-g`, and the
engine at `-O0` under ASan is slow enough to change what you are able to reach
in a session. Pass `-D CMAKE_BUILD_TYPE=Debug` by hand if a report needs
unoptimised frames.

`ENABLE_ASAN` turns `USE_MIMALLOC` off by itself - the two replace the same
allocator - so the preset does not repeat it.

Runtime options, set by `./launch.sh` and by the test preset, overridable by
exporting your own:

```
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1
UBSAN_OPTIONS=print_stacktrace=1
```

`detect_leaks=0` is worth starting with. The object model frees on refcount and
a clean exit is not one of its guarantees, so the leak report at shutdown
buries everything found while playing. Turn it on deliberately, for a run whose
purpose is leaks.

`print_stacktrace` is not a UBSan default, and a report without it names a
source line with no path to it. Add `halt_on_error=1` once the list is being
worked through; UBSan prints and continues otherwise, which is what you want
for a first pass.

**What to expect.** This is C++03 code from 2005 with its own object model and a
lot of casting, so the first run is loud. Misaligned loads, enum values outside
their enum, and signed overflow are the usual three.

**`AILogic` is the one place where a fix has to be weighed against
determinism.** It is `/fp:strict` on MSVC and multiplayer is lockstep, so a
change to the arithmetic there is an ASYNC risk even when it silences a real
diagnostic. Note also that `/fp:strict` is inside `if(MSVC)`, so the GCC and
clang builds of `AILogic` currently have no float-determinism flag at all - see
`docs/CleanupRoadmap.md`.

## TSan

Its own tree, for the reason above. `RelWithDebInfo` again, and TSan's slowdown
is the worst of the three.

`ENABLE_TSAN` turns `USE_MIMALLOC` off: TSan intercepts the allocator to know
which thread owns a block.

```
TSAN_OPTIONS=history_size=4
```

`history_size` raises the chance the other thread's stack is still recorded. It
costs memory.

**What to expect.** The simulation is single-threaded by design, so the races
worth finding are in the parts around it - the sound mixer, the loader, the
network code, the lobby server. The noise comes from the libraries the build
does not compile itself: DXVK is a prebuilt `.so`, so TSan sees its memory
accesses with no annotations and reports its internal synchronisation as races,
and the same goes for the graphics driver and for SDL3.

Expect to need a suppressions file with `called_from_lib` entries for the ones
that are not ours:

```
TSAN_OPTIONS=suppressions=$PWD/tsan.supp:history_size=4
```

That file does not exist yet, and TSan errors out if it is named and missing,
which is why neither the preset nor `launch.sh` references it. Build with
`--no-editor` to drop the wxWidgets and GTK threads out of the reports as well.

## TySan

TypeSanitizer, `Linux-x64-TySan`, short name `tysan`. The odd one out in three
ways: it is about the language rather than about memory or threads, it exists
in clang only, and its preset therefore picks the compiler as well as the flag
by inheriting `base-linux-clang`.

What it does is record, for every byte, the type it was last written with, and
report a read or write through an lvalue of an incompatible type. That is the
strict-aliasing class, the one `docs/Determinism.md` argues can produce an
ASYNC and the one that silently miscompiled the DXT decoder under clang at
`-O2` while GCC happened to produce the right answer. **No warning on either
compiler reports it at any level**, which is the whole argument for the tree:
`-Wstrict-aliasing=3` says which sites *could* break, and this says which ones
actually do it as the game runs.

```
./build.sh  tysan            # configure, build, install to ~/bk2-tysan
./test.sh   tysan
./launch.sh tysan
```

**Its own tree, and clang refuses the alternative.** `-fsanitize=type` cannot
be combined with `address` or `thread`: the driver rejects the pair outright,
so there is no build to discover it in. `CMakeLists.txt` says so at configure
time instead. UBSan composes with it, the way it composes with ASan, and the
preset leaves that off so a run reports one class at a time.

**The static-only runtime decides how the flag is scoped, and this is the one
thing to know before editing the wiring.** TySan ships as
`libclang_rt.tysan-x86_64.a` with no shared object beside it. An instrumented
translation unit therefore leaves `__tysan_init` and
`__tysan_instrument_with_shadow_update` undefined in whatever it is linked
into, and only an executable's link line pulls the archive in and resolves
them. Our 36 shared libraries allow that, since an undefined symbol in a `.so`
is a load-time question. **Boost's do not**: `boost_filesystem` links with
`-Wl,--no-undefined`, so instrumenting it is a hard link error and the build
stops there, which is exactly what the first build of this tree did.

So the flag is not in the top-level `CMakeLists.txt` with the other three. It
is in `Versions/Temporary/Engine/Sources/CMakeLists.txt`, scoped the way
`/utf-8` and `/permissive-` are, which puts it on our own code and nothing
else: 1,414 translation units, 31 shared libraries and 60 executables carry
it, and zero `_deps` targets do. That is also the better scope on the merits,
since the dependencies are `-isystem` here and are not diagnosed, and boost
and SDL type-pun deliberately. The blind spots it leaves are googletest,
`third_party/flessd` and `vendor/libgr2`.

`ENABLE_TYSAN` turns `USE_MIMALLOC` off, and for a different reason than ASan
and TSan do. TySan has no allocator of its own; it interposes `malloc`,
`calloc`, `realloc` and `free` to *forget* a block's type as it is freed. Let
mimalloc take those calls and the runtime keeps the dead object's type, so the
next allocation landing on the same bytes is reported as a violation. The
failure mode is false reports rather than missing ones.

```
TYSAN_OPTIONS=print_stacktrace=1
```

There is no `halt_on_error` to turn off: TySan registers no flags of its own and
the common set does not carry one, so a report is a survey item and not a crash,
whatever is asked for. Two consequences: this is the one sanitizer prefix whose
**exit status still means what it says** (measured: 0 with five violations
reported, despite `exitcode=1` being listed), unlike TSan's 66, and
`./test.sh tysan` will not fail a test on a violation, so read the output rather
than the status. Note also that an unrecognised `TYSAN_OPTIONS` entry is
accepted **silently**, with no "unrecognized flag" warning, so a typo looks
exactly like a flag that had no effect.

**Symbolization has to be arranged.** The runtime looks for a plain
`llvm-symbolizer` and Debian packages only the versioned names, so an
unconfigured run prints bare addresses under "invalid path to external
symbolizer". `./launch.sh tysan` finds the highest `llvm-symbolizer-N` on
`PATH` and passes `external_symbolizer_path=`; the ctest preset does not, since
a preset cannot run a search, so export `TYSAN_OPTIONS` yourself when a test
run needs source lines.

**What to expect, and the one thing that will mislead.** `RelWithDebInfo`, like
the other two, which is also the optimisation level the aliasing question is
asked at. The cost is that the optimiser can fold away the very access the
check would have made: the control used to verify this wiring, a `float`
written and read back through an `int *`, reports at `-O0` and is silent at
`-O2` because clang constant-folds it. So **a silent run is not a clean tree**,
and a site that matters at `-O2` is best confirmed by configuring the same
preset with `-D CMAKE_BUILD_TYPE=Debug` and comparing. The 7 known
`-Wstrict-aliasing=3` sites are listed in `DiagnosticsNextSession.md`, but note
that the one that used to head that list,
`Common_RTS_AI/BasePathUnit.h:219`, is **not reachable by this tool at all**:
both types in that cast are unions, so every access is char-tagged and TySan is
silent on it. The comment at the accessor has the probe that settled it instead.
**That generalises: TySan cannot see a union-to-union pun in either
direction.** With `Misc/Geom.h` built out of unions, that is most of the
engine's geometry.

### How to drive it, and the one way that cannot work

A mission start is the way in:

```
./launch.sh tysan --data retail --mission Custom/Missions/Razvedka/MapInfo.xdb --software-vk
```

The prefix carries no `Data`, so `--data` is not optional here.

**The replay harness cannot be combined with it.** `--replay NAME --verify`
would be the obvious non-interactive driver, and it dies inside the sanitizer
runtime before the first tick: `__tysan_instrument_with_shadow_update` takes a
SIGSEGV under `fmt`'s buffer append, reached from `ReportReplayLine` in
`CCommandsHistory::LoadHistory`, i.e. while the replay is being loaded. The same
replay plays out and verifies 229 of 229 checksums on the GCC release build, so
this is the tool and not the harness. It is the crash class that already takes
six of the unit tests down.

Both ways of driving it end in SIGSEGV eventually, so **read the log rather than
the exit status**, and expect a run to be a sample of the mission rather than
all of it.

### The ignorelist, and why the tree needs one to be readable at all

`cmake/tysan-ignorelist.txt` holds it, the flag is added beside
`-fsanitize=type` in `Versions/Temporary/Engine/Sources/CMakeLists.txt`, and the
file states the measurement behind each entry. There is one entry, and without
it a run that reaches the main menu writes **1 GB of log, 100,214 of whose
100,452 reports are one function**: `IXmlSaver::AddFPData`, serializing every
`CVec3` and `CQuat` in the database.

That one is a modelling artifact, settled from the instrumented IR rather than
argued: `CVec3` is a union, clang gives a union a *scalar* TBAA node parented on
`omnipotent char`, and TySan turns that into a descriptor with a single member,
char at offset 0, so a `float` access at offset 0, 4 or 8 has nothing to match
against; a non-union struct of three floats carries a member list and matches.
The bytes get that type from TySan's own module constructor, which types every
global by its declared type
(`__tysan_set_shadow_type( @VNULL3, @..._ZTS5CVec3, 12 )`), and the typing
travels into heap records through the shadow propagation of a struct copy. The
aliasing question underneath it is answered the other way round: clang tags a
union member access *may-alias* (`omnipotent char`), so there is no
non-aliasing assumption for the optimiser to act on, and the probe prints the
right answer on clang `-O0` through `-O3` and on GCC.

Four things about the mechanism that are worth knowing before adding an entry:

- `fun:` and `src:` work, a `[type]` section header is honoured, and a `type:`
  entry is **accepted and silently does nothing**.
- an entry that matches nothing leaves every report in place, which is the
  negative control that this file is not a blanket off switch.
- a `fun:` entry holds at `-O2`, and the reason is worth knowing: clang drops
  the `sanitize_type` attribute from the listed function and the inliner then
  refuses to inline it into an instrumented caller, so the body stays out of
  line and unchecked. Measured by disassembling `main` with and without the
  entry. It also means an entry costs the whole function's coverage, not just
  the reports you were looking at.
- editing the file changes no compiler command line, so force a rebuild of the
  tree afterwards rather than trusting an incremental one.

The reductions that **failed** are kept in
`.diag-tmp/tysan-control/unions.cpp`, so nobody repeats them: typing the object
with a whole-object assignment reproduces nothing, because an aggregate copy is
tagged `tbaa.struct` with char members. A union *member* write is tagged char
too. Only the declared type of a global, or a copy from one, types bytes as
`CVec3`.

CI cannot run this: the clang job pins clang-19, which has no TySan. Local
tool, clang 22 and up.

## Running against a prefix

`cmake --install` installs code only - no `Data`, no `Profiles`, no
`splash.bmp`. Every prefix but the default one needs the data set up once
before the game will start; `./launch.sh` checks for it and prints these:

```
ln -sfn "$PWD/Versions/Current/Data" "$HOME/bk2-asan/Data"
cp -r Versions/Current/Profiles "$HOME/bk2-asan/Profiles"
cp Versions/Current/splash.bmp "$HOME/bk2-asan/splash.bmp"
```

To run one of the standalone game data trees under a sanitizer instead - retail,
FoTR, Liberation, Total Conversion - use `--data`, which picks the data
independently of the configuration that picks the code:

```
./launch.sh asan+ubsan --data retail
./launch.sh --list-data
```

## Coverage

Not wired yet. When it is, it belongs beside the three sanitizer options as
`ENABLE_COVERAGE`, and it should use clang's source-based coverage
(`-fprofile-instr-generate -fcoverage-mapping`, then `llvm-profdata` and
`llvm-cov`) rather than gcov and lcov: accurate region and branch counts, no
`.gcda` corruption from parallel test runs, and a far more usable report.

Two things specific to this codebase will need scripting once. There are 25
shared libraries, so the profile has to be written as
`LLVM_PROFILE_FILE=...%p%m.profraw` - `%m` exists precisely for multiple DSOs -
and then merged across all of them, with `llvm-cov show -object` per `.so`. And
`third_party/` and `vendor/` have to be excluded from the report or the
percentages mean nothing.
