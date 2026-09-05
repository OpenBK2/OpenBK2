
# AGENTS.md

## Project overview
In this repo, you'll find the (slightly modified) source code of an old RTS game called "Blitzkrieg 2".
It was ported from the old C++03 (+ VS 2003) to more modern C++17 (+ cmake) among other things.
The actual source code is in: `Versions/Temporary/Engine/Sources` folder
Some additional dependencies are in `third_party/` folder
Game also has multiplayer mode and replay system, which are based on command syncing + deterministic simulation

## Coding
Few rules:
- If you have any questions - ask before coding anything!
- On Windows, don't try build anything with regular system/CMD/Powershell CMake, this project used Visual Studio CMake and VS Developer Environment. On linux, you can use system CMake.
- Add some comments when you make or change the code to make it clear what was done

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
- Scintilla - old bundled copy of Scintilla library
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
- ED_RTS - combines other editor modules together
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

miscellaneous:

- librcs - custom revision control client library, outdated (predates Git and Subversion), not really used
