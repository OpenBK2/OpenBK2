# libgr2

A native reader and animation runtime for the Granny 2 (`.gr2`) files Blitzkrieg 2
ships, meant to replace RAD Game Tools' proprietary `granny2.dll`.

**Status: static geometry, for half the shipped corpus.** The loading path is
written end to end: `GrannyReadEntireFileFromMemory`, `GrannyReadEntireFile`,
`GrannyFreeFile`, the Oodle1 codec, the type tree walk, and `GrannyGetFileInfo`
with the conversion behind it. `GrannyGetMemberTypeSize` and
`GrannyGetTotalObjectSize` come with it. The other 48 entry points are stubs that
return a null, a zero or a false.

Two things stand between that and a running game. Oodle0 is not written, and it
is what the other 6,016 of the retail install's 13,582 GR2 files use; no file
mixes the two codecs, so each unlocks its own half. And nothing animates: the
pose and control entry points are still stubs, so track groups and animations are
counted but not converted.

Both halves are checked against the real DLL rather than against themselves.
Oodle1: 8,636 files, 140.9 MB, decoded through both and compared byte for byte.
`GrannyGetFileInfo`: 600 files walked through both and compared field by field,
including names, counts, parent indices, transforms, vertex strides, triangle
groups, bone bindings and mesh pointer identity. All identical.

Nothing is wired into the engine: `Sources/CMakeLists.txt` does not reference this
directory, and the game still links the vendored DLL.

The background is in [docs/GrannyReplacement.md](../../../../../../docs/GrannyReplacement.md),
which measured this game's corpus (83,184 unique GR2 files across three installs,
one dialect: File Format 6, little endian, 32-bit pointers), surveyed seven open
source implementations, and set out the milestones referred to below. How the
engine uses Granny today is in
[docs/Granny3DUsage.md](../../../../../../docs/Granny3DUsage.md).

## Building

Standalone, and it needs nothing but a C++17 compiler:

```powershell
cmake -S Versions/Temporary/Engine/Sources/vendor/libgr2 -B out/build/libgr2 -G Ninja
cmake --build out/build/libgr2
cmake --build out/build/libgr2 --target libgr2-verify-exports
```

`libgr2-verify-exports` is the acceptance test for this stage: it reads the built
library's export table and fails unless it holds exactly the 54 names in
`exports.txt`. On x86 it accounts for MSVC decorating `__stdcall` exports as
`_Name@N`, which is what the real `granny2.dll` exports too.

The output is named `granny2.dll` on x86 and `granny2_x64.dll` on x64, matching
the vendored DLL, because the engine loads it by name.

## Checking it against the real thing

`scripts/port/gr2diff.py` loads both this library and the vendored
`granny2_x64.dll` into one process, reads every `.gr2` it can find through each,
and reports where they disagree. That is possible because this library
reproduces the Granny API rather than inventing a neutral one: the same bindings
drive either, and the structures compared are `granny211.h`'s, which is the
contract both have to meet.

```powershell
python scripts/port/gr2diff.py --jobs 8            every install it knows about
python scripts/port/gr2diff.py --limit 500         a quick sample
python scripts/port/gr2diff.py --report out.json   every difference, in full
```

It prints a histogram before a list, since over eighty thousand files what is
useful is "InverseWorld4x4 differs in 12 files" rather than twelve thousand
lines. Files this library refuses because of a codec it does not implement yet
are counted apart from files the two read differently, because during a port most
of those are one milestone away rather than wrong.

This is the measurement that matters for this project, and it cannot be a unit
test: it needs the DLL and it needs the corpus, and the corpus is 83,184 files of
Nival's copyrighted data. The unit tests are what run without either.

## Tests

googletest, fetched at the pin the rest of the tree uses, or reused from the
engine build when there is one. `LIBGR2_BUILD_TESTS` is on standalone and off
when this is a subdirectory of something else.

```powershell
cmake --build out/build/libgr2 --target libgr2-tests
ctest --test-dir out/build/libgr2 -L obk2-test --output-on-failure
```

Tests link `gr2_static`, a second static build of the same sources, rather than
the DLL. The container, the type tree walker and the two Oodle decoders are
internal and none of them is among the 54 exports, and a static link needs no DLL
beside the executable, which is what lets these run on a machine that has never
seen `granny2.dll`.

`test/MinimalGr2.h` records the file layout as measured, with the census behind
each constant, and builds a buffer shaped like a shipped `.gr2`: sections with
content, pointer fixups between them, and a header a test can reach in and break
one field of. `ReadEntireFileFromMemory.cpp` drives the public entry points and
`FileContainer.cpp` checks what a successful load produced, which is why the
tests link the static library.

The corpus itself cannot be committed, so the sweep over real data takes a
directory from `LIBGR2_TEST_GR2_DIR` and skips when it is unset. A `.pak` is an
ordinary ZIP archive, so extracting one is a one-liner.

Tests that cannot pass until a milestone lands are `DISABLED_`, so they are
written-down acceptance criteria rather than a permanently red suite. Two live
tests mark what is still missing and fail the day it arrives:
`GetFileInfo.StillReturnsNull`, and
`ReadEntireFileFromMemory.RefusesCompressedSectionsUntilTheCodecsExist`.

## The call trace

Every entry point records the call before it returns, so a run against this
library says which of the 54 are actually reached, in what order, and with what
arguments. The engine references all 54, but a reference is not a call: some sit
on paths the shipped data never takes, and the linker cannot tell. That is what
orders the milestones above.

Two levels, so one run answers both questions:

- **trace**, every call, as `<ordinal>  Name( Arg=value ... )`. Strings are
  printed as strings, since they name the file being read or the bone being
  looked up; other pointers become addresses, which is enough to match a later
  call against an earlier one.
- **warn**, once per entry point, `Name is not implemented`. Running at warn
  gives the list of entry points reached and nothing else, which is what stays
  useful once some of them are real.

The log goes to `granny_calls.log` beside the executable, truncated per run, and
on Windows also to the debugger's output window. `LIBGR2_LOG_LEVEL` sets the
level; it defaults to `trace`, and every line is flushed, because the process
this is meant to observe is expected to stop abruptly and the tail is the part
worth reading.

### What the first run said

Three calls before the game gave up on the null files it got back:

```
1  GrannyReadEntireFileFromMemory( MemorySize=293795 ... )
2  GrannyReadEntireFileFromMemory( MemorySize=5511 ... )
3  GrannyGetFileInfo( File=0x0 )
```

`GrannyReadEntireFileFromMemory` is therefore the first Granny function the game
calls, with nothing at all before it. In particular **the allocator entry points
are dead**: `InitializeGrannyMemoryMap` in `3Dmotor/GrannyMemoryMap.cpp` is the
only caller of `GrannyGetAllocator` and `GrannySetAllocator`, and nothing calls
`InitializeGrannyMemoryMap`. They still have to be exported, since the engine
links them, but they can stay stubs indefinitely and Granny's own allocator is
the one that runs.

This is the trace earning its keep on its first outing, and it says the order to
work in: everything begins at `src/File.cpp`.

## Shape

The public header, `include/gr2/granny.h`, reproduces the Granny API rather than
inventing a neutral one. Same names, same signatures, same `__stdcall`, same DLL
file name. That is a deliberate and temporary choice: it means the engine can be
relinked without touching a line of `3Dmotor` or `SceneB2`, and on Windows both
implementations can be loaded into one process and asserted against each other
call for call. That leverage disappears the moment the API changes, so the
refactor onto a format-neutral skeleton and pose interface comes last, not first.

Types split two ways. The runtime handles (`granny_file`, `granny_control`,
`granny_local_pose`, and so on) are opaque permanently: this library owns them
and the engine only holds pointers. The data records (`granny_file_info`,
`granny_mesh`, `granny_skeleton`, `granny_transform`, `granny_curve2`, and the
rest) are opaque only for now: the engine walks their fields directly in
twenty-odd places, so each has a layout that has to be reproduced exactly, and
each gains its real definition in the milestone that first needs it.

## Layout

| file | entry points | milestone |
|---|---|---|
| `src/Allocator.cpp` | 2 | linked, never called, see the trace above |
| `src/File.cpp` | 4 | container, fixups and file info |
| `src/Oodle1.cpp` | 0 | the codec 7,566 of 13,582 retail files use |
| `src/Convert.cpp` | 0 | the file's structures into granny211.h's |
| `src/TypeTree.cpp` | 2 | members resolved through the file's own type tree |
| `src/Mesh.cpp` | 2 | M2, geometry |
| `src/Skeleton.cpp` | 1 | M2, bone lookup by name |
| `src/Transform.cpp` | 2 | M3, position, orientation, scale-shear |
| `src/Model.cpp` | 3 | M3, model instances and their clock |
| `src/Pose.cpp` | 11 | M3, curve sampling, local and world pose, skinning matrices |
| `src/Animation.cpp` | 7 | M4, binding a clip to a model, track masks |
| `src/Control.cpp` | 20 | M4, playback, looping, ease curves |

M4 is the part no open source project has written, because importers and viewers
never need a playback layer. Build the record-and-replay harness before it.

## Constraints

- **Nothing from the engine.** No `System/Basic.h`, no `CObjectBase`, no
  `CPtr<T>`, no `IBinSaver`, no Boost. Pure C++17 and the standard library, so
  that `git subtree split --prefix=...` extracts this with its history intact
  once the API has settled.
- **Presentation only.** Animation in this engine never reaches `AILogic`, so a
  replacement has to be visually correct, not bit-exact against `granny2.dll`.
  The container and the codecs are the opposite: those are byte-exact or wrong.
- **32-bit files, on both hosts.** The files store 32-bit pointers. Nothing here
  memory-maps; it allocates and populates, which is what lets x86 and x64 share
  one path.
