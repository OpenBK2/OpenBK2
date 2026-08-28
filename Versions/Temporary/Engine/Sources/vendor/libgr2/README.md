# libgr2

A native reader and animation runtime for the Granny 2 (`.gr2`) files Blitzkrieg 2
ships, meant to replace RAD Game Tools' proprietary `granny2.dll`.

**Status: every shipped file loads and converts in full, and curves sample.** The
loading path is written end to end: `GrannyReadEntireFileFromMemory`,
`GrannyReadEntireFile`, `GrannyFreeFile`, both Oodle codecs, the type tree walk,
and `GrannyGetFileInfo` with the conversion behind it, which now covers track
groups, animations and their curves as well as the geometry.

Models instantiate, sampling one with nothing bound gives the skeleton's rest
pose, the hierarchy walks into world and skinning matrices, and
`GrannyEvaluateCurveAtT` evaluates a curve at a time. So every model can be drawn
in bind pose, skinned, and the engine can read a scalar channel out of a clip.

The playback layer is written too: a clip binds to an instance through the
builder, a `granny_control` carries its clock, speed, loop count and two ease
curves, and `GrannySampleModelAnimations` blends every control bound to an
instance by weight. So models move.

None of that layer has prior art. Every open source Granny project stops at
reading a file, because an importer or a viewer never needs playback. It was
measured instead, by scripting the real DLL through the sequences the engine
issues and reading the observables after every step, and the same scripts replay
against both implementations. Three things still differ and they are listed in
docs/GrannyReplacement.md under "What still differs".

**All 21,720 unique GR2 files across the three installs read identically to the
real `granny2.dll`**, in every field compared: names, counts, parent indices,
transforms, vertex strides and their component lists, vertex bytes, triangle
groups and indices, bone bindings, mesh pointer identity, every track group and
animation, and every curve sampled at nine values of t drawn from its own knots.
See `scripts/port/gr2diff.py` below.

Nothing is wired into the engine: `Sources/CMakeLists.txt` does not reference this
directory, and the game still links the vendored DLL.

The background is in [docs/GrannyReplacement.md](../../../../../../docs/GrannyReplacement.md),
which measured this game's corpus (21,720 unique GR2 files across three installs,
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

## The 54 entry points

**52 implemented, 2 stubs, 0 reached-but-missing.** A run of the game logs no
"not implemented" line at all.

The verification column says how each was checked. *corpus* is
`scripts/port/gr2diff.py` over all 21,720 distinct GR2 files, comparing against
the real DLL. *replay* is the companion harness, which scripts both
implementations through fifteen call sequences per file and compares the
transcripts. Everything also has unit tests.

### Reached by the game (38)

| entry point | verified |
|---|---|
| `GrannyReadEntireFileFromMemory`, `GrannyFreeFile` | corpus |
| `GrannyGetFileInfo` | corpus |
| `GrannyGetTotalObjectSize`, `GrannyGetMemberTypeSize` | corpus |
| `GrannyGetMeshTriangleGroupCount`, `GrannyMeshIsRigid` | corpus |
| `GrannyFindBoneByName` | corpus |
| `GrannyMakeIdentity` | corpus |
| `GrannyInstantiateModel`, `GrannyFreeModelInstance`, `GrannySetModelClock` | corpus |
| `GrannyNewLocalPose`, `GrannyFreeLocalPose`, `GrannyGetLocalPoseBoneCount`, `GrannyGetLocalPoseTransform` | corpus |
| `GrannySampleModelAnimations` | corpus + replay |
| `GrannyNewWorldPose`, `GrannyFreeWorldPose`, `GrannyBuildWorldPose` | corpus |
| `GrannyGetWorldPose4x4`, `GrannyGetWorldPoseComposite4x4` | corpus |
| `GrannyBeginControlledAnimation`, `GrannySetTrackGroupTarget`, `GrannySetTrackGroupAccumulation`, `GrannyNewTrackMask`, `GrannySetSkeletonTrackMaskFromTrackGroup`, `GrannySetTrackGroupModelMask`, `GrannyEndControlledAnimation` | replay |
| `GrannyControlIsComplete`, `GrannyGetControlClampedLocalClock`, `GrannyGetControlDurationLeft`, `GrannyGetControlEffectiveWeight` | replay |
| `GrannySetControlSpeed`, `GrannySetControlLoopCount`, `GrannySetControlForceClampedLooping` | replay |
| `GrannyFreeControlOnceUnused` | replay |
| `GrannyFreeControl` | unit tests |

### Implemented but not reached in a normal run (16)

Reachability was worked out from the engine sources rather than from the absence
of a log line, since one run is not every map.

| entry point | reachable? | what reaches it |
|---|---|---|
| `GrannyPostMultiplyBy` | **yes** | bone mutators, from `SetBoneMutator`: six sites in `B2_M1_World` for helicopter and mechanical-unit gun recoil. It appears in traces of runs where something fires |
| `GrannyGetControlSpeed` | **yes** | `CSkeletonAnimator::GetMarkTimes`, which has callers, and `ApplyGlobalMovementCorrection` |
| `GrannyEvaluateCurveAtT` | **only on some data** | needs a track group with vector tracks. Exactly 5 of the corpus's 21,720 files have any, and the three that are not mod content are in the Total Conversion install. The base game and Fall of the Reich never reach it |
| `GrannyReadEntireFile` | **editor only** | `SceneB2/TerraTools.cpp` compiles it into a game module, but its only callers are `CreateDebris` and `PassabilityProfile`, both under `ED_B2_M1`, and `BUILD_EDITOR` does not build |
| `GrannySetControlEaseIn`, `GrannySetControlEaseInCurve`, `GrannySetControlEaseOut`, `GrannySetControlEaseOutCurve` | no | gated on `fTransitHalfDuration > 0`, and the only thing that writes it, `CSkeletonAnimator::SetGlobalAnimTransit`, has no callers |
| `GrannyEaseControlIn`, `GrannyEaseControlOut` | no | only `CSkeletonAnimator::FadeIn` and `FadeOut`, whose only callers are inside `GAnimation.cpp`'s own restore path, gated on flags only they set |
| `GrannyCompleteControlAt` | no | needs an `AddAnimation` caller passing `tEndTime`, and every one of them takes the `-1` default; `SetEndTime` has no callers either |
| `GrannySetControlActive` | no | only fires when `GrannyControlIsComplete`, which needs `GrannyCompleteControlAt` |
| `GrannySetControlRawLocalClock` | no | only `CSkeletonAnimator::SetLocalTime`, which has no callers |
| `GrannyGetControlDuration` | no | only `CSkeletonAnimator::GetDuration`, which has no callers |
| `GrannyGetAllocator`, `GrannySetAllocator` | no, **and still stubs** | only `InitializeGrannyMemoryMap` in `3Dmotor/GrannyMemoryMap.cpp`, which nothing calls. Granny's own allocator is the one that runs |

`Script/` has no binding to the animator, so nothing reaches these from a
scenario either.

The twelve dead ones are all one feature: **animation transitions**. Cross-fades,
timed clips and scrubbing are built into `CSkeletonAnimator` and never switched
on. They are implemented and replay-verified here anyway, because they are what
the engine would need the moment anything calls `SetGlobalAnimTransit`.

The two stubs can stay stubs. They have to be *exported*, because the engine
links them, but nothing calls them.

### Where the two implementations still differ

Both are invisible to the game and neither is on a path it takes. See
docs/GrannyReplacement.md, "What still differs", for the measurements.

- A local clock landing exactly on a multiple of the period. A float knife edge,
  and the same pose either way for a looping clip.
- The sign of a sampled quaternion, on 1.3% of samples. q and -q are the same
  rotation, and every consumer here is invariant to it.

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
test: it needs the DLL and it needs the corpus, and the corpus is 21,720 files of
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

**The per-call trace is compiled in for Debug builds only**, or for any build
configured with `-DLIBGR2_TRACE=ON`. It was unconditional while this library was
a set of stubs and the log was the whole deliverable. Now that the entry points
do work it is a formatted, flushed line on a function the game calls 17,880 times
in a few seconds of play, which makes the game a slide show and animation
impossible to judge. The warn-once "not implemented" line is not affected and
stays in every build, because a release build reaching something unwritten is
exactly what somebody needs to be told about.

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
| `src/Oodle0.cpp` | 0 | the codec 6,016 of 21,720 files use |
| `src/Oodle1.cpp` | 0 | the codec the other 15,704 use |
| `src/Convert.cpp` | 0 | the file's structures into granny211.h's, three exporter vintages of them |
| `src/TypeTree.cpp` | 2 | members resolved through the file's own type tree |
| `src/Mesh.cpp` | 2 | triangle group count, and whether a mesh needs skinning |
| `src/Skeleton.cpp` | 1 | bone lookup by name |
| `src/Transform.cpp` | 2 | position, orientation, scale-shear, and composing two |
| `src/Model.cpp` | 3 | model instances, their clock and the controls bound to them |
| `src/Curve.cpp` | 1 | curve sampling: constant, linear, and a non-uniform quadratic B-spline |
| `src/Pose.cpp` | 10 | local pose, rest-pose sampling, world pose and the composite matrices |
| `src/Animation.cpp` | 7 | binding a clip to a model instance, and track masks |
| `src/Control.cpp` | 20 | playback: the clock, speed, looping, and the ease curves |

The playback layer is the part no open source project has written, because
importers and viewers never need one. It was measured with a record-and-replay
harness rather than derived.

## Constraints

- **Nothing from the engine.** No `System/Basic.h`, no `CObjectBase`, no
  `CPtr<T>`, no `IBinSaver`, no Boost. Pure C++17 and the standard library, so
  that `git subtree split --prefix=...` extracts this with its history intact
  once the API has settled.
- **Presentation only.** Animation in this engine never reaches `AILogic`, so a
  replacement has to be visually correct, not bit-exact against `granny2.dll`.
  The container and the codecs are the opposite: those are byte-exact or wrong.
- **Reproduce the original's defined behaviour, refuse its undefined behaviour.**
  `granny2.dll` access violates on a null model, a null pose, a null model
  instance and a negative bone count, among others. None of that is reproduced.
  A crash, a hang or an out of bounds read is not a contract, and any caller that
  depended on one was already broken. Where the DLL does define an answer, even
  an odd one, that answer is matched and the measurement is recorded next to the
  code: `GrannyGetLocalPoseTransform` returning null outside the range,
  `GrannyFindBoneByName` writing the bone count on a miss, a fresh local pose
  being zeroed rather than set to identity.
- **32-bit files, on both hosts.** The files store 32-bit pointers. Nothing here
  memory-maps; it allocates and populates, which is what lets x86 and x64 share
  one path.
