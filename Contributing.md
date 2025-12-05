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

assuming installation into the `C:\Games\bk2` directory, the commands to copy will be:
```cmd
robocopy Versions\Current\Profiles\ C:\Games\bk2\Profiles\ /S /E >NUL
robocopy Versions\Current\Data\ C:\Games\bk2\Data\ /S /E >NUL
xcopy Versions\Current\splash.bmp C:\Games\bk2\
```

copying will take a while - there are **MANY** small files.

# launching the game

the game executable is `bin\Game.exe`. therefore, if game was installed into `C:\Games\bk2`, then `C:\Games\bk2\bin\Game.exe` should be launched.

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
- [Scintilla](Versions/Temporary/Engine/Sources/Scintilla) - old bundled copy of [Scintilla](https://scintilla.org/) library
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
- [ED_RTS](Versions/Temporary/Engine/Sources/ED_RTS) - combines other editor modules together
- [MapEditor](Versions/Temporary/Engine/Sources/MapEditor) - common dialog windows for editor
- [MapEditorLib](Versions/Temporary/Engine/Sources/MapEditorLib) - few more dialogs

server:
- [Server](Versions/Temporary/Engine/Sources/Server) - Server main executable

other small utilities:
- [dbcodegen](Versions/Temporary/Engine/Sources/dbcodegen) - utility to generate XML databases (`types.xml`)
- [dbindex](Versions/Temporary/Engine/Sources/dbindex) - database index utility (generates `index.bin` file)
- [dbstruct](Versions/Temporary/Engine/Sources/dbstruct) - database structure utility (converts `xcb` files into packaged binary format)
- [ELK_A7](Versions/Temporary/Engine/Sources/ELK_A7) - used for translation and spell-checking
- [FontGen](Versions/Temporary/Engine/Sources/FontGen) - font generator (in the format game understands)
- [ShaderCompiler](Versions/Temporary/Engine/Sources/ShaderCompiler) - compiles shaders from custom format, shaders are in [GfxShaders.txt](Versions/Temporary/Engine/Sources/3Dmotor/GfxShaders.txt), compiled shaders are in [GfxShaders.cpp](Versions/Temporary/Engine/Sources/3Dmotor/GfxShaders.cpp)

test code:
- [TestClient](Versions/Temporary/Engine/Sources/TestClient) - test application for headless multiplayer testing
- [TestDB](Versions/Temporary/Engine/Sources/TestDB) - some tests for database routines ([libdb](Versions/Temporary/Engine/Sources/libdb) library)
- [TestParsing](Versions/Temporary/Engine/Sources/TestDB) - some tests for parsing routines ([Parser](Versions/Temporary/Engine/Sources/Parser) library)

miscellaneous:
- [librcs](Versions/Temporary/Engine/Sources/librcs) - custom revision control client library, outdated (predates Git and Subversion), not really used

the game engine is referred as "enigma" in various sources, however, this name is not used in source code.

the naming convention (see also [NivalProjectNames.doc](Versions/Temporary/Engine/NivalProjectNames.doc)):
- Blitzkrieg I: `A7`, `B1`
- Blitzkrieg II: `A10`, `B2`

`M1` seems to be a reference to another version of engine or game, see `VERSION_DEV_M1`, which modifies behavior in several places.

# branches

there are few branches where main development takes its place:
- [port](https://github.com/OpenBK2/OpenBK2/tree/port) - the main development branch where all major work is done. it includes porting to modern compiler, fixing game bugs (like crashes and asyncs), performance optimizations, etc.
- [editor](https://github.com/OpenBK2/OpenBK2/tree/editor) - work in progress to restore Map Editor executable build
- [linux](https://github.com/OpenBK2/OpenBK2/tree/linux) - work in progress to port game code to Linux, rewriting Windows-specific code to portable alternatives
- [server](https://github.com/OpenBK2/OpenBK2/tree/server) - work in progress to restore Server executable build
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
