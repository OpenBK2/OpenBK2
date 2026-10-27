# Profiling a map load

Players report that maps with many units take a long time to load, total
conversions worst of all. This is how to turn that into a number and then into a
profile.

## What a load actually is

`CInterfaceMission::NewMission` (`GameX/InterfaceMissionInternal.cpp`) is the
whole of it: one blocking call, no frames drawn in between. Every way of starting
a mission -- `--mission`, `--campaign`, `--replay`, the campaign menus, a
multiplayer lobby -- arrives at `CICMission::PostCreate`, which calls it and then
pushes the mission interface.

That shape is why there is no Google Benchmark target for this and why one would
be the wrong instrument. `NewMission` reaches the scene, the `GView`, the UI
screens, the camera, the sound system and the cursor, so a benchmark harness
around it would have to bring up DXVK, Vulkan, the UI and the database first --
that is `Game`'s own `main` rewritten, and it would then measure a different
process to the one players run. The game is made the subject instead.

## The number

Every mission start now logs its load, whatever started it:

```
map load: Scenario/Campaigns/GER/Chapter1/GER1.0/MapInfo.xdb 10x10 patches in 8.431 s (screens 0.512, ai_init 0.004, load_map 2.903, ai_after_map_load 3.611, world_update 0.402, load_graphics 0.999)
```

It goes to `log.txt` and the in-game console, so it is in the logs a player
attaches to a bug report. The phases are the units a complaint can be attributed
to:

| phase | what it covers | prime suspect for |
| --- | --- | --- |
| `screens` | loading screen, `CWorldClient`, notifications, reinforcement and superweapon panels | UI texture loads |
| `ai_init` | `IAILogic::Init` | nothing; it is bookkeeping |
| `load_map` | `CWorldClient::LoadMap` -- terrain, the `.b2m`, the database records behind it | map size, database lookups |
| `ai_after_map_load` | `IAILogic::InitAfterMapLoad` -- creating every unit on the map | **unit count** |
| `world_update` | `PostMapLoad` and the first two `CWorldClient::Update`s | first-time visual object creation |
| `load_graphics` | `GView::LoadEverything` -- every model and texture the mission needs | model and texture count |

Read that line first. It says which of the six to profile, and a profile of the
wrong one is a wasted half hour.

## Making the process profilable

`--exit-after-load` loads the map and leaves, printing the same line to stdout.
Without it the process stays in the mission and the samples taken while playing
bury the ones taken while loading.

```sh
./launch.sh release --data totalconv \
    --mission Custom/Missions/Razvedka/MapInfo.xdb --exit-after-load
```

It needs a map to load, so pass one of `--mission`, `--campaign` or `--replay`;
launch.sh refuses the flag on its own rather than leaving the run sitting at the
main menu. The run takes SDL's dummy audio backend for the reason `--verify`
does: SDL's PulseAudio backend deadlocks on exit when the device is idle, which
is exactly the state a run that exits the moment the map is in leaves it in.

## Recording it

`--profile` adds the recorder, and `--profile perf` picks perf instead of the
default perforator:

```sh
BK2_PERFORATOR=~/projects/perforator/perforator/cmd/cli/perforator \
./launch.sh release --data totalconv \
    --mission "Custom/Missions/Mission Impossible/mapinfo.xdb" \
    --exit-after-load --profile
```

A perforator run leaves **two** files, `profile-<stamp>.pprof` and
`profile-<stamp>.html`, from one recording. `--format` takes a single value, and
pprof is the one that keeps the data: the flame graph renders from it afterwards
with `perforator profile flamegraph pprof`, while nothing turns an HTML flame
graph back into a profile. So the HTML is to look at and the pprof is what tools
read:

```sh
go tool pprof -top profile-<stamp>.pprof
go tool pprof -top -cum -nodecount=40 profile-<stamp>.pprof
go tool pprof -http=: profile-<stamp>.pprof          # graph, flame, peek, source
perforator profile text pprof -i profile-<stamp>.pprof
perforator diff ...                                  # against a second recording
```

`--profile-out` names a base: the extension is supplied, and a known one is
replaced first, so `/tmp/mi`, `/tmp/mi.pprof` and `/tmp/mi.html` all name the
same pair. perf writes `.data` for `perf report`.

### Reading the HTML without a browser

perforator's pprof and its flame graph are separate recording decisions rather
than two views of one thing, and the pprof has been the weaker of the two. The
HTML is not a dead end though: the page carries the whole profile in
`window.__data__`, base64 over gzip over JSON, and
`scripts/perforator_html_to_collapsed.py` turns that into collapsed stacks,
which is what every other tool reads.

```sh
scripts/perforator_html_to_collapsed.py profile-<stamp>.html > profile.folded
scripts/perforator_html_to_collapsed.py profile-<stamp>.html --self | head -20
scripts/perforator_html_to_collapsed.py profile-<stamp>.html \
    --filter 'NewMission' --self        # self time inside the map load only
```

Weights are `eventCount`, which for a cycles profile is cycles and is what the
flame graph's widths show; `--samples` counts each sample once instead.

Both tools **launch the game themselves** rather than attaching to it, which is
the whole reason they are wired in here. perforator takes `--pid`, `--cgroup`
and `--whole-system`, so the obvious shape is `--pid $(pidof Game)` -- and that
cannot win against a map load, because by the time the id exists the load may
already be under way and the samples that matter are the ones it missed. Given a
command after `--` it starts the process, traces that pid, and stops when the
child exits ("Stopping the profiler because child subprocess finished"). So the
recording begins with the process and ends with it, and there is no duration to
guess at: a `--duration` cut short in the middle of `ai_after_map_load` looks
like a profile of `load_map`.

### Privileges, and not leaving root-owned files behind

perforator is eBPF and wants root; launch.sh uses sudo for it. The game must
**not** inherit that: it writes `Profiles`, `log.txt` and the crashpad database
into the data tree, and one run as root makes the next run as the user fail on
files it cannot open. So `setpriv --reuid --regid --init-groups` sits between
the profiler and the game. setpriv execs rather than forking, so the process
perforator registered is still the one that becomes `Game`. launch.sh refuses to
profile with perforator if setpriv is missing rather than quietly running the
game as root. The profile itself is written as root and chowned back afterwards,
which is why a profiling run does not `exec`.

perf needs `kernel.perf_event_paranoid` lowered instead. It is 2 on this machine
and perf is refused even a `cpu-clock` software event, so launch.sh warns and
tells you the sysctl:

```sh
sudo sysctl kernel.perf_event_paranoid=1   # or -1 to see kernel frames too
```

perf gets `--call-graph=dwarf` rather than frame pointers: Release maps to
`RelWithDebInfo` and carries `-g`, but not `-fno-omit-frame-pointer`, so a frame
pointer walk gives stacks that stop inside the engine's shared libraries. And
`-F 199` rather than more, because dwarf unwinding copies a stack snapshot per
sample and a higher rate on a process this size fills the buffer faster than it
drains. `BK2_PERF_EXTRA='-F 499'` raises it when a few hundred samples inside
the load is not enough; `perf report --stats` reports what was lost.

**On a hybrid CPU, perf records two events.** `perf evlist` shows
`cpu_atom/cycles/P` and `cpu_core/cycles/P`, and `perf report` prints a section
per event, so the first table on screen can be the handful of samples that
landed on an efficiency core rather than the thousand that did the work. Check
`perf report --stats` for the per-event counts before reading any percentage.

`BK2_PERFORATOR` names the binary when it is not on PATH, and
`BK2_PERFORATOR_EXTRA` adds flags: `--freq` to change the sampling rate,
`--log-level debug` when a recording comes back empty. `BK2_PROFILE_SUDO`
replaces or (set empty) removes sudo, for a binary that has been given the
capabilities outright.

The checked-out `perforator/cmd/cli/perforator` is a symlink into
`~/.ya/build/symres`, which is cleared when `ya` garbage collects, so it goes
stale between sessions; launch.sh reports a dangling link as such rather than as
"not found".

### `Mission.LoadProfileDelay`

Still there, and no longer needed for either tool above, since both start the
process. It is for a recorder that can only attach: the game prints its own
process id and sleeps that many seconds before the first `StepApp`, after the
database is open and the renderer is up, so what it delays is the load and
nothing else.

```sh
./launch.sh release --data totalconv --mission <map> --exit-after-load \
    --setvar Mission.LoadProfileDelay=15
```

## Where to look when a phase is slow

`libdb` and the VFS already count their own time: `NDb::SegmentProfiler` and
`NVFS::VFSSegmentProfiler` (`libdb/GameDatabase.cpp`, `System/WinVFS.cpp`) are
called from `StepApp` and report, through `DbgTrc`, how many seconds a one-second
segment spent loading database objects and answering `DoesFileExist`. They are
blind to a load, because loading happens inside a single `StepApp` and no segment
boundary falls during it -- but the counters they read are accumulating the whole
time, so they are the obvious thing to sample around `NewMission` if `load_map`
turns out to be the slow phase.

There is also a knob worth measuring both ways. The `deep_map_load` variable
(`s_bDeepMapLoad`, off by default) makes `NewMission` raise `NDb::SetLoadDepth`
to 100 around the load, so the database pulls in each record's dependencies
eagerly instead of on first dereference. That trades load time for stutter while
playing, and which way it should go is not obvious for a total conversion, which
has far more records behind each unit than the retail data does:

```sh
./launch.sh release --data totalconv --mission <map> --exit-after-load \
    --setvar deep_map_load=1
```

Compare the `load_map` and `ai_after_map_load` figures against a run without it.
