# Headless test harness: what exists, what blocks it, how to drive it

The determinism work in `docs/Determinism.md` is mostly gated on being able to
replay a recorded game headlessly and compare per-segment checksums. This is the
record of a spike into whether that is possible, what it costs, and how the
result would be driven and extended.

Nothing here is built yet.

## The simulation is already separable from the interface

The fear going in was that mission bring-up would be entangled with the
interface layer, because `GameX/InterfaceMissionInternal.cpp` is 4380 lines and
the real `StartMission` call sits at line 4102. That turned out to be a red
herring. Bring-up is not in that file.

- **`CreateAI()` is already `AILOGIC_EXPORT`** (`AILogic/CreateAI.cpp`) and is
  called from `Game/main.cpp:304`. The simulation can be created with no GameX
  involvement.
- **`IAILogic` is a complete simulation API**:
  `Init( ICheckSumLog *, const NDb::SMapInfo *, const NDb::SAIGameConsts *, IAIScenarioTracker * )`,
  `InitAfterMapLoad`, `PostMapLoad`, `ToggleWarFog`, `Segment`, `ClearAI`,
  `IsSuspended`, `SetNetGame`, `NetGameStarted`.
- **The tick pump is about ten lines**, in `CSinglePlayerTransceiver::DoSegments`:
  `pTimer->BeginSegments()`, then
  `while ( pTimer->NextSegment() ) { pCmdsHistory->ExecuteSegmentCommands( nCommonSegment, this ); pAI->Segment(); ++nCommonSegment; }`.
- **The whole init sequence is `CInterfaceMissionBase::NewMap`**, in a 146-line
  file. Of its dozen statements, four are `IAILogic` calls and the rest are
  camera limits, cursor, light, music and sound.
- **`CICMission::PostCreate` confirms the direction of dependency.** The
  interface is handed `(map, transceiver, scenario tracker, player)` as a
  consumer. It does not own simulation state.
- **A replay carries its own map.** `CreateSinglePlayerTransceiver( replay, pAI )`
  loads the history and `pTrans->GetMap()` returns the recorded
  `NDb::SMapInfo`, so there is no menu navigation to simulate.

## What actually blocks it: terrain

One line in `CWorldClient::LoadMap` is load-bearing:

```cpp
pTerraManager->SetAIObserver( pAI->CreateTerraAIObserver( nSizeX, nSizeY ) );
NScene::LoadTerrain( pTerraManager, pMapInfo, szMapFilePath );
```

The simulation's `AIMap`, `Terrain` and `StaticMapHeights` are populated as a
side effect of the renderer loading terrain, pushed through `ITerraAIObserver`.
`NScene::LoadTerrain` and `ITerraManager` live in `SceneB2`, which links
`3DLib 3Dmotor B2_M1_Terrain DebugTools Image Input libdb MemoryLib Misc Sound Stats_B2_M1 System UI zlib granny`
plus SDL3. Linking the terrain loader pulls in the entire renderer.

Worse, the AI's terrain is **derived, not parsed**. `CTerraGen::Load` runs a
pipeline: parse `terra.bin`, then `UpdateHeightsAfterRivers`,
`PutAllCragsOnTerrain`, `PutAllRiversOnTerrain`, `UpdateRiversDepthes`,
`AddAllRoads`, `UpdateHeightsAfterCrags`, `UpdateNormals`,
`UpdateAllNeededPrecipices`. Rivers carve the heights the AI later reads.

### Size of the thing

`CTerraGen` is **not** a small class. Its methods are spread across 26 `.cpp`
files totalling about 10,936 lines, plus roughly 1,033 lines of headers
(`GenTerrain.h` 402, `TerrainInfo.h` 428, `TerraGen.h` 203). An earlier estimate
of 703 lines was wrong: it measured only the file bearing the class name.

The encouraging part is that the AI and graphics halves are already separated at
the connector layer:

- **`TerraAIConnector.cpp`, 302 lines**, contains every `pAIObserver->` call in
  the tree: `UpdateAITerraTypes`, `SetTerraTypesToAI`, `UpdateAIInfo`,
  `UpdateAllAIInfo`, `PutCragToAI`, `PutRiverToAI`, `PutRoadToAI`,
  `PutRiverCliffsToAI`, `PutAllWaterToAI`, `RegenerateAIInfo`. It reads from
  `terrainAIInfo.heights` and `.terrTypes`, and `TerrainAIInfo.h` is a dedicated
  20-line struct holding exactly that view.
- **`TerraGFXConnector.cpp`, 861 lines**, is the symmetric graphics half.

The problem is the layer underneath. Both connectors read data produced by the
derivation builders (`TerraHeight` 19 methods, `RiversBuilder` 23,
`CragsBuilder` 19, `RoadsBuilder` 11, `PrecipicesManager` 7, `TileMasks` 6,
`TerraGeomModifier` 6), and those are members of the same class, interleaved
with the graphics builders. Separating them is splitting a 12,000-line class
with height-mutating derivation in the middle.

`ITerraAIObserver` itself is a narrow, pure-data interface, which is what makes
the extraction thinkable at all:

```cpp
UpdateHeights( x1, y1, x2, y2, const CArray2D<float> & )
SetTerraTypes( const std::vector<NDb::STerrainAIProperties> & )
UpdateTypes( x1, y1, x2, y2, const CArray2D<uint8_t> & )
AddRoad / AddRiver / AddCrag / AddWaterLine( const NDb::SVSOInstance * )
AddRiverCliffTiles( const std::list<SVector> & ) / FinalizeUpdates()
```

## Three options

**A. Headless real engine. Recommended.** Run the real game with no window and a
software Vulkan device, driven by a replay. Real `SceneB2`, real terrain
pipeline, so **drift is impossible by construction**, which is the property that
matters most in a verification tool. `lvp_icd.json` is present, so lavapipe is
available and DXVK can run with no GPU. Costs: software rasterisation is slow,
CI needs mesa, and it renders frames nobody looks at. Refinable later with a
d3d9 null device or a DXVK path that skips most rasterisation.

**B. Extract the terrain data core.** The proper long-term fix, priced at weeks.
Split `CTerraGen` into a data and derivation core moved to `B2_M1_Terrain`,
taking `TerraAIConnector.cpp` and `TerrainAIInfo.h`, leaving the graphics layer
in `SceneB2`. Acceptance criterion is clean: terrain renders identically and the
observer receives byte-identical heights and types.

**C. A parallel headless terrain reader. Rejected.** It would re-implement a
derivation pipeline, not a file parser, so it would drift. Drift here is not a
maintenance nuisance; it means the harness validates a simulation that is not
the one that ships.

Note that A makes two exotic ideas from `docs/Determinism.md` harder: the
`aarch64` cross-build under qemu and Fil-C as an oracle both assumed a
renderer-free binary. Neither survives DXVK and software Vulkan. Those need B.

**Do A now, B when throughput demands it.** See the fuzzing section for what
makes that demand concrete.

## It is not a chicken-and-egg problem

The apparent circularity, needing the harness to refactor safely and needing a
refactor to build the harness, is an artefact of insisting the harness be a
lean, renderer-free binary. Option A is purely additive and therefore carries no
ASYNC risk, because no code the simulation executes is modified:

- `Game/main.cpp` can already **launch straight into a mission from the command
  line**. `szEditorMap` and `--editor-map=` exist so the editor can start a map
  directly, bypassing menus. That was the hard part.
- Replay playback is already a shipping feature (`CICMission` with `bReplay` and
  `szReplayFileName`, `GameX/InterfaceReplaySaveLoad.cpp`).
- Divergence detection already exists: `AddChecksumLog` returns false when a
  recomputed checksum differs from the recorded one.
- `ProcessCommandLine` has an extensible flag pattern to hang `--replay=` on.

What is new is a flag, a mode that skips window creation, and an exit code.

### The general escape, in order of preference

1. **Build the instrument without touching its subject.** Additive only. Pay in
   performance and elegance, pay nothing in risk.
2. **Meanwhile do the work whose safety is provable rather than tested.** The
   file hygiene phases (diff equivalence is a mathematical property), dead code
   removal (no callers means no behaviour), diagnostic-only fixes.
3. **Then use the harness for the risky refactor.** Record replays against the
   old code, refactor, replay, compare.
4. **For any specific refactor, look for a narrower oracle than the full
   harness.**

### Move 4 applied to the terrain extraction

Option B does not need the determinism harness at all. It needs one question
answered: does the observer receive the same bytes? That is roughly twenty lines
of additive instrumentation at `ITerraAIObserver::UpdateHeights` and
`UpdateTypes`, dumping `terrainAIInfo.heights` and `.terrTypes` after load,
across a set of maps, before and after the refactor, compared with `cmp`.

It is a **stronger** oracle than a replay, not a weaker one: a replay samples
whatever ground one playthrough walked over, a dump is exhaustive across the
whole map. Cheaper and better. This unblocks B independently of everything else.

## Driving it

Two different needs want different answers.

**Replaying recorded games needs no control surface.** The replay file is the
script: it carries the start RNG seed and the per-segment command list, and
`ExecuteSegmentCommands` feeds them in. No input simulation, no scripting, no
IPC.

**Authoring new tests wants Lua.** `AILogic/Scripts.cpp` registers **152 script
commands**, and they are not only queries:

- orders: `GiveCommand`, `GiveQCommand`, `AttackGroupCreate` / `AddUnit` /
  `StartAttack`, `ChangeFormation`
- mutation: `DamageObject`, `DamageAllUnits`, `SetAmmo`, `AddAmmoToAllUnits`,
  `CallReinforcement`, `RemoveAllUnitsTmp`, `ReturnAllUnits`
- observation: `GetUnitState`, `GetObjectHPs`, `GetUnitRPGStats`,
  `GetUnitListInArea`, `GetNUnitsOfType`, `IsUnitInArea`, `GetGameTime`
- randomness: `RandomFloat`, `RandomInt`

Crucially `Scripts.cpp` lives **inside** `AILogic`, so scripts execute on every
client through the same command path multiplayer uses. A Lua-driven test is
deterministic by construction and exercises the real lockstep path.

Only assertions are missing, and `CallAssert` shows how cheap they are, since it
exists already as a stub that fires unconditionally:

```cpp
int CScripts::CallAssert( struct lua_State *pState )
{
    NI_ASSERT( false, "You are welcome!" );
```

`Test.Assert( cond, msg )`, `Test.ExpectDestroyed( id )` and
`Test.Finish( ok )` are one row each in the registration table near line 316.

### Rejected ways of driving it

**Python or ctypes driving the live simulation.** Either it routes commands
through `SendAICommand`, in which case it duplicates what Lua does with far more
plumbing, or it bypasses it, in which case the test is neither reproducible nor
representative of multiplayer and so validates the wrong path. It would also
need blocking per-segment IPC, which is fragile and would itself become a source
of non-determinism. `IAILogic` is a `CObjectBase` with cross-DLL
`CastToObjectBase` casting, which is awkward to project through a C ABI anyway.

Python and YAML do have a good role: **offline generation**. Author scenarios in
YAML, generate Lua or a synthetic command file, let the engine consume the
artefact. Authoring ergonomics, zero new runtime surface, full determinism, and
the generated output is diffable.

**Simulated mouse and keyboard.** The most fragile layer available, and
unnecessary: `MouseTranslator` to command is a thin layer the replay system
already bypasses by recording at the command level. Commands are also the
representation the netcode transmits.

## RNG and test fixtures

`RandomFloat` and `RandomInt` draw from the simulation stream:

```cpp
RandomFloat -> NRandom::Random( 0.0f, 1.0f ); RecordRandomCall();
RandomInt   -> NRandom::Random( 0, n - 1 );   RecordRandomCall();
```

A second pool is mechanically possible: `System/RandomGen.h` exports
`IRandomSeed`, `SetRandomSeed( IRandomSeed * )` and `CreateRandomSeedCopy()`,
and `NWin32Random` in `Misc/Win32Random.h` is precedent for a separate
presentation generator kept out of the simulation stream.

**But the existing functions cannot be repointed.** 255 call sites across 55
shipping campaign scripts (249 `RandomInt`, 6 `RandomFloat`). Changing which
stream they draw from changes the sequence for every campaign mission,
invalidating saves, replays and balance, which is exactly the class of change
that needs the harness and so cannot be a step toward building it. A fixture
pool would have to be new functions alongside the existing ones.

**And a second pool does not escape determinism.** Lua runs inside `AILogic` on
every client, so a fixture pool must still be seeded identically everywhere and
still be part of the saved and checksummed state. What it buys is decoupling: a
fixture draw advances only the fixture counter, so adding randomness to a
scenario stops shifting the combat stream and existing replays survive scenario
edits.

**For tests, prefer no randomness at all.** Fixed coordinates, fixed types,
fixed timings. Random placement makes the fixture part of what is being
measured, and then the seed has to stay stable forever or the test's meaning
drifts. Determinism by construction beats determinism by seeding.

## Fuzzing

Put the randomness **outside** the simulation. The fuzzer generates a case with
its own seed, writes it as a command stream or a Lua scenario, and the engine
replays it deterministically. No new RNG pool inside `AILogic`.

A lockstep engine gives this for free: **a failing fuzz case is already a replay
file**, so every finding is a self-contained reproducible regression test, and
minimisation is truncating the command list.

### Oracles, cheapest first

1. **Crash, hang, assert or sanitizer report.** Strongest signal, free.
2. **Run twice, compare.** Catches uninitialised reads, address dependence,
   iteration order.
3. **Cross-configuration divergence.** Windows against Linux, libstdc++ against
   libc++, `-O0` against `-O2`. Catches the UB and unspecified-behaviour
   classes, needs no expected output, never rots.
4. **Invariant assertions.** Needed when the failure is a wrong number rather
   than a crash.

### What random search is bad at

Overflow-shaped bugs, the kind where a counter breaks because an improbable
number of things happen in one frame, are close to unreachable by random play.
Random search finds the middle of the distribution, not the extremes.

Two better answers. First, `-fsanitize=integer` catches unsigned overflow,
implicit truncation and implicit sign change **directly**, at the instant the
arithmetic happens, with a stack trace. That is the detector for that bug class
and it is nearly free. Second, extreme cases are usually cheaper to write
deliberately than to find randomly, and this engine makes them directly
expressible: `DamageAllUnits` is a registered script command, as are
`DamageObject`, `SetAmmo` and `CallReinforcement`.

So: use deliberate extreme-value scenarios for the cases you can think of, and
the fuzzer for the ones you cannot.

### Throughput is what justifies option B

`IGameTimer` has `SetSpeed` and `GetMaxSpeed`, so replays already run faster
than real time. But option A software-rasterises frames nobody looks at, and
fuzzing wants thousands of runs. **Fuzzing is the use case that pays for the
terrain extraction**, where the determinism harness alone does not. That is the
trigger for doing B.

Coverage-guided fuzzing is probably not worth it here: a game tick is slow
enough that the iterations per second that makes AFL-style fuzzing effective is
not reachable even with a lean runner.

## Invariants, with unit death as the worked example

There is no `CheckInvariants`, `ValidateState` or equivalent anywhere in the
tree, so this is greenfield. `theBalanceTest` is prior art for test
instrumentation living inside the simulation.

### Read the deregistration, do not brainstorm

`Die()` is virtual with 25 implementations, all of which chain to
`CAIUnit::Die` (`AILogic/AIUnit.cpp:656`), which calls `PrepareToDelete`
(`AIUnit.cpp:581`). That function **is** the invariant list, written by the
original authors:

```cpp
SetAlive( false );                              theBalanceTest.UnitDead( this );
Stop();                                         GetTerrain()->RemoveTemporaryUnlocking( GetUniqueId() );
theAAManager.RemoveAA( pAntiArtillery );        theGroupLogic.DelUnitFromGroup( this );
theGroupLogic.DelUnitFromSpecialGroup( this );  DelCmdQueue( GetUniqueId() );
theAckManager.UnitDead( this );                 theCombatEstimator.DelUnit( this );
pUnitInfoForGeneral->Die();                     theFeedBackSystem.RemovedAllFeedbacks( nUniqueID );
GetState()->TryInterruptState( 0 );             SetOffTankPit();
theGroupLogic.UnregisterSegments( this );
```

Every deregistration implies a postcondition: after death the unit is absent
from that subsystem. That enumerates eleven invariants mechanically. More
usefully, **omissions from the list are candidate bugs**, so comparing it
against intuition is a better exercise than enumerating from scratch. Killer
experience and fog of war are not in it; experience appears to go through
`theStatistics.UnitDead( this )` in `Die`, and fog through visibility
recalculation, so both are elsewhere.

### Death is deferred, so the hook goes after reaping

```cpp
PrepareToDelete();
theGraveyard.AddToSoonBeDead( this, fDamage );
if ( fromExplosion && GetStats()->IsInfantry() && IsFree() )
    timeToDeath = curTime + NRandom::Random( 0, 1000 );
else
    timeToDeath = curTime;
```

Postconditions do not hold when `Die()` returns. The unit is deregistered but
still in the graveyard awaiting reaping, so an invariant check placed after
`Die()` would report false failures. It belongs after the graveyard reaps.

That window is itself a bug surface: a unit persists in a half-dead state for up
to 1000 ms of game time, which makes "can a dead unit accept commands or use an
ability" a real question. `CSoldier::Die` transfers detonatable charges to a
formation member and raises `EID_ABILITY_ENABLE` during exactly that window.

Note also that the death path **draws from the simulation RNG**, so the number
of draws depends on how many infantry die from explosions.

### The simultaneous-death gap

The stagger above applies only to infantry dying from explosions, and the
comment above it reads:

```
// for soldier's only, to ensure that soldiers don't die simualteneously
// better to create executors for that
```

So simultaneous mass death was a known problem, worked around with randomised
stagger, and the comment marks the workaround as the wrong design. **Vehicles
get `timeToDeath = curTime`, all simultaneous, with no mitigation.** A test that
destroys many vehicles in one frame aims directly at the unmitigated case.

### Two classes of invariant

**Structural** invariants are cheap, universal and assertable from state alone:
absent from groups, command queues, the AA manager, the combat estimator, the
feedback system, pathfinding locks. Enumerable from `PrepareToDelete`, needing
no before-state.

**Behavioural deltas** are not invariants: killer gained experience, fog
recomputed, attackers' orders changed. They need a before and after snapshot, so
they belong in Lua scenario tests, where `GetUnitRPGStats`, `GetObjectHPs` and
`GetUnitState` already exist to observe them.

### A generic detector for the invariants nobody enumerated

Reuse the serializer. Every simulation object implements
`operator&( IBinSaver & )`, and `CCheckSumSaver` already walks the entire
serializable simulation state every segment. An auditor variant of that walk
could collect every unit id and pointer it encounters and assert that none
refers to a reaped unit.

That gives a generic dangling-reference detector for the whole class "something
still holds a dead unit", including the subsystems nobody thought to enumerate,
built on machinery that exists and is already exercised each tick. Cost is
O(state) per check, so it belongs in debug and fuzz builds, run once per segment
rather than once per death.

## Where replay verification stands

Built and working. A single-player custom mission records and verifies end to
end: a full-state checksum every segment, 116 of 116 samples matching, the same
result on repeated runs. The replay carries the ISAAC seed it started from, the
checksum samples with the random draw count at each one, and enough of the
starting state to reproduce it.

```
./launch.sh debug --data fotr --setvar checksumperiod=1 --setvar checksum_trace=1
# play, then in the console: save_replay NAME
./launch.sh debug --data fotr --replay NAME --verify --software-vk
# exit status 0 only if it played out and every recorded sample matched
```

### Sampling is a property of the recording

A sample is not a snapshot. `CAILogic::checkSum` is a member and
`UpdateCheckSum` folds each walk of the state into it with `adler32`, so a value
depends on every sample before it. Two runs are only comparable if they sampled
at the same points, which is why the playback takes `checksumperiod` and
`light_checksum_count` out of the replay rather than from the run doing the
verifying. To compare more finely, record more finely.

Getting this wrong costs real time, because the failure wears a convincing
disguise. Verifying a replay at a period other than the one it was recorded at
reports a divergence on the first compared sample, nothing matching after it,
and the random draw counts agreeing exactly, since those do not depend on
sampling. That is indistinguishable by eye from a simulation whose starting
state differs, and it was read as exactly that twice before the cause was found.

### What is verified and what is not

- **Custom mission, single player.** Verified, as above.
- **Campaign mission.** Not yet. The start kind, difficulty and tutorial flag
  are recorded now, but the campaign and the chapter are not, and a replay
  recorded before those fields existed reads them back as zero, which selects
  the custom path. Needs a fresh campaign recording to say whether restoring
  the three is enough.
- **Multiplayer.** The path that always worked, but unverified by checksum
  comparison, since it predates the samples being stored.

## Next steps

1. Re-record a campaign replay and verify it, which is the one case the
   recorded start state was written for and the only one still open.
2. Three assertion commands in Lua.
3. The terrain dump oracle, which unblocks option B independently.
4. `-fsanitize=integer` plus deliberate extreme-value scenarios.
5. Invariant checks after graveyard reaping, starting from `PrepareToDelete`.
6. The `IBinSaver` dangling-reference auditor.
7. Option B when fuzzing throughput demands it.

## Open questions

- How much data `ITerraAIObserver` needs fed to it in practice, which prices
  option B properly.
- Whether units are created by `InitAfterMapLoad` from the map database or by
  `CWorldClient`, since the latter would mean a second producer to deal with.
- Whether a windowless DXVK on lavapipe needs a surface, and if so how to
  provide one headlessly.
