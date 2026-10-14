
# AGENTS.md

## Project overview
In this repo, you'll find the modified source code of an old RTS game called "Blitzkrieg 2".
It was ported from the old C++03 (+ VS 2003) to more modern C++17 (+ cmake) among other things.
The game is now multiplatform, supporting Windows and Linux. Any new change to the code should be multiplatform compatible (tests too), unless it's said otherwise.
The actual source code is in: `Versions/Temporary/Engine/Sources` folder
Some additional dependencies are in `third_party/` folder
Game also has multiplayer mode and replay system, which are based on command syncing + deterministic simulation
Along with the game, there are Map Editor and Server apps for it.

## Coding
Few rules:
- If you have any questions or if anything is ambiguous - ask before coding anything!
- On Windows, don't try build anything with regular system/CMD/Powershell CMake, this project used Visual Studio CMake and VS Developer Environment. On linux, you can use system CMake.
- Add some comments when you make or change the code to make it clear what was done
- Projects like AILogic, Stats_B2_M1, B2_M1_World and similar need to be deterministic!
- Linux exports every function by default, but windows does not! So make sure to mark function for exporting in appropriate header file
- The game database schema and its C++ structs are generated from the .cll files by dbcodegen: change the .cll and regenerate, never the generated DB*.h/.cpp or types.xml. See "adding things to the game database" in Contributing.md
- Write code in the house style from the first line rather than leaving it to the formatter. It is in `.clang-format` and spelled out under "Code style" below
- Sources are UTF-8 with LF endings, a newline on the last line, no whitespace at the end of a line, and tabs two columns wide. `.editorconfig` declares it, `.gitattributes` pins the endings and `.pre-commit-config.yaml` enforces it, locally and in CI. Install it once per clone with `pre-commit install`. See "file hygiene" in Contributing.md

### Code style

The whole tree is formatted, and `.clang-format` is enforced on whole files before a
commit and again in CI. Write it this way to begin with: a patch that needs reformatting
is a patch whose diff is bigger than the change it makes.

```cpp
// Allman braces. Spaces inside the parentheses of a call and a condition, none
// in a C-style cast. Tabs to indent, two columns wide.
const float GetDistance( const CVec2 &vStart, const CVec2 &vEnd )
{
	// Braces on every control statement, however short the body. This is a
	// safety rule, not a style one: the next person to add a line here must not
	// be able to add it outside the condition.
	if ( fDiff > 0.0f )
	{
		return (std::min)( fDist1, fDist2 );
	}
	else
	{
		return 0.0f;
	}
}

// A blank line between definitions, so where one ends and the next begins does
// not have to be worked out from the braces.
inline float Lerp( const float a, const float b, const float t )
{
	// One statement per line. Never if ( x ) { f(); } on one line, and never a
	// one-line loop.
	for ( int i = 0; i < nCount; ++i )
	{
		Use( i );
	}

	return a + ( b - a ) * t;
}

// One enumerator per line, so adding one is a one-line diff.
enum EKind
{
	KIND_FIRST,
	KIND_SECOND
};

class CThing
{
	OBJECT_NOCOPY_METHODS( CThing );
	int nID;     // trailing comments line up with their neighbours,
	float fGain; // in a column of spaces rather than tabs

public:
	// An empty body stays empty, and that is the only thing that stays on one line.
	CThing() {}

	// A signature goes on its own line and the body below it, accessors included.
	int GetID() const
	{
		return nID;
	}
};
```

The rest of it, and the reasons, are the comments in `.clang-format`. What it will do to
your code if you do not:

- wrap at 120 columns, aligning the continuation under what it continues
- put one argument, parameter or list element per line once they do not fit on one line,
  so a long call and an array both read as a single column
- put a `template <...>` header on its own line
- put a blank line between definitions, and `} // namespace NFoo` on a namespace that
  closes far from where it opened
- move a one-line function body below its signature, and drop a semicolon that follows a
  body (`void f() {};`), which closes nothing
- leave your includes and `using` declarations in the order you wrote them, since
  `stdafx.h` has to come first; it never sorts them
- leave comments alone: it does not rewrap them, and it does not split a long string
  literal, so a log line stays greppable

Run it on what you wrote:

```bash
python scripts/hygiene/clang-format-sources.py path/to/File.cpp
python scripts/hygiene/clang-format-sources.py --check            # whole tree, no writes
```

Do not add an exemption for code we own. Generated sources, third-party code and the
parser's fixture corpus are skipped already; if a table genuinely reads better as a grid,
wrap that block in `// clang-format off` and `// clang-format on` with a line above
saying why. The directive has to be the whole comment: anything appended to it and it
does nothing.

Other things worth knowing before writing a line:

- `ASSERT` and `NI_ASSERT` never evaluate their argument in this build, so nothing with a
  side effect belongs inside one
- use `boost/predef` for platform and compiler detection (`BOOST_OS_WINDOWS`, not
  `_WIN32`), and note these are `#if`, not `#ifdef`
- no em-dashes or en-dashes anywhere, including comments; a plain hyphen
- comments carry information. A row of punctuation is not a section heading

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

- dbcodegen - compiles the .cll type descriptions into types.xml and the DB*.h/.cpp sources (structs, serializers, meta info, checksums); run by hand after a .cll change (see "adding things to the game database" in Contributing.md); -nocopy writes to the temp directory only
- dbindex - database index utility (generates index.bin file)
- dbstruct - database structure utility (converts xcb files into packaged binary format)
- FontGen - font generator (in the format game understands)
- ShaderCompiler - compiles shaders from custom format, shaders are in GfxShaders.txt, compiled shaders are in GfxShaders.cpp; run the regenerate-gfxshaders target
- ShaderAsm - D3D9 shader assembler (vs.1.1, ps.1.1, ps.1.4, ps.2.0) that ShaderCompiler uses in place of D3DX; matches D3DXAssembleShader token for token

test code:

- TestClient - test application for headless multiplayer testing
- TestDB - the database schema (TestType.cll) and records libdb/test runs the editor database against; its types build into a test-only library
- Parser/test - the Parser library (the .cll grammar) against a corpus of cases that must parse or must be rejected; formerly TestParsing
