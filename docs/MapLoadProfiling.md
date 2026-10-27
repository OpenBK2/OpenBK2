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

### The attach race, and `Mission.LoadProfileDelay`

A profiler that attaches by process id cannot win against this: by the time
`pidof Game` answers, the load may already be under way, and the samples that
matter are the ones it missed. So the game will wait:

```sh
./launch.sh release --data totalconv \
    --mission Custom/Missions/Razvedka/MapInfo.xdb --exit-after-load \
    --setvar Mission.LoadProfileDelay=15
```

It prints its own process id and sleeps that many seconds before the first
`StepApp`, which is where the load happens. Zero by default, so an ordinary run
never pauses. The wait is after the database is open and the renderer is up, so
what it delays is the load and nothing else.

A recorder that launches the program itself -- `perf record -- ...` -- does not
need this.

## Recording it

Both recorders below need privileges this machine does not grant by default.
`kernel.perf_event_paranoid` is 2 here and perf is refused even a `cpu-clock`
software event, so a recording needs

```sh
sudo sysctl kernel.perf_event_paranoid=1   # or -1 to see kernel frames too
```

and perforator, which is eBPF, needs to run as root outright -- which is why
`bk2.sh` in that checkout ends with `chown sse4 ./flame.html`. The game itself
needs no privileges and `--exit-after-load` works without any of this; it is
only the sampling that does.

### perf

Always present, and it wraps the command, so there is no race to lose.
`--call-graph=dwarf` rather than frame pointers: the Release presets are
`RelWithDebInfo` and carry `-g`, but not `-fno-omit-frame-pointer`, so a frame
pointer walk gives truncated stacks through the engine's shared libraries.

```sh
./launch.sh release --data totalconv --mission <map> --exit-after-load --env
# take rundir and the environment from that, then:
cd ~/bk2-totalconv/bin
perf record -g --call-graph=dwarf -F 199 -o /tmp/mapload.data -- \
    env DXVK_WSI_DRIVER=SDL3 SDL_AUDIO_DRIVER=dummy \
        LD_LIBRARY_PATH=$PWD ./Game --mission=<map> --exit-after-load
perf report -i /tmp/mapload.data --stdio --sort=dso,symbol | head -60
```

`-F 199` and not more: dwarf unwinding copies a stack snapshot per sample and a
high rate on a process this size fills the buffer faster than it drains.

### perforator

`perforator record` takes `--pid`, `--cgroup` or `--whole-system`; it does not
launch a command, so this is where `Mission.LoadProfileDelay` earns its keep.
`bk2.sh` in the perforator checkout is the existing one-liner:

```sh
./perforator/cmd/cli/perforator record --log-level debug --debug \
    --duration 2m -o ./flame.html --pid $(pidof Game)
```

Start the game with a delay long enough to get that running, then attach inside
the window. `--duration` wants to outlast the load; a profile cut short in the
middle of `ai_after_map_load` looks like a profile of `load_map`.

Note that the checked-out `perforator/cmd/cli/perforator` is a symlink into
`~/.ya/build/symres`, which is cleared when `ya` garbage collects, so it needs
rebuilding before a session.

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
