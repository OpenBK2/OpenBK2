# Clone the repository

install [Git for Windows](https://git-scm.com/downloads/win)

and clone the repository with the following command:

```bash
git clone --recursive https://github.com/OpenBK2/OpenBK2.git
```

# Visual Studio 2022

Install [Microsoft Direct SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812).

Install [Microsoft Visual Studio 2026](https://visualstudio.microsoft.com/downloads/) with the following workloads:
- Desktop development with C++

Use `File -> Open -> Folder` menu (Ctrl+Shift+Alt+O) to open the cloned repository folder

go to `Project ->Edit CMake Presets for b2` menu

Edit the `configurePresets` part of the file, `CMAKE_INSTALL_PREFIX` path. e.g. to install game into the `C:\Games\bk2` directory, use the following:

```cmake
  "configurePresets": [
    {
      "name": "base-windows",
      "hidden": true,
      "displayName": "Base Windows Ninja Preset",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/out/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "CMAKE_VERBOSE_MAKEFILE": "ON",
        "CMAKE_INSTALL_PREFIX": "C:/Games/bk2"
      }
    },
```

save the file (Ctrl+S).

once CMake configuration is completed, use `Build -> Install b2` menu.

# copying data files

copy the following files and directories manually into the game directory:
- [Versions/Current/Profiles](Versions/Current/Profiles)
- [Versions/Current/Data](Versions/Current/Data)
- [Versions/Current/splash.bmp](Versions/Current/splash.bmp)
- [Versions/Current/Editor](Versions/Current/Editor) - only needed for the map editor

assuming installation into the `C:\Games\bk2` directory, the commands to copy will be:
```cmd
robocopy Versions\Current\Profiles\ C:\Games\bk2\Profiles\ /S /E >NUL
robocopy Versions\Current\Data\ C:\Games\bk2\Data\ /S /E >NUL
robocopy Versions\Current\Editor\ C:\Games\bk2\Editor\ /S /E >NUL
xcopy Versions\Current\splash.bmp C:\Games\bk2\
```

the `Editor` directory is easy to skip and the editor does not complain when it
is missing - it comes up with a generic title and most of the `File` menu greyed
out. `ConstUserData.xml` in it names `MainObjectType` as `MapInfo`, and
`CDWGDBBrowser::UpdateCommand` enables `File -> New` and `File -> Open` only
when that is set, so without the file there is nothing the editor will agree to
open. `Editors.xml`, `Exporters.xml`, `Extractors.xml` and `Filters.xml` are
read from the same place.

copying will take a while - there are **MANY** small files.

# launching the game

the game executable is `bin\Game.exe`. therefore, if game was installed into `C:\Games\bk2`, then `C:\Games\bk2\bin\Game.exe` should be launched.

# collecting crash reports

The game and editor start `crashpad_handler` from beside their executable. The
build and install targets place it there automatically. Startup writes
`crashpad-status.txt` beside the executable and inside the selected report
database; the game also copies the status into `log.txt`. Check this file after
launch: it records whether initialization succeeded, the absolute handler path,
the database path, and the reason if capture could not start.

Reports normally live in `bin/crashpad_db`. If that directory cannot be created
or initialized, Windows uses `%LOCALAPPDATA%/OpenBK2/crashpad_db`; Linux uses
`$XDG_STATE_HOME/OpenBK2/crashpad_db`, or
`$HOME/.local/state/OpenBK2/crashpad_db` when `XDG_STATE_HOME` is unset. The status
file gives the actual location. Uploads are disabled; reports stay local.

After a crash, collect the newest `.dmp` from that database (Windows stores it
under `reports`), `crashpad-status.txt`, and `log.txt` before launching again.
Include the exact build, map/mod, OS, GPU and what happened just before the
failure. Retain the matching executables, DLLs and PDBs from that build for
debugging; PDBs from a different rebuild do not match the dump.

Capture is best effort. Startup status confirms initialization, not that the
handler survived the entire session. Process kills, raw Windows fast-fail,
failures before the entry point and unavailable disk space can still leave no
dump. Windows fast-fail bypasses ordinary exception handlers; covering it via
Crashpad's WER module requires separate installation/registry integration. The
pinned Crashpad also does not restart a dead handler on Windows/Linux, and its
Windows `DumpWithoutCrash` call can wait indefinitely if that handler dies.
Fatal C++ exceptions caught by the game's entry point request such a dump before
logging/displaying the error; their dump stack is the catch site, not the
already-unwound throw site.

For automated checks, see the [Crashpad integration tests](Versions/Temporary/Engine/Sources/port/test/README.md).

# exporting Granny3D models from the editor

In the object browser's `<Model>` tree, right-click a model file and choose **Export Granny3D to GLB**. The options window offers **Export textures as separate files**, **Convert textures to TGA**, and **Mirror along X axis**, all checked by default. Click **Export** to select a destination in the save dialog. Cancelling that dialog or encountering an export error returns to the options. The command reads the mounted game/mod resources without changing the database.

The default GLB scene contains `Geometry` and its skeleton. A separate, non-default `AIGeometry (hidden)` scene contains the collision meshes and their own rigs, keeping them out of the default view. The export uses the Model's separate Skeleton when present, otherwise Geometry's embedded skeleton, and remaps skin bindings by bone name.

X mirroring applies to geometry, normals, triangle winding, skeleton bind transforms, and every animation key, including the AI rig. Uncheck it to retain the original export orientation.

Skeletal animations are sampled at 30 fps into one `AllAnimations` clip: `Model.Animations` in order, then additional unique `Skeleton.Animations`. Each original clip keeps its endpoints, with a one-frame boundary before the next clip. Scene and animation extras record clip names, zero-based first/last frames, start/end seconds, and the frame rate. Set Blender's scene frame rate to 30 fps before importing to match these frame numbers. Game-controlled movement and procedural effects are not baked into the animation.

By default, textures are converted to TGA and written into a `<GLB name>_textures` folder beside the GLB, which references those files directly. Special characters in folder/file names are escaped for portable relative paths. Keep the GLB and its texture folder together when moving the export. TGA references target Blender's image loader and are outside standard glTF image support. Unchecking TGA conversion writes original DDS files with PNG fallbacks instead. Unchecking separate files disables TGA conversion and embeds original DDS bytes through `MSFT_texture_dds`, plus PNG fallbacks, as before.

Diffuse textures are connected to materials; legacy bump, gloss, mirror, and detail textures are retained with their roles in material extras because their channels do not directly match glTF PBR inputs. Unsupported DDS encodings and bone shear produce an export error. Mixed GLTF/Granny resource references are also rejected.

The `GrannyGltf_test`, `GrannyGltfTexture_test`, and `GrannyModelExport_test` targets cover the conversion and complete database export path. Tests using the optional game-data corpus skip when it is absent. The full export test checks for its model fixtures as well as the database index, since CI's sparse checkout contains the index without those models. Set `OBK2_GRANNY_TEST_DATA_DIR` to test against a different data directory. The full export test writes sample GLBs under the build directory's `granny-export-validation` folder.

# looking around the code

all the code is within [Versions/Temporary/Engine/Sources](Versions/Temporary/Engine/Sources) directory

main game:
- [3DLib](Versions/Temporary/Engine/Sources/3DLib) - 3D geometry: vectors, matrices, quaternions and so on
- [3Dmotor](Versions/Temporary/Engine/Sources/3Dmotor) - the main 3D engine ([Direct3D9](https://learn.microsoft.com/en-us/windows/win32/direct3d9/dx9-graphics) based): graphics, effects, lights, etc.
- [AILogic](Versions/Temporary/Engine/Sources/AILogic) - the behavior of various unit types, their abilities and commands
- [B2_M1_Terrain](Versions/Temporary/Engine/Sources/B2_M1_Terrain) - few additional terrain objects for `M1`
- [B2_M1_World](Versions/Temporary/Engine/Sources/B2_M1_World) - few additional units for `M1` (e.g. helicopters, paratroopers)
- [Client](Versions/Temporary/Engine/Sources/Client) - network client code (multiplayer)
- [Common_RTS_AI](Versions/Temporary/Engine/Sources/Common_RTS_AI) - path finding utilities
- [DebugInfo](Versions/Temporary/Engine/Sources/Common_RTS_AI) - in-game debugging overlay
- [Game](Versions/Temporary/Engine/Sources/Game) - main game executable
- [GameX](Versions/Temporary/Engine/Sources/GameX) - various in-game interface dialogues
- [Image](Versions/Temporary/Engine/Sources/Image) - reading and writing various image formats (DXT, TGA, DDS, PSD, etc.)
- [Input](Versions/Temporary/Engine/Sources/Input) - mouse and keyboard input ([DirectInput](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418273(v=vs.85)) based)
- [libdb](Versions/Temporary/Engine/Sources/libdb) - reading and writing various game resources from database (serialization)
- [Main](Versions/Temporary/Engine/Sources/Main) - main game loop, connecting all other subsystems together (like 3Dmotor, Sound, Input, etc) and loading the mods
- [MemoryLib](Versions/Temporary/Engine/Sources/MemoryLib) - custom memory allocator, not really used anymore
- [Misc](Versions/Temporary/Engine/Sources/Misc) - small basic routines, like random, filesystem, timer, string utilities, etc.
- [Net](Versions/Temporary/Engine/Sources/Net) - general network utilities, like IP address parsing and so on
- [Parser](Versions/Temporary/Engine/Sources/Parser) - custom parser for `cll` files ([bison](https://www.gnu.org/software/bison/) based), grammar is in [lang.l](Versions/Temporary/Engine/Sources/Parser/lang.l) and [lang.y](Versions/Temporary/Engine/Sources/Parser/lang.y)
- [SceneB2](Versions/Temporary/Engine/Sources/SceneB2) - terrain and map, objects like rivers, roads
- [Script](Versions/Temporary/Engine/Sources/Script) - Lua script commands (see available command in [Script.cpp](Versions/Temporary/Engine/Sources/Script/Script.cpp)), used in scenarios (custom missions) and in game console (press tilde)
- [Server_Client_Common](Versions/Temporary/Engine/Sources/Server_Client_Common) - common code between client and server, essentially an implementation of multiplayer network protocol
- [Sound](Versions/Temporary/Engine/Sources/Sound) - sound and music
- [Stats_B2_M1](Versions/Temporary/Engine/Sources/Stats_B2_M1) - mostly generated code for unit abilities and their actual stat numbers
- [System](Versions/Temporary/Engine/Sources/System) - generic basic classes, like file streams, XML parsing, ZIP archive support, etc.
- [UI](Versions/Temporary/Engine/Sources/UI) - user-interface controls rendered in-game (buttons, scroll-bars, windows, etc.)
- [UISpecificB2](Versions/Temporary/Engine/Sources/UISpecificB2) - additional UI controls

editor:
- [B2_MapEditor](Versions/Temporary/Engine/Sources/B2_MapEditor) - Map Editor main executable
- [ED_B2](Versions/Temporary/Engine/Sources/ED_B2) - chapter editor
- [ED_B2_M1](Versions/Temporary/Engine/Sources/ED_B2_M1) - main editor implementation
- [ED_Common](Versions/Temporary/Engine/Sources/ED_Common) - some basic UI controls for editor
- [MapEditor](Versions/Temporary/Engine/Sources/MapEditor) - common dialog windows for editor
- [MapEditorLib](Versions/Temporary/Engine/Sources/MapEditorLib) - few more dialogs

server:
- [Server](Versions/Temporary/Engine/Sources/Server) - Server main executable

other small utilities:
- [dbcodegen](Versions/Temporary/Engine/Sources/dbcodegen) - compiles the `.cll` type descriptions into `types.xml` and the `DB*.h/.cpp` sources (structs, serializers, meta info, checksums); run by hand after a `.cll` change, see [adding things to the game database](#adding-things-to-the-game-database); `-nocopy` writes to the temp directory only
- [dbindex](Versions/Temporary/Engine/Sources/dbindex) - database index utility (generates `index.bin` file)
- [dbstruct](Versions/Temporary/Engine/Sources/dbstruct) - database structure utility (converts `xcb` files into packaged binary format)
- [FontGen](Versions/Temporary/Engine/Sources/FontGen) - font generator (in the format game understands)
- [ShaderCompiler](Versions/Temporary/Engine/Sources/ShaderCompiler) - compiles shaders from custom format, shaders are in [GfxShaders.txt](Versions/Temporary/Engine/Sources/3Dmotor/GfxShaders.txt), compiled shaders are in [GfxShaders.cpp](Versions/Temporary/Engine/Sources/3Dmotor/GfxShaders.cpp); run the `regenerate-gfxshaders` target
- [ShaderAsm](Versions/Temporary/Engine/Sources/ShaderAsm) - D3D9 shader assembler (vs.1.1, ps.1.1, ps.1.4, ps.2.0) that ShaderCompiler uses in place of D3DX; matches `D3DXAssembleShader` token for token

test code (see also the unit tests and benchmarks section below):
- [3Dmotor/test](Versions/Temporary/Engine/Sources/3Dmotor/test) - unit tests for the ported SIMD and MMX routines, checked against the original assembly
- [3Dmotor/benchmark](Versions/Temporary/Engine/Sources/3Dmotor/benchmark) - benchmarks for the same routines
- [TestClient](Versions/Temporary/Engine/Sources/TestClient) - test application for headless multiplayer testing
- [TestDB](Versions/Temporary/Engine/Sources/TestDB) - the schema and records that [libdb/test](Versions/Temporary/Engine/Sources/libdb/test) runs the editor database against; regenerate-db rebuilds its sources from TestType.cll
- [Parser/test](Versions/Temporary/Engine/Sources/Parser/test) - the [Parser](Versions/Temporary/Engine/Sources/Parser) library (the .cll grammar) against a corpus of cases that must parse or must be rejected; formerly TestParsing

the game engine is referred as "enigma" in various sources, however, this name is not used in source code.

the naming convention (see also [NivalProjectNames.doc](Versions/Temporary/Engine/NivalProjectNames.doc)):
- Blitzkrieg I: `A7`, `B1`
- Blitzkrieg II: `A10`, `B2`

`M1` seems to be a reference to another version of engine or game, see `VERSION_DEV_M1`, which modifies behavior in several places.

# adding things to the game database

where to start when a feature needs a new unit stat, weapon parameter, ability, enum value or a whole new record type.

game content lives in a typed object database: the records are the `.xdb` files under `Versions/Current/Data`, and their schema is [Versions/Current/Data/types.xml](Versions/Current/Data/types.xml), which the game and the editor load. neither the schema nor the C++ structs that read the records are written by hand. both are generated by [dbcodegen](Versions/Temporary/Engine/Sources/dbcodegen) from the `.cll` type descriptions, which are the single source of truth:

- `types.xml`
- in each module, the `DB*.h/.cpp` files (and `Stats_B2_M1/RPGStats.h/.cpp` and a few others) that sit next to a `.cll` of the same name. they carry the structs, the binary and XML serializers, the meta info and the checksums.

a generated file edited by hand is overwritten on the next run, so make every change in the `.cll` and regenerate.

## finding the right .cll

each module lists its `.cll` files in its `CMakeLists.txt` `SOURCES`. the ones touched most:
- [Stats_B2_M1/RPGStats.cll](Versions/Temporary/Engine/Sources/Stats_B2_M1/RPGStats.cll) - units, weapons, shells, squads, abilities
- [Stats_B2_M1/DBMapInfo.cll](Versions/Temporary/Engine/Sources/Stats_B2_M1/DBMapInfo.cll) - maps, objectives, reinforcements
- [3Dmotor/DBScene.cll](Versions/Temporary/Engine/Sources/3Dmotor/DBScene.cll) - models, materials, textures, effects
- [ED_B2_M1/dbbuildersb2.cll](Versions/Temporary/Engine/Sources/ED_B2_M1/dbbuildersb2.cll) - the editor's builders, such as the New Map dialog

`base.cll` and `game.cll` in the root of the sources declare the attributes and the basic types (`CVec3`, file references, ...) every other file uses.

## writing it

```
// maximum missile turn rate, in radians per second
float _TurnRateRad = 1.048;
```

- the field name after `_` is the name in `types.xml` and the `.xdb` files. the C++ member gets a Hungarian prefix from its type (`fTurnRateRad`).
- the last `//` line above a field becomes its tooltip in the editor. a note that is not meant as a tooltip goes in `/* */`, which the generator drops.
- write float defaults with a decimal point (`1.0`, not `1`), or `types.xml` records the default as an int.
- binary chunk ids are numbered from field order, and saves, replays and packed data depend on them. append new fields at the end of the class. a field that has to go elsewhere gets `[chunkID = N]` with an id no field of that class has used. a pinned field does not use up a number, so the fields after it keep theirs.
- enum values are stored as numbers too. add new ones at the end, before the `_COUNT`-style last entry.
- a new class needs a `[typeID = 0x...]` that no other `.cll` uses. add `[export]` when another DLL uses it.
- anything the generator cannot express, such as a method on a struct, goes in a hatch. an `#include "include_X.h"` inside a class is pasted into the generated struct. `[cppExternal] #include "include_X_cpp.h"` adds definitions to the generated `.cpp`, and `[hExternal]` adds an include to the generated header. `RPGStats.cll` and `GameX/DBConsts.cll` show all three.
- a new `.cll` file has to be added to its module's `SOURCES` by hand, like a `.cpp`.

## regenerating

the build never runs dbcodegen: the generated files are committed, and a dedicated target rewrites them.

```powershell
cmake --build out/build/Windows-x64-Release --target regenerate-db
```

it needs `BUILD_EDITOR`, which is on by default, because `types.xml` collects the types of every module, the editor's included. it rewrites only the files whose content changed, so a run that changes nothing touches nothing and triggers no rebuild. `git diff` should show only what the `.cll` change asked for. `python scripts/check_db_schema.py` then confirms that `types.xml` and the generated sources agree.

CI runs the same target after the build and fails when it changes anything, so a hand edit to a generated file, or a `.cll` change committed without regenerating, does not get in.

## the data side

the schema only makes the field exist. it takes effect once `.xdb` records set it, and new buttons or abilities also need UI records. [OBK2_Examples](OBK2_Examples) holds the data-side counterparts of recent features. read those, and [OBK2_Examples/StratBomberSelectionButtonImpl/note.txt](OBK2_Examples/StratBomberSelectionButtonImpl/note.txt) in particular, before adding a similar feature. player-visible changes go in [Changes.md](Changes.md).

# unit tests and benchmarks

tests and benchmarks are `EXCLUDE_FROM_ALL`, so a normal build never touches them.
build and run them with:

```
cmake --build out/build/Windows-x64-Debug --target run-unittests
cmake --build out/build/Windows-x64-Release --target run-benchmarks
```

these are ordinary targets, so they also work from an IDE target list. underneath they
are `ctest` with a label:

```
ctest --test-dir out/build/Windows-x64-Debug -L obk2-test
ctest --test-dir out/build/Windows-x64-Release -L obk2-benchmark
ctest --test-dir out/build/Windows-x64-Debug -L obk2            # both
```

the label matters because the dependencies register tests of their own. `-L` takes a
regular expression, which is why the two labels are `obk2-test` and `obk2-benchmark`
rather than `obk2` and `obk2-benchmark`: the shorter name would match both.

## adding one

a test is one line in the test directory's `CMakeLists.txt`:

```cmake
add_unit_test(MyThing 3Dmotor Misc)                       # links these libraries
add_unit_test(MyThing 3Dmotor Misc SOURCES extra.cpp)     # and compiles these too
```

a benchmark is either `add_benchmark(MyThing)`, which builds it three times with
`/arch:SSE2`, `/arch:AVX` and `/arch:AVX2`, or a plain `add_executable` followed by
`register_benchmark(MyThing_benchmark)`.

either way it is picked up automatically: built by the aggregate target, run by label,
and included in the table posted to a pull request. nothing needs adding to the
workflow, and nothing lists executables by name.

## writing them

things that have gone wrong here before:

- **seed fixed, not random.** `test/random.h` seeds from `std::random_device`, so a
  failure it finds cannot be reproduced. these comparisons are the kind that fail on
  one input in a million. use a local generator with a literal seed.
- **cover the tail.** the SIMD kernels process two or four elements per iteration and
  handle the remainder separately, which is where indexing goes wrong. run counts
  1..17 as well as a large one, not just a round number.
- **measure a tolerance, do not guess one.** where an implementation cannot be
  bit-exact, assert a bound and print the worst difference actually seen, so a
  regression that widens it shows as a number rather than as a pass. an approximation
  like `_mm_rcp_ss` has *relative* error, so the bound usually has to scale with the
  value rather than being a constant.
- **benchmark in Release only.** a debug build inverts SIMD-versus-scalar comparisons:
  templated code gets no inlining at `/Od`. the same benchmark reported glm 10x slower
  than the original MMX in Debug and 3.3x faster in Release.
- **build the inputs outside the timed loop.** generating a random rotation matrix
  costs far more than transforming a vector by one. a benchmark that did this inside
  the loop reported two implementations as near equal no matter what they did.
  `benchmark::DoNotOptimize` on the result, and tune runs with Google Benchmark's
  environment variables (`BENCHMARK_MIN_TIME`, `BENCHMARK_REPETITIONS`) rather than
  hard-coded flags.

## MMX references and x86

several tests compare a port against the original MMX. MSVC accepts inline `__asm` on
x86 only, so anything using it cannot build for x64 and is registered for x86 alone.
new references belong in `Versions/Temporary/Engine/Sources/3Dmotor/test/original/`,
written as SSE2 intrinsics: SSE2 defines these integer operations exactly as MMX does,
and unlike MASM it builds on every compiler and every platform that targets x86.
`test/original/MMXPrimitives.h` is the worked example.
`test/original/sentinel` exists to prove such a replacement matches the assembly it
replaces, on x86, where both forms can run side by side.

# branches

there are few branches where main development takes its place:
- [port](https://github.com/OpenBK2/OpenBK2/tree/port) - the main development branch where all major work is done. it includes porting to modern compiler, fixing game bugs (like crashes and asyncs), performance optimizations, etc.
- [main](https://github.com/OpenBK2/OpenBK2/tree/main) - an original source code the way it was released, as is, no modifications

# reporting crashes

game uses [crashpad](https://chromium.googlesource.com/crashpad/crashpad) library for the crash reporting.
if you have experienced a crash, look for the `crashpad_db/reports` directory nearby game executable
(so if game is in `C:\Games\bk2`, then look for files in `C:\Games\bk2\bin\crashpad_db\reports`).
if you have any `dmp` (crash dump) files in mentioned directory, feel free to submit a bug report at https://github.com/OpenBK2/OpenBK2/issues/new.
include some important information:
- version of the game
- what happened just before the crash (did you use some specific unit or ability?)
- which map / scenario / campaign did you play?
- did you enable any mods?
- some information of your system (use [dxdiag](https://learn.microsoft.com/en-us/previous-versions/bb219726(v=vs.85)) or similar utilities, like [cpu-z](https://www.cpuid.com/softwares/cpu-z.html), [gpu-z](https://www.techpowerup.com/gpuz/), etc.)
