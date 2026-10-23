
# AGENT LEARNINGS

This file is meant for AI agents only, both for read and write purposes.

- UI text fitting: glyph advances and padded line cells are not visible letter bounds. PT Sans's initial `J` can extend left of its origin. Try the smallest translation that fits the visible letters before removing runtime font enlargement, and use that same corrected origin when drawing placed captions. Correct horizontal overhang per line before combining paragraph bounds; different lines can overhang opposite edges. Keep intentional scroll viewport clipping out of font fitting. Round wrapping constraints inward and autosized bounds outward when converting between screen pixels and virtual coordinates.
- Font regressions should use the actual control, style, parent clip, and shipped font. Encyclopedia names occur in both the body-font list and the centered heading; testing only the heading misses list failures. Include 800x600, the 1024x768 reference size, widescreen and ultrawide sizes, and 4K.
- Broader glyph tests can use the six runtime sizing code pages from `NCodePages`. Check `CFace::HasGlyph` so replacement glyphs do not masquerade as coverage of unsupported characters, and exclude raw markup delimiters from plain-text samples. UI bound checks and independent bitmap coverage tests complement each other.
- Windows builds here use Visual Studio's `Launch-VsDevShell.ps1` and bundled CMake. If sandboxed Ninja stalls before compilation, stop only the processes started for that build and retry through the approval tool. The bundled formatter is under `VC/Tools/Llvm/x64/bin`; pass it to the hygiene wrapper with `CLANG_FORMAT`.
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
