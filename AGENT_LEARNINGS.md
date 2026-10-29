
# AGENT LEARNINGS

This file is meant for AI agents only, both for read and write purposes.

- Mechanical Binoculars use `ACTION_COMMAND_USE_SPYGLASS`; `ACTION_COMMAND_USE_BINOCULAR` is a separate legacy command. A turret's `TurnHor` does not enforce its horizontal constraint, so aim states must clamp it and use hull assistance when appropriate. For towed guns use `CArtillery::TurnToDir` to preserve packing before rotation, and leave turret control to installation actions while they run. Serialized artillery fixtures use chunks 4/5 for guns/turrets, unlike military cars' 3/4; factory-only checks cannot verify actual aiming.

- Stationary autonomous attacks need filtering during gun/target selection, not only after the winning target is chosen. `CanShootToUnit` permits movement; use `CanShootToUnitWOMove` for range and firing feasibility. Revalidate cached targets and active attacks after visibility or Hold Fire changes. Persist the stationary marker in both the command (including its copy constructor) and attack state so queue insertion and save/load cannot restore pursuit behavior.

- Additional fog sectors must share the unit's visibility mask. Store the maximum covered radius in `nOldRadius` so moving, shrinking, deleting, or transferring a unit removes every old contribution exactly once. Turret angles come from simulation-time interpolation, so stationary units with platform sights need periodic fog updates too. Platform zero is the hull; platform N maps to turret N - 1, using `GetPlatform` for constructor remapping.
- `OBJECT_NOCOPY_METHODS` constructs a default instance during factory creation and cleanup. Helper objects using it need a valid default constructor; non-owning references cannot supply that state. Resource destructors are protected, so regression fixtures should own generated resources with `CObj` instead of putting them on the stack.

- UI text fitting: glyph advances and padded line cells are not visible letter bounds. PT Sans's initial `J` can extend left of its origin. Try the smallest translation that fits the visible letters before removing runtime font enlargement, and use that same corrected origin when drawing placed captions. Correct horizontal overhang per line before combining paragraph bounds; different lines can overhang opposite edges. Keep intentional scroll viewport clipping out of font fitting. Round wrapping constraints inward and autosized bounds outward when converting between screen pixels and virtual coordinates.
- Font regressions should use the actual control, style, parent clip, and shipped font. Encyclopedia names occur in both the body-font list and the centered heading; testing only the heading misses list failures. Include 800x600, the 1024x768 reference size, widescreen and ultrawide sizes, and 4K.
- Broader glyph tests can use the six runtime sizing code pages from `NCodePages`. Check `CFace::HasGlyph` so replacement glyphs do not masquerade as coverage of unsupported characters, and exclude raw markup delimiters from plain-text samples. UI bound checks and independent bitmap coverage tests complement each other.
- Windows builds here use Visual Studio's `Launch-VsDevShell.ps1` and bundled CMake. If sandboxed Ninja stalls before compilation, stop only the processes started for that build and retry through the approval tool. The bundled formatter is under `VC/Tools/Llvm/x64/bin`; pass it to the hygiene wrapper with `CLANG_FORMAT`.
- The `python` and `py -3` commands can resolve to inaccessible Microsoft Store executables. For hygiene scripts, use the working interpreter recorded as `_Python3_EXECUTABLE` in the configured build's `CMakeCache.txt`.
- Fail fast when loading the Visual Studio environment: use `$ErrorActionPreference = 'Stop'` and, if scripts are disabled, a process-scoped execution-policy bypass before `Launch-VsDevShell.ps1`. Otherwise PowerShell can continue into CMake without SDK library paths and report unrelated missing libraries such as `ole32.lib`. In Windows PowerShell, restore `Continue` before redirecting native build output and check `$LASTEXITCODE`; otherwise a CMake warning on stderr can terminate an otherwise valid build.
- A Windows `LNK1140` can come from accumulated linker PDB state rather than current source size. In October 2026, `AILogic.pdb` had grown to 6.34 GB; regenerating it produced a 178 MB PDB. Treat that cleanup as recovery, not proof that the underlying cause is fixed. Repeated single-object links reproduced growth with `/INCREMENTAL`, even with `/PDBTMCACHE:NO`; `/INCREMENTAL:NO` stayed stable. The Release presets use RelWithDebInfo, which inherits incremental linking from CMake unless explicitly overridden. Switching the flag does not compact an existing PDB, so old oversized files need one regeneration. Keep `/debug` for crash symbols. VS Code's later `Failed to prepare executable target ... undefined` message was secondary to this link failure.

## Bulk sed over a name used by two layers

Mistake: replacing `theGroupLogic.GenerateGroupNumber()` with the simulation variant across
AILogic with one `sed` also rewrote `CAILogic::GenerateGroupNumber`, which is the client's
path to the same function, and a commented-out line. Caught only by reading the diff.

Avoid it: when a function is called from both the client and the simulation, list the call
sites with `grep -n` first and edit them by line, or anchor the pattern to the caller's
context. Read `git diff` of every touched file before building.

## Replays: which build and which player

- A replay records the build id that made it (`szImageId`). Find the branch that build came
  from before trusting a crash or a divergence: a command class missing from the current
  checkout loads as null.
- `--replay=` defaults to player 0. Use `--replay-player=N` or the console command
  `replay "<name>" <player>` for another viewpoint.
- `--setvar=name=value` is unambiguous; current builds also accept `--setvar name=value`.

## Replay disconnects and time labels

- A report of `s 3895` can mean elapsed seconds, not the command segment. Read the
  replay's checksum times and segment duration before choosing a dump time. This
  multiplayer recording used 200 ms segments, so its 3894-second disconnect was
  applied at common segment 19470.
- Player removal is simulation input: it stops and transfers units and clears the
  scenario's presence flag. Record the removal at its applied simulation boundary,
  before that segment's orders. A synchronized network drop by itself is not a
  replay command. `last_match_packets.txt` can establish a missing event's exact
  tick; do not infer it from checksums or silently alter historical recordings.
- Use isolated profiles and binaries for replay verification. Manual interaction,
  including pausing, invalidates an unattended baseline; restart it before drawing
  conclusions. Keep the original recording and all its checksum samples when
  testing a recovered copy.

## Intermittent replay divergence and original-build diagnostics

- Repeat each baseline before attributing a difference to the selected replay
  player. The same original executable and recording can alternate between the
  matching and diverging result; one run per viewpoint is not a controlled result.
- Inspect the original build's argument parser. In build 3ac99c1, quoting a simple
  flag such as `--replay-verify` leaves the quotes in the parsed token and silently
  disables the flag. Pass simple flags bare and confirm the verification banner.
  That build also needs the console `replay "name" player` form for its viewpoint.
  Launch with the isolated `bin` directory as the working directory: its startup
  configuration paths are relative to `../Profiles`, not to the executable path.
- RNG history record indices and the actual generator draw counter are separate
  counters. Match call sites and actual draw counts before interpreting offsets.
- Enlarging a history vector can change allocation layout and hide memory bugs.
  Preserve the original allocation when adding diagnostics, or attach a debugger
  to a normally launched isolated process shortly before the event. A debugger
  attached at startup can itself change heap behavior.
- Light simulation checksums cover visible unit state and shells, but not every
  AI state, scheduling deadline or RNG state. Equal checksums do not establish
  equal hidden simulation state immediately before a divergence.

## Repair targets and cached typed pointers

- Trains and artillery are not `CTank` subclasses. The repair-state ASYNC came
  from reading `IsTrackDamaged()` through an invalid cast of an artillery target.
  Use the actual runtime type for class-specific state, and clear the cached
  tank pointer whenever a new non-tank target is selected.
- Serialized caches can preserve a valid pointer to the wrong previous target.
  Refresh derived target pointers before use as well as on target selection.
  In regression fixtures, inspect the saved cache as a common base pointer;
  loading it as `CPtr<CTank>` can hide an invalid train cache by casting it to null.

## Simulation waits and regression fixtures

- This Lua VM cannot suspend a native predicate by sleeping and returning.
  Preserve its function, arguments and caller PC until a later simulation segment;
  handle CALL, TAILCALL and a native StartThread entry separately. Synchronous
  callbacks must report an error when the condition still requires waiting.
  Existing stack, PC and sleep serialization can preserve the pending call.
- Exercise error reporting in Release too. Lua source metadata is omitted there,
  but stack formatting still needs an initialized source pointer.
- In DLL-backed factory tests, an inline null cast can be optimized away and fail
  to load the registering library. Use an actual exported symbol as the link
  anchor. Inspect each inheritance level's serializer before populating fixture
  fields; an infantry formation pointer belongs inside its soldier base record.
- PowerShell does not expand Bash brace or wildcard path operands for `rg`.
  Search a directory with `-g` filters or pass explicit paths instead.

## Editor resource paths and deletion

- A file picker can succeed while the property parser silently rejects its
  result. Check the complete picker-to-controller path, including Windows drive
  colons, and resolve game-relative sources against both the active mod and Data.
- Deleting MPConsts reinforcement links exercised a scalar array undo path that
  appended an empty child name and cast reference metadata to a struct. Preserve
  the element value itself; test non-empty links, repeated removal and undo, and
  clearing the whole array through the real property grid.
- Editor-only database references must own their identity, not borrow raw target
  manipulators. A deleted target can outlive its browser row in another open
  property view. Include renaming, missing targets and saved deletions in tests.
- Pending deletions must suppress on-disk rediscovery until Save or Discard.
  Repeated deletion must be idempotent so recreating a name cannot leave a second
  queued file removal that deletes the replacement.
- For a Windows gtest that catches an access violation without a useful stack,
  run with GTEST_CATCH_EXCEPTIONS=0 to let TestCrashTrace report the fault location.
- Release-only GUI test validation missed wxWidgets' Debug CRT `new` rewrite.
  With module-local mimalloc overrides, that allocates on the CRT heap and frees
  through mimalloc during wxEntryCleanup. Define `__NO_VC_CRTDBG__` in the wx
  allocator hook, and reproduce allocator changes in CI's Debug configuration.
- GUI fixtures must keep narrow argv storage alive through wxEntryCleanup and
  report wx assertions to gtest across startup and teardown. A modal assertion
  dialog otherwise hides the original failure behind a CI timeout.

## Towing approach and attachment order

- A nearby stopped truck must let the artillery pack and rotate toward it before
  aiming for the authored tow point, which may be across the gun or a map border.
  Use a one-sided stopping distance so a simulation step cannot skip a narrow band.
- Hook proximity during a turn does not establish completed attachment geometry.
  Verify the precision path's final position and facing before accepting it; otherwise
  overlapping units can repeatedly turn, stop early, and restart without translating.
- In serialized fixtures, preserve target-position chunks when changing a state's
  phase: missing vector chunks can reset the target and trigger movement interruption.
  Serialize SAIAngle as its nested integer record, and derive geometry from the engine's
  direction vectors rather than assuming heading zero points along the X axis.
- For unattended Windows mission runs, use an isolated profile/data overlay and
  `--setvar=force_app_always_active=1` so losing window focus cannot stall the script.

## Replay command clock ordering

- Multiplayer applies commands before advancing the segment timer. The replay
  transceiver advanced it first, so CAILogic::UnitCommand refreshed curTime one
  tick ahead and contaminated subsequent group commands in that segment.
  In MatchFrom_2026-10-25_20-05-50, marker 8371 preceded camouflage; two fresh
  viewpoints first disagreed with the recording at 1685000 ms. Removing only
  that marker, or suppressing only UnitCommand's clock write in a diagnostic
  DLL copy, independently restored all 1686 checksum/RNG samples through 28:06.
  The clock-only diagnostic then matched all 4200 samples in the original build.
  Fix the dispatch ordering, preserving suspended timer advancement, rather
  than changing group allocation or dropping recorded markers.
- Light replay checksums omit ability deadlines and scheduling state. The first
  checksum mismatch can be later than the command that introduces the error.
  Verify the exact original DLLs and preserve original recordings and checksums
  when constructing a targeted diagnostic copy.
- Do not treat that old recording as a validation oracle for a newer branch's
  changed simulation. Prove the cause using the original build, then validate
  the source fix with focused timer/dispatch tests against the current branch.

## Artillery crew visibility

- Serving soldiers retain their cached visibility and rely on artillery to update
  it. Attaching a previously hidden crew to an already-visible gun needs an explicit
  visibility sync for every party, plus the normal client notification, even if the
  gun's visibility did not change. Recalculate after ownership changes so the new
  owner sees its crew immediately.
- The infantry factory creates `CInfantry`, a subclass of `CSoldier`. Serialized
  fixtures need both base-class chunks before the `CAIUnit` visibility fields.
- A Windows DLL-only relink can leave the test directory's copied DLL stale when
  the import library does not change. Check the copy's hash or timestamp and refresh
  it before judging a regression test against the new implementation.

## Multiplayer base warnings

- Base-circle threats emit `EFB_KEYBUILDING_ATTACKED` in `CBuildingSimple::Segment`,
  before friendly defenders can cancel capture progress. Recipient filtering belongs
  only around the local feedback call; it must not gate enemy detection or capture state.
