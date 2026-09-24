
# AGENTS.md

## Project overview
In this repo, you'll find the modified source code of an old RTS game called "Blitzkrieg 2".
It was ported from the old C++03 (+ VS 2003) to more modern C++17 (+ cmake) among other things.
The game is now multiplatform, supporting Windows and Linux. Any new change to the code should be multiplatform compatible, unless it's said otherwise.
The actual source code is in: `Versions/Temporary/Engine/Sources` folder
Some additional dependencies are in `third_party/` folder
Game also has multiplayer mode and replay system, which are based on command syncing + deterministic simulation
Along with the game, there are Map Editor and Server apps for it.

## Coding
Few rules:
- If you have any questions - ask before coding anything!
- On Windows, don't try build anything with regular system/CMD/Powershell CMake, this project used Visual Studio CMake and VS Developer Environment. On linux, you can use system CMake.
- Add some comments when you make or change the code to make it clear what was done
- Projects like AILogic, Stats_B2_M1, B2_M1_World and similar need to be deterministic!
- Linux exports every function by default, but windows does not! So make sure to mark function for exporting in appropriate header file
- Legacy .cll generator files are no longer used at all, instead, write changes directly to their result files

### Determinism coding rules
1. Do not iterate over `std::unordered_map` and `std::unordered_set`, since the iteration order is not specified! Use `det_map` and `det_set` instead or thier sorted std variants. It is allowed to use their iteration to make a sorted (by Unique IDs and with no ties!) vector/list and then iterate over that for simulation. 
2. Do not using client specific functions/variables in simulation code. E.g. `CDiplomacy::GetMyNumber()` or `CCommonUnit::IsSelectable()`
3. Bugs, uninitialized variables, special rare hacks or undefined behaviour will likely break determinism too
4. Do not use bad sorting operators that have ties, which have non deterministic effects

While great majority of the code is deterministic by defalt, a single wrong bit is enough to ruin everything. So the aim should be to have gameplay simulation code 100% deterministic, no buts or excuses there.
Using floating point math is fine for now as cross platform MP isn't done yet, but floating point types will be replaced with deterministic fixed point types andmath in the future.

### Game code projects
Projects are just folders in `Sources` directory.
main game:

- 3DLib - 3D geometry: vectors, matrices, quaternions and so on
- 3Dmotor - the main 3D engine (Direct3D9 based): graphics, effects, lights, etc.
- AILogic - the behavior of various unit types, their abilities and commands, requires strict determinism in code (for MP)!
- B2_M1_Terrain - few additional terrain objects for M1
- B2_M1_World - few additional units for M1 (e.g. helicopters, paratroopers)
- Client - network client code (multiplayer)
- Common_RTS_AI - path finding utilities
- DebugInfo - in-game debugging overlay
- Game - main game executable
- GameX - various in-game interfaces
- Image - reading and writing various image formats (DXT, TGA, DDS, PSD, etc.)
- Input - mouse and keyboard input (DirectInput based)
- libdb - reading and writing various game resources from database (serialization)
- Main - main game loop, connecting all other subsystems together (like 3Dmotor, Sound, Input, etc) and loading the mods
- MemoryLib - custom memory allocator, not really used anymore
- Misc - small basic routines, like random, filesystem, timer, string utilities, etc.
- Net - general network utilities, like IP address parsing and so on
- Parser - custom parser for cll files (bison based), grammar is in lang.l and lang.y
- SceneB2 - terrain and map, objects like rivers, roads
- Server_Client_Common - common code between client and server, essentially an implementation of multiplayer network protocol
- Sound - sound and music
- Stats_B2_M1 - mostly generated code for unit abilities and their actual stat numbers
- System - generic basic classes, like file streams, XML parsing, ZIP archive support, etc.
- UI - user-interface controls rendered in-game (buttons, scroll-bars, windows, etc.)
- UISpecificB2 - additional UI controls

editor:

- B2_MapEditor - Map Editor main executable
- ED_B2 - chapter editor
- ED_B2_M1 - main editor implementation
- ED_Common - some basic UI controls for editor
- MapEditor - common dialog windows for editor
- MapEditorLib - few more dialogs

server:

- Server - Server main executable

other small utilities:

- dbcodegen - utility to generate XML databases (types.xml)
- dbindex - database index utility (generates index.bin file)
- dbstruct - database structure utility (converts xcb files into packaged binary format)
- ELK_A7 - used for translation and spell-checking
- FontGen - font generator (in the format game understands)
- ShaderCompiler - compiles shaders from custom format, shaders are in GfxShaders.txt, compiled shaders are in GfxShaders.cpp

test code:

- TestClient - test application for headless multiplayer testing
- TestDB - some tests for database routines (libdb library)
- TestParsing - some tests for parsing routines (Parser library)
