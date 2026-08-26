# Where the port stopped

Written 2026-08-26. Part of the Linux port notes; see
[LINUX_PORT.md](../../LINUX_PORT.md) for the index.

This is the resume point, nothing else. What is still to do lives in
[PORT_ROADMAP.md](PORT_ROADMAP.md), what has been applied in
[PORT_REPLAY.md](PORT_REPLAY.md), why the codebase behaves as it does in
[PORT_FINDINGS.md](PORT_FINDINGS.md), and the graphics stack in
[DXVK.md](DXVK.md).

## State of the branches

| | |
|---|---|
| `linux2` | see `git log -1` |
| Windows clone `C:\projects\OpenBK2` | same |
| WSL clone `~/src/OpenBK2` | same |
| `origin/linux2` | `b9b480aab`, **eighty-five** commits behind, a plain fast-forward. The user asked to hold the push |
| `netcode_bugfixes` (base) | `48e12ca2b` |
| commits on `linux2` past the base | 264 |

An earlier note said `origin/linux2` was at `f119e72ac` and forty-eight commits
behind. It is not: the user pushed `b9b480aab` themselves after that was written.

`linux2-verified` is at `e72702cd7`, still the pre-rebase tip and still orphaned
in both clones. It has to be re-pointed by hand after the next verified run.

`linux2-strays` at `1f97a93c6` still holds the parked scratch files.

## Where the game stops

**The data path is finished.** A Windows trace recorded on 2026-08-26 and a
Linux one, compared with `scripts/port/diff-platform-trace.py`, are **identical
for all 100 events Linux produces**. Same files, same order. Linux gets exactly
as far as Windows and stops where Windows goes on to build the main menu:

```
ui/game/menu/mainmenu_windowscreen.xdb        <- Windows continues here
ui/game/menu/mainmenu/mainmenu_windowsimple.xdb
ui/game/menu/main/exit_windowmsbutton.xdb
```

So there is no missing file, no wrong path and no remaining case problem. **The
fault is in code**, at main menu construction.

Getting there took one rename: the code asks for `Cursors/` and the install had
`cursors/`, which cleared **27 of 74** VFS failures. The other 47 were never
real, being served from inside the paks. Only the directory case differed; every
filename already matched.

### What AddressSanitizer says

`-DENABLE_ASAN=ON` builds and runs. The report:

```
ERROR: AddressSanitizer: SEGV on unknown address 0x1dd93b82da05
The signal is caused by a READ memory access.
    #0  virtual thunk to CWindowScreen::DestroyContents()
    #1  CObjectBase::DestroyDelayed()          Basic.cpp:124
    #2  CObjectBase::ReleaseObj(int, int)      Basic.cpp:69
    ...
    #11 NMainLoop::PopInterface()              MainLoopInternal.cpp:141
    #12 CICPreviousMenu::Exec()                GameX/InterfaceMisc.cpp:219
    #13 ProcessInterfaceCmds                   MainLoopInternal.cpp:246
```

**Three things in that the plain backtrace did not give:**

- **`CICPreviousMenu::Exec()` is what starts it.** The game is going *back* a
  menu, so `PopInterface` pops the interface stack and releases the screen. The
  crash is on the way out of a screen, not the way into one.
- **The faulting frame is a virtual thunk**, the multiple inheritance `this`
  adjustment for `CWindowScreen : public CWindow, public IScreen`. A thunk
  faulting on a read means the object it was handed is not what it claims to be:
  a garbage or already-freed vtable pointer, rather than a fault inside the
  function body.
- **`rcx = 0xf5f5f5f5f5f5f5f5`** in the register dump, a poison fill. Consistent
  with the object having already been destroyed.

ASan calls it "SEGV on unknown address" rather than a use-after-free, which
means the memory is not one of its tracked heap allocations. That fits an object
the engine destroyed through its own machinery: `DestroyDelayed` keeps
`toDestroy` and `toInvalidate` lists, and an object reaching either twice would
produce exactly this - `DestroyContents` called on a corpse.

**So the next question is whether this is a double release.** `CObjectBase`
self-deletes through `delete this`, and `DestroyContents` destroys in place and
placement-news the object back while preserving the refcounts. A watchpoint on
that object's refcount through `CICPreviousMenu::Exec` would settle it.

## Superseded: where the game stopped before the traces were compared

**The whole loading chain works.** The VFS mounts `Data`, opens `data.pak` and
the five patch paks, reads `types.xml` and `index.bin`, the database loads
objects, seven config files execute including the two that follow `LoadProfile`,
the profile's own `user.cfg` and `input.cfg` are read, and the game gets as far
as building its UI. The file trace ends on window definitions:

```
UI/Game/Debug/DebugWindow_WindowSimple.xdb
UI/GameStatsWindow_WindowStatsSystem.xdb
UI/DebugStatsWindow_WindowStatsSystemShared.xdb
```

It then segfaults **during teardown**, unwinding the interface stack:

```
UIScreen.h:29   CWindowScreen::DestroyContents
Basic.cpp:124   CObjectBase::DestroyDelayed
Basic.cpp:69    CObjectBase::ReleaseObj
                ~CObj<IInterfaceBase>, from NMainLoop::interfaces
```

Nothing is logged before it. `UIScreen.h:29` is the `OBJECT_NOCOPY_METHODS`
macro, so `DestroyContents` is generated: it calls the destructor explicitly,
saves `nRefData` and `nObjData`, placement-news the object back, and restores
them. The crash is inside that, so the next question is whether it is the
destructor or the reconstruction, and which member.

**Two things to be careful of here.** The stack is being torn down at all, which
means something upstream decided to exit and did not say why; that may be the
real fault and this only its consequence. And `DestroyContents` reads
`this->nRefData` after the destructor has run, which is reading a destroyed
object - benign for a POD in practice, on both platforms, but worth knowing
before trusting anything it reports.

### How it got here, and what each fix was

Every one of these was silent, and each hid the next.

| what | where |
|---|---|
| `DXVK_WSI_DRIVER` unset, `Direct3DCreate9` threw before logging a line | `81d792220` |
| backslash separators through the whole startup chain; **zero** config files loaded | `5fe3d15c1`, then constants and `JoinPath` in `5bb2331ff` |
| UTF-16 data read as `wchar_t`, which is four bytes off Windows | `8eb019d46` |
| nothing said when a file could not be opened | `673a0d373` |
| VFS base path concatenation, and my own `JoinPath` regression | `e92f9f8e8` |
| VFS base path converted to backslashes, so every lookup missed | `ac1c5698e` |
| `CProfiler( ... ) : szPath( szPath )`, initialised from itself | `ac1c5698e` |

That last one is worth reading twice. It has been undefined behaviour since it
was written, Windows survived it because the stack happened to be benign, and
GCC at `-O0` handed the allocator a **two terabyte** length. mimalloc took it as
a huge page request and sat zeroing it, which presents as a slow debug build and
is not one.

**How that was told apart from slowness, since the first guess was wrong:** over
eight seconds the log did not grow by a byte, `/proc/<pid>/io` `read_bytes`
stayed at zero, and one thread held every jiffy. A parse reads and prints; a spin
does neither. Check that before believing a build is merely slow.

### Debugging notes for this codebase

- **Every module is a shared library**, so a gdb breakpoint set before `run`
  does not resolve and silently does not exist. Use
  `set breakpoint pending on`, or conclude wrongly that the code never ran.
  That mistake was made twice here.
- `OPENBK2_FILE_TRACE=1` gives a line per file open, successes included, and
  covers VFS lookups that never reach a syscall so strace cannot see them.
- `timeout` without `-s KILL` leaves the game running after the shell dies.
  Three stale processes accumulated before this was noticed.

## One uncommitted change, left on purpose

`System/Streams.cpp` has a local override forcing the file trace on:

```cpp
static const bool bEnabled = true;// getenv( "OPENBK2_FILE_TRACE" ) != 0;
```

**Do not commit it.** `OPENBK2_FILE_TRACE=1` in the environment does the same
thing, and a build that always traces every file open is not what anyone else
wants. It is left in the working tree rather than reverted because it belongs to
whoever put it there, and because a Windows run is easier to capture with the
flag hardcoded than with an environment variable.

Revert it with `git checkout -- Versions/Temporary/Engine/Sources/System/Streams.cpp`
when it has served its purpose.

## Handing over to a real Linux machine

**WSL is out of road.** It builds, installs and starts the game, but it has no
usable display stack for what comes next: DXVK wants a real Vulkan device and a
real compositor, and llvmpipe under WSLg is not going to answer the questions
that are left. Everything below the renderer has been taken as far as it can be
from here.

What travels with the branch: the whole tree builds and links, `Game` runs, and
`docs/port/` and `LINUX_PORT.md` are committed rather than left local so that
they arrive with it. They were in `.git/info/exclude` until this point.

**The first thing to do on the new machine** is reconfigure with debug
information, because the build directory used here was plain `Release`:

```bash
cmake -S . -B linux-build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build linux-build
cmake --install linux-build --prefix ~/bk2
# game data is not installed by CMake, see CLAUDE.md
cd ~/bk2/bin && ./Game
```

`RelWithDebInfo` matters more than usual now: `Game` links Boost.Stacktrace
through the libbacktrace backend, and without `-g` a trace resolves to library
names and offsets rather than to functions and lines.

Run from `~/bk2/bin`, not through a launcher elsewhere. The game resolves data
against the **working directory**: `NMainLoop::GetBaseDir` is the parent of it,
which is why the installed `blitzkrieg2.desktop` sets `Path=`. No
`LD_LIBRARY_PATH` is needed any more.

## The build loop no longer needs the user in it

The old note said "the user pastes a GCC error". That is no longer the shortest
path. From the Windows session, `wsl.exe -e bash -lc '...'` reaches the WSL clone
and its `linux-build` directory directly, so a whole-tree `ninja -k 0` and its
log are one tool call away.

Better still, a **single translation unit can be compiled against the Windows
working tree before anything is committed**, which takes the commit-sync-build
round trip out of the inner loop. This is what every fix below was checked with:

```bash
cd ~/src/OpenBK2/linux-build
ninja -t commands <the .o target> | tail -1 > /tmp/cmd.txt
# then, in that command line:
#   /home/sse4/src/OpenBK2/Versions  ->  /mnt/c/projects/OpenBK2/Versions
#   redirect -o to /tmp, and drop the ccache prefix
bash /tmp/cmd2.sh
```

Safe in a way that copying files into the WSL clone is not: nothing is written
inside `~/src/OpenBK2`, so the next fast-forward cannot refuse.

Verified builds still go through the clone, in the old order: **commit, sync,
build.**

```bash
cd ~/src/OpenBK2 && git fetch windows && git merge --ff-only windows/linux2
```

The Windows side needs a developer shell before cmake will work. Without
`vcvarsall` every translation unit fails on `cstddef`:

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1 && cmake --build C:\projects\OpenBK2\out\build\Windows-x64-Release --parallel'
```

**Never run two builds against `linux-build` at once, and never sync the clone
while one is running.** This produced `internal compiler error: Bus error` from
cc1plus three separate times, which looks like a source problem and is not.

Two wrong guesses were made first, so they are written down here rather than
repeated. It was not memory: it came back at `-j 3` with 18 GB free. It was not
simply a stale precompiled header either, though deleting one does clear it.

The cause is two `ninja` invocations in the same build directory, or a
`git merge` landing in the clone while a build reads it. Both were done here.
Two ninjas write the same `cmake_pch.hxx.gch` while other translation units have
it mapped, and GCC maps a `.gch` rather than reading it, so a file that changes
underneath faults instead of failing to parse. `ccache -sv` corroborates it:
every one of its 299 errors is "Input file modified during compilation", 100% of
them.

The tell is the four lines above the error, which name what the compiler was
reading, usually `<module>/CMakeFiles/<module>.dir/cmake_pch.hxx`. To recover:

	find linux-build/Versions -name cmake_pch.hxx.gch -delete

then rebuild, with nothing else touching the tree. Grep every log for
`internal compiler error` before trusting its error count, and never read an ICE
as a source problem.

**Build Windows before committing, not after.** The loop had been commit, then
verify both, which worked until it did not: an export missing from a System
header linked fine on Linux and failed with LNK2019 on Windows, and a
Boost.Process commit had to be dropped after it had already been fast-forwarded
into the WSL clone, which then needed `git reset --hard windows/linux2` to get
back. MSVC catches two classes GCC cannot: a missing `SYSTEM_EXPORT` on a symbol
used across a DLL, and header conflicts like Asio's winsock2 against
`port/socket.h`'s winsock.

Filter that build's output on `error C|error LNK|fatal error`, not on "error":
every cl.exe and link.exe command line carries `/errorReport`, so a plain match
reports the whole build as failing. Do not add `FAILED` to that pattern either:
PowerShell's `Select-String` is case-insensitive by default, and a reconfigure
prints Google Benchmark's feature probes as "Performing Test HAVE_POSIX_REGEX --
failed to compile", which are not errors.

## Where each side stands

- **Windows x64 Release**: green at `befe52516`, and after each of the
  fifty-one commits made on 2026-08-26. **x86 Release is green too**, which CI
  builds on every push and which nothing had checked since the port began.
- **Linux**: **the whole tree compiles.** No translation unit fails, anywhere.
  All **25** modules link, up from sixteen:

  | | |
  |---|---|
  | link | MemoryLib, Misc, System, Parser, Script, libdb, Image, 3DLib, 3Dmotor, Net, DebugTools, Sound, Server_Client_Common, B2_M1_Terrain, Client, Common_RTS_AI, Input, AILogic, SceneB2, UI, UISpecificB2, B2_M1_World, Main, GameX, Stats_B2_M1 |

  **A whole-tree `ninja -k 0` succeeds.** `Game` is an ELF 64-bit executable
  with all 187 of its shared library dependencies resolved, and nothing in the
  tree fails to build. That is the first time it has ever linked.

  It links against a **Granny stub**, not against Granny: every one of its 54
  entry points returns null, zero or false and records that it was called. So
  this is a binary that builds and starts, not one that plays. See
  [Granny, and the log that says what to port](PORT_ROADMAP.md#granny-and-the-log-that-says-what-to-port).

  The link phase, unlike the compile phase, does not cascade: a linker reports
  every unresolved symbol in one pass, so the counts along the way were totals
  rather than floors. 107, then 66, 63, 54, 0.

- **There are no platform blockers left.** `intrin.h`, D3D9, Win32 windowing and
  dinput are all done, and Granny is stubbed rather than blocking.

### What the forty-one commits did

| commit | what |
|---|---|
| `f292aedab` | `this->` on 23 references in CDecksSet, CStObjGlobalIter, CStObjCircleIter, CAreaMap, CStructAreaMap |
| `001aff8a6` | deleted `CSmoothRotatedArray2D::Clear`, an unfinished body with no callers |
| `f875f6357` | `System/FastMath.h` in StaticObjectRotation.h, for `NMath` |
| `26b032309` | `windows.h` in `Sound/SFX.h`, for `HWND` |
| `f2d69f239` | NetLogger's `_strdate`, `_strtime`, `_ftime64`, `__timeb64` to chrono and strftime |
| `01ae93c27` | `Misc/BitData.h` in AIMap.hpp, for the `CArray2D1Bit` it defines |
| `a3cf61a8f` | `SYSTEMTIME` in the savegame and replay lists becomes `SSystemTime` in port/time.h |
| `7953c65a0` | `DeleteFile` becomes `NFile::RemoveFile`, eight call sites |
| `6e1a5b3d4` | deleted CBresenham3, MakeLine3, ScanLine3, BresenhamFilledEllipse |
| `49865985d` | deleted GetLoadInfo and its functors in Formation.cpp |
| `9bf8882fc` | deleted `CIter` and `CLineIter`, which nothing instantiates |
| `15a387b48` | `StaticObject.h` in StaticObjectsIters.h, for `CExistingObject` |
| `d2b87b8d2` | GetLongTickCount on steady_clock instead of FILETIME |
| `f7b59d371` | `strnicmp` in WingScaleMutator becomes a prefix compare over NStr::ASCII_tolower |
| `3b0284753` | WindowTextView stops taking the address of a temporary rect |
| `c8d6a2b1c` | deleted `CProgrammedReactionsAndChecks::SetInterface`, which assigns to a member that does not exist |
| `92188cda7` | deleted CMPUIMessageTranslator's operator&, which made the name ambiguous in CInterfaceMPScreenBase |
| `95d6fd6b3` | the anonymous union in `CWaterPatch::SGridType` becomes `CVec3 pos` |
| `3df7da43d` | the profile name is read out of NGlobal::CValue through GetString |
| `ec4bfdad8` | `GroupLogic.h` in Behaviour.cpp, for the `CGroupLogic` it names |
| `0b4dbd976` | deleted the three `extern CGroupLogic theGroupLogic` that no translation unit uses |
| `7b676aab6` | `CBasicGun::CParallelGuns` becomes protected, like the member it names |
| `0a9ebf99b` | deleted the two explicit calls to the `CTerraAIObserver` constructor |
| `39e575fd9` | `AddObject`'s low level model gets a null pointer instead of `false` |
| `6472aa3b8` | deleted GameX's copy of the font generator, 139 error lines of GDI that nothing called |
| `055b4f3f4` | Scintilla moved inside `if(BUILD_EDITOR)`, the only thing that links it |
| `6c73c6ad4` | deleted the CRT debug heap setup in WinMain, inert in Release and superseded by mimalloc |
| `6a04eb6e2` | WinMain's `SetErrorMode` guarded on Windows |
| `7f9592283` | WinMain's log directory comes from `NFile::GetNormalizedCurrDir` |
| `745f2a4bc` | the LAN test hook launches through the new `port/process.h` |
| `c668c755c` | `NFile`'s working directory helpers get `SYSTEM_EXPORT` |
| `14395c084` | a `.desktop` entry is installed off Windows |
| `0141ec6b5` | the RIFF, ICO and DIB readers move out of WinCursor into `System/WinImageFormats` |
| `456282bac` | that reader lists every entry, picks by size and depth, and reads 1 and 4bpp |
| `109df01b8` | `main.ico` installs beside `splash.bmp`, so one file serves both readers |
| `f59b3003f` | `SDL_SetAppMetadata`, so a Wayland compositor can match the desktop file |
| `bd854d83f` | `GetBaseDir` built with `parent_path`; it returned a path that did not exist off Windows |
| `1486af42e` | the window icon is read from `main.ico` and every size handed to SDL |
| `0d70302ff` | crashpad's paths go through `FILE_PATH_LITERAL`; `Game/main.cpp` now compiles |
| `76d15cf24` | the crash handler is installed beside the game, which nothing was doing |
| `11a8b8557` | `_control87` and `_controlfp` become `NWin32Helper::MaskAllFloatingPointExceptions` |

Measured whole-tree totals along the way: 2370, 450, 426, 388, 289, 266, 256,
213, 206, 67, 36, 42, 38, 37, 34, 2, 0. The 36 to 42 step went **up** on purpose:
see below.

### What the last ten commits did

The first four ended the compile phase; the rest took the link of `Game` from
107 undefined symbols to 54.

| commit | what |
|---|---|
| `c76c510f8` | the `dinput.h` include leaves `Bind.cpp`, which named nothing it defines |
| `619b571b9` | deleted IsMouseDisabledDebug and the four empty Start/StopSaveInput stubs, none of which has a caller, and with them the Windows 2000 GetVersionEx check and InitInput's bDebugMouse |
| `40101f80c` | the Input device layer splits at eight functions; SDL behind them off Windows, DirectInput unchanged on it; `port/dinput.h` carries the vocabulary and the scan code table |
| `179ec9719` | SdlMouseButtonToOffset reports MOUSE_BUTTON5, 6 and 7, which kiKeyInfoList names and SDL numbers straight on past X2 |
| later | ffmpeg, `main`, CTerraGen, the D3DX stub and the Granny stub, which took the link of `Game` from 107 undefined symbols to 0 |
| last | `$ORIGIN` and the runtime dependency install, the exception guard, and Boost.Stacktrace on the throw point |
| `44e46cb67` | the system FFmpeg through pkg-config off Windows, with a 5.1 floor; the MSVC zip was being downloaded everywhere and its DLLs handed to GNU ld |
| `d4c23352d` | WinMain and main both call RunGame; ProcessCommandLine takes the arguments already separated, which only Windows has to do itself |
| `3fa976f8d` | CTerraGen's FindCrag, FindCragInfo and FindRiver lose the `inline` that kept them out of every other translation unit |
| `d56d4db1b` | the D3DX shader effects get a stub off Windows, because the path is unreachable on Windows too |
| `ad55d4a6c` | the stub gets the `d3d9.h` it needs for LPCSTR and for a complete IDirect3DVertexShader9 |
| `befe52516` | the stub records the *other* shader system, the one that works: GfxShaders.txt to committed bytecode to DXVK |

### The fonts cluster was dead code

The largest thing on the board, 139 of the 206 lines left before it went, turned
out to be unreachable. `GameX/Font*.cpp` is a stale copy of the `FontGen/` tool,
which is a separate unwired source directory; nothing in the tree calls into
`NFontGen`.

**The game never rasterises a glyph.** `NDb::SFont` points at a prebuilt DDS
atlas, `GLocale` pulls a prebuilt `CFontFormatInfo` out of the "Fonts" resource
by uid, and `GText` lays out with atlas rectangles, A and B+C advances and a
pairwise kerning map. No `.ttf`, `.fon` or `.otf` ships anywhere. So no font
library is needed, and the format cannot express shaping or mark positioning even
if one were added. The full write-up, including which library `FontGen` would
want if it is ever ported, is in
[Fonts: the game does not rasterise glyphs](PORT_ROADMAP.md#fonts-the-game-does-not-rasterise-glyphs-on-any-platform).

**Ask this question first of every remaining cluster.** Two of the four biggest
items today were dead code, not porting work.

### The savegame constraint that came out of this

`SSavegameEntry` and `CInterfaceReplaySaveLoad::SReplayEntry` hand a timestamp to
`IBinSaver`, which has no `operator&` for a plain struct and therefore writes
`sizeof(T)` **raw bytes** through `DataChunk`. Those entries sit inside the
interface stack that `MainLoopInternal.cpp:183` serialises into a savegame, so
the field layout is an on-disk format.

That is why `a3cf61a8f` introduces `SSystemTime`, eight `uint16_t` in
SYSTEMTIME's order with a `static_assert` on the size, instead of the `time_t`
that would otherwise be the right type. **Check this before changing the type of
any member of a struct that reaches IBinSaver without its own operator&.**

## What is left, by cause

**A missing-header fatal is a floor, not a count**, and it can hide a great deal
or nothing at all. `6c73c6ad4` proved the first half: deleting one `crtdbg.h`
line took the count from 36 to **42**, because seven errors further down
`Game/main.cpp` had been hidden behind it all day. The `dinput.h` pair proved the
second: 2081 lines behind them and the count went to **zero**, because `Bind.cpp`
needed nothing at all and `Input.cpp` needed its device layer replaced and
nothing else.

Nothing is left to compile. What follows is the link of `Game`, in five clusters,
and **three of the five read as CMake wiring rather than as porting work**:

| cluster | symbols | what it looks like |
|---|---|---|
| ~~Granny~~ | ~~54~~ | stubbed: `cmake/granny.cmake` builds `vendor/granny/GrannyStub.cpp` off Windows, and every call is logged |
| ~~`NGfx::CPixelShader`, `CVertexShader`~~ | ~~9~~ | done in `d56d4db1b`: stubbed, because the path is unreachable on Windows too |
| ~~`CTerraGen::FindCrag`, `FindCragInfo`, `FindRiver`~~ | ~~3~~ | done in `3fa976f8d`: all three were defined `inline` in a `.cpp` |
| ~~`main`~~ | ~~1~~ | done in `d4c23352d`: WinMain and main both call RunGame |
| ~~ffmpeg~~ | ~~38~~ | done in `44e46cb67`: the system libraries, through pkg-config |

The ffmpeg one was worth the diagnosis it got, because the failure did not look
like what it was. `cmake/ffmpeg.cmake` downloaded a prebuilt **MSVC** zip on every
platform. Off Windows `find_library` then found no import library, but the
`file(GLOB "*.dll")` beside it matched, so five Windows DLLs were on `libUI.so`'s
link line. **GNU ld skips a PE file without a word**, so the module compiled, the
link "succeeded" as far as UI was concerned, and 38 symbols went undefined two
targets later. Check what is actually on a link line before believing a library
is merely unlinked.

The window icon is **done**, both halves of it, and written up in
[The window icon off Windows](PORT_ROADMAP.md#the-window-icon-off-windows-and-why-there-is-no-one-answer---done):
`SDL_SetWindowIcon` with every size the file carries, which covers X11 always and
Wayland where the compositor has `xdg_toplevel_icon_v1`, plus an app id and an
installed `.desktop` file for the rest of Wayland. No image library was needed.

Fixing it turned up a separate bug worth knowing about: **`GetBaseDir` did not
work off Windows at all**, and that is why the splash screen never appeared.

[The last two Windows headers](PORT_ROADMAP.md#the-last-two-windows-headers-crtdbgh-and-shellapih)
records the `crtdbg.h` and `ShellAPI.h` work, and the more useful negative result
beside it: **the game never asks the OS for a file.** No `GetOpenFileName`, no `SHBrowseForFolder`, no `shlwapi`, no
shell path helpers, no virtual folders, anywhere in a module that is built. Saves
and replays are listed by `NFile::GetDirectoryFiles` and drawn with the engine's
own widgets, so no file-dialog replacement is needed from SDL or anywhere else.

## Standing instructions from the user, in their words

- "port as is, just modernizing and replacing non-standard functions"
- unrelated fixes go in separate commits, even small ones
- if a function has no callers anywhere, delete it rather than port it - git
  keeps it
- no `Co-Authored-By` trailer, no em-dashes, always brace conditionals, and a
  commit subject has to be true of the patch

The editor cannot be built on any platform: it needs the private `stingray`
library, which is not in the repo. Do not try, and do not treat editor code as
verifiable - it is fixed on inspection only.

**Do not copy files over `/mnt/c` into the WSL clone to test before committing.**
It leaves untracked copies that make the next fast-forward refuse, and a build
script that ignores the refusal then reports on the old commit. That happened
three times on 2026-08-25. Compile the single TU against `/mnt/c` instead, as
above.

## Picking it back up

**Nothing fails to compile.** The next milestone is a linked `Game`, and it has
now been attempted: it fails with 107 undefined symbols in the five clusters
above.

### Start here, in this order

Cheapest first, and the first three are almost certainly build wiring:

**It has been run, and it starts.** `cmake --install` now produces a tree that
executes: `6abd38679` added `$ORIGIN` to the install RPATH so the game finds its
own modules, and a `file(GET_RUNTIME_DEPENDENCIES)` rule that installs the
third-party libraries it needs, which today is `libboost_filesystem` for
`libSystem.so`. **No `LD_LIBRARY_PATH` is required any more.**

The run gets through the splash screen, opens and frees Lua stacks, and reaches
DXVK, which prints its banner and then throws `dxvk::DxvkError` **before
enumerating a single adapter**. Nothing more appears at `DXVK_LOG_LEVEL=debug`
and `Game_d3d9.log` stops after the three banner lines.

It is not a missing library: the Vulkan loader `libvulkan.so.1` is installed,
the ICDs are there, `vulkaninfo` reports llvmpipe at API 1.4.335, and both
`DISPLAY` and `WAYLAND_DISPLAY` are set under WSLg.

**The exception cannot be caught, and that is a finding rather than a gap in the
guard `200385c14` added.** A backtrace of the abort:

```
#4  __GI_abort ()
#5  ??? () at /usr/lib/x86_64-linux-gnu/libgcc_s.so.1
#6  __gxx_personality_v0 () at /usr/lib/x86_64-linux-gnu/libstdc++.so.6
#7  _Unwind_RaiseException_Phase2 () at /home/sse4/bk2/bin/libdxvk_d3d9.so.0
```

The unwinder resolves **inside `libdxvk_d3d9.so`**, not in the process's
libgcc: DXVK Native carries its own. Phase two of the unwind finds no handler
frame it can reach and calls `abort()`, so the throw dies at the library
boundary and never reaches `RunGameGuarded`. That is also why the
"terminate called after throwing an instance of 'dxvk::DxvkError'" line
disappeared once the guard went in: nothing is calling `std::terminate` any
more, the unwinder is aborting on its own.

Two consequences. The guard is still worth having, because it catches anything
the engine itself throws, but it can do nothing about DXVK. And **DXVK throwing
out of `Direct3DCreate9` at all is the bug**: a C entry point should return null.

**`SDL_WINDOW_VULKAN` was tried and is not the answer.** The theory was that
DXVK's SDL3 WSI needs SDL to have loaded the Vulkan library, which SDL does when
a window is created with that flag. Adding it to `WinFrame.cpp:509` changes
nothing: same abort, same place. The change was reverted rather than left in as
a guess.

Game data is still not installed, which is expected and separate: `CLAUDE.md`
says to copy `Versions/Current/Profiles`, `Versions/Current/Data` and
`splash.bmp` into the prefix by hand. The splash failure in the run above is
that, not a bug.

Once it draws, the next thing is the game will
stop somewhere, and `granny_calls.log` beside the executable will say which
Granny entry points were reached, in what order, and with what arguments. That
list, in that order, is the porting plan for Granny.

Expect it to stop during loading. Everything below `GrannyReadEntireFile`
returns null, so the first model to be loaded is likely where it ends.

Two things to check when it does run, both recorded as unknowns rather than
problems:

- whether the working directory is right. The game resolves everything against
  it: `RunGame` opens `../profiles/startup.cfg` and `NMainLoop::GetBaseDir` is
  the parent of the current directory.
- whether the data is where it looks. Game data is not installed by CMake; see
  the note in `CLAUDE.md` about copying `Versions/Current/Profiles`,
  `Versions/Current/Data` and `splash.bmp` into the install prefix. `libUI.so`'s rpath already shows
   `third_party/uesp-esoapps/common/granny/win64`, so this is the same shape the
   ffmpeg one turned out to be: a Windows binary on a Linux link line. `third_party/uesp-esoapps` supplies
   it; the question is whether it builds off Windows at all. Ask this one before
   the others if the answer is no, because it changes what "a linked Game" means.

**Ask of every cluster whether it is wiring before treating it as porting.**
Two of the four biggest items on 2026-08-26 turned out to be dead code, and the
five above look more like CMake than like C++.

### Still open, blocking nothing

Two findings from the dinput work, neither of which blocks anything:

- **`+cursor_x` and `+cursor_y` have no consumer.** Every shipped `input.cfg`
  binds them to `MOUSE_AXIS_X`/`Y` and no engine code reads a command by either
  name. The cursor comes from `win_mouse_move` through `WinFrame`.
- **The POV angle is a delta on Windows too.** `FillEventInfo` sets
  `nParam = dwData - dwLastValue` for every control type, and the POV branch of
  `PumpMessages` then hands that difference to `cos`/`sin` as though it were the
  angle. Pre-existing, not introduced by the port and not fixed by it.


Two `-Wenum-compare` in a 3Dmotor build, at `GfxBuffers.cpp:107`, comparing
`ETrueBufferUsage` against `EBufferUsage`, more likely a bug than noise. The
null-reference deref at `3Dmotor/GCombiner.cpp:403` in the live SIMD skinning
path is still there. The 1703 `-Wwrite-strings` warnings are gone (`b30bc55b1`).

**Read [One GCC message, several unrelated causes](PORT_ROADMAP.md#one-gcc-message-several-unrelated-causes)
before trusting any error count from a build log.** Today confirmed it twice
over: three headers accounted for 1244 of the 2370 lines the day started with,
and seven fresh errors only became visible after the cascades above them were
cleared.

The WSL clone now carries two untracked strays of its own,
`Versions/Temporary/Engine/ELK/1.upd` and `elk.xml`, which a build writes into
the source tree. They are the same two the Windows clone has, they are not at a
path any commit adds, and they do not block a fast-forward. The Windows clone
carries the strays it has carried all along; nothing is half-applied to recover.

The port notes live under `docs/port/`, which `.git/info/exclude` keeps
untracked, so nothing in this directory is ever part of a commit.
