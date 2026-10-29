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

## user profiles and saved logins

The game and editor use one per-user `Profiles` directory:

- Windows: `%LOCALAPPDATA%\OpenBK2\Profiles` (resolved through Known Folders).
- Linux: `$XDG_DATA_HOME/OpenBK2/Profiles`, or `~/.local/share/OpenBK2/Profiles`
  when `XDG_DATA_HOME` is unset, empty, or relative.
- macOS: `~/Library/Application Support/com.OpenBK2Team.OpenBK2/Profiles`
  for an ordinary desktop app; Foundation resolves the user domain for a sandboxed app.

On first launch, the installation's `Profiles` defaults are copied into a private
staging directory and published only after the copy completes. The installation
template is never modified. An existing user directory wins and is never merged
with new defaults. Missing templates, incomplete profiles and write failures are
reported before startup configurations run.

The installation is located relative to the executable (`<installation>/bin`),
independently of the working directory. For development builds and isolated test
runs, `OPENBK2_DATA_DIR` can name the installation root and
`OPENBK2_USER_DATA_DIR` can name an alternate application-data directory. Both
overrides must be absolute; the latter contains `Profiles`, not its contents.
Set both for tests against a separate installation to avoid sharing real profiles.

The map editor also keeps its `Editor` folder beside `Profiles`: for example,
`%LOCALAPPDATA%\OpenBK2\Editor` on Windows, `~/.local/share/OpenBK2/Editor` on
Linux, and `~/Library/Application Support/com.OpenBK2Team.OpenBK2/Editor` on
macOS. The same application-data overrides apply. The installed `Editor` folder
is copied once, without changing it; an existing user copy wins. If neither copy
exists, startup reports the missing folder. Settings, dialog positions, filters,
icon caches and the editor log use the user copy. Database resources under
`Data/Editor` still belong to the game or active mod.

**File > Clear Icon Cache** deletes the user copy's `IconCache` directory without
touching settings or installation defaults. Icons are generated again when needed;
restart the editor to refresh icons already displayed. **File > Open Editor Folder**
opens the user folder in the system file manager.

Legacy `exec .\profiles\...` paths resolve to the user directory. Nested config
includes can also be relative to their including file. Resource settings such as
`game_root = ../Data/GameRoot.xdb` resolve through the existing mod-aware database.

Remembered online usernames and passwords are stored together in Windows
Credential Manager, Linux Secret Service, or macOS Keychain. Only the remember
preference and an opaque profile identifier are stored in ordinary files.
Credentials are saved after successful authentication. Clearing the remember
checkbox requests their deletion. If secure storage is unavailable, login still
works for the current session and the game reports that it cannot remember it;
there is no plaintext fallback. Old `StoredLogin` and `StoredPassword` config
variables are ignored and omitted from subsequent settings saves.

Linux builds require `libsecret-1-dev` (libsecret 0.19 or later). Remembering a
login also requires an available, unlocked Secret Service provider in the user's
desktop session, such as GNOME Keyring or a compatible KDE wallet service.
Restoring a backup of `Profiles` on another machine does not transfer its secrets.

`GeneralSettings_test` exercises migration and config resolution with temporary
directories. `SecuredSettings_test` tests credential encoding and failure cases.
Set `OPENBK2_TEST_NATIVE_CREDENTIALS=1` when running the latter to additionally
write, read, replace and delete a uniquely named temporary OS credential.

## executable

the game executable is `bin\Game.exe`. therefore, if game was installed into `C:\Games\bk2`, then `C:\Games\bk2\bin\Game.exe` should be launched.

## straight into a mission or a campaign

the menus are not the only way in. these arguments start the thing named and skip
everything before it, which is what makes a map worth testing reachable in one line:

```cmd
Game.exe --mission="Custom/Missions/Alternative/MapInfo.xdb"
Game.exe --campaign="Scenario/Campaigns/GER/Campaign.xdb;0;0;normal"
Game.exe --replay=my_replay
```

on Linux `./launch.sh` passes the same three through as `--mission`, `--campaign` and
`--replay`, and takes care of the working directory:

```sh
./launch.sh release --mission Custom/Missions/Alternative/MapInfo.xdb
./launch.sh release --campaign 'Scenario/Campaigns/GER/Campaign.xdb;0;0;normal'
```

`--mission` names a **MapInfo record**, by the path of its `.xdb` inside `Data`: custom
missions live under `Custom/Missions/`, and a campaign's own mission records work too, for
instance `Scenario/Campaigns/GER/Chapter1/GER1.0/MapInfo.xdb`. it starts as a single
mission, the way the custom mission screen starts one, on the easiest difficulty and with
no campaign state.

`--campaign` names a **Campaign record**, optionally followed by a chapter, a mission and
a difficulty, separated by semicolons: `campaign;chapter;mission;difficulty`. the chapter
is an index from 0; the mission is an index into that chapter's `missionPath` or the name
of its MapInfo record; the difficulty is `easy`, `normal`, `hard`, `veryeasy` or a number.
anything left off is 0, so `--campaign="…/Campaign.xdb"` starts the first mission of the
first chapter on easy. unlike `--mission` this one runs the mission with the campaign
behind it, so the briefing, the chapter and the rank are all in the state the campaign
would have put them in.

`--editor-map=` does what `--mission=` does and is what the editor's **Start Mission in
Game** passes; it stays for that. given more than one of these, the game takes the first
of `--editor-map`, `--mission`, `--campaign`, `--replay` and ignores the rest.

**the equivalent console commands are `map` and `mission`** (tilde key), and this is where
the argument grammar comes from:

```
map "Custom/Missions/Alternative/MapInfo.xdb"
mission "Scenario/Campaigns/GER/Campaign.xdb" 0 0 normal
mission_ger1.0
```

the third line is one of the aliases `Profiles/alias_*_campaign.cfg` defines for every
shipped campaign mission, two per mission: `map_ger1.0` for the map on its own and
`mission_ger1.0` for the same mission inside its campaign. `showbind`-style listing is
`help`.

**do not put `map` or `mission` in `autoexec.cfg`.** it looks like it should work and it
cannot: `game.cfg` is read after `autoexec.cfg` and its last line is `main_menu`, which
unregisters the scenario tracker the mission start just made. the queued mission then
reaches the main loop with no tracker, which is what the command-line arguments exist to
avoid - they are applied after both configs. a `mission` line at the very end of
`game.cfg`, after `main_menu`, is the only config form that is not undone.

## a map record is not a map

`MapInfo.xdb` and the terrain file `map.b2m` are separate things, and the data tree ships
only the first half: **of the fourteen missions under
`Versions/Current/Data/Custom/Missions`, not one has a `map.b2m` next to its
`MapInfo.xdb`**, and neither do most of the test maps under `Data/Test/TestMaps`. the maps
themselves are in the repository, under `Complete/`, in the 2005 layout each record still
names in its `MapFilesPath` field; the game reads the terrain out of the folder the record
is in (`NDb::GetFolderName`), so nothing there is found. starting one of those used to
crash, the terrain manager keeping an unloaded terrain while the map's own crags and
entrenchments dig into it. now:

- **`cmake --install` installs those maps beside their records**, 405 of them and about
  130 MB, so a build is enough to have missions to play. `-DINSTALL_TEST_MAPS=OFF` turns
  it off, which is what CI does. `cmake/testmaps.list` is the pairing, generated from the
  records by `python scripts/gen_testmaps_list.py`, and `--check` says whether it is
  stale. mind what the destination is on Linux: with `Data` a symlink into the source
  tree, as the launch instructions above set it up, the maps land in the source tree and
  show up as untracked files.
- the custom mission screen lists only records whose map file is there, and writes to the
  log and the console how many it dropped. after an install that is seven of the twelve
  records it finds.
- `map`, `mission`, `--mission=` and `--campaign=` refuse a map with no terrain file and
  say which file is missing, instead of starting it.
- a terrain file for a **different** map than the record describes is reported and
  replaced by generated flat terrain: `terrain file is for a 8x8 map and the record is
  7x7, generating instead`. a record's `MapFilesPath` survives renaming and copying, so
  the data does contain such pairs - `Custom/Missions/Razvedka` is one, which is why the
  generator leaves it out - and before the check they walked off the end of the
  per-patch arrays and crashed in `UpdateGfxInfo`.

six records cannot be helped from this side: `alx1`, `New_ALX`, `test_alx_1` and
`vetal_map` name map folders that are not in the repository at all, and `6x6` and `New
Map` are the other way round, a `map.b2m` in the data tree with no record to start it
with.

a mission of your own needs no copying: the editor saves the terrain into the folder its
record is in (`NEditor::MakeMapPath` is `NDb::GetFolderName`), which is the folder the game
reads it from. the `Data\Maps\CustomMissions\<name>\` folders are the original layout,
still named by each record's `MapFilesPath` and still used by the editor's exporter.

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
- a new class needs a `[typeID = 0x...]` that is unused across all source files, including C++ save/load registrations and their enum constants (such as `AILogic/AIClassesID.h`). Database resources and simulation objects share the same object factory; checking only `.cll` files misses collisions. add `[export]` when another DLL uses it.
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

# determinism, checksums and replays

multiplayer and replays are lockstep: only player commands travel, so every client has
to run the same simulation tick for tick, and any divergence is an ASYNC. the engine can
checksum its own state as it goes, record a game as the commands that produced it, and
play that recording back comparing the checksums it took the first time. all of it is
driven by global variables, which is why this section exists: several are named nothing
like what they do, and two are dead.

## setting a variable

three ways, each overriding the one before it:

```
setvar count_checksum = 1               # a line in Versions/Current/Profiles/game.cfg
setvar count_checksum = 1               # typed in the in-game console, tilde key
--setvar count_checksum=1               # on the command line, repeatable
```

the command line form is applied after the config files and after the defaults that
`--replay-verify` sets, so it always wins. it takes no spaces around the `=`, where the
console and the config files want them. `--setvar=count_checksum=1`, as one argument,
does the same.

a variable needs no declaration in the code to be settable. most of the ones below are
read with `NGlobal::GetVar( "name", default )` and nothing else, so a typo in the name is
silently the default.

## recording a replay

`game.cfg` already turns checksumming on, so an ordinary game is sampling its state.
what it is not doing is sampling finely: the shipped `checksumperiod = 1000` takes one
sample per second of game time, and `light_checksum_count = 1` walks only each unit's
position, facing and hit points.

record with both widened, play for as long as the thing being chased needs, then save:

```
./launch.sh release --data fotr --setvar light_checksum_count=0 --setvar checksumperiod=1
# play, then in the console:
save_replay my_replay
```

**the sampling is a property of the recording, not of the playback.** a checksum value is
a running adler32 carried across the whole run, so a sample taken after ten walks of the
state cannot be compared against one taken after a single walk. a playback therefore
takes the period and the depth out of the replay file and ignores whatever this run was
started with. to compare more finely, record more finely.

`setvar replay_autosave = 1` writes a replay by itself whenever a mission is won or lost,
named for the map, the outcome and the time. a losing run makes a short recording, which
is usually the useful kind.

## verifying a replay

```
./launch.sh release --data fotr --replay my_replay --verify
```

underneath that is `Game --replay=my_replay --replay-verify`, which turns on
`checksum_verify`, plays the recording out and leaves. it prints how the recording was
sampled, then one line at the end:

```
replay verify: 230 samples recorded every 1 ms, full state
replay played out at segment 229: 229 checksums matched
```

the exit status is 0 only when the replay played out, nothing diverged **and at least one
sample was compared**, so a replay carrying no checksums at all reports failure rather
than success. that makes it usable from a script, with one caveat: a sanitizer build
**replaces the exit status with 66** whenever the sanitizer reported anything, so under
`*-asan` or `*-tsan` read the "checksums matched" line instead of the status.

for long recordings, `--setvar=replay_verify_speed=10` runs an unattended verification
at the maximum normal game speed (11 times real time). it still executes every simulation
segment and compares every recorded sample. the default, -1, leaves the speed controls
alone; 0 through 10 select normal through maximum speed and override automatic slowdown
only during `--replay-verify`.

verify a replay against the build that recorded it before comparing anything else.
only then is a difference between two configurations meaningful:

```
for c in release debug asan; do
  ./launch.sh $c --data fotr --replay my_replay --verify; echo "$c -> $?"
done
```

a replay records what built it, so a playback says so when the simulation modules have
been rebuilt or the map has been edited since. the line is deliberately not a verdict:
build ids differ between a release build and a sanitizer build of identical sources, and
comparing those two is the ordinary case, so the report names the configurations and
leaves the conclusion to whoever knows which comparison they meant.

### as which player

a replay is played as player 0 unless told otherwise. `--replay-player N` plays it as
player N, the way picking a team on the replay screen does, and `Game` takes it as
`--replay-player=N`. the console form is `replay "my_replay" N`.

the simulation must come out the same whoever watches, so verifying one replay as each
player who was in the game is a check of its own: a divergence that appears for some
players and not others is simulation code reading which player is local. a number the
replay has no player for is refused with a message rather than played.

```
for p in 0 1 2 3; do
  ./launch.sh release --data fotr --replay my_replay --replay-player $p --verify; echo "$p -> $?"
done
```

a replay recorded by a build with a command type this build does not have stops before
the first such command, says where, and fails the verification.

## the variables

| variable | default | what it does |
| --- | --- | --- |
| `count_checksum` | 1, from game.cfg | take state checksums as the game runs. ignored while a replay plays, so it cannot fight the recorded values |
| `checksumperiod` | 1000, from game.cfg | milliseconds of game time between samples. 1 samples every tick |
| `light_checksum_count` | 1, from game.cfg | 1 walks position, facing and hit points per unit. **0 walks the full simulation state**, which is slower and catches far more |
| `checksum_verify` | 0, set by `--replay-verify` | compare each playback sample against the recording. acts only while a replay plays |
| `checksum_trace` | 0 | print every sample as it is taken, recording or playback, with its game time and value |
| `checksum_entry_dump_time` | -1 | at that game time, write every field of one sample's walk to a file. set it on both runs and diff the two files to find which field differs |
| `start_replay_time` | 0 | seconds of game time to wait before checksumming at all |
| `start_count` | -1 | leave entries at or below this count out of the checksum |
| `break_count` + `break_segment` | -1 | stop in the debugger at one field of one sample, by the entry number the dump above gives and the segment |
| `replay_trace` | 0 | print each segment, game time and command as a playback feeds them in |
| `async_dump_time` | 0 | at that game time in milliseconds, write the whole async bundle to `async_dump_at_time.txt`, including the ring of recent RNG call sites. point it at the time a failing run reported, in a configuration that passes, to get the other half of the diff |
| `trace_async_events` | 0 | write resource readiness, executor create, schedule and destroy, and the transparency queue to `async_events_trace.txt` |
| `resource_sync_load` | 0 | read every resource on the calling thread, which takes the loader thread and the machine's speed out of the picture |
| `replay_autosave` | 0 | save a replay when a mission is won or lost |
| `save_on_async` | 1, from game.cfg | in multiplayer, write a savegame and a checksum report when clients disagree. clears itself after firing once |
| `pause_on_async` | 1, from game.cfg | in multiplayer, pause instead of dropping the diverged client |
| `mp_replays_enabled` | 0 | show the save replay button on the multiplayer statistics screen |

these are engine state rather than switches, and setting them by hand only confuses the
thing that reads them: `History.Playing`, `Replay.ExitWhenFinished`, `Replay.Result`,
`MP_async`, `Current.Mission`.

## traps

**`break_segment` and `segment_break` are different variables.** both live in
`Common_RTS_AI/ChecksumSaver.cpp` and both pair with `break_count`, but the first stops
at a field inside a chunk and the second stops at an object. setting one and expecting
the other's breakpoint is a wasted run.

**`one_sum_per_segment` does nothing.** `game.cfg` sets it and no code reads it.

**`checksum_overwrite` does nothing.** it is registered and bound to a variable that
nothing reads. `game.cfg` has it commented out, which is as much effect as uncommenting
it would have.

**a divergence found with a replay recorded by an older build is not news.** it will
reproduce perfectly every time, which is exactly what a real defect looks like. this is
what the recorded build stamp is for; read the first lines a playback prints before
believing the last one.

the longer-lived findings, the hazards this is all pointed at and the sites that still
carry them, are in `docs/Determinism.md`.

# file hygiene

the sources are UTF-8 without a byte order mark, end their lines with LF, end their last
line with a newline, carry no whitespace at the end of a line, indent with tabs two
columns wide, and are formatted by clang-format. four things hold that in place, and they
agree with each other:

- [.editorconfig](.editorconfig) is what visual studio, clion, vs code and rider all read
  without a plugin, so an editor stops reintroducing what the hooks strip
- [.clang-format](.clang-format) is the code style, applied to whole files. see
  [formatting](#formatting) below
- [.gitattributes](.gitattributes) pins the sources to LF on checkout. this is the part
  that does not depend on anyone's local settings: a default git for windows, and the
  github windows runners, check text out as CRLF
- [.pre-commit-config.yaml](.pre-commit-config.yaml) is the enforcement, before a commit
  is made locally and again in CI through
  [.github/workflows/hygiene.yml](.github/workflows/hygiene.yml)

## setting up the hooks

hooks are not cloned, so install them once per clone:

```bash
pipx install pre-commit     # or pip install pre-commit
pre-commit install
```

the first run downloads the hooks, including the pinned clang-format, and takes a minute.
after that they run on staged files only, which costs about as long as reading the files
the commit touches. a hook that changes a file fails the commit and leaves the fix in the
working tree: look at it, `git add` it and commit again.

to check the tree the way CI does, or to see what a hook would do without committing:

```bash
pre-commit run --all-files
pre-commit run clang-format --all-files     # one hook
```

needs pre-commit 3.2 or newer. debian still packages 2.21, which refuses the pinned hook
revision with an error that names a config key rather than the cause.

nothing stops you committing without the hooks installed; CI runs the same checks and
will say so.

while there, install the commit message hook too. it strips the assistant attribution
trailers this history does not use:

```bash
cp scripts/git-hooks/commit-msg .git/hooks/commit-msg
```

copy it rather than pointing `core.hooksPath` at `scripts/git-hooks`: pre-commit refuses
to install at all while that is set ("cowardly refusing to install hooks with
`core.hooksPath` set"), and both hooks have to live in the same place.

## formatting

[.clang-format](.clang-format) is the one style, and it applies to whole files. it was
not chosen to leave the old layout alone: it is chosen for code that is easy to read,
easy to maintain and hard to make a mistake in, which is why the commit that introduced
it touched nearly every source.

it is not a style invented here either. of the 179 settings clang-format exposes, 134
already agree with both the LLVM and the Google style. what differs is this tree's own
identity (allman braces, `f( a )` spacing, tabs, 120 columns), four cases where their
default would break something here (include order, `using` order, comment rewrapping and
string splitting), and a short list chosen to be stricter than both: braces inserted on
every control statement, one element per line, and a body below its signature. each of
those carries its reason in a comment next to it.

what it does, in the parts worth knowing before you write a line:

- allman braces; `void Foo( int n )` and `if ( x )`, with spaces inside the parentheses
- tabs for indentation, two columns wide, spaces for alignment
- a 120 column limit, comments exempt
- **braces on every control statement**, inserted if you leave them out. this is the one
  setting that changes code rather than layout, and it is deliberate: a braceless body is
  how "goto fail" shipped
- one statement per line: no `if ( x ) { f(); }` on one line, no one-line loops
- one argument, parameter, list element and enumerator per line once they do not fit on
  one line. enums and arrays therefore read as a single column
- a body goes below its signature, accessors included, so
  `int GetID() const { return nID; }` is four lines. an empty body stays empty:
  `CThing() {}`
- a blank line between definitions, and `} // namespace NFoo` where a namespace closes
  far from where it opened. these two are what keep a file from reading as one block
- a semicolon after a function body is removed, since `void f() {};` closes nothing
- includes are sorted, in groups: the prelude, then project headers, then the generated
  `*_export.h`, then the standard library, then everything else. each `stdafx.h` fences
  its own list with `clang-format off`, because those are hub headers whose order is
  still load-bearing. `using` declarations are left in the order you wrote them
- comments are never rewrapped, and a long string is never split into adjacent literals

### running it

the hook does it for you on commit. to do it by hand, over the whole tree or over
particular files:

```bash
python scripts/hygiene/clang-format-sources.py                 # format everything
python scripts/hygiene/clang-format-sources.py --check         # report, change nothing
python scripts/hygiene/clang-format-sources.py path/to/File.cpp
```

the script is a thin wrapper around `clang-format --style=file`; what it adds is the skip
list, which cannot be written as a path pattern because most of it is decided by the
banner a generator wrote at the top of the file. its docstring lists what it skips.

it uses whatever `clang-format` is on PATH, or `$CLANG_FORMAT`. the hook instead installs
**clang-format 22.1.8** and uses that, because the output changes between releases: a
different version will reformat lines it disagrees with 22.1.8 about, and then the hook
will disagree with you. to match it exactly by hand:

```bash
pipx install clang-format==22.1.8
```

an editor that formats on save should be pointed at the same version. clion and visual
studio both read `.clang-format` from the repository root on their own; vs code needs the
`xaver.clang-format` extension or `C_Cpp.formatting: clangFormat`.

### headers and include order

sorting includes is only safe because a header does not depend on what the file
including it happened to include first. that was not true until recently: thirty headers
named a type without including whatever declares it, and `System/BinSaver.h` named
`CMemoryStream` while leaving `System/Streams.h` to somebody else. when you add a header,
include what you use in it, even if the prelude would have covered you.

the preludes themselves are the exception and are fenced off: `stdafx.h` files are
hand-ordered hub headers, and sorting one breaks the module.

to find out whether a header stands on its own:

```bash
python scripts/hygiene/check-header-selfcontained.py            # with the module prelude
python scripts/hygiene/check-header-selfcontained.py --strict    # without it
```

needs a configured build, because it reads the real compile flags from
`compile_commands.json`. with the prelude, 1424 of 1634 headers pass; `--strict` is the
standard that would let `stdafx.h` stop being load-bearing, and far fewer pass it. that
is a backlog item, not a rule yet. `include-what-you-use` is the tool for working it
down.

### when the formatter is wrong

a table whose rows carry meaning is the case where one element per line says less than a
grid. wrap it:

```cpp
// Laid out as a table on purpose: sin( i * 2pi / 512 ), eight to a row
// clang-format off
static float fSinTable[ N_SIN_TABLE_PERIOD + 1 ] =
{
	0, 0.0122715f, 0.0245412f, ...
};
// clang-format on
```

the directive has to be the whole comment. `// clang-format off -- because reasons` does
nothing at all, so put the reason on its own line above it. four tables in the tree use
this: the sine and arcsine tables in `System/FastMath.cpp`, the bit mask table in
`System/BitStreams.cpp` and the embedded cursor in `MapEditorLib/CursorsWx.cpp`. prefer
it over reaching for an exemption: it is local, it is visible, and it says why.

### what is left alone

nothing here is an exemption for our own code. everything under
`Versions/Temporary/Engine/Sources` that we wrote is formatted, including the tests and
the benchmarks. what is skipped is skipped because somebody or something else owns its
layout:

- **generated sources**, because the generator decides their shape and would undo the
  edit on its next run, which is how CI would find out: the 101 dbcodegen outputs,
  `3Dmotor/GfxShaders.*` from ShaderCompiler, the bison and flex output in `Parser`, and
  the editor resource tables `scripts/port/rc2*.py` write. change the generator, not its
  output
- **third-party code**: the vendored Lua in `Script` (but not Nival's own `lsaver.cpp`
  beside it), `Sources/vendor`, `third_party`, and the 1990s STL documentation under
  `Soft`
- **`Parser/test/cases`**, the `.cll` grammar's fixture corpus. every byte is input to
  `NLang::Parse`, including the mixed line endings and the missing final newlines, so
  tidying one edits the test
- **`3Dmotor/test/original`**, by a `.clang-format` of its own with `DisableFormat`.
  those files are transcriptions of the implementations the ported SIMD routines are
  checked against, and staying diffable against their source is what they are for
- the **`.cll` type descriptions** keep their trailing whitespace, because dbcodegen
  copies a tooltip comment into `types.xml` verbatim, trailing space included, so
  rewriting one is a game data change rather than a whitespace one. git still holds them
  to LF through `.gitattributes`
- the **game data trees**. the records are already LF where they are text, the localised
  text is UTF-16LE from end to end, and both are written by the editor and the database
  tools rather than by hand

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
