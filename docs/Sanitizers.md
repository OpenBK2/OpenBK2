# Sanitizer and coverage builds

What each sanitizer build is for, what it will report first, and which runtime
options it needs.

The configurations are presets in `CMakePresets.json` - `Linux-x64-ASan+UBSan`
and `Linux-x64-TSan`, which `asan+ubsan` and `tsan` are short for - so the IDE,
ctest and the scripts all read one definition.

```
./build.sh asan+ubsan        # configure, build, install to ~/bk2-asan
./test.sh  asan+ubsan        # build the unittests target, then ctest
./launch.sh asan+ubsan       # run Game out of ~/bk2-asan
./launch.sh asan+ubsan --data retail    # ... against the retail data tree
```

## Why they are separate trees

ASan and TSan cannot share a binary: both map shadow memory over the whole
address space and want the same regions. `CMakeLists.txt` refuses the
combination outright.

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
