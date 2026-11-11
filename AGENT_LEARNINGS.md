
# AGENT LEARNINGS

This file is meant for AI agents only, both for read and write purposes.

- Do not validate UTF-8 by round-tripping through wchar_t. Linux iconv can
  preserve values above U+10FFFF that Windows rejects. Validate shortest UTF-8
  encodings and Unicode scalar ranges explicitly, with boundary tests for
  surrogates, supplementary characters, truncation and legacy 5/6-byte forms.

- Resolve app_name.txt from the installation root before selecting either user
  folder. Keep portable name validation shared across platforms, including
  Windows device names, and append the variant beneath an explicit user-data
  override too. Test with explicit roots rather than the user's real AppData.

- `ResolveDataPathCase` takes a base directory ending in a separator and a
  relative path, and returns only the relative portion. For absolute settings
  paths, split `root_path()` from `relative_path()` and restore the root after
  resolving. Passing an absolute path with an empty base searches the CWD on
  Linux; Windows hides the mistake because its case resolver is a no-op.
- For standalone WSL checks, stage cached headers in native temporary storage;
  searching many Boost include directories through `/mnt/c` is very slow.
  Use `-iquote` for local System headers: `-I` on the case-insensitive Windows
  mount lets `System/Time.h` shadow the standard `<time.h>` header.

- Physical Editor settings and database IDs under Data/Editor are different
  namespaces. Redirect physical files, not VFS IDs, and keep szStartFolder as
  the installation root for game resources. Audit direct CFileStream/ifstream
  consumers as well as the shared XML helpers. Module menus are assembled in
  CWxMainFrame::AddMenuResources, where shared File actions can be added once.

- Editor startup errors should use NMessage::Error with an explicit title before
  user data is loaded. Including port/messagebox.h introduces an SDL dependency
  that MapEditor does not declare. The Linux Clang CI job skips the editor, so
  passing it does not validate changes to editor code.

- Per-user profiles must be initialized before startup.cfg, including in the editor.
  Nested exec calls need a saved include context and storage class; the old handler
  prepended `..` and could reload installation defaults after profiles moved.
- Known Folder declarations are hidden by this tree's XP-era SDK target macros.
  Keep modern Windows API adapters in a small translation unit without the legacy
  PCH. Standalone tests that include Windows headers also need NOMINMAX before
  Boost UUID headers; otherwise min/max macros produce misleading template errors.
- Secret Service lookup attributes are public metadata. Store the username inside
  the protected payload, and check locked items when deleting: password_clear only
  removes unlocked entries. Keyring calls belong off the login UI thread.

- `CMilitaryCar::Segment` updates the ability state, then guns, then turrets. An idle gun can therefore request a turret return after Binoculars has aimed it. Suppress the return at execution time for the turret held by the active ability, rather than clearing the request only during aiming. Check cancellation and loaded states as well as unrelated turrets.

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

- Automatic exit from existing entrenchment must use `ACTION_MOVE_LEAVE_SELF_ENTRENCH`, including the formation factory whitelist. Casualties rebuild a squad's public command mask and can remove its dig-in ability while survivors remain dug in. Requeueing movement ahead of a rejected public ability command creates an infinite queue retry. Test the real `Segment`, preserved movement order, and completion in a live mission.
- A partial binary fixture resets omitted primitive fields to zero. Mistake: assigning unique IDs before loading merged the formation and soldier command queues. Assign distinct IDs after loading or serialize them explicitly. Internal action IDs are above 1000 and must never index a public command bitmask. Inspect the concrete state type when it has no `GetName` override.
- Legacy Lua `Sleep` takes 50 ms ticks while simulation advances in 200 ms segments. Truncating values 1 through 3 to zero prevents yielding; asynchronous waits need a minimum of one segment. Mission fixture scripts require `return 0;` instead of bare `return;`, and live console traces require `game_writelog=1` (written to `console.txt`).
- Death/removal callbacks do not necessarily remove an already-dead or stale member. Consume stale container entries or traverse a bounded snapshot, preserve normal death callbacks, and protect units whose ownership changed. Dead soldiers normally retain their owner backlinks for corpse processing; clearing those indiscriminately changes unrelated behavior.

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

## Autonomous aircraft targeting

- The legacy light-AA estimator can select an aircraft without any current firing
  solution. Zero-setup artillery uses the general mech attack state, which can pursue
  it. Restrict both acquisition and the saved automatic attack state to prevent a
  plane leaving range from restarting pursuit; explicit orders need their own path.
- When testing DLL-backed command queues, check exports for every new out-of-line
  call, including existing accessors such as GetCurCmd. Missing one surfaces only at
  the final Windows link after a large header rebuild.

- gtest here is a DLL on Windows but its `GTEST_LINKED_AS_SHARED_LIBRARY` is install-interface only, so tests compile without dllimport: functions link, data (`GTEST_FLAG_SET`) does not. `add_unit_test` now defines it. Never put it on the gtest target, because gtest_main defines `main` with `GTEST_API_`.
- A benchmark or test that includes `3Dmotor/stdafx.h` gets `windows.h` without NOMINMAX; it only compiles beside `<limits>::max()` because glm's `_fixes.hpp` happens to undefine the macros. Do not read "the neighbouring benchmark compiles" as proof.
- A test that never linked has never run. When fixing a link failure, run it under CI's configuration: `NormalTransform_test` took 17 s on Clang, 137 s on RelWithDebInfo and 727 s on Windows Debug, past ctest's 600 s. Time heavy tests in the Debug tree before pushing, and check every workflow on the commit, not only the linked job.
- `static_cast<int>` of NaN or infinity is undefined, and clang really does differ from `cvttss2si` (0x80 and 0xff against 0). When SSE and scalar kernels agree with each other but not with a live baseline, on one compiler only, look for undefined behaviour in the baseline.
- Mistake (2026-11-04): ran `python -I - <<'EOF'` with an empty heredoc ahead of a `python -c` edit, and the shell hung on the REPL it started. The memory note already forbade bare `python -`. Pattern to avoid: any `python -` at all, even as a no-op. Better: put the edit in a script file in the scratchpad and run `python -I script.py`, or use `python -I -c` alone.
- A tool that replays something the model editor did must load the editor's configs (`startup.cfg`, `editor.cfg`, `autoexec.cfg`), not the game's. `game.cfg` sets `camera_near_plane = 40`, so editor icon shots taken from closer than 40 (jeeps, motorcycles, small guns) rendered empty, and it ends with `main_menu`. Check `camera_near_plane`/`camera_far_plane` first when a render comes back blank.
- Mistake (2026-11-05): ran a bare `python -` again, as a leftover no-op in a chained command, and it hung until stopped. Never chain `python -` into anything; use `python -I -c` or a script file.
- `.gitignore` carries the macOS `Icon?` pattern, and this clone matches case-insensitively, so a new `scripts/icons/` folder is silently ignored and never shows in `git status`. Run `git check-ignore -v` on a new folder whose files do not appear as untracked.
- Zip entry names without flag bit 11 are CP437 by the format, and older mod paks use it (MOD-18: `Sturmm\x94rser`). `CZipFile` now converts them to UTF-8 when it reads the directory; before that, `u8path` in `NVFS::PreprocessPath` threw and ended the process. A new Windows executable that writes files named from game data needs `Game/utf8.manifest` in its sources, or narrow Win32 calls use the ANSI code page and produce mojibake paths.

## Editor icon exports

- Hardware UI quads already use bilinear filtering, but their atlas has only one
  mip level. Large icon reductions need separate mipmapped textures, not just a
  sampler-state change. Generate alpha-weighted mips at load time so existing paks
  benefit too; the outline compositor needs its own filtered CPU sampling.

- Mistake (2026-11-05): exported high-resolution icons with their pixel dimensions
  in Texture Width/Height. The 42x42 UI then sampled a transparent corner, although
  archive loading and image roundtrip tests passed. Keep the logical UI footprint
  separate from DDS dimensions and verify actual UI drawing, including outlines.
  The software texture iterator also assumed power-of-two dimensions; test 42x42,
  rectangular non-power-of-two images, and one-pixel dimensions as well as 512x512.

- `CArray2D` is indexed `[y][x]`. The legacy square icon crop hid swapped axes;
  exercise non-square source and output sizes when changing image dimensions.
- `CStringManager::SplitFileName` returns backslashes even on Linux. Normalize
  its directory result before joining a physical output path.
- Icon camera fields are editor-only metadata, available through `IObjMan`.
  A private `IGameView` and camera can render units without replacing the open
  editor scene; lock manual scrolling on that camera as well as auto positioning.
- A new test linking MapEditorLib needed an explicit `gr2` dependency for its
  Windows runtime copy. The `granny::granny` interface dependency did not put
  `granny2_x64.dll` beside that executable; inspect DLL imports on 0xc0000135.
- Archive exports need the editor binding serializer to retain hidden camera
  metadata without saving unrelated edits. Package the texture dimensions and
  unit references as well as DDS pixels, and test them through the game VFS.
- `wxZipOutputStream` marks non-ASCII names as UTF-8 when given `wxConvUTF8`;
  `wxZipEntry::SetFlags` is protected. Store UTC wall-clock entry timestamps to
  match the VFS's comparison with loose files.
- A headless test must not call `Is3DActive` to anchor the renderer DLL: it
  dereferences the graphics device. `GetFrameTransitionCapture2` returns the
  static target holder without requiring a device. Tests linking mech stats
  also need the FFmpeg runtime PATH and a UTF-8 manifest for non-ASCII fixtures.
- On Windows, `std::filesystem::rename` can reject replacement of a mounted
  archive even with delete sharing. `ReplaceFileW` succeeds while existing
  mapped readers retain the old bytes; use it as the atomic writer's fallback
  and test replacement with both old and new archive mounts alive.
- IconRender and editor batch exports now share `SceneB2/UnitIconRendering`.
  Keep camera corrections in `scripts/iconrender/icon_overrides.txt`; CMake embeds
  that file for both callers. The editor supplies its saved lighting/grid and
  reads live camera metadata, while the CLI reads the hidden fields from XML.
- Camera regression fixtures must call `IsPropertyModified` before changing a
  value to enable its lazy baseline capture. Checking only after the edit reports
  false even when the unsaved value is correctly retained.
- Files embedded at CMake configure time must be included in every build job's
  sparse checkout, including tests and benchmarks. SceneB2's shared icon overrides
  live under scripts/iconrender, outside the engine source tree; a full local
  checkout hid the missing CI dependency on Windows and Linux.
- Linux CI promotes enum-mixing warnings to errors. Use EPixelFormat constants
  when selecting between pixel formats, since SPixel4444::ID and SPixel8888::ID
  belong to separate anonymous enums. Use wxSizerFlags for combined layout and
  border settings instead of bitwise operations across wxWidgets enum types.
- Running IconRender from the Windows build tree needs `gr2` in its link closure
  and the FFmpeg `bin` directory on PATH for codec DLL dependencies. An exit code
  of 0xc0000135 before any output is a loader failure, not an empty icon render.
- Mistake: the private icon view omitted Scene::AddObject's model precache and
  treated WaitForLoad(true) as a blocking load. It only sets a flag for Draw;
  geometry can finish loading after first-frame bounds were evaluated. Precache
  the model and call LoadEverything before RenderTarget advances the DG frame.
  A few isolated renders missed this: full 512x512 batches failed on different
  valid units across cold runs, while the corrected batches rendered all 396
  model-backed records in the installed 403-unit data set.

- Mech weapon selection uses `CTankShootEstimator::ChooseGun`. Its infantry MG
  preference sets shots and bursts to zero, subtracting one relax time from the
  score. Check actual weapon records before blaming that branch: BK2E-GBC's DT,
  Besa and Browning vehicle MGs are tagged WEAPON_HEAVY_CANNON. The ordinary
  estimator also charges an entire burst even when its first shot can kill;
  the common vehicle flamethrower's two-shot burst scores 4 seconds before travel
  and turning. Identify flames by shell trajectory, not weapon category.
- Legacy SWARM scans call `LookForTarget`, which immediately returns for a live
  current enemy. Updating only the scan's cached best gun cannot fix weapon
  choice. Flame-equipped units now explicitly reevaluate that enemy at the scan
  cadence between bursts. Release only the previous gun during a switch;
  `CMechAttackUnitState::StopFire` stops every weapon. Secondary free fire cannot
  move the hull controlled by the active gun, so the close-range flame preference
  does not close the gap after an MG engages outside flame range.
- Headless firing fixtures need more than selection fixtures: initialize the
  owner's path/collision state and animation controller, the fog grid used by
  MG point ratings, and a current command for empty-ammo reselection. Free
  soldiers need wMaxAngle = 65535, and spatial visibility needs nVisIndexInUnits
  as well as visible4Party. Missing these produced fixture crashes and false
  firing rejections; exercise the real secondary-fire scan to catch them.

## Host migration and recorded removal ownership

- Compare packet-level applied removal segments before investigating combat UB.
  In the 2026-11-09 match, backend removal reached the replacement host before
  the departing host's leave packet. Local promotion caused rejection of that
  in-flight packet: Musikant removed slot 0 at 7782, other survivors at 7781.
  Recovered replay copies differing only in that tick reproduced both exact
  checksum and RNG states. Host handoff needs a converged boundary before it
  executes; taking the minimum after simulation passes it cannot repair history.
- Command history retains CPtr, not CObj. A local CObj in PlayerRemoved invalidates
  its recorded CDropPlayerCommand on scope exit, reconstructing it with player -1.
  The replay then misreports the invalid object as an unknown command type. Test
  the real recording ownership lifecycle, not only raw-new insertion into history.
- Preserve original replay bytes and checksum samples when recovering invalid
  removal records from packet logs. Label diagnostic copies and record changed
  validity bytes, player fields, segment keys, and hashes. Build-ID disagreement
  must remain visible even when full checksum/RNG reproduction succeeds.
- Apply queued removals in slot order at command dispatch, after the manager
  drains packets. Applying inside a packet handler makes simultaneous transfers
  depend on arrival order. Remember applied cutoffs and stop on an earlier
  correction after execution; this simulation has no rollback.
- A packet P's commands execute at P + pack size + latency. Releasing a leaver's
  ordinary receipt barrier must still require the command prefix before removal.
  Keep a separate timeout for missing input from an already disconnected peer.
- Host authority can change twice before a correction arrives. Preserve the
  retiring host's correction authority for already-known departures, and defer
  a future host's authenticated self-departure until its preceding handoff.
  Exercise actual manager packet handling and recorded removal order, as well
  as the portable scheduling policy.

## Reliable transport and timeout voting

- A retransmission alias can outlive its ACK record after immediate send failure
  or a timer jump that expires the record before the retry creates the alias.
  Retire the old alias at every packet-ID reuse, including empty transmissions.
  Test a complete 16-bit cycle and preserve valid delayed-ACK retry chains.
- A bounded ring is not a message-size limit when messages can stream through
  it. Both outgoing enqueue and incoming frame assembly must make incremental
  progress. Exercise the former boundary, multiple large frames, a missing
  leading fragment, reordered/duplicated packets, and malformed length headers.
  Allocate according to received data rather than an untrusted advertised size.
- Timeout votes represent current observations. Match current eligible voters,
  scrub departed voters, renew continued waits, and withdraw or expire approvals
  after recovery. Different peers can observe recovery at different times;
  test two actual managers and delivery of a newly generated retry.
- CMake's copied runtime DLL can remain stale when only the DLL implementation
  changes and a dependent test does not relink. Verify test-local DLL hashes
  against built modules before accepting results after network-library changes.
- Manual MP pause returns from DoSegments before refreshing lag reports, while
  packet handling and timeout voting continue. Received command data can leave
  a stale manager lag mask that two paused observers use to kick a recovered
  peer. Test accepted receipt packets and fresh votes through actual handlers;
  suppressing simulation must not suppress required-input status refresh.
- The original explicit remote pause packet changed only metadata. Its UI waited
  for the separate five-second missing-input threshold, and owner/observer
  allowances remained different after resume. Distinguish simulation gating,
  pause notification, and budget ownership when diagnosing a late pause screen.
- Keep raw receipt waits separate from explicit pause reasons, and charge their
  union once. Settle and rebase the active interval before applying a received
  remaining-budget snapshot. Publish the owner's charged allowance on resume;
  observers' later-starting clocks cannot reconcile themselves to it otherwise.
- Verify recovered input after draining the entire packet batch and before kick
  consensus. A queued vote can precede the recovery packet in that same batch.
  Test both the no-kick case and a genuinely missing peer that still times out;
  excluding every voter alone can conceal stale observation bugs.
