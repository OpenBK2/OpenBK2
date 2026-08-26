# Linux port roadmap

Everything still to do. Work already applied lives in
[PORT_REPLAY.md](PORT_REPLAY.md); reference material about why the codebase
behaves as it does lives in [PORT_FINDINGS.md](PORT_FINDINGS.md).

Part of the Linux port notes; see [LINUX_PORT.md](../../LINUX_PORT.md) for the index.

Most of the small items were found while replaying commits, and none of them
block the replay. Do them **after** it lands, as tidy-up commits on `linux2`,
so they do not churn cherry-pick context.

---

## Large - needs a decision or a replacement dependency

| | item | where |
|---|---|---|
| [ ] | **Granny 3D** is proprietary and Windows-only. **Stubbed 2026-08-26, which is what made `Game` link.** The 54 entry points return nothing and log every call; the next step is reading `granny_calls.log` from a run: [Granny, and the log that says what to port](#granny-and-the-log-that-says-what-to-port). Candidate replacement: [opengr2](https://github.com/arves100/opengr2). How the engine uses it: [docs/Granny3DUsage.md](../Granny3DUsage.md) | [Hard blockers beyond tier 1](#hard-blockers-beyond-tier-1) |
| [x] | ~~**D3D9** - the guard was only ever applied to the link line; the D3D9 `.cpp` files are still in `3Dmotor`'s `SOURCES`~~ done in "add cmake/dxvk.cmake and use DXVK for D3D9 off Windows". 3Dmotor builds and links. DXVK implements no d3dx9, so `GShaderFX.cpp` became Windows-only, and its nine entry points are stubbed off Windows because the path is unreachable there too: [D3DX, and why the shader effects needed a stub](#d3dx-and-why-the-shader-effects-needed-a-stub-rather-than-a-port) | [DXVK.md](DXVK.md) |
| [x] | ~~**HWND and the message loop**, 44 files. The largest single surface~~ done for the window in "give WinFrame an SDL implementation off Windows", with `SplashScreen` and `WinCursor` beside it; System links. DXVK's `windows.h` supplies the types elsewhere | [DXVK.md](DXVK.md) |
| [x] | ~~**DirectInput.** The last platform blocker~~ Done 2026-08-26. Bindings are keyed by name, so nothing had to migrate - but the `DIK_*` codes stayed rather than becoming `SDL_Scancode`, because `kiKeyInfoList` is a two hundred line table and one copy is checkable where two are not | [DirectInput over SDL](#directinput-over-sdl---done) |
| [ ] | **AILogic transitively drags in the whole renderer.** Decoupling is surgery on module boundaries, not a CMake guard | [Why AILogic is NOT in tier 1](#why-ailogic-is-not-in-tier-1) |
| [ ] | **ffmpeg** - `cmake/ffmpeg.cmake` downloads an MSVC prebuilt zip; needs a system-libav path | [Hard blockers beyond tier 1](#hard-blockers-beyond-tier-1) |
| [x] | ~~**crashpad** - supported on Linux, needs libcurl~~ builds on Linux: `libcrashpad_client.a` and the `crashpad_handler` executable both come out of the tree, `Game/main.cpp` compiles against it, and `0d70302ff` gave it portable paths. `76d15cf24` installs the handler, which nothing had been doing on either platform | [Hard blockers beyond tier 1](#hard-blockers-beyond-tier-1) |
| [x] | ~~**versioninfo** - `.rc` files, Windows-only~~ not a blocker. `add_version_info` hands the target a generated `.rc`, which CMake only feeds to a resource compiler on Windows and ignores elsewhere, while `REVISION_NUMBER_STR` and `BUILD_DATE_TIME_STR` are defined on both. Off Windows the binary simply carries no version resource, which is the right answer for ELF | [Hard blockers beyond tier 1](#hard-blockers-beyond-tier-1) |
| [ ] | **Cross-platform multiplayer** is a harder problem than cross-platform build, and separate from it | [Cross-platform multiplayer](#cross-platform-multiplayer-is-a-separate-harder-problem) |

## Small - mechanical, just not part of the replay

| | item | where |
|---|---|---|
| [ ] | Retire the printf family. Steps 1 and 4 landed together as "format debug traces with fmt instead of va_list and _vsnprintf"; steps 2, 3 and 5 remain | [Retiring the printf family](#retiring-the-printf-family-what-is-actually-left-and-with-what) |
| [x] | ~~1703 `-Wwrite-strings` warnings~~ done in "make the autodetect card name const": 1699 of them were one `char *` field in `pciids/detect.h`, one per row of a 1700 row table. A 3Dmotor build now emits 2 warnings |  |
| [x] | ~~The `GetObject`/`C4273` items~~ overtaken: `NFile::CopyFile` turned out to be missing `SYSTEM_EXPORT`, which is what the `CopyFileA` link error actually was. The `-A` analysis stands and nothing else is broken by it | [The Windows SDK's -A macros](#the-windows-sdks--a-macros-and-why-parenthesising-does-not-defeat-them) |
| [ ] | Animated cursors are static off Windows. The six `.ani` files decode to their first frame; animation needs a timer driving `SDL_SetCursor` | [Animated cursors off Windows](#animated-cursors-off-windows) |
| [ ] | The clipboard is ANSI on Windows and UTF-8 off it, so the two disagree about what can be copied. `CF_UNICODETEXT` closes it | [The clipboard loses characters on Windows](#the-clipboard-loses-characters-on-windows) |
| [x] | ~~`SYSTEMTIME` is a field in three structures that `IBinSaver` writes, so replacing it changes a stored layout~~ done in "replace SYSTEMTIME in the savegame and replay lists with a portable struct" (`a3cf61a8f`). The layout was kept: those structures do reach a savegame | [SYSTEMTIME inside serialised structures](#systemtime-inside-serialised-structures---done-and-what-it-settled) |
| [ ] | 7 `-Wformat-security` sites, all original code. `AILogic/MPLog.h` also calls unbounded `vsprintf` into a 2048-byte static | [-Wformat-security](#-wformat-security-7-sites-all-original-plus-two-other-bugs-found-beside-them) |
| [ ] | `%I64u` in `AILogicInternal.cpp:1203` and `Shell.cpp:226` - MSVC-only, GCC rejects it. Both sit in the ASYNC diagnostics | [-Wformat-security](#-wformat-security-7-sites-all-original-plus-two-other-bugs-found-beside-them) |
| [ ] | `volatile bool` used as a cross-thread synchronisation primitive (`bIsFileReading`, `bIsReady`, `bExit`, `bActive`). Wants `std::atomic<bool>` | [Threading primitives](#threading-primitives-worth-replacing) |
| [ ] | `ReleaseFileRequestHolder` can free a request the loader thread is about to read. It clears `holdRequests` when the queue is empty, but the loader pops before it reads, so the last request is unreferenced while still in use | [The resource loader's holder race](#the-resource-loaders-holder-race) |
| [ ] | `WinFrame`'s `msgList` is single-producer single-consumer by construction | [Threading primitives](#threading-primitives-worth-replacing) |
| [ ] | `Sound/SoundEngine.cpp` declares `critSection` and never locks it. Dead; delete it | [Threading primitives](#threading-primitives-worth-replacing) |
| [ ] | Windows character typedefs after commit 37 removed `WCHAR`: `TCHAR` 103 uses, `LPCSTR` 24, `CHAR` 21, `LPSTR` 9. `TCHAR` is the awkward one, it means different things under `UNICODE` | [Windows types in code added after the type sweep](#windows-types-in-code-added-after-the-type-sweep) |
| [ ] | Paths built with a raw backslash separator: 152 string literals in 75 files, though 116 of those are editor or otherwise unbuilt and some of the rest are registry keys rather than paths. Off Windows a backslash is an ordinary filename character, so `"foo\\bar"` becomes one directory named `foo\bar` | [Path separators](#path-separators-are-built-into-string-literals) |
| [ ] | A resource that fails to load crashes rather than degrading: `CGrannyMeshLoader::Recalc` guards the loader but not the file inside it, so a missing, truncated or corrupt `.gr2` reaches Granny as a null pointer | [A failed resource load is a crash](#a-failed-resource-load-is-a-crash) |
| [ ] | The enum string conversions are generated by hand-rolled codegen. Wants `boost::describe` | [Enum conversions](#the-generated-enum-conversions-want-boostdescribe) |
| [ ] | `CVariant` hand-manages a union of raw pointers. Wants `std::variant`; three bugs found in it so far | [CVariant](#cvariant-should-be-a-stdvariant) |
| [ ] | `EXTERNVAR` is still `__declspec(dllimport) extern`, and nobody owns fixing it | [`__declspec` inventory](#__declspec-inventory-and-the-one-nobody-fixes) |
| [ ] | 83 HRESULTs tested with `== D3D_OK` / `== S_OK` rather than `SUCCEEDED`, so a non-zero success reads as failure. 79 in `3Dmotor`, 4 in `ShaderCompiler` | [Testing an HRESULT](#testing-an-hresult-against-s_ok-is-not-the-com-idiom) |
| [x] | ~~`OutputDebugString` called directly from 46 places in nine modules~~ done in "route every debug trace through the one function that knows the platform"; ShaderCompiler (3) and dbstruct (2) left, as unbuilt |  |
| [ ] | `__rdtsc` has no non-x86 fallback, and on x86 it should be `__rdtscp` where CPUID says so. Deferred: portability first | [`__rdtsc` has no non-x86 fallback](#__rdtsc-has-no-non-x86-fallback) |
| [ ] | `DWORD`/`WORD`/`BYTE` keep reappearing in new code written after the type sweep. Re-run the check after every rebase | [Windows types in code added after the type sweep](#windows-types-in-code-added-after-the-type-sweep) |
| [x] | ~~No compiler cache wired~~ done in "route compiles through ccache". Full x64 Release rebuild with objects deleted 681 s -> 194 s, 99.9% hit rate, PCH kept. What is left is linking, which a compiler cache does not touch |  |
| [ ] | `cmake/versioninfo.cmake` sets `BUILD_DATE_TIME` at line 10 but reads `${BUILD_DATETIME}` at line 47. The names do not match, so `BUILD_DATE_TIME_STR` compiles as `""` and no binary carries a build date, CI included. One-line fix, but note it would make CI's per-run timestamp bust the ccache for every target using `add_version_info` |  |
| [ ] | The benchmark harness ignores its own `/arch:` flag, so the AVX and AVX2 variants are really SSE2 | [The benchmark harness](#the-benchmark-harness-ignores-its-own-arch-flag) |
| [ ] | `dbcodegen/main.cpp` and `dbstruct/main.cpp` include `System/CmdLine.h`, which row 52 deleted. Neither has CMake wiring, so nothing reports it; `boost::program_options` is the replacement when either is revived | [Commit 52](PORT_REPLAY.md#commit-52-deletes-a-parser-two-unbuilt-utilities-still-call) |
| [ ] | **A pointer through an `int`** at `AILogic/Scripts.cpp:2943`, truncating to 32 bits. Broken on Windows x64 too, and the only instance in the tree. Not fixed because nothing calls `ReturnScriptIDs` - it appears in no shipped pak - so it cannot be reached | [A pointer through an int](#a-pointer-through-an-int-in-cscriptsreturnscriptids) |
| [ ] | **Reading the shipped data on a case sensitive filesystem.** The engine's own paths are fixed, but the data carries backslashes and its own casing - `startup.cfg` says `exec .\profiles\consts.cfg` - and this project does not author the data. Normalise on the way in, convert the data once, or mount it case insensitively | [Reading the shipped data](#reading-the-shipped-data-on-a-case-sensitive-filesystem) |
| [ ] | **No pinned ISA baseline off Windows.** `cmake/arch.cmake` and `AILogic`'s `/fp:strict` are both inside `if(MSVC)`, so no `-march` is passed and `-DARCHITECTURE=` does nothing. Nothing is wrong today, since the unflagged baseline has no FMA to contract into, but the ISA then comes from whatever the distribution configured GCC with | [The build has no pinned ISA baseline](#the-build-has-no-pinned-isa-baseline-off-windows-and-that-is-a-determinism-item) |
| [ ] | Two dead OS version checks left after row 56: `Game/SysKeys.cpp:66` and `FontGen/FontGen.cpp:294`. The third, `Input/Input.cpp:310`, went with `619b571b9`. The fourth, `GameX/FontInfo.cpp:117`, went with `6472aa3b8`. All test for Windows 9x or 2000. `GetVersionEx` is also deprecated and reports 6.2 on Windows 8.1 and later without a compatibility manifest, so three of them do not measure what they read as measuring | [Commit 56](PORT_REPLAY.md#commit-56-the-fallback-is-not-dead-code-after-all) |
| [ ] | `Common_RTS_AI/ChecksumSaver.cpp:143` and `:197` call `breakpoint()` bare, on a checksum mismatch. With no debugger attached that terminates the process on every platform, so an ASYNC diagnostic kills the client instead of logging. They look like they want `breakpoint_if_debugging()`, but that changes when the game dies during a desync, so it is a decision rather than a fix | [Commit 48](PORT_REPLAY.md#commits-48-and-49-are-one-commit-they-are-halves-of-the-same-paper) |
| [ ] | Every file size in the mapping and archive path is `int`: `CMMFile::GetFileSize`, `CZipFile::nTotalSize`, `CDataStream`'s four pointers' arithmetic. An archive of 2 GB or more overflows and the failure is silent. `data.pak` is already ~954 MB in a full install, so this is one repack away, not hypothetical | [Commit 56](PORT_REPLAY.md#commit-56-the-fallback-is-not-dead-code-after-all) |
| [ ] | Scratch files still parked on `linux2-strays` | [Scratch files parked on `linux2-strays`](#scratch-files-parked-on-linux2-strays) |
| [ ] | `ED_B2_M1` (4) and `MapEditorLib` (2) still have bare `min`/`max`. Only matters if `-DBUILD_EDITOR=ON` is ever turned on | [PORT_REPLAY.md](PORT_REPLAY.md#closing-the-sceneb2-gap-not-a-replayed-commit) |
| [ ] | Five editor call sites still reference the GUID helpers commit 32 deleted | [PORT_REPLAY.md](PORT_REPLAY.md#commit-32-guid-to-boostuuid-the-byte-order-has-to-be-preserved) |
| [ ] | Assert-condition rot, e.g. a bare `string::npos` in an assert condition at `Game/main.cpp:279` | [PORT_FINDINGS.md](PORT_FINDINGS.md#checking-the-code-no-build-compiles) |
| [ ] | `AILogicInternal.cpp:1048` guards the multiplayer zero-seed reset with `!GetGameType() == EGT_SINGLE`, which parses as `(!GetGameType()) == EGT_SINGLE`. It is correct only because `EGT_SINGLE` is 0; reordering that enum silently inverts which games reset their RNG seed | [Commit 50](PORT_REPLAY.md#commit-50-the-generator-was-never-the-problem-the-seeding-was) |
| [ ] | 967 live `StrFmt` calls remain in targets no build compiles | [PORT_REPLAY.md](PORT_REPLAY.md#commit-25-what-the-format-string-audit-found) |
| [x] | ~~16 dependent types still written without `typename`, all of the form `std::vector<T>::const_iterator`~~ done in "add the typename a dependent type name needs when the scope is a template-id"; the widened rerun swapped one entry for another: `AILogic/ListsSet.h` was missing from the list, and `3DLib/Ring.h` turned out to need no `typename`, so the count stayed at 16 | [Two-phase lookup](#two-phase-lookup-the-sweeps-are-not-finished) |
| [x] | ~~41 dependent member typedefs used as a scope without `typename`~~ done in "add the typename a dependent member typedef needs when it is used as a scope"; a third shape, found by GCC once passes 1 and 2 stopped masking the files it lives in | [Pass 3](#pass-3-of-the-typename-sweep-a-dependent-member-typedef-used-as-a-scope) |
| [x] | ~~`?:` between a wrapper and the type it converts to, which GCC rejects because neither conversion is preferable~~ ten sites done in "give each ambiguous ?: a single operand type". Left open as a *class*: it cannot be scanned for, only compiled for, so more will surface | [`?:` between two mutually convertible types](#-between-two-mutually-convertible-types) |
| [ ] | `SAIAngle` declares `operator SAIAngle()`, a conversion to its own type, which can never be called. One line to delete, unrelated to the `?:` work | [`SAIAngle` converts to itself](#saiangle-converts-to-itself) |
| [x] | ~~18 sites naming an enum through `enum X` where nothing declared it, which only MSVC invents~~ done in "declare the enums that are only ever named through an elaborated specifier". Compile-to-find, not grep-to-find: the same spelling is legal at ~180 other sites | [`enum X` naming an enum nothing has declared](#enum-x-naming-an-enum-nothing-has-declared) |
| [x] | ~~`3Dmotor/Cache.h` writes `typename typedef` on three lines~~ done in "put the typename in the right place in Cache.h's three typedefs". Each of the three needed a *different* correction, and the two broken typedefs were cascading into 64 phantom sites across two other classes | [One GCC message, several unrelated causes](#one-gcc-message-several-unrelated-causes) |
| [ ] | **`3Dmotor/GLightPerVertexDispatch.cpp` wants `<intrin.h>`** only for `__cpuid`, `__cpuidex` and `_xgetbv`, choosing between the SSE2/AVX/AVX2 lighting kernels. GCC has `<cpuid.h>` equivalents. 73 lines, no design decision - **the smallest item left in the tree** | [Where the compile actually stops](#where-the-compile-actually-stops-2-translation-units-one-blocker) |
| [ ] | **`Image/ImageDDS.cpp` splits at line 340**: the DDS reader above is already portable via the standalone `Image/DDS.h`, the writer below genuinely calls D3D9 and D3DX and is reached only from the editor's TextureExporter. Guard the writer off Windows; no rewrite needed | [Where the compile actually stops](#where-the-compile-actually-stops-2-translation-units-one-blocker) |
| [ ] | `Parser/lang.tab.c` and `lex.yy.c` are committed bison/flex output and the build compiles them directly, so editing `lang.y` or `lang.l` alone does nothing. Measured: bison 3.8.2 and flex 2.6.4 regenerate both cleanly with identical token numbering, but wiring the tools in buys nothing - the grammar has changed twice in twenty years and its consumer `dbcodegen` is not built | [The bison/flex parser](#the-bisonflex-parser-what-it-is-and-why-it-is-not-regenerated) |
| [ ] | ~122 files include `<zconf.h>` without needing it, inherited from the original. Harmless under `ZLIB_COMPAT`; removing them needs a build per file to prove nothing leans on the transitive include | [zconf.h is not a porting bug](#zconfh-is-not-a-porting-bug) |
| [ ] | 19 `COLORREF` uses left, all in MFC targets that cannot build off Windows. A `uint32_t` swap is **not** safe there: `COLORREF` is `unsigned long`, `uint32_t` is `unsigned int`, and `MapEditor/SortTreeControl` passes `COLORREF *`. Cosmetic, not a portability blocker | [The Windows SDK's `-A` macros](#the-windows-sdks--a-macros-and-why-parenthesising-does-not-defeat-them) |
| [ ] | `System/DB.h:155` carries a comment claiming `(GetObject)` avoids the Windows SDK's `#define GetObject GetObjectA`. It does not - these are *object-like* macros, so parentheses change nothing; measured against cl.exe. The comment misinforms and should go either way | [The Windows SDK's `-A` macros](#the-windows-sdks--a-macros-and-why-parenthesising-does-not-defeat-them) |
| [ ] | `NDb::GetObject` is declared twice with inconsistent dllexport, `System/DB.h:155` with `SYSTEM_EXPORT` and `libdb/Db.h:28` with none, which is the `C4273` in every libdb build. One line | [The Windows SDK's `-A` macros](#the-windows-sdks--a-macros-and-why-parenthesising-does-not-defeat-them) |
| [ ] | Only `Game.exe` gets `utf8.manifest`, so only it has a UTF-8 ANSI code page. `B2_MapEditor` and the editor manifests set no `activeCodePage`, leaving them on the legacy page and unable to open non-ASCII paths | [Everything resolves to the `-A` entry points](#everything-resolves-to-the--a-entry-points) |
| [x] | ~~`libdb/DBWatcherClient.cpp` `#import`s a binary `.tlb`~~ done in "build the XDBWatcher COM client only where #import exists"; stray-byte errors went 1149 to 0. Originally:; GCC reads `#import` as its own deprecated include-once directive and lexes the type library as source, which is 1149 error lines from one file. Investigated: the COM service has never worked on any machine and the TU is dead in `Game.exe`. Fix agreed, not applied - build the TU only `if(MSVC)` | [`#import` of a type library](#import-of-a-type-library-what-xdbwatcherclient-actually-is) |
| [x] | ~~`libdb/RenameNode.cpp:99` and `:313`~~ done in "check the watcher singleton in RenameNode before calling through it". Originally: called `Singleton<IDBWatcherClient>()->` without the guard the other two call sites use, and `NSingleton::Singleton` returns 0 when unregistered. Editor-only path, so unreachable today, but a real null dereference | [`#import` of a type library](#import-of-a-type-library-what-xdbwatcherclient-actually-is) |
| [ ] | `3Dmotor/Cache.h` writes `typename typedef` on three lines, which is ill-formed and only MSVC takes. Fixing them uncovers a dependent-base lookup problem in the same file, so it is one commit, not three lines | [`typename typedef` in 3Dmotor/Cache.h](#typename-typedef-in-3dmotorcacheh) |
| [ ] | 20 explicit template specializations still at class scope, which only MSVC allows. `libdb/ObjMan.h`'s 16 are done in "move IObjMan's SetValue and GetValue specializations to namespace scope"; the rest are editor or unwired, so nothing reports them. The old count of 39 was three too high | [The remaining 20](#the-remaining-20-class-scope-explicit-specializations) |
| [ ] | `ED_B2_M1/MapInfoState.cpp:876` calls `IEditorScene::AddObject` with six arguments where seven are required, passing `false` for the `ESceneObjAnimMode`. It has never been compiled by anything and the missing arguments would have to be guessed, so `39e575fd9` left it while fixing the two real ones in `B2_M1_World` | none |
| [x] | ~~The GDI font surface in `GameX/Font*.cpp`, 139 error lines and the largest cluster left in a Linux build~~ done in "delete GameX's copy of the font generator, which nothing calls" (`6472aa3b8`). It was dead code: the game loads prebuilt atlases and never rasterises a glyph, so no font library is needed | [Fonts: the game does not rasterise glyphs](#fonts-the-game-does-not-rasterise-glyphs-on-any-platform) |
| [x] | ~~Scintilla fails off Windows, 31 error lines in 16 objects~~ done in "build Scintilla only with the editor, which is all that links it" (`055b4f3f4`). It was being compiled unconditionally for consumers that all live behind `BUILD_EDITOR`. Replacing the vendored copy with a current Scintilla is a separate job | none |
| [x] | ~~`crtdbg.h` in `Game/main.cpp` and `ShellAPI.h` in `GameX/MPLANTest.cpp`~~ both done, in `6c73c6ad4` and `745f2a4bc`. The first uncovered seven Win32 calls further down `main.cpp`, of which `SetErrorMode` and `GetCurrentDirectory` are also done | [The last two Windows headers](#the-last-two-windows-headers-crtdbgh-and-shellapih) |
| [ ] | `port/socket.h:23` includes **winsock.h**, WinSock 1. Anything that needs Boost.Asio needs winsock2.h, and the two cannot be in one translation unit: MSVC stops with "WinSock.h has already been included". That is what turned back the first attempt at replacing `ShellExecute` with Boost.Process, and it will turn back every other Asio-dependent Boost library the same way, Boost.Process, Boost.Beast and Boost.Redis among them. Moving the shim to winsock2 unblocks all of them at once, but it is the network layer of a lockstep game, so it wants its own commit and its own testing | [The last two Windows headers](#the-last-two-windows-headers-crtdbgh-and-shellapih) |
| [ ] | `AILogic/CMakeLists.txt` adds `/fp:strict` under `if(MSVC)` and nothing for any other compiler, so on GCC the simulation is built with `-ffp-contract=fast`, which fuses `a*b+c` into one rounding instead of two. Latent, not live: `cmake/arch.cmake` also only acts under `if(MSVC)`, so the Linux build is baseline x86-64 with no FMA to fuse into. It becomes real the moment anyone passes `-march=`. `-ffp-contract=off` is the piece of `/fp:strict` that matters | [Floating-point control](#floating-point-control---better-news-than-it-looks-and-now-done) |

---

## Reaching a Linux build

### Tier 1 - the only subset reachable without a renderer - overtaken

This was the staging plan while the renderer was a blocker. It is no longer how
the tree stands: as of 2026-08-26 the renderer is not a blocker, the whole tree
compiles, twenty-three modules link and what is left is linking `Game`. Kept as
the record of the original plan.

```
MemoryLib -> Misc -> System -> Parser -> libdb
Net -> Server_Client_Common -> Client
Script, 3DLib, DebugTools, Image
```

12 of 28 modules. External deps: zlib-ng, boost, fmt. This covers `IBinSaver` serialization,
the DB layer and the netcode - the same ground `netcode_bugfixes` touches. All twelve link
today, along with 3Dmotor, Sound, B2_M1_Terrain and Common_RTS_AI.

### Why AILogic is NOT in tier 1

`AILogic` links `Stats_B2_M1` and `B2_M1_World`, and:

- `Stats_B2_M1` -> `3Dmotor`, `Sound`, `UI`
- `B2_M1_World` -> `3Dmotor`, `SceneB2`, `Main`, `Sound`
- `B2_M1_Terrain` -> `3Dmotor`, `Sound`

So the simulation module transitively drags in the whole renderer. Decoupling that is real
surgery on module boundaries, not a CMake guard. Treat it as a separate project.

### Hard blockers beyond tier 1

- **Granny 3D** (`cmake/granny.cmake`) - Windows-only proprietary `.lib`/`.dll`. The old
  `linux` branch still links `granny::granny` unconditionally in `3Dmotor`. Possible
  replacement: https://github.com/arves100/opengr2
- **D3D9** - `cmake/dxsdk.cmake` is `if(WIN32)`-guarded on the `linux` branch, but the D3D9
  `.cpp` files are still in `3Dmotor`'s `SOURCES`. The guard was only applied to the link line.
- **`cmake/ffmpeg.cmake`** - downloads an MSVC prebuilt zip. Needs a system-libav path on Linux.
- **`cmake/crashpad.cmake`** - Sentry crashpad does support Linux, needs libcurl.
- **`cmake/versioninfo.cmake`** - `.rc` files, Windows-only.
- **`cmake/arch.cmake`** - MSVC-only `/arch:` flags; a no-op elsewhere, harmless.

None of these existed on the `linux` branch's top-level `CMakeLists.txt`; they were added to
`port` afterwards, so they are new work.

### The long tail - Windows API surface still to migrate

**Historical.** Measured on `netcode_bugfixes`, before any of it was done, and
kept as the original inventory. **None of it is left as of 2026-08-26**: every
row here now compiles, or was found to be unreachable code. The FP control word
row, marked determinism-critical below, was closed in `11a8b8557`; the
DirectInput row in `40101f80c`. Granny, in the line below the table, is not on
this list and is now the largest of the five link clusters.

File counts across `Engine/Sources` at the time:

| category | files | notes |
|---|---|---|
| HWND / message loop | 44 | the big one; window + input plumbing |
| timing (QPC / timeGetTime / GetTickCount) | 38 | commit #23 covers part of this |
| D3D9 / D3DX | 15 | DXVK-native or a new backend |
| Win32 file API | 12 | `std::filesystem` covers most |
| Win32 threads / sync | 11 | `std::thread` / `std::mutex`; commit #34 starts it |
| FP control word | 8 | **see below - determinism-critical** |
| COM | 4 | |
| LoadLibrary / GetProcAddress | 4 | `dlopen`/`dlsym` |
| registry | 3 | config storage |
| DirectInput | 3 | SDL3 already vendored |
| inline `__asm` | 2 | |
| MMX/SSE intrinsics | 1 | mostly already ported in `3Dmotor` |

Plus Granny (proprietary, Windows-only) for model loading.

### A pointer through an int in CScripts::ReturnScriptIDs

Found 2026-08-26, not fixed, because nothing can reach it. Recorded so that the
next person to read the line does not have to work out whether it matters.

`AILogic/Scripts.cpp:2943`:

```cpp
const int nPtr = script.GetObject( i );
CObjectBase *pObj = reinterpret_cast<CObjectBase*>( nPtr );
```

`Script::Object::operator int()` is `GetInteger()`, so a pointer that arrives as
a Lua number is truncated to 32 bits and the `dynamic_cast` on the next line
runs on the remains. **This is broken on Windows x64 as well**, not only off
Windows: it is a 32 bit era assumption rather than a platform one, and it can
only ever have worked in the x86 build.

The type that would have carried it was already to hand. A Lua number is a
`double`, `Object::GetNumber()` returns one, and a double holds 53 bits exactly,
which is enough for every pointer any of these platforms hands out.

**Why it is not fixed:**

- **It is the only one.** A sweep of every built module for a pointer cast to or
  from `int` returns this single line. Everything else already uses a type that
  is wide enough. It is a defect, not a class.
- **Nothing calls it.** `ReturnScriptIDs` is reachable only from Lua, and the
  string appears in no shipped `.pak` - not in the 1.5 GB `data.pak`, not in any
  of the five patches. No push side that would put a `CObjectBase *` into a Lua
  number could be found either, so the mechanism looks half built rather than
  used.

So it cannot be a current crash, with this data or any data that ships.

**If it is ever touched, deleting it is probably the answer** rather than
widening the type to `intptr_t`. Widening preserves a mechanism with no caller
in code, no reference in data, and no working counterpart, which is the case the
standing instruction covers: if a function has no callers anywhere, delete it
rather than port it. That is the call already made for the font generator and
the D3DX shader effects. Checking `origin/main` would settle whether the push
side ever existed.

### Reading the shipped data on a case sensitive filesystem

Open, and the largest thing between the port and a running game. Raised
2026-08-26 after the code side of it was cleared.

The engine's own paths are fixed: `NFile::PATH_SEPARATOR`, `DIR_DATA`,
`DIR_PROFILES`, `DIR_MODS` and `JoinPath` give it one spelling and one
separator. **The data has neither**, and this project does not author the data.

Two ways it bites, both seen:

- **Separators.** `Profiles/startup.cfg` says `exec .\profiles\consts.cfg`.
  A backslash is an ordinary filename character off Windows, so the whole thing
  is one name that does not exist.
- **Case.** The same line says `profiles` while `Versions/Current` ships
  `Profiles`. Retail Fall of the Reich ships `data`, `profiles` and `mods` in
  lower case, so the two distributions do not even agree with each other. On
  Windows all of it worked and none of it was visible.

And it is not only the configs. Map, mod and campaign data reference paths the
same way, and a `.xdb` written by the editor will carry whatever the author's
filesystem gave it.

**Three ways to answer it, none free:**

1. **Normalise on the way in.** Every path the engine accepts from data gets
   separators folded and is resolved case insensitively against the real
   directory. Robust against data nobody controls, and it is the only option
   that works on data already published. It is also the rule
   `System/FilePath.cpp` deliberately rejected for `CreatePath`, on the grounds
   that rewriting separators "would hide those callers rather than fix them" -
   which is right for a caller in this repository and wrong for a string that
   arrived from a file.
2. **Convert the data once**, at install or in a tool.
   `scripts/port/patch_cfg.py` is the throwaway version of this and rewrote four
   configs. It cannot reach inside `.pak` archives, and it makes every
   third-party mod a thing that has to be converted before it will load.
3. **A case insensitive view of the data directory**, from the packaging rather
   than the engine: a `ciopfs` mount, or a normalising overlay. No code changes,
   and it moves the problem onto whoever installs the game.

**Note the asymmetry that decides it.** The engine's own paths are a closed set
this repository can fix once. The data's are an open set: a map published in
2007 cannot be recompiled, and a mod published tomorrow will be authored on
Windows by someone who will never test it anywhere else. So whatever is chosen
has to make unmodified, unmodifiable data work.

Option 1 restricted to the *data* boundary - the VFS, the config reader, the
`.xdb` loader - and not applied to paths this repository builds itself, is
probably the answer. That keeps the FilePath.cpp reasoning intact: the callers
inside the tree are still fixed rather than hidden, and only what arrives from
outside is forgiven.

### The build has no pinned ISA baseline off Windows, and that is a determinism item

Parked 2026-08-26, deliberately not implemented. Written down because it is
invisible until it bites and easy to lose across a machine handover.

`cmake/arch.cmake` puts every `/arch:` flag inside `if(MSVC)`, and
`AILogic/CMakeLists.txt` does the same with `/fp:strict`. Off Windows that means
**no architecture flag is passed at all**, and `-DARCHITECTURE=AVX2` silently
does nothing.

The trap is not that the build follows the machine it runs on: nothing passes
`-march=native`. It is that with no `-march` at all, GCC uses **the default its
distribution configured it with**, and those have been drifting upward - RHEL 9
builds for `x86-64-v2`, and there is steady pressure toward `v3` elsewhere. Same
source, same compiler version, two distributions, different baseline ISA. For a
lockstep simulation that has to agree with an MSVC build bit for bit, that is
worse than a machine-dependent default would be, because it is stable enough to
look correct until somebody else builds it.

Measured on 2026-08-26, nothing is wrong yet. The Linux build passes no `-march`,
so the baseline is plain x86-64 with SSE2 and no FMA in the instruction set, and
`objdump` finds zero `vfmadd` or `vfmsub` in `libAILogic.so`. GCC's default
`-ffp-contract=fast` has nothing to contract into. It is a loaded gun rather than
a fired one.

What the wiring will want, when it is time:

- **`-march=x86-64`** pinned in a non-MSVC branch, which is the baseline MSVC
  targets on x64. An explicit `-march=x86-64-v2` is equally defensible as long as
  it is chosen rather than inherited.
- **`-ffp-contract=off`**, which is the one GCC default that actually diverges
  from `/fp:strict` once FMA becomes reachable. Clang also wants
  `-ffp-model=strict`.
- **`-mtune=` left alone.** It changes scheduling, not results, so it can stay
  generic without costing determinism.

Note that clang-cl would pick `/fp:strict` up on its own, since CMake sets `MSVC`
true for it. Linux clang would not.

### Floating-point control - better news than it looks, and now done

Row 47 converted everything with a standard equivalent: the helper classes in
`Misc/Win32Helper.h`, the three `_RC_CHOP` database sites, `GroupLogic.cpp`'s `_RC_NEAR` and the
three rounding assertions. **What is left is exception masking**, in two files:

- `AILogic/AILogicInternal.cpp` (2 sites) and `GameX/MPTransceiver.cpp` (2 sites) - mask all FP
  exceptions at sim init, and on x86 set `_PC_24` in the same call. `feholdexcept` masks all of
  them portably, but it clears the status flags and is built to pair with `feupdateenv`; nothing
  standard masks selectively or unmasks at all. So these want a small platform helper:
  `_controlfp` on Windows, `fedisableexcept` on glibc, nothing at all where the default already
  has them masked. The `_PC_24` half has no portable equivalent and has to stay behind a
  Windows-x86 guard.

  Worth being clear about what this is for: masking decides whether arithmetic **traps**, not
  what it **computes**. An unmasked divide-by-zero raises; a masked one yields `Inf`, and both
  produce the same bits when nothing traps. These calls therefore protect against a client
  crashing where its peers did not, which is a divergence in control flow, not in the numbers.
  All-masked is already the default on every target here; the calls exist because D3D9 and some
  drivers were known to unmask behind the process's back.

**The codebase already has an x64 path.** Every one of these sites is `#ifdef _M_AMD64`, and the
x64 branch drops `_PC_24` because x87 precision control is meaningless under SSE2. That is also a
standing x86/x64 divergence in code that is required to stay in sync: the two builds parse the
database with different precision. That x64 branch is the model for Linux, and everything in it
maps to something portable:

| Windows | portable equivalent |
|---|---|
| `_MCW_RC` (`_RC_CHOP`, `_RC_NEAR`) | `fesetround(FE_TOWARDZERO / FE_TONEAREST)` - `<cfenv>`, standard |
| `_MCW_EM` (exception masks) | `fedisableexcept()` (glibc), or just the default all-masked state |
| `_MCW_DN` (denormals) | `_MM_SET_FLUSH_ZERO_MODE` / `_MM_SET_DENORMALS_ZERO_MODE` |
| `_PC_24` (x87 precision) | **no equivalent, and none needed** if targeting x64 |

So a portable `CControl87Guard` is a contained job - provided the Linux target is x64. On 32-bit
Linux gcc defaults to x87 math and `-mfpmath=sse -msse2` would have to be forced, which is a good
reason not to bother with x86 Linux at all.

#### Done, in `11a8b8557`, and what was measured

`NWin32Helper::MaskAllFloatingPointExceptions` replaces all four sites. Three
arms: Windows x86 keeps the whole-control-word write with `_PC_24`, Windows x64
keeps the `_MCW_EM` call, and everything else gets
`fedisableexcept( FE_ALL_EXCEPT )`.

Two things were measured rather than argued, because this is lockstep code:

**Masking does not change what arithmetic computes.** Over inexact, divide by
zero, invalid, overflow and underflow, every result bit was identical before and
after `fedisableexcept( FE_ALL_EXCEPT )`, and identical again after an
enable/disable round trip. `fedisableexcept` moves the mask bits of MXCSR and of
the x87 control word and leaves the rounding, precision and denormal fields
alone; dumped both registers to confirm. glibc starts with everything masked
already, so off Windows the call is a no-op unless something unmasked them.

**Windows machine code is unchanged.** Disassembling the two objects before and
after:

| function | instructions | differing |
|---|---|---|
| `CAILogic::Init` | 464 | 0 |
| `CMPTransceiver::DoSegments` | 119 | 0 |
| `CAILogic::Segment` | 368 | 1 |

The one is `mov edx,534h` becoming `mov edx,526h`: the `__LINE__` that
`RecordRandomCall` hands `NRandom::RecordCall`, shifted by the fourteen lines the
change removed. That value goes into a 512 entry ring buffer only `DumpRecords`
reads. It is not in the checksum, does not go on the wire, and does not touch the
generator state.

#### Still open beside it: AILogic gets /fp:strict on MSVC and nothing on GCC

`AILogic/CMakeLists.txt` adds `/fp:strict` under `if(MSVC)` and sets nothing for
any other compiler. GCC's default is `-ffp-contract=fast`, which lets it fuse
`a*b+c` into a single FMA with one rounding instead of two.

Not biting today: `cmake/arch.cmake` only acts under `if(MSVC)`, so the Linux
build is baseline x86-64 with no FMA to contract into, confirmed by `__FMA__`
being undefined. It becomes real the moment anyone passes `-march=` or teaches
`arch.cmake` about GCC. `-ffp-contract=off` on that target is the cheap insurance
and is the piece of `/fp:strict` that matters here; `-frounding-math` is not
needed, since AILogic never changes the rounding mode away from nearest.

### Cross-platform multiplayer is a *separate*, harder problem

Same-platform determinism (Linux client vs Linux client) is achievable with the above. Windows
client vs Linux client in the same lockstep match is not, without more work: MSVC's CRT and glibc
implement transcendental functions differently, and none of them are correctly rounded, so
identical inputs give different last-bit results and the sim desyncs.

Call sites in the simulation (`AILogic` + `Common_RTS_AI`):

- **at risk**: `exp` 9, `tan` 4, `atan2` 4, `log` 1 - **18 total**
- safe: `sqrt` 10 and `fmod` 2 are correctly rounded per IEEE-754 and agree across libms

18 call sites is small enough to replace with own implementations if cross-platform MP ever
becomes a goal. Treat it as a later milestone, not a blocker.

---

## Detail for the small items

### SYSTEMTIME inside serialised structures - done, and what it settled

**Done** on 2026-08-26 in `a3cf61a8f`. The open question below was "do these ever
reach a file". The answer is yes, so the layout was kept rather than the type
changed. That part is worth remembering.

Three structures held a `SYSTEMTIME` and handed it to `IBinSaver`:

| where | chunk |
|---|---|
| `GameX/SaveLoadHelper.h`, `SSavegameEntry::time` | 6 |
| `GameX/SaveLoadHelper.h`, `SReplayInfo::timeFile` | 3 |
| `GameX/InterfaceReplaySaveLoad.h`, `SReplayEntry::time` | 3 |

#### The rule this established

`IBinSaver::Add` picks its path by `sizeof( (*p) & (*this) )`. A type with an
`operator&` taking `IBinSaver&` gives `int`, four bytes, and is serialised member
by member. Anything else falls to the global
`template<class T> char operator&( T &, IBinSaver & )` at `BinSaver.h:536`, gives
one byte, and `CallObjectSerialize`'s `SInt2Type<1>` overload writes **`sizeof(T)`
raw bytes** through `DataChunk`.

So a plain struct with no `operator&` of its own is written as a byte image, and
its layout is an on-disk format even though nothing in the source says so.

`CInterfaceSaveLoadMenu` and `CInterfaceReplaySaveLoad` are both
`REGISTER_SAVELOAD_CLASS`ed and serialise their entry lists, chunk 5 and chunk 3.
`MainLoopInternal.cpp:183` writes the whole interface stack with
`saver.Add( 3, &interfaces )`. So these bytes can land in a savegame.

`DoSaveGame` does queue `ML_COMMAND_PREVIOUS_MENU` before `ML_COMMAND_SAVE_GAME`,
popping the menu first, so in practice the save screen is probably not on the
stack when the file is written. "Probably" was not worth betting a save format
on.

#### What replaced it

`port/time.h` gets `SSystemTime`, the same eight `uint16_t` in the same order,
with `static_assert( sizeof( SSystemTime ) == 16 )`. Beside it,
`GetLocalSystemTime` in two forms, and an `operator<` that skips `wDayOfWeek`
because that field is derived and would sort Sunday ahead of Monday inside a day.

`GetFileAttributesEx` / `FileTimeToLocalFileTime` / `FileTimeToSystemTime` became
`NFile::GetLastWriteTime` plus one conversion, and `SystemTimeToFileTime` with its
`ULARGE_INTEGER` cast became that `operator<`.

**Before changing the type of any member of a struct that reaches `IBinSaver`
without its own `operator&`, check whether the enclosing object is
`REGISTER_SAVELOAD_CLASS`ed.** The chunk id is not the only thing that is stable.

`Misc/Time64.h`'s `GetSystemTimeAsFileTime` went the same day in `d2b87b8d2`, to
`steady_clock`. `GameX/MultiplayerTestNet.cpp:223` still calls `GetSystemTime`,
but that translation unit compiles clean on Linux today, so whatever it fills is
no longer a `SYSTEMTIME`; it is not an open item.

### Fonts: the game does not rasterise glyphs, on any platform

Investigated 2026-08-26, when the GDI surface in `GameX/Font*.cpp` was the largest
remaining cluster in a Linux build, at 139 error lines. It turned out to be dead
code, deleted in `6472aa3b8`, and no font library is needed to run the game off
Windows.

#### What made it look like a rendering problem

`GameX/FontChars.cpp`, `FontData.cpp` and `FontInfo.cpp` are pure GDI:
`CreateFont`, `CreateCompatibleDC`, `CreateDIBSection`, `TextOut`,
`GetTextMetrics`, `GetCharABCWidths`, `GetKerningPairs`, `EnumFontFamiliesEx`.
They are in `GameX`'s `SOURCES`, so they compile with the game.

Nothing calls them. A tree-wide search for `NFontGen` finds only those three
files, their three headers, and one `friend` declaration in
`3Dmotor/FontFormat.h`. `FontData.h` and `FontChars.h` were included by no
translation unit at all; `FontInfo.h` only by the two `.cpp` beside it.

The live copy is **`FontGen/`**, a separate source directory with `int main`, its
own `FontFormat.h` and its own fork of the same routines built around an
`SFontInfo` instead of a `CFontInfo`. It has no CMake wiring, and the tool ships
prebuilt as `Tools/FontGen.exe`. What is in `GameX` is a stale copy of it.

#### What the runtime actually does

```
Fonts/Body/Font.xdb  ->  NDb::SFont { pTexture, uid, nHeight, nThickness,
                                      bItalic, bAntialiased, ePitch,
                                      eCharset, szFaceName }
        |                                   |
        v                                   v
Data/bin/fonts/<uid>                   Texture.dds
  CFontFormatInfo                        the glyph atlas, 256x256 DXT3
  { chars: uint16_t -> STFCharacter,
    kerns: (prev<<16|c) -> int,
    nHeight, nExternalLeading,
    nAveCharWidth, nMaxCharWidth,
    cCharSet, wDefaultChar }
```

`GLocale.cpp`'s `AddAllAvailableFonts` walks every `SFont` record, pulls the
prebuilt blob through `CResourceOpener( "Fonts", uid )` and pairs it with the
texture. The `szFaceName`, `nHeight` and the rest are FontGen's *inputs*, kept in
the record so an atlas can be regenerated; nothing at runtime reads them to
choose a face.

Layout is `GText.cpp:340`, and it is the whole engine:

```cpp
fX += ( sCharacter.nA + pInfo->GetKern( *iChar, wcLastChar ) ) * scale.x;
pLayout->AddRect( fX - fShift, nY, CTRect<float>( x1, y1, x2, y2 ), colour );
fX += sCharacter.nBC * scale.x;
```

One glyph per UTF-16 code unit, advance by A, pairwise kern, a quad from the
atlas, advance by B+C. `UI/UIML.cpp` and `UI/mlVisObjects.cpp` do the same.

**The format cannot express shaping, bidi, cluster reordering or mark
positioning.** `STFCharacter` is a rectangle and three integers, and the map is
keyed by one `uint16_t`. That is a property of the data, not of the rasteriser,
so no library choice can change it.

#### What ships

| record | face | charset | height | weight | antialiased |
|---|---|---|---|---|---|
| body | System | RUSSIAN | 18 | 400 | yes |
| h1 | Impact | ANSI | 38 | 400 | yes |
| h2 | Impact | ANSI | 20 | 400 | yes |
| numeric | Arial | ANSI | 14 | 400 | no |

Their four `Texture.dds` are not loose on disk; they come out of a pak. What is
loose under `Data/Fonts` is 20 more prebuilt atlases: 13 in `1024x768`
(courier_new, garamond, impact, timesnewroman at assorted heights and weights),
5 in `800x600`, and `common/system.dds` and `system2.dds`. `Data/bin/fonts` holds
35 blobs, 7 to 12 KB each.

**No `.ttf`, `.fon` or `.otf` ships anywhere in the tree.** Those face names
resolved against fonts installed on the build machine in 2005.

#### If FontGen is ever ported, which library

Only relevant to regenerating atlases off Windows. It is a tooling project, not a
blocker for the game.

`FontGen` needs four things: rasterise a face at a pixel height, weight and
slant; per-glyph A, B and C advances; the kerning pair table; a bitmap to blit
into an atlas.

- **FreeType alone covers all four.** `FT_Load_Glyph` gives
  `metrics.horiBearingX` (A) and `horiAdvance` (B+C), `FT_Get_Kerning` reads the
  legacy `kern` table that `GetKerningPairs` was reading, and `FT_Render_Glyph`
  gives the bitmap.
- **HarfBuzz buys nothing**, because the format has no shaping.
- **Pango, Cairo, Skia and SDL_ttf** are layout or 2D engines wrapping FreeType.
  One call of each would be used and the whole dependency carried.
- **DirectWrite is not available**: DXVK Native ships 78 headers, d3d8 through
  d3d12 and DXGI. There is no `dwrite.h`, no `d2d1.h` and no `usp10.h`.
- The `.fon` question only arises for the *body* font, whose face is `System`,
  the Windows raster font shipped as an NE executable. FreeType's `winfnt` driver
  reads FNT and the NE container, so even that needs nothing extra.

One detail for anyone chasing visual parity: `FontData.cpp:72` kept **only the
green channel** as alpha, `(uint32_t(pBitmapBits[i + 1]) << 24) | 0x00ffffff`,
out of a 24-bit BGR DIB, and `FontInfo.cpp:136` asked GDI for quality `6`,
spelled as a bare literal, which is `CLEARTYPE_NATURAL_QUALITY`. So the shipped
atlases are the green plane of a subpixel render used as coverage.
`FT_RENDER_MODE_LCD` and taking green is the analogue. Byte-identical output is
not reachable and does not matter: an atlas is regenerated whole.

### The last two Windows headers: crtdbg.h and ShellAPI.h

Both are one small thing each, and neither is a game feature. Investigated
2026-08-26.

#### `crtdbg.h`, `Game/main.cpp:96` - deleted in `6c73c6ad4`

Three calls at the top of `WinMain`, not guarded by anything:

```cpp
_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
const int nLeakId = -1;
_CrtSetBreakAlloc( nLeakId );
```

That is the MSVC CRT debug heap: track every allocation, dump what is still live
at exit, and break on allocation number *n*. Three reasons it does nothing here:

1. **`_CrtSetBreakAlloc( -1 )` is a no-op by definition.** -1 is the value that
   means "do not break", so the third line is inert as written.
2. **The whole block is inert in Release.** `ucrt/crtdbg.h` defines
   `_CrtSetDbgFlag(f)` as `((int)0)`, `_CrtSetBreakAlloc(a)` as `((long)0)` and
   `_CrtSetReportMode(t, f)` as `((int)0)` when `_DEBUG` is not defined, which is
   what CI builds.
3. **mimalloc has taken the allocator.** Four lines above, `main.cpp` includes
   `mimalloc-new-delete.h` and calls `mi_version()` specifically to keep mimalloc
   in the import table "so its redirect DLL can replace the CRT allocator for
   every game module before initialization". Allocations no longer reach the CRT
   heap, so a CRT leak dump has nothing to report even in Debug.

Deleted rather than guarded: modern tooling covers what it was for, and mimalloc
has its own accounting behind `MIMALLOC_SHOW_STATS`. The three sibling copies, in
`ELK_A7`, `Server` and `TestClient`, stay. They are separate programs, none uses
mimalloc, and none of those directories has CMake wiring.

**It raised the error count, from 36 to 42.** `crtdbg.h` was a missing-header
*fatal*, and a fatal stops the translation unit where it stands, so it had been
hiding seven errors further down `main.cpp`: crashpad's `base::FilePath`
constructed from `wchar_t` literals at :70-72, which is `std::string` on POSIX;
`SetErrorMode( SEM_FAILCRITICALERRORS )` at :98; `GetCurrentDirectory` at :106;
and `MAKEINTRESOURCE` at :136. That is the entry point's own Win32 surface, and
it was always there.

#### `ShellAPI.h`, `GameX/MPLANTest.cpp:155`

One call, inside `#ifdef LAN_TEST_ENABLED`, which is on whenever `_FINALRELEASE`
is not:

```cpp
void CLANTester::RunShellCommand( const std::wstring &wszCommand )
{
	std::vector<TCHAR> winCommand( wszCommand.begin(), wszCommand.end() );
	winCommand.push_back( '\0' );
	ShellExecute( 0, "open", &( winCommand[0] ), "", "", SW_SHOWNORMAL );
}
```

Its two callers are `CLANTester::EndGame` and `CLANTester::AsyncDetected`, which
run whatever the globals `LANTEST.ExecuteOnEnd` and `LANTEST.ExecuteOnAsync`
hold and then exit the game. It is a hook for an automated LAN test to invoke
something when a run finishes or desyncs.

`std::system` is the portable equivalent and is closer than it looks: both hand a
command line to the shell. `SDL_OpenURL` is not the analogue - it opens a URL or
a file in the desktop's default handler, and these are commands. Nothing else in
the tree calls `ShellExecute`.

#### What is *not* used, which is the more useful half

`ShellAPI.h` and `shlwapi.h` also carry the shell's path helpers, the virtual
folder namespace and the common file dialogs. **The game uses none of it.**

A search across every module that is built finds no `GetOpenFileName`, no
`GetSaveFileName`, no `OPENFILENAME`, no `SHBrowseForFolder`, no
`SHGetFolderPath`, no `SHGetSpecialFolderPath`, no `PathAppend` or
`PathCombine`, and no `shlwapi` link. Every `CFileDialog` in the tree is in
`ELK_A7`, the MFC localisation tool, which has no CMake wiring.

The game never asks the OS for a file. Savegames and replays are chosen in
`CInterfaceSaveLoadMenu` and `CInterfaceReplaySaveLoad`, which build their own
lists by scanning a directory through `NFile::GetDirectoryFiles` and draw them
with the engine's own widgets. So no file-dialog replacement is needed, from SDL
or anywhere else, and the shell's virtual folders never come into it.

### The window icon off Windows, and why there is no one answer - done

Investigated 2026-08-26. `Game/main.cpp:138` passes `MAKEINTRESOURCE(IDI_MAIN)`
to `NWinFrame::SFLB1_InitApplication`, which is how the taskbar, alt-tab and the
task manager get a branded icon instead of a generic one. PE has a resource
section for that and ELF does not, so this is the one item in `main.cpp` that is
a design question rather than a substitution.

#### What exists already

- `Game/icon.rc` compiles `Game/main.ico` into the executable, and it is already
  guarded: `Game/CMakeLists.txt:53` has `if(WIN32)` around `target_sources`.
- `System/WinFrame.cpp:479`, the SDL implementation, takes the icon argument as
  an unnamed `LPCSTR` and ignores it. Off Windows the window has no icon today.
- **`System/WinCursor.cpp:107` already decodes this container.** `DecodeCursor`
  reads an ICO/CUR directory, a `BITMAPINFOHEADER`, an optional palette, a
  bottom-up colour bitmap of double height and a 1bpp AND mask, into an
  `SDL_Surface` in `ARGB8888`. It handles type 1, the icon form, explicitly.
- Nothing calls `SDL_SetAppMetadata` or sets `SDL_HINT_APP_ID`, and there is no
  `.desktop` file anywhere in the tree. `install()` puts `Game` in `bin` and
  nothing else.

Two gaps in reusing the decoder: it reads only the **first** directory entry, and
it rejects any bit depth other than 8, 24 or 32. `main.ico` holds six images, and
the first is 48x48 at **4bpp**, so as written it would be refused. It also has
48x48, 32x32 and 16x16 at 8bpp, so entry selection plus a 4bpp path, or entry
selection alone, is the whole of the work.

#### What the platforms actually offer

Measured against the SDL3 that is used here, 3.4.2, the distro one on Linux:

| | mechanism | status |
|---|---|---|
| X11 | `SDL_SetWindowIcon` sets `_NET_WM_ICON` | works, and every window manager reads it |
| Wayland | `SDL_SetWindowIcon` uses `xdg_toplevel_icon_v1` | works **where the compositor implements it**. `libSDL3.so` carries the strings `xdg_toplevel_icon_manager_v1` and `wayland: cannot set icon; required xdg_toplevel_icon_v1 protocol not supported`, so SDL both supports it and reports when the compositor does not |
| Wayland, otherwise | `app_id` matched against an installed `.desktop` file | the only universal path, and the one GNOME wants |

SDL's own header says it: `SDL_HINT_AUDIO_DEVICE_APP_ICON_NAME` notes that the
windowing system icon "may be set with SDL_SetWindowIcon (**or via desktop file
on Wayland**)". And `SDL_HINT_APP_ID` is documented as the string "used by
desktop compositors to identify and group windows together, as well as match
applications with associated desktop settings and icons".

So the honest answer to "is there a universal, standard way": **no single one,
but two that compose**, and a complete job wants both. Both were done, on
2026-08-26, in `14395c084`, `109df01b8`, `f59b3003f`, `bd854d83f` and
`1486af42e`.

1. `SDL_SetWindowIcon` with real pixels. Covers X11 always and Wayland on
   compositors that have `xdg_toplevel_icon_v1`.
2. An `app_id` set before `SDL_Init`, plus a `.desktop` file installed under
   `share/applications` with a matching name and an `Icon=` key, plus the icon
   under `share/icons/hicolor/<size>/apps`. Covers the rest of Wayland, and is
   what makes the icon show in launchers and docks rather than only on the
   window.

#### Getting the pixels there

`#embed` is C23 and C++26; GCC 15 has it for C but relying on it for C++ here is
premature. The alternatives, in order of how little they add:

- **Load `main.ico` at runtime from the install directory.** The game already
  reads `splash.bmp` that way, from `NMainLoop::GetBaseDir()`, so this needs an
  `install(FILES)` line and nothing else. An icon that fails to load leaves the
  window as it is today.
- **Generate a byte array at build time** with a small CMake step, which keeps it
  in the binary and matches what the PE did.

Either way the decoding is `DecodeCursor` with the two gaps closed, so no image
library is needed for this and neither is SDL_image.

#### What was actually built

The runtime path, since loading beat both `#embed` and a generated array: the
file is tiny and is opened once at startup.

- `Game/blitzkrieg2.desktop.in` is configured with the install prefix and
  installed to `share/applications`. `Path=` is set, because the game resolves
  everything against the working directory and an `Exec=` without it starts a
  game that finds no data.
- `main.ico` installs to the top of the prefix, beside `splash.bmp`, so the
  desktop entry and the run-time loader read one file.
- `SDL_SetAppMetadata( "Blitzkrieg II", 0, "blitzkrieg2" )` in
  `NSdl::InitVideo`, before the video subsystem comes up. That identifier must
  stay the basename of the desktop file.
- `NWinFrame::SetIcon` reads the file, decodes **every** entry through
  `NWinImage`, gives SDL the largest and adds the rest with
  `SDL_AddSurfaceAlternateImage`, so the desktop picks per slot rather than
  scaling one size to all of them.
- `Game/main.cpp` keeps `MAKEINTRESOURCE( IDI_MAIN )` behind `BOOST_OS_WINDOWS`
  and passes 0 off it.

One thing had to be fixed first, and it was not about icons at all:
`NMainLoop::GetBaseDir` did not work off Windows. It built the parent of the
working directory by appending `".."` and a backslash to a string, and off
Windows a backslash is an ordinary character in a file name, so
`lexically_normal` had nothing to collapse. Measured, it returned
`/opt/bk2/bin\..\`. **That is also why the splash screen never appeared**, since
`main.cpp` asks for `GetBaseDir() + "splash.bmp"`. `bd854d83f` builds it with
`parent_path` and confines the lower-casing to Windows, where paths are case
insensitive; off Windows folding the case renamed the directory.

### Case-insensitive compare: why not boost::algorithm

`f7b59d371` replaced the tree's one built-module `strnicmp`, in
`SceneB2/WingScaleMutator.cpp`, with a prefix compare over `NStr::ASCII_tolower`
rather than `boost::algorithm::istarts_with`. Boost was considered and measured,
so that it does not have to be re-argued.

**It is locale dependent, by design.** From
`boost/algorithm/string/compare.hpp`:

```cpp
is_iequal( const std::locale& Loc=std::locale() ) : m_Loc( Loc ) {}
bool operator()( const T1& Arg1, const T2& Arg2 ) const
{ return std::toupper<T1>(Arg1,m_Loc)==std::toupper<T2>(Arg2,m_Loc); }
```

and `istarts_with` takes `const std::locale& Loc = std::locale()`, so the default
argument copies the **global** locale on every call.

**It is about ten times slower.** Measured with the vendored Boost headers at
SceneB2's own flags, `-O3 -std=gnu++17`, 72 Granny-style bone names against the
prefix `"Wing_"`:

| | ns per compare |
|---|---|
| `ASCII_tolower` prefix compare | 1.6 - 2.0 |
| `boost::istarts_with`, locale defaulted per call | 17.1 - 18.6 |
| `boost::istarts_with`, locale hoisted | 12.7 - 12.8 |
| `boost::starts_with` with `is_iequal` hoisted | 13.2 |
| `strncasecmp` | 2.9 |

Two costs, and the gap between rows two and three separates them. The `std::locale`
copy per call is about 5 ns. The remaining 13 ns is per character, per operand:
`std::toupper(c, loc)` is `use_facet<ctype<char>>(loc).toupper(c)`, and libstdc++'s
`toupper` is `{ return this->do_toupper(__c); }` with `do_toupper` **virtual**
(`bits/locale_facets.h:238` and `:647`), so it cannot inline and calls out to
libstdc++.so once per character. `ASCII_tolower` is four branchless integer
operations.

**It is also not on SceneB2's include path.** That target's compile line carries
40 Boost include directories, but not `libs/algorithm/include` or
`libs/range/include`; using it needs a CMake change plus the Range, MPL, Iterator
and Regex headers StringAlgo pulls in.

Two honest qualifications:

- The speed is not the argument. `Setup` runs once per model over a few dozen
  bones, so 11 ns of difference is invisible. What decides it is locale
  independence and not adding a dependency to a target that has none.
- `strnicmp` was **itself** locale sensitive, using the thread's `LC_CTYPE`, so
  Boost is arguably the closer translation. It does not matter here: the tree
  never calls `setlocale`, `std::locale::global` or `imbue` anywhere, so the
  global locale is `"C"`, and Granny bone names are ASCII. The codebase already
  leans this way on purpose - `GlobalVars.h`'s `ParseFloat` picks `from_chars`
  because it is "the only standard parser specified to ignore LC_NUMERIC".

The one remaining `stricmp` is `ED_B2_M1/MechUnitRPGStatsBuilder.cpp:154`, on a
file extension, in the editor.

### The clipboard loses characters on Windows

`UI/WindowEditLine.cpp` copies and pastes through `CF_TEXT`, the ANSI
clipboard. Copying runs the selection through `NStr::ToMBCS`, which cannot
represent anything outside the process code page, so a player typing Cyrillic
into the chat box on a Western code page copies out mangled text. Pasting runs
`NStr::ToUnicode` the other way with the same limit.

Off Windows this is already fixed, not by intent but because SDL's clipboard is
UTF-8: `GetClipboardText` and `SetClipboardText` there convert code point by
code point and lose nothing. So the two platforms now disagree about what can be
copied, which is the reason to write this down rather than leave it.

The fix on Windows is `CF_UNICODETEXT` instead of `CF_TEXT`: the buffer holds
`wchar_t` rather than `char`, `ToMBCS` and `ToUnicode` drop out of both paths,
and the `GlobalAlloc` size stops being a byte count. It is a behaviour change on
a platform that currently works, which is why it was not folded into the port.

Worth doing with the rest of the narrow-string encoding work, since the same
`ToMBCS` appears elsewhere and has the same limit everywhere it does.

### Animated cursors off Windows

`System/WinCursor.cpp` decodes cursors itself off Windows, because SDL manages
cursors but parses no cursor file format. A `.cur` is read in full. A `.ani` is a
RIFF `ACON` whose `fram` list holds one `icon` chunk per frame, each of them a
complete `.cur`, and only the first is taken.

So the six animated cursors in `Versions/Current/Data/Cursors` show as a still
frame off Windows: `AttackNotValid`, `AttackValid`, `Default`, `Move`,
`reinforcement`, `test`. Windows is unaffected, keeping `LoadCursorFromFile`,
which animates.

SDL has no animated cursor and will not grow one: the frames would have to be
decoded into an array of `SDL_Cursor*` and a timer would call `SDL_SetCursor` on
each tick. The frame delays live in the `anih` header's default rate and in the
optional `rate` chunk, in units of 1/60 s, and the `seq` chunk can reorder frames,
so a faithful version reads both rather than assuming a fixed interval.

Worth doing only once something runs and someone can see the cursor.

### Windows types in code added after the type sweep

Upstream keeps adding `DWORD`/`WORD`/`BYTE` in new code, and the sweep (#6) predates it.
This is **not** cosmetic: MSVC's `DWORD` is `unsigned long` while `uint32_t` is `unsigned
int` - different types - so a header the sweep converted stops matching a `.cpp` that still
says `DWORD` (this actually broke `SGameConsts::GetMPDataVersionChecksum`).

Re-run this check after any rebase onto a newer `netcode_bugfixes`:

```bash
git diff <sweep-commit> HEAD -- Versions/Temporary/Engine/Sources \
  | grep '^+' | grep -wE 'DWORD|WORD|BYTE'
```

Legitimately left alone: `ELK_A7` and `TestParsing` (not in the built subdirectory list),
`Parser/LangNode.cpp` (the strings `"WORD"`/`"DWORD"` are schema token names, not types), the
editor projects `ED_*`/`MapEditor*` (`BUILD_EDITOR=ON` only), and D3D9 signatures where the
sweep deliberately used `unsigned long` so the override matches the COM interface.


### The benchmark harness ignores its own `/arch:` flag

`3Dmotor/benchmark/CMakeLists.txt`:

```cmake
function(add_benchmark_arch filename flag suffix)
    ...
    target_compile_options(${target_name} PRIVATE /arch:SSE2)   # <-- ${flag} never used
endfunction()

function(add_benchmark filename)
    add_benchmark_arch(${filename} "/arch:SSE2" "")
    add_benchmark_arch(${filename} "/arch:AVX"  "_avx")
    add_benchmark_arch(${filename} "/arch:AVX2" "_avx2")
endfunction()
```

`${flag}` is accepted and then ignored - every variant compiles with `/arch:SSE2`. So
`MMXTransformVector_benchmark`, `_benchmark_avx` and `_benchmark_avx2` are three builds of
identical code, and any SSE2-vs-AVX-vs-AVX2 numbers taken from them are meaningless.

Fix is one line (`PRIVATE ${flag}`), but do it **after** the replay. Note `/arch:` is
MSVC-only syntax, so the same function needs a compiler guard before it can build on Linux
- worth doing both changes together.


### Scratch files parked on `linux2-strays`

Three files rode along with the export-macro commit but belong to neither it nor
the build. They were removed from that commit and parked as a single commit on
branch `linux2-strays` (based on the corrected `linux2` tip). Revisit after the
replay:

| file | lines | what to do |
| --- | --- | --- |
| `3Dmotor/test.cpp` | 549 | gtest experiments on the MMX routines, sitting in the source dir instead of `test/`. Decide whether anything here is not already covered by `3Dmotor/test/*`, then either move the useful cases into a proper `<name>_test.cpp` or drop it. |
| `Sound/sandbox.cpp` | 75 | SDL3 / SDL_mixer scratch program. Only relevant once the Sound backend is actually ported off DirectSound; keep it as a reference until then. |
| `3Dmotor/test/CalcPointLightAttenuation.cpp` | 36 | A real unit test that was never registered. Wire it up with `add_unit_test(CalcPointLightAttenuation)` and confirm it passes; this is the one worth reviving first. |

None of the three is referenced by any `CMakeLists.txt`, so none of them has ever
been compiled and none can be assumed to build as-is.


### `__rdtsc` has no non-x86 fallback

Commit 16 gives `Misc/HPTimer.cpp` a header for `__rdtsc`, but the intrinsic
itself is x86 only, and `<x86intrin.h>` does not exist on other architectures.
That is now the **only** thing left in the file: "calibrate the TSC against
steady_clock instead of QueryPerformanceCounter" removed the
`QueryPerformanceCounter`, `QueryPerformanceFrequency`, `GetTickCount` and
`_LARGE_INTEGER` uses, and row 19 had already taken the `Sleep`.

**An earlier draft of this entry proposed dropping the TSC for
`std::chrono::steady_clock` and deleting the calibration. Do not.** The
calibration is the cheap part, once per update rather than once per reading,
and the TSC is the point: reading it is a register access of a few nanoseconds.
`clock_gettime` is only comparable while the kernel's clocksource is `tsc`. When
it falls back to `hpet` or `acpi_pm` the vDSO cannot serve the call from user
space and every reading becomes a syscall, roughly 20 ns to about 1 us, which
has already been observed on this project as a flame graph dominated by
timekeeping. Check with
`/sys/devices/system/clocksource/clocksource0/current_clocksource`.

So the remaining work is narrower than it looked: a non-x86 branch in
`GetCounter`, not a rewrite. `__builtin_readcyclecounter` is the natural
candidate, with `CNTVCT_EL0` on arm64, falling back to `steady_clock` where
neither is readable from user space, and only that fallback needs the
calibration bypassed since it is already in known units.

This matters sooner than it looks: the stated next targets include macOS, which
is predominantly arm64.

**On x86 the read itself should become `__rdtscp` where CPUID offers it.**
Deferred deliberately: the file is portable now, and this is a correctness and
accuracy question rather than a build one.

One correction to the reasoning, because it changes what the work is. Dynamic
frequency - SpeedStep, Turbo, P-states, AVX-512 licence throttling - does *not*
skew the TSC on anything modern. Since roughly Nehalem and the AMD parts of the
same era the TSC is **invariant**: it ticks at a fixed reference rate decoupled
from the core clock and does not halt in C-states, which is exactly why the
kernel is willing to use it as a clocksource at all. CPUID leaf 0x80000007
EDX bit 8 reports it; Linux surfaces it as the `constant_tsc` and `nonstop_tsc`
flags. So the periodic recalibration in `UpdateHPTimerFrequency` is a relic of
pre-invariant parts, not something holding a modern reading together, and the
counter is not returning garbage on account of frequency scaling.

What `RDTSCP` actually buys is different, and still worth having:

- **It is partially serialising.** It waits for prior instructions to retire,
  where plain `RDTSC` can be reordered in both directions by out-of-order
  execution. Over short intervals that reordering is real noise. Full fencing
  still wants an `LFENCE` after it.
- **It reports where it ran.** `ECX` comes back with `IA32_TSC_AUX`, which the
  OS loads with the CPU or node id, so a thread that migrated mid-measurement
  can be detected. That matters on multi-socket boxes whose TSCs are not
  synchronised across packages, where a migration can make a delta negative.

Neither is universal: `RDTSCP` is CPUID leaf 0x80000001 EDX bit 27, absent on
early x86-64 including some Core 2, so it needs a runtime check with an
`__rdtsc` fallback. `3Dmotor/GLightPerVertexDispatch.cpp` already has the
pattern to copy for that, including the trap that XGETBV faults unless OSXSAVE
was checked first; it is `<intrin.h>` only today and would want GCC's
`<cpuid.h>` alongside. The spelling is the same on both compilers:
`__rdtscp( &nAux )`.

Two hazards to keep in the same commit when it happens. A genuinely
non-invariant TSC should fall back to `steady_clock` rather than pretend the
calibration fixes it, since recalibration only tracks the average rate over the
last window and cannot correct variation inside it. And a hypervisor may trap
`RDTSC` outright, at which point it is slower than `clock_gettime` and the whole
premise inverts, so the choice is worth measuring once at startup rather than
assuming.


### `__declspec` inventory, and the one nobody fixes

Commit 20 converts the single `__declspec(noinline)`. Taking stock of what is
left in **tracked** files that the default build compiles, since the generated
`<MODULE>_export.h` headers account for most raw hits and are CMake's business:

| site | spelling | who fixes it |
| --- | --- | --- |
| `System/WinFrame.{h,cpp}`, `System/GResource.{h,cpp}`, `System/RandomGenInternal.{h,cpp}`, `Main/MainLoopInternal.cpp` | `__declspec(dllexport)` on the `SFLB*` entry points | upstream commit 41, `411de6e46`, exactly these 7 files |
| `Misc/Win32Helper.h` | `__declspec(deprecated)` on the critical section `Enter`/`Leave` | gone by the `linux` tip; commit 34 replaces that class with `std::mutex` |
| `Misc/Tools.h` | `#define EXTERNVAR __declspec(dllimport) extern` | **nothing** |
| `Common_RTS_AI/aiobjectbase.cpp` | `__declspec(naked) _penter` | inert, the whole hook is inside a block comment |
| `3Dmotor/test/original.h` | `__declspec(align(8))` | verbatim reference copy, not in the default build |

**`EXTERNVAR` is the gap.** It still carries `__declspec(dllimport)` at the
`linux` branch tip, so no commit in the replay addresses it, and it is not
decorative: it declares globals that genuinely cross DLL boundaries, including
`theExecutorContainer` used from four `AILogic` translation units and
`renderStats` in `3Dmotor`.

The fix is the same shape as commit 15: route it through the module's
generated export macro rather than a hardcoded `dllimport`, or drop the
attribute and let the linker resolve it, which is what happens on ELF anyway.
It needs its own commit and a check that each user links the module that
defines the variable.


### _vsnprintf is a gap the replay does not close

Commit 28 adds `<cstdarg>` to the only two varargs functions left in the
engine, `DbgTrc` in `Misc/Tools.cpp` and `Report` in `System/CmdLine.cpp`. Both
then call `_vsnprintf`, the MSVC spelling, and nothing in the remaining table
(rows 29 to 44) touches it. Row 52 has since deleted `CmdLine.cpp` outright,
so `DbgTrc` is the only one left.

Standard `vsnprintf` has been available since C++11 and is not a rename:

| | `_vsnprintf` | `vsnprintf` |
|---|---|---|
| on truncation | returns -1 | returns the length it *would* have written |
| null terminator on truncation | not written | always written |

Both call sites pass `BUF_SIZE - 1` into a buffer of `BUF_SIZE` that starts
zeroed, so the missing terminator is currently masked rather than harmless.
Switching the spelling without also dropping the `- 1` would silently change
how a truncated trace line ends.

Deferred by agreement: it is a behaviour change, not a portability shim, so it
belongs in its own commit after the replay rather than folded into 28.


### Retiring the printf family: what is actually left, and with what

Commit 25 removed most of this already. The remaining surface, measured rather
than guessed:

| | count |
|---|---|
| printf-family call sites outside vendored Lua | 57 |
| ... inside `Script/` (vendored Lua) | 16 |
| `_vsnprintf` | 3, `CmdLine.cpp`'s went with row 52 |
| `swprintf` | 0, all removed by row 57 |
| functions that actually declare `...` | ~10 |

Most of the 57 are plain `sprintf( szBuf, "FPS = %f", fFPS )`. Those never
needed a `va_list` and become `fmt::format` with no ceremony. Only the
forwarding functions have a real problem, and one of them is not even
formatting.

**A parameter pack alone does not replace `va_list`.** A pack exists only at
the template, and every module here is a shared library: `DbgTrc` is
`MISC_EXPORT`, and a template cannot be exported from a DLL. The replacement is
two pieces, a pack at the API and a type-erased argument store behind it. That
store, `fmt::format_args`, is the actual `va_list` replacement.

```cpp
// Tools.h - inline template, no export
template< typename... Args >
void DebugTrace( fmt::format_string< Args... > fmtStr, Args &&... args )
{
	DbgTrcV( fmtStr.get(), fmt::make_format_args( args... ) );
}

// Tools.h - the one exported, non-template symbol
MISC_EXPORT void DbgTrcV( fmt::string_view fmtStr, fmt::format_args args );
```

This is how fmt itself is built (`format` calls `vformat`), and the reason is
the same one that applies here: the formatting machinery gets instantiated once
instead of at all several hundred `DebugTrace` sites.

**The cheaper variant keeps the format strings.** `<fmt/printf.h>` ships in the
vendored fmt 12 and `fmt::sprintf` / `fmt::vsprintf` are not deprecated there,
only their `wchar_t` overloads are. They are variadic-template and type-safe,
take no `va_list`, and still speak `%d`, so none of the existing format strings
need rewriting:

```cpp
MISC_EXPORT void DbgTrcV( fmt::string_view fmtStr, fmt::printf_args args );

template< typename... Args >
void DebugTrace( const char *pszFormat, const Args &... args )
{
	DbgTrcV( pszFormat, fmt::make_printf_args( args... ) );
}
```

Given that commit 25 was 291 files of format-string translation and still took
five rounds of the assert check to settle, the variant that does not touch the
strings is the one to prefer.

**Compile-time checking is not free here.** `fmt::format_string` is only
consteval-checked under C++20. This tree is C++17, so the checks are runtime
unless literals are wrapped in `FMT_STRING(...)`. `std::format` is C++20 and
`std::print` is C++23, so neither is available yet; fmt's API mirrors them
closely enough that the eventual swap is mechanical.

**Two things the survey turned up that are not portability problems.**

`AILogic/MPLog.h:19` calls `vsprintf`, the unbounded one, into a
`static char buff[2048]`. That is an overflow waiting on a long enough log
line, independent of any port.

The survey missed a second one: `Parser/ErrorsAndMessages.h` held an inline
`Msg( char *s, ... )` doing the same thing into a `static char[10000]`. It was
dead - the identifier appeared nowhere else in the tree - and GCC found it only
because the header never included `<cstdarg>`. Deleted rather than fixed, in
"remove Msg, a dead trace helper in the parser". Worth assuming the counts above
are lower bounds for the same reason: they were taken by grepping for call
sites, which does not see a definition nobody calls.

`System/BinSaver.h:93` and `System/XmlSaver.h:70,177` are not format functions
at all. The `...` there is the worst-conversion-rank overload tiebreaker,
paired with an `SInt2Type<1>*` overload:

```cpp
template< class T > void CallObjectSerialize( ..., T *p, ... )              // fallback
template< class T > void CallObjectSerialize( ..., T *p, SInt2Type<1> *pp ) // preferred
```

A pack is the wrong replacement for that; it wants `if constexpr` or tag
dispatch. It also sits in `BinSaver`, which is on-disk-format adjacent, so it
deserves its own commit and its own care.

**Order to do this in, after the replay:**

1. ~~`_vsnprintf` to `vsnprintf` in `Tools.cpp`~~ and 4. ~~`DbgTrc` /
   `DebugTrace` to the `printf_args` two-layer form~~ - **done together**, and
   they had to be. Step 1 alone was committed first, then folded in: dropping
   the only `_vsnprintf` in the 3Dmotor link left `DxErr.lib`'s own reference
   unresolved, because the UCRT only ever has that symbol in an object that
   calls it. `legacy_stdio_definitions` now hangs off `dxsdk::dxerr`.
   `D3DASSERT` took the same two-layer shape as `DbgTrc`; its success test
   moved into the header wrapper so the per-draw path still formats nothing.
   The `printf_args` variant held up: the format strings were kept, and only
   24 sites needed touching. Seven were unscoped enums, which fmt 12 does not
   format implicitly and which need `fmt::underlying` - that one was not
   predicted here. Two were real defects the type checking exposed: a `CDBID`
   class passed to `%d` in `libdb/Variant.cpp`, and every `%d` given a
   `size_t`. 224 call sites in the editor targets stay unverified, since those
   cannot be built at all.
2. `vsprintf` in `MPLog.h`, which is the overflow.
3. The plain `sprintf` call sites, mechanically, to `fmt::format`.
5. The `BinSaver` / `XmlSaver` ellipsis tiebreaker, separately and carefully.


### The generated enum conversions want boost::describe

For every enum in the database schema, dbcodegen emits a pair by hand:
`EnumToString`, a switch returning a string literal per enumerator, and
`StringToEnum_NDb_<Name>`, a chain of string comparisons. `libdb/CodeGenType.cpp`
writes both as text. There are 83 of these pairs across 22 files, all committed to
the tree, and Boost.Describe does the whole job with an annotation and a compile time
list (user, 2026-08-23).

What that would remove, beyond the codegen: every pair goes out of step with its enum
the moment an enumerator is added and the generator is not re-run, which nothing in
this build does; the `SKnownEnum<>` specialisation emitted alongside each pair; and
the generator's own string plumbing, which is where the namespace bug below came
from.

Two constraints on the replacement. The `default:` arm of every EnumToString returns
the *first* enumerator's name rather than failing, and the string is what reaches the
XML on save, so a rewrite that starts throwing or returning an empty string changes
what a bad value writes to disk. And the names are the enumerator spellings exactly,
`SEASON_WINTER` and so on, which is what `boost::describe` yields by default, so the
on-disk vocabulary survives.

Boost.Describe is C++14 and header only, and Boost is already a dependency here.

Noticed while fixing "stop the enum codegen qualifying a definition with its own
namespace", which corrected the generator and the 83 files it had already produced.


### CVariant should be a std::variant

`libdb/Variant.h` is a tagged union written by hand: an `EVariantType` next to an
anonymous `union` of `SBlob*`, `std::string*`, `std::wstring*`, `CDBID*` and
`SMultiVariant*`, with `Clear()` and `Copy()` switching over the tag to `new` and
`delete` the right arm. `std::variant` is the whole of that, and the class should
end up there rather than being patched arm by arm (user, 2026-08-23).

Not a small job, because `operator&( IBinSaver & )` writes the tag and then the arm
under fixed chunk ids, so the on-disk shape has to survive the rewrite exactly.

Three separate defects were found in it while porting, none fixed beyond the first,
because each wants the rewrite rather than another patch:

- `SBlob::~SBlob` called `delete[]` on the `void*` the buffer is held in, which is
  undefined. Fixed in "delete CVariant's blob through the type it was allocated as",
  since it was a live compile error off MSVC. The buffer wants to be a
  `std::vector<uint8_t>`, which would delete the destructor, the copy constructor
  and all three `new uint8_t[]` at once.
- `SMultiVariant()` leaves `m_pMultiVariantMap` uninitialised, and `~SMultiVariant`
  asserts it non-null and then deletes it. `CVariant::operator&` calls exactly that
  constructor on the reading path, and `ASSERT` compiles out of a release build, so
  a `VT_MULTIVARIANT` that fails to deserialise deletes a garbage pointer. Holding
  the map by value fixes it and removes a `new`/`delete` pair.
- `SBlob`'s `m_bDelete` makes the buffer optionally non-owning, and the only way to
  clear it is `SetDestructorDeleted`, whose two callers are both in `MapEditor` and
  both leak: `CVariant( pData, nSize )` always allocates, `new uint8_t[0]` included,
  and they switch the delete off exactly when `nSize` is 0. The aliasing constructor
  `SBlob( void * )` that the flag exists for is never called at all.

`std::any` is the other candidate and is the weaker one: the tag is serialised, so
the set of alternatives has to stay closed and enumerable.


### Testing an HRESULT against S_OK is not the COM idiom

`SUCCEEDED( hr )` / `FAILED( hr )` exist because success is a range, not a
value. `S_FALSE` is the general second success code; D3D adds its own, and the
D3D9Ex present path returns `S_PRESENT_OCCLUDED` and `S_PRESENT_MODE_CHANGED`
when the device is occluded or the mode changed, both of them successes. Code
written as `if ( hr == D3D_OK )` works on every machine where the call happens
to return exactly zero and starts reading a healthy device as an error only in
the situations those codes exist to describe, which is when it is hardest to
reproduce.

The tree is inconsistent about it: 83 direct equality tests against
`D3D_OK`/`S_OK` in the modules that build, 79 of them in `3Dmotor` and 4 in
`ShaderCompiler`, against roughly 60 uses of `SUCCEEDED`/`FAILED`, sometimes
within a few lines of each other.

`NGfx::D3DASSERT` in `3Dmotor/Gfx.h` is one of the 83 and gates its whole error
path on the test, so a non-zero success logs an error and trips the assert under
a debugger. It reads `S_OK` rather than `D3D_OK` only because the header must
not pull in `d3d9.h`; same value, same flaw. It inherited the test unchanged
from the body it replaced.

This is not a sweep. Each site has to be read for whether it meant "succeeded"
or genuinely meant "returned exactly zero" - `TestZBufferFormat` calling
`CheckDeviceFormat`, for one, plausibly wants to treat a non-zero success as
"no". A blind substitution would change control flow.


### -Wformat-security: 7 sites, all original, plus two other bugs found beside them

These are not port regressions: every one blames to 3726f2d9d "Add engine
sources", the original import. MSVC has no -Wformat-security equivalent on by
default, so GCC is simply the first compiler to look at them.

`System` is now clear of them. Row 53 removed `Logger.cpp:32` along with the
buffer it wrote into, and row 55 fixed `ConsoleBufferInternal.cpp:133`, which
was the last one there. What remains is in projects the Linux build has not
reached yet.

| project | sites |
|---|---|
| AILogic | BalanceTest.cpp:151, 153, 155, 157, 159, 161, 163 |
| libdb | Logger.cpp:24 |
| Server_Client_Common | NetLogger.cpp:78 |
| Game | main.cpp:238 |
| ED_B2_M1 | XMLExport.cpp:148 (editor, not built) |
| dbstruct | main.cpp:47 (not built) |

AILogic/Shell.cpp:225 looks like one to a grep but is not; its format string is
on the following line.

**The ten that build are done**, in "pass log text as an argument, not as a
format string": AILogic's seven, `libdb/Logger.cpp`, `NetLogger.cpp` and
`Game/main.cpp` all take `fmt::print` with the text as an argument. A fresh scan
for a non-literal format across `printf`, `fprintf` and `sprintf` returned
exactly the twelve listed here, so this inventory was complete.

The two that remain are both benign or unreachable. `dbstruct/main.cpp:47` has no
CMake wiring. `ED_B2_M1/XMLExport.cpp:150` passes a buffer that the four lines
above it fill with nothing but tab characters, so no `%` can occur in it - but
those same lines are a real overflow: `buffer` is `char[256]`, `nAmount` is
unchecked against that, and `buffer[nAmount] = 0` writes at the caller's index.
Editor only, so unverifiable either way.

These are real, not just noisy. Each passes log text as the format string, and
log text is exactly where a % turns up: unit names, file paths, error messages.
A %s reads a garbage pointer, a %n writes through one. System/Logger.cpp is the
worst placed since it is the general file logger. Minimum fix is
fprintf( f, "%s", buf ); the proper fix is the fmt migration in [Retiring the printf family: what is actually left, and with what](#retiring-the-printf-family-what-is-actually-left-and-with-what).

The two overruns described below are gone with it. They are kept here because
the shape recurs: a conversion call handed the whole buffer as its size, then
indexed at the returned length.

**Two unrelated bugs sitting next to them, both also original:**

System/Logger.cpp CFileDumper passes cbMultiByte = 1024 to WideCharToMultiByte
against char szBuffer[1024], so nLength can come back as exactly 1024 and
szBuffer[nLength] = 0 writes one past the end. CDebugDumper just below is worse:
szBuffer[nLength++] = '\n' then szBuffer[nLength] = 0 can write two past.

AILogic/AILogicInternal.cpp:1203 and AILogic/Shell.cpp:226 use %I64u, an
MSVC-only length modifier that GCC will not accept. Both are in the ASYNC
diagnostics, which is the code most wanted in working order when chasing a
desync. Wants %llu or PRIu64.

Detection note for later: grepping for a printf call with a non-literal first
argument misses any argument containing parentheses, which is most of them
here since they are almost all .c_str(). The pattern that actually works is to
list printf-family calls and filter out the ones containing a double quote.

### Threading primitives worth replacing

Commit 34 swapped `CCriticalSection` for `std::mutex`, which was a faithful
translation. The surrounding code is still doing by hand several things the
standard library and Boost already provide, and replacing them would delete
code rather than translate it.

- The cross-thread flags `bIsFileReading`, `bIsReady`, `bExit` and `bActive`
  are `volatile bool`. `volatile` is not a synchronisation primitive on any
  compiler; these want `std::atomic<bool>`.
- `GResource`'s request pipeline was a producer/consumer queue assembled from
  two mutexes, a `std::list`, a manual `CEvent` and a hand-managed loader
  thread. Row 46 replaced the event with a `std::condition_variable` and the
  thread with `std::thread`, and deleted the second mutex, which was locked by
  one thread and so protected nothing. What is left is a plain mutex and
  condition variable, which is the right shape; an SPSC queue would be a
  performance change, not a correctness one.
- `WinFrame`'s `msgList` has exactly one producer, the window procedure, and
  one consumer, the main loop. That is SPSC.
- `NetA4` serialises 17 entry points on a single mutex. Worth measuring before
  touching, but MPMC or a finer split is the obvious direction.
- `Sound/SoundEngine.cpp:23` declares `critSection` and never locks it. It was
  dead before commit 34 and the commit converted it rather than deleting it.

Each of these changes behaviour under threads rather than translating it, so
they belong after the replay, in their own commits, with their own testing.

### Path separators are built into string literals

`NFile::CreatePath` deliberately does not normalise what it is given. Rewriting
separators in the one function that consumes paths would make the callers that
build them wrong invisible, and those callers are the actual defect. The same
argument applies to any other sink: fix the construction, not the consumption.

Measured over the tree, excluding vendored Script/ and Arch/: 152 string
literals embed a backslash as a separator, across 75 files.

| project | literals |
|---|---|
| ED_B2_M1 | 75 |
| ELK_A7 | 19 |
| MapEditor | 13 |
| MapEditorLib, SceneB2 | 6 each |
| Game, GameX | 5 each |
| 3Dmotor | 4 |
| System, Main, AILogic | 3 each |
| TestDB, ED_Common, Parser | 2 each |
| FontGen, B2_MapEditor, Server_Client_Common, B2_M1_World | 1 each |

Two things make this smaller than it looks. 116 of the 152 are in editor or
unbuilt targets, leaving roughly a third in code the default build compiles.
And some of those are not paths at all: `MainFrameParams.cpp` uses
`"Software\\%s\\%s\\%s"` as a Windows registry key, where the backslash is
correct and must stay.

Worth doing incrementally rather than as a sweep, since each site needs a look
to decide whether it is a path, a registry key, or a display string.

### The resource loader's holder race

`holdRequests` exists to keep a `CPtr` on every queued request so it survives
until the loader thread has read it. `ReleaseFileRequestHolder` drops all of
them at once, guarded by `requests.empty()`.

That guard is not enough. The loader pops a request off `requests` and only
then reads it, so between the pop and the `Read()` the queue is empty while the
request is still in use. A `ReleaseFileRequestHolder` landing in that window
drops the last reference and the loader reads a destroyed object. `IsValid` is
checked before `Read`, not during it, so it does not close the window either.

This predates the port: the original held a second mutex, `pendingCheck`,
across the pop and the read, which looks like it was meant to cover exactly
this, but `ReleaseFileRequestHolder` never took that mutex and no other code
ever locked it. Row 46 deleted it rather than leaving a lock that protects
nothing.

The fix is for `ReleaseFileRequestHolder` to know whether a request is in
flight, not just whether the queue is empty - a counter incremented on pop and
decremented after `Read`, checked under the same mutex. Worth doing with the
`bIsFileReading` flag above, since that is the same question asked from the
other side.

### A failed resource load is a crash

Found 2026-08-22 by way of the uid byte-order bug, which made every geometry
lookup miss. The lookup miss was the bug; the crash on top of it is separate
and survives the fix.

`3Dmotor/GObjectInfo.cpp`, `CGrannyMeshLoader::Recalc`:

```cpp
if ( ( pGrannyFile->GetValue() == 0 ) || ( pSkeletonFileInfo && pSkeletonFileInfo->GetValue() == 0 ) )
{
    pValue = 0;
    return;
}
try
{
    granny_file *pFile = pGrannyFile->GetValue()->pFile;
    ...
    granny_file_info *pData = GrannyGetFileInfo( pFile );
    granny_model *pModel = pData->Models[0];
```

The guard establishes that the loader object exists. It says nothing about the
file inside it. `CGrannyMemFileLoader::RecalcValue` assigns
`pValue->pFile = GrannyReadEntireFileFromMemory( ... )`, which returns null for
an empty or malformed buffer, and `CFileRequest::Read` leaves the buffer empty
whenever the resource is not found. So `pFile` is null for a missing file, a
truncated file, a corrupt archive entry, or a save that names a resource this
build cannot resolve, and Granny dereferences it.

Three things to fix together:

- Check `pGrannyFile->GetValue()->pFile` in the same guard that checks the
  loader, and set `pValue = 0` when it is null, which is what the existing
  early return already does for the loader case.
- `GrannyGetFileInfo` can return null in its own right, and `pData->Models[0]`
  is indexed without consulting `ModelCount`.
- `RecalcValue` wraps its body in `catch ( ... ) {}` and swallows whatever went
  wrong, leaving `pFile` null with no record of why. At minimum it should say
  which resource failed.

The `try` around the crashing code does not help: dereferencing a null pointer
is not a C++ exception, so it unwinds nothing.

### The Windows SDK's `-A` macros, and why parenthesising does not defeat them

Investigated 2026-08-24 after a `C4273` on `NDb::GetObjectA` surfaced during the
`RenameNode` work. Nothing changed; none of it blocks the Linux port, because the
macros do not exist off Windows.

#### The paren trick does not work on these

`System/DB.h:155` reads:

```cpp
// conflict with #define GetObject GetObjectA (Windows SDK)
SYSTEM_EXPORT CResource *(GetObject)( const CDBID &dbid );
```

**That comment is false and the parentheses do nothing.** Measured against `cl.exe`:

| written | expands to |
|---|---|
| `GetObject` | `GetObjectA` |
| `(GetObject)` | `(GetObjectA)` |
| `SendMessage` | `SendMessageA` |
| `(SendMessage)` | `(SendMessageA)` |

`GetObject` and `SendMessage` are **object-like** macros - `#define GetObject GetObjectA`
- and an object-like macro expands on the identifier alone. Wrapping the name in
parentheses only defeats a **function-like** macro, which needs a following `(` to
trigger; that is why `(min)(a, b)` works and this does not. Fix the comment even if
nothing else changes, because it currently misinforms.

#### Why the tree works anyway

The mangling is **uniform**. `CObstacle::GetObject()`, `script.GetObject( 1 )` and every
other engine use are declared *and* called through the same macro, so the whole tree
consistently agrees the identifier is `GetObjectA`. It only bites where one translation
unit sees `windows.h` and another does not, or on a compiler with no macro at all - and
in that second case it self-resolves, since GCC sees plain `GetObject` everywhere. This
is a Windows-only concern.

#### What is actually broken: the C4273

`NDb::GetObject` is declared twice with different linkage:

| | |
|---|---|
| `System/DB.h:155` | `SYSTEM_EXPORT CResource *(GetObject)( const CDBID & )` |
| `libdb/Db.h:28` | `class CResource *GetObject( const CDBID & )`, no export macro |

Same namespace, same signature, inconsistent dllexport. That is a real one-line bug and
is independent of the macro; the macro only makes the diagnostic read as `GetObjectA`.

#### Options, none urgent

- Fix the false comment at `System/DB.h:155`. Cheap.
- Fix the `C4273` by giving `libdb/Db.h:28` the same export macro, or deleting the
  duplicate declaration.
- Rename `NDb::GetObject` so it stops colliding at all. Bigger, touches call sites, and
  retires the problem properly.

A blanket `#undef` is **not** available: about sixty sites call the real Win32
`SendMessage`. `#undef GetObject` alone would be safe - nothing calls GDI's - but a
rename is the cleaner end state.

### Everything resolves to the `-A` entry points

True, and worth stating precisely because it sounds worse than it is.

`Game/utf8.manifest` sets `activeCodePage` to UTF-8 and is wired into
`Game/CMakeLists.txt`, so the process ANSI code page **is** UTF-8 and the `-A` functions
take UTF-8 paths. Non-ASCII install paths work, on **Windows 10 1903+**. Below that the
manifest is ignored, `GetACP()` stays on the legacy code page, and the failure mode is a
path that cannot be opened - not garbled text, since localised text is UTF-16 from end
to end and never passes a narrow conversion. See [[opk2-encoding-facts]] in the session
memory for the measurements behind that.

**The gap: only `Game.exe` carries the manifest.** `B2_MapEditor` is the other
executable, and the editor manifests (`ED_B2/res/ED_B2.manifest`,
`MapEditor/res/MapEditor.manifest`) set no `activeCodePage` at all, so the editor and the
unwired utilities are still on the legacy code page. Adding the same manifest to them is
the obvious fix whenever the editor becomes buildable.

None of this applies on Linux, where narrow is UTF-8 natively.

### `#import` of a type library: what XDBWatcherClient actually is

`libdb/DBWatcherClient.cpp:4` is `#import "../XDBWatcherClient/XDBWatcherClient.tlb"`.
That one line is **1149 of the error lines** in a whole-tree Linux build, and it is
the single biggest entry in the census. Investigated 2026-08-24; nothing changed yet.

#### Why it explodes on GCC

`#import "*.tlb"` is an MSVC extension that reads a COM type library and *generates*
C++ wrappers. The generated files are checked out in the repo root, untracked, so the
output is readable without a build: `XDBWatcherClient.tlh` and `.tli`. They define
`namespace XDBWatcherClient` with `IDBWatcherClient : IDispatch`, a
`_COM_SMARTPTR_TYPEDEF` giving `IDBWatcherClientPtr`, `__declspec(uuid(...))` so
`__uuidof` resolves, and throw-on-failure wrapper methods.

GCC is not confused by the directive and it is **not** Objective-C or a C++20 module.
GCC has `#import` as a **deprecated GCC extension meaning "include once"**, the
pre-`#pragma once` idiom. Verified directly:

```
warning: '#import' is a deprecated GCC extension [-Wdeprecated]
```

and then it includes the file. Pointed at a binary `.tlb` it lexes the binary as
source text, which is where every `stray '\NNN' in program` comes from.

#### What the service is

The `.tlb` is built from a **C# project in this repo**,
`XDBWatcherClient/DBWatcherClient.cs`: a COM-visible .NET class, ProgId
`Nival.XdbWatcher`, CLSID `79a32419-46a1-4f8b-b044-13b69dad4ca0`. It connects over
**.NET Remoting** to `tcp://localhost:4300/CRemoteRefsAnswerer` and answers "which
.xdb files reference this one" - reference tracking for the editor's rename and
delete paths.

All three pieces are in the tree and none is built by CMake:

| | |
|---|---|
| `XDBWatcherClient/` | the COM shim the C++ talks to |
| `XDBRefsAnswerer/RefsAnswerer.cs` | `CRemoteRefsAnswerer : MarshalByRefObject` |
| `XDBWatcher/` | a WinForms app that hosts it |

They are .NET Framework 1.1-era `.csproj`, and `System.Runtime.Remoting` does not
exist in modern .NET at all.

#### It has never worked here, by design

The CLSID is **not registered on this machine** - checked `HKLM\SOFTWARE\Classes\
CLSID`, `WOW6432Node`, and `HKCU`, plus the `Nival.XdbWatcher` ProgId. All absent.
Registration would be `regasm`, not `regsvr32`, since it is a .NET assembly.

It goes unnoticed because the failure is swallowed on purpose. `_com_ptr_t`'s CLSID
constructor calls `CoCreateInstance`, gets `REGDB_E_CLASSNOTREG` and **throws
`_com_error`**; the constructor wraps that in `try { } catch (...) { bFailed = true; }`
and every call afterwards short-circuits on `if ( bFailed ) return EResult::FAILED`.
So the editor silently loses reference tracking and carries on.

#### Reachability

`DBWatcherClient.cpp` **is** in libdb's `SOURCES`, so it compiles into libdb.dll. But
`RegisterSingleton()` is called from exactly one place, `MapEditor/MapEditorApp.cpp:203`,
in the map editor, which cannot be built. **In `Game.exe` the singleton is never
registered, so the code is dead there.**

The header `libdb/DBWatcherClient.h` is included by three built TUs
(`EditorDatabase.cpp`, `RenameNode.cpp`, `Wrapper/ResourceManagerInternal.cpp`) and by
the editor. It contains only the interface and one declaration - **no COM**. All the
COM is in the `.cpp`, which is why excluding just that TU is enough.

`NSingleton::Singleton` returns 0 when nothing is registered (`System/Singleton.cpp:33`),
and the four consumers do not agree about that:

| site | guarded |
|---|---|
| `EditorDatabase.cpp:476` | yes, `if ( pClient = Singleton<>() )` |
| `Wrapper/ResourceManagerInternal.cpp:160` | yes |
| `RenameNode.cpp:99` | **no** - `Singleton<>()->GetReferencingObjects( ... )` |
| `RenameNode.cpp:313` | **no** |

The two unguarded ones are a null dereference. Not reachable from the game -
`GameDatabase.cpp:98` asserts the rename path is editor-only - but they are a real bug
independent of the port, and want their own commit.

#### The agreed fix, not yet applied

Exclude the TU rather than guard the file: the header is COM-free, so nothing built
needs the `.cpp`. Add `DBWatcherClient.cpp` to libdb's `SOURCES` only under `if(MSVC)`.

The predicate is **MSVC, not Windows**. `#import "*.tlb"` is a compiler extension, and
`comdef.h` / `_com_ptr_t` / `bstr_t` are MSVC's COM support; clang-cl and MinGW would
fail on it too. For the C++ side of any future guard the house convention is
`boost/predef.h` and `BOOST_COMP_MSVC` - `boost/predef.h` is already used 147 times in
the tree, mostly as `BOOST_OS_WINDOWS` in the `stdafx.h` preludes.

There is no precedent in the tree for *conditional sources* - only conditional link
libraries and source properties - so this establishes the pattern. That is the
direction CLAUDE.md already wants: it flags the D3D9 sources still sitting
unconditionally in `3Dmotor`'s `SOURCES` as a defect.

On non-MSVC this leaves `RegisterSingleton()` declared and undefined. Nothing built
calls it, so nothing fails to link; the editor is Windows-only anyway.

### Where the compile actually stops: 2 translation units, one blocker

First measured 2026-08-24, when it was nineteen translation units and four
blockers. Re-measured 2026-08-26: **three of the four are done and one is left.**

Re-measured again the same day, after the dinput work: **all four are done.**

| blocker | TUs | where | state |
|---|---|---|---|
| ~~dinput~~ | 2 | `Input`: Input.cpp, Bind.cpp | done, through SDL |
| ~~D3D9~~ | 13 | `3Dmotor` ×12 plus `Image/ImageDDS.cpp` | done, through DXVK Native |
| ~~Win32 windowing~~ | 3 | `System`: SplashScreen, WinFrame, WinCursor | done, through SDL |
| ~~`intrin.h`~~ | 1 | `3Dmotor/GLightPerVertexDispatch.cpp` | done |

Missing headers: none. **The whole tree compiles.** A whole-tree `ninja -k 0`
now stops at the link of `Game`, which is a different phase and is written up in
[Linking Game](#linking-game-what-the-compiler-wall-was-hiding).

The ~535 figure the original measurement mentioned was what queued *behind* these
rather than independent work, and that reading held: clearing the blockers and
the language defects behind them took the count 2370 to 2 over one day.

#### The two that were still small - both done since

Kept because the analysis is the record of how each was resolved, not because
either is open. `intrin.h` and `Image/ImageDDS.cpp` both compile.

**`3Dmotor/GLightPerVertexDispatch.cpp`** - 73 lines, and `<intrin.h>` is wanted only for
CPU feature detection: `__cpuid`, `__cpuidex` and `_xgetbv`, used to choose between the
SSE2, AVX and AVX2 lighting kernels. GCC has direct equivalents in `<cpuid.h>`
(`__get_cpuid`, `__get_cpuid_count`) and `_xgetbv`. A small shim in `port/` clears one TU
and needs no design decision. **This is the smallest remaining item in the tree.**

**`Image/ImageDDS.cpp`** - splits cleanly at line 340 and does **not** need a rewrite.

- Above 340 is the DDS *reading* path: `RecognizeFormatDDS`, `LoadImageDDS`,
  `DecompressARGB`, mip generation. Already portable. `Image/DDS.h` is a standalone
  `<cstdint>` header written explicitly to parse DDS "without needing to include ddraw.h".
- Below 340 is the *writing* path - `WriteDDS`, `ConvertAndSaveAsDDS`, `SaveAsDDSWithDX`,
  `ConvertAndSaveAsDDSWithDX` - and every one takes an `IDirect3DDevice9 *`. These call
  `CreateTexture`, `D3DXLoadSurfaceFromMemory` and `D3DXSaveTextureToFile` for real, so
  the include is not merely for format constants.

What makes it cheap is who calls the writing path: `ConvertAndSaveAsDDSWithDX` has exactly
one caller, `ED_Common/TextureExporter.cpp`, which is editor asset tooling.
`ConvertAndSaveAsDDS` is `static` and its only call site, `GLightmapCalc.cpp:89`, sits
inside a `/* */` block writing to `C:\test\`. `WriteDDS` and `SaveAsDDSWithDX` have no
external callers at all. The reading path is what the game uses - `LoadImageDDS` has four
callers, `RecognizeFormatDDS` two.

So the treatment is the one "build the XDBWatcher COM client only where #import exists"
used: guard the D3D half out off Windows and keep the loader. No rewrite, no new
dependency.

#### The three that were decisions rather than fixes - one left

All three were decided and done: D3D9 through DXVK Native, windowing and dinput
through SDL.


- **D3D9.** Thirteen TUs, but the whole renderer sits behind them. DXVK-native supplies a
  `d3d9.h` on Linux and is the short path; anything else is a new backend, which is a
  rewrite of `3Dmotor`. See also the D3D9 entry in the large-items table, which notes the
  `.cpp` files are still unconditionally in `3Dmotor`'s SOURCES.
- **Windowing.** Three TUs, and SDL is already a dependency (`cmake/sdl.cmake`), so the
  plumbing exists. `WinCursor` is arguably Windows-only by nature and may want a guard
  rather than a port.
- **dinput.** Two TUs. SDL's keyboard and joystick layer was the target, and it did pair
  with the windowing work, though not in the way this line expected: the two share an
  event loop so completely that the second one to arrive could not poll it. See
  [DirectInput over SDL](#directinput-over-sdl---done).

### DirectInput over SDL - done

Done 2026-08-26 in `c76c510f8`, `619b571b9` and `40101f80c`.

#### Only a third of Input.cpp was DirectInput

`Bind.cpp` included `dinput.h` and named nothing it defines: a leftover from when
the binding layer and the device layer were one file. One of the two fatals was
a deleted line.

In `Input.cpp` the device layer is `DirectInput8Create`, the two enumeration
callbacks, `SetCooperativeLevel`, `Acquire`, the buffered `GetDeviceData`, the
`GetDeviceState` resync, `DIPROP_GRANULARITY` and `GetObjectInfo`. Everything
else is platform-neutral and stayed shared and unchanged: the `kiKeyInfoList`
name table, the `actionIDs`/`nameIDs`/`idNames` registry, the `_DBLCLK`
synthesis, the POV decomposition into an X and a Y, `ReleaseKeyboardState`, the
message list and all of `Bind.cpp`.

Eight functions now form the seam, one implementation each per platform:
`OpenDevices`, `CloseDevices`, `AcquireDevices`, `IsWindowFocused`,
`PumpDeviceEvents`, `ReadDeviceData`, `ReadDeviceState`, `ReadControlGranularity`
and `ReadControlLocalName`. The DirectInput side of each is the code that was
already inline in `PumpMessages`, `ResyncDevice`, `GetControlInfo` and
`GetControlLocalName`, moved rather than rewritten, so the Windows behaviour is
readable as unchanged from the diff.

#### Windows could not switch, so this is a backend split

SDL input needs an SDL window: the queue is fed by that window's own message
loop. On Windows `SFLB1_InitApplication` creates a raw Win32 window with its own
`WndProc`. `SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER` would wrap a foreign
`HWND`, but SDL then subclasses the window proc and fights `WinFrame`'s. So the
split is `#if BOOST_OS_WINDOWS` inside the module, the shape `System/WinFrame.cpp`
already uses.

#### The codes stayed DIK_, and that was the point

An action id is `(device << 24) | code` and never leaves the process, so the
codes could have been renumbered off Windows. They were not. `kiKeyInfoList` is
two hundred lines, and a second copy keyed by `SDL_Scancode` would be two hundred
lines to keep in step by hand, in a table where one wrong row silently moves a
key's binding. `port/dinput.h` carries the `DIK_`, `DIMOFS_` and `DI8DEVTYPE_`
vocabulary off Windows plus a single table of `DIK_`/`SDL_Scancode` pairs that
both directions of the translation read, so the table can be checked by reading
it. It is the same argument `port/virtualkey.h` makes for keeping the `VK_`
spelling, and the same idiom.

Nine of the table's keys have no SDL 3 scan code and never fire: `DIK_AT`,
`DIK_COLON`, `DIK_UNDERLINE`, `DIK_AX` and `DIK_UNLABELED` are Japanese and AX
layout keys, `DIK_ABNT_C2` collides with `DIK_NUMPADCOMMA` on
`SDL_SCANCODE_KP_COMMA`, and `DIK_CALCULATOR`, `DIK_MYCOMPUTER` and `DIK_MAIL`
are launch keys SDL 3 dropped. They keep their names and their action ids. **No
cfg anywhere in the tree binds one.**

#### The mode question: SDL's queue is buffered mode

Worth being explicit about, because the three ways of reading input do not map
onto SDL the way they map onto Win32.

- **Immediate** (`SDL_GetKeyboardState`) would break the module. Every `SMessage`
  is an *edge*, the `_DBLCLK` synthesis needs the time a key went down, and the
  accumulators in `BindInternal.h` integrate `power × dt` over event timestamps.
  It is used only by `ReadDeviceState`, which asks what is held down right now.
- **`WM_KEYDOWN`-style** is the one to be wary of, but the queue is not what makes
  it bad: Win32 key messages are *cooked*, with auto-repeat folded in, the layout
  applied and unreliable left/right. SDL key events carry a raw `SDL_Scancode`.
- **Buffered** is what SDL gives: the backend appends every transition with a
  nanosecond timestamp and nothing is lost between two drains. That is
  `DIPROP_BUFFERSIZE = 1024` plus `GetDeviceData`, minus the overflow the old code
  has to detect and resync from.

Latency is unchanged: DirectInput was also drained once per frame.

#### One queue, two pumps

**This is the part that needed a decision.** `NWinFrame::PumpMessages` already
`SDL_PollEvent`s in a loop and would eat every key and mouse event before
`NInput::PumpMessages` ran. Both need keyboard events - `WinFrame` for the VK
codes the UI reads, `Input` for the physical binds - so `SDL_PeepEvents` cannot
split them.

`SDL_AddEventWatch` is what was chosen. The watch fires as each event is pushed,
so `Input` accumulates a device buffer of its own while `WinFrame` polls
normally; `PumpDeviceEvents` calls `SDL_PumpEvents` and `ReadDeviceData` drains
that buffer. Structurally identical to what DirectInput does, and no change to
`System`. The watch may run on whichever thread pumped the event, hence a mutex
covering `watchedEvents`, `dwWatchSequence`, `nMouseState` and `bWatchOverflow`
and nothing else - `WinFrame` already has `static std::mutex msgs` for the same
reason.

The alternative considered was a raw-event callback registered with `WinFrame`,
making it the sole pump. No mutex and deterministic ordering, but it moves when
Input's queue fills and makes `PurgeEvents`/`PurgeUIEvents` reach back into
`NWinFrame::PumpMessages`.

#### Four numbers that had to be preserved, not passed through

The layers above the device are calibrated against DirectInput's units, so a
naive port would have been silently wrong rather than broken:

| | DirectInput | SDL | what breaks |
|---|---|---|---|
| stick axis | `DIPROP_RANGE` ±10000 | ±32767 | `POWER_MIN_LIMIT` in `BindInternal.h` is an absolute threshold, and every `bindconfigure` coefficient is scaled against the range: 3.3× off |
| mouse wheel | 120 per notch, `DIPROP_GRANULARITY` 120 | 1.0 per notch | `ProcessCommandMessage` divides by the granularity, and `bindconfigure MOUSE_AXIS_Z 30` assumes 120: 120× too fast |
| POV | hundredths of a degree, `0xFFFFFFFF` centred | `SDL_HAT_*` bitmask | the cos/sin at `Input.cpp` reads it as an angle |
| mouse buttons | left, right, middle | left, middle, right | every shipped cfg binds the camera rotate to `MOUSE_BUTTON2` |

`FillEventInfo` reports `nParam` as a **delta** for every control type, and
`CDoubleAccumulator::Add` sums those deltas back into an absolute position. So
the SDL backend keeps its own running totals for the mouse axes and lets
`dwLastValue` do the subtraction, rather than reporting positions.

Auto-repeat is dropped: a DirectInput device reports one transition down and one
up however long a key is held, and a repeat would re-fire every event bind.

#### The joystick support is generic HID binding, and it survives

There is no dedicated gamepad code and never was. `EnumDevices(DI8DEVCLASS_GAMECTRL)`
synthesizes names - `GAMEPAD0_AXIS_X`, `JOYSTICK0_BUTTON3`, `GAMECTRL0_POV_X` -
that a user can then bind by name. Nothing outside `Input.cpp` mentions a
joystick and **no shipped cfg binds one**.

`SDL_Joystick` rather than `SDL_Gamepad`, because the original names controls
positionally by axis, hat and button index, which is the raw model. Axes 0-5 map
to `_AXIS_X/Y/Z/RX/RY/RZ`, hat 0 to `_POV`, buttons 0-31 to `_BUTTON<n>`.
`_SLIDER<n>` and `_KEY<n>` were already declined by the Windows callback.

There is **no deadzone anywhere in the input system**, and there never was:
`DIPROP_DEADZONE` is never set and the DirectInput default is zero. The only
threshold is `POWER_MIN_LIMIT = 10` in `CDoubleAccumulator::GetDelta`, which is
platform-neutral, applies to `CT_LIMAXIS` only, and is why the axis rescale above
is not cosmetic.

Enumerated once at startup, as `EnumDevices` was. A stick plugged in later is not
picked up, because the names are positional and a cfg that has been read has
already resolved them to action ids.

#### Two things found along the way

**`+cursor_x` and `+cursor_y` have no consumer.** Every shipped `input.cfg` binds
them to `MOUSE_AXIS_X`/`Y`, and no engine code anywhere reads a command by either
name. The game cursor comes entirely from `win_mouse_move` through `WinFrame`.
`MOUSE_AXIS_X`/`Y` are still live, but only for the `LALT`+button camera drag.

**The scancode lookup is a linear scan, and measurably does not matter.**
`SdlScancodeToDirectInputKey` walks 135 rows per key transition. Measured at
-O2: 9.2 ns for the keys an RTS player presses, which sit early in the table,
and 42.9 ns worst case for a key the table does not carry. It runs once per
physical transition, after the auto-repeat check, so at 100 transitions a second
- a player hammering hotkeys with rollover - the worst case costs **4.3 us of
CPU per second**, or 0.07 us out of a 16700 us frame. Mouse motion is the event
that actually arrives at 1000 Hz and it never reaches this table.

If it is ever changed, **do not reach for perfect hashing**. `frozen` and
Boost.MPH are for sparse or non-integral keys; `SDL_Scancode` is 0..511, dense
and already an integer, so the perfect hash is the identity function and a
direct-indexed `uint8_t[SDL_SCANCODE_COUNT]` beats any of them: one load, no
probe, and 512 bytes against the pair table's 1080. Neither library is vendored
and neither should be added for this.

The version to write, if the reason is readability rather than speed, derives
the lookup from the authored pair table at compile time so that
`directInputKeys[]` stays the single source of truth:

```cpp
struct SForwardTable
{
	uint8_t nKey[SDL_SCANCODE_COUNT];
	constexpr SForwardTable() : nKey()
	{
		for ( int i = 0; i < DIRECT_INPUT_KEY_COUNT; ++i )
			nKey[directInputKeys[i].scancode] = (uint8_t)directInputKeys[i].nKey;
	}
};
constexpr SForwardTable forwardTable;
```

Measured at 1.7 ns hot and 1.9 ns worst case, so 5.5x and 25.7x, on a cost that
was already 0.0004% of one core. The reverse direction wants `uint16_t[256]`,
because a scancode does not fit in a byte. About 25 lines for both, +1 KB of
`.rodata`, no dependency.

**The camera drag stalls at the screen edge off Windows.** `MOUSE_AXIS_X`/`Y`
come from `SDL_EVENT_MOUSE_MOTION`'s `xrel`/`yrel`, and in a windowed,
non-relative pointer those go to zero once the pointer reaches the edge of the
display, so a `LALT`+button camera rotate stops turning. Windows does not have
this because `System/WinFrame.cpp` calls `ClipCursor`. The fix is
`SDL_SetWindowRelativeMouseMode` or `SDL_SetWindowMouseGrab` for the duration of
the drag, which has to be reconciled with the cursor position the `win_mouse_move`
path reads. Not attempted: it needs the game running to judge, and nothing can
run yet.

**The POV angle is a delta on Windows too.** `FillEventInfo` sets
`nParam = dwData - dwLastValue` for every type, and the POV branch of
`PumpMessages` then feeds that difference to `cos`/`sin` as though it were the
angle. A pre-existing bug, not introduced by the port and not fixed by it.

### Linking Game: what the compiler wall was hiding

First reached 2026-08-26, immediately after the dinput work. **The tree compiles
in full**; `ninja -k 0` now gets as far as linking `Game` and stops there.
Twenty-three shared libraries link. One target fails.

107 undefined symbols, in five clusters. None of them is a language or a
platform-header problem, which is what the whole compile phase was:

| cluster | symbols | what it looks like |
|---|---|---|
| Granny | 54 | `third_party/uesp-esoapps` is not being linked into `3Dmotor` |
| ffmpeg | 38 | `UI` does not link `avcodec`/`avformat`/`avutil` off Windows |
| `NGfx::CPixelShader`, `CVertexShader` | 9 | `GShaderFX.cpp` is `target_sources`d only under `if(WIN32)`, and something still calls into it |
| `CTerraGen::FindCrag`, `FindCragInfo`, `FindRiver` | 3 | a source not in `SceneB2`'s SOURCES, or a missing export |
| `main` | 1 | `Game/main.cpp` defines `WinMain` |

**Ask which of these is a wiring problem before treating any of them as a port.**
Three of the five read as CMake rather than code.

### Granny, and the log that says what to port

Stubbed 2026-08-26. **This is what made `Game` link for the first time.**

Granny 3D is proprietary and ships as a Windows `.lib` and `.dll` and nothing
else. `third_party/uesp-esoapps/common/granny` carries `win32/` and `win64/`
directories and a header; there is no Linux build to link, and no system package
to ask for the way FFmpeg had one. Porting it is a project of its own, and
[opengr2](https://github.com/arves100/opengr2) is a partial reimplementation
rather than a drop-in.

**What is in the tree now is a stub that answers the first question that project
has to ask: which entry points actually matter.**

Linking already narrowed the several hundred functions `granny211.h` declares
down to the **54** this tree references. But a reference is not a call. Some of
those sit on paths the shipped data cannot reach - the shader effects were
exactly that, referenced from `GfxRender.cpp` and unreachable - and a linker has
no way to tell the difference. Only running the thing does.

So `vendor/granny/GrannyStub.cpp` defines all 54, each returning null, zero or
false, and each recording:

- that it was called, with a global ordinal, so the **order** is visible
- its **arguments**, with `char const*` logged as the string rather than the
  pointer, because that is what names the model being loaded
- a per function **count**, dumped by an `atexit` handler along with the ordinal
  at which each was first reached

Only the first eight calls of each function are written out in full.
`GrannySampleModelAnimations` and `GrannyGetWorldPose4x4` run per bone per frame
and would otherwise bury the loading sequence, which is the part worth reading.

The file is generated from the declarations in `granny211.h` and committed
rather than generated during the build, because the generator needs the header
and the current undefined-symbol list together and the second is not a build
input.

**The deliverable is `granny_calls.log`, written beside the executable.** Read it
in order: the entry points at the top are the ones to port first, and anything
that never appears is a candidate for not porting at all.

One detail worth keeping: `granny_allocate_callback` is a function pointer
typedef, and a function pointer is not an object pointer, so it cannot be cast
to `void *`. `GrannySetAllocator` takes two of them and is where the engine
installs its own allocator, so they are reinterpreted to `uintptr_t` rather than
dropped.

### D3DX, and why the shader effects needed a stub rather than a port

Settled 2026-08-26 in `d56d4db1b`. The question asked was whether the missing
D3DX shader and effect management could be reimplemented, given HLSL on one side
and SPIR-V on the other. The answer turned out not to matter, but the shape of it
is worth keeping.

**DXVK already does the translation people expect to be the hard part.** What it
takes in is compiled D3D9 shader bytecode and what it emits is SPIR-V; that is
its job. GLSL and Cg do not enter into it at any point. So an engine that shipped
prebuilt shaders would need nothing here at all.

What DXVK does not supply is **D3DX**, which is not part of Direct3D and is two
separate things:

1. the **HLSL compiler**, source to SM1-3 bytecode - `D3DXCreateEffectCompiler`
   and `ID3DXEffectCompiler::CompileShader( hFnc, "vs_1_1", ... )`
2. the **.fx effect runtime** - `ID3DXEffect`, its techniques, passes, parameter
   binding and `ID3DXEffectStateManager`

`3Dmotor/GShaderFX.cpp` uses both, because it reads `FX//GfxPS.fx` and
`FX//GfxVS.fx` as **source** and compiles them at run time. Both pieces do exist
in the open if this is ever revisited: **vkd3d-shader** has an HLSL compiler that
targets d3dbc, which is exactly the SM1-3 bytecode DXVK then consumes, and
**Wine's d3dx9** has an `ID3DXEffect`. It is mapped territory, not research.

**None of that was needed, because the path is dead three times over**, and each
reason is on its own sufficient:

1. **Nothing turns it on.** `InitShaderFX` is reached from one place,
   `GSceneInternal.cpp`, inside `if ( bIsTwilight )`. `bIsTwilight` is set false
   in `GView.cpp:371` and the only line that would ever set it true,
   `pScene->SetTwilight( rand() & 1 )`, is commented out at `GView.cpp:1205`.
   `IView::SetTwilight` has no caller outside `3Dmotor`.
2. **The shaders do not ship.** Neither `FX/GfxPS.fx` nor `FX/GfxVS.fx` exists
   anywhere in this repository, in the data or in a pak. The two `.fx` files that
   do exist, `3Dmotor/GfxMainPS.fx` and `GfxMainVS.fx`, are different shaders
   under different names, are not installed, and contain **no `technique` block
   at all**, so they are not even in the format `D3DXCreateEffect` reads.
3. **If it ran it would crash.** `InitShaderFX` logs "Couldn't open file" for
   each missing `.fx` and then returns **true** with `pPSEffect` and `pVSEffect`
   left null, and `CPixelShader`'s constructor dereferences `pPSEffect` with no
   check. `GfxRender.cpp:1360` dereferences `pVShader` unguarded as well.

So `Monochrome`, `Twilight` and `GausianBlur` in `GPostEffects.cpp` have never
run in this tree on any platform. **The stub is what the Windows build already
does, not a reduction of it.** Point 3 is a latent null dereference on Windows
that is worth fixing on its own terms if the flag is ever wired up.

This is the third time a cluster that looked like the hardest thing on the board
turned out to be unreachable, after the font rasteriser and the GameX font
generator. Ask the question first, every time.

#### The four checks that were put to this afterwards, and what they found

Each was a reasonable way the conclusion could have been wrong. None weakened it
and three strengthened it.

**Could `bIsTwilight` arrive true from a savegame, a replay, a map, a mod or a
multiplayer command?** It is serialised, in `CScene` as chunk 26 and in `CView`
as chunk 29, so the question is a fair one. **No.** For any stream to carry true,
some build must have written true, and none ever did: in the **pristine released
source**, `origin/main`, `GView.cpp:364` sets it false and `GView.cpp:1183`
already reads `//pScene->SetTwilight(rand()&1);`, commented out exactly as it is
here. Beyond that it is not exposed to Lua or the console, `types.xml` contains
the word "twilight" zero times so no `.xdb` can reach it, nothing outside
`3Dmotor` mentions it at all, and it is presentation state in `CView`, which
lockstep never transmits because only player commands go over the wire.

**Is `ShaderCompiler/` the real shader toolchain, and is `GfxShaders.txt` its
source?** Yes to both, and it is a **different system that already works off
Windows**. `GfxShaders.txt` is a shader *assembly* macro language - `mad`,
`m4x4`, `func`/`locals`, register names - which `ShaderCompiler` turns into the
D3D9 bytecode arrays committed in `3Dmotor/GfxShaders.cpp` (`0xfffe0101` is
vs_1_1, `0xfffe0200` is vs_2_0). `GfxRender.cpp` hands those straight to
`CreatePixelShader` and `CreateVertexShader`, and **DXVK compiles them to
SPIR-V**. No D3DX anywhere on that path, at build time or run time.

**Did the developers wire that compiler into the build?** They wired it and then
switched it off. `origin/main`'s `3Dmotor.vcproj` carries a custom build step on
`GfxShaders.txt` invoking `ShaderCompiler.exe` from a network share, producing
`GfxShaders.h` and `GfxShaders.cpp`, with `ExcludedFromBuild="TRUE"` in **every**
configuration: Debug, FastDebug, BetaRelease and Release. It was run by hand and
its output committed, which is why the generated files are in the tree and why
nothing has to run it today.

This also corrects a loose end. `GfxMainPS.fx` and `GfxMainVS.fx` are **not
orphans**: they are the inputs named by `GfxShaders.txt`'s `[HLSL] Main` section
and become the `vsHLSLMain0` and `vsHLSLMain1` bytecode. That is why they carry
no `technique` block - they were never effect files. And `Monochrome`,
`Twilight`, `GausianBlur` and `Render2DVS` appear nowhere in `GfxShaders.txt`
either, so not even the build-time shader source ever carried the three dead
effects.

**Do any retail installs ship the missing shaders, so that bytecode could be
recovered?** No. Fall of the Reich's ten paks were searched, `data.pak` among
them at 1.5 GB, for `GfxPS`, `GfxVS`, `technique`, `vs_1_1` and `ps_1_`, in ANSI
and in UTF-16. Zero hits. The method was validated first against known content in
the same file - `.xdb` appears 323407 times, `.dds` 16461, `Effects` 14659 - so
plaintext names are greppable there and the absence is real rather than an
artefact of the archive format. The other install has no `FX` directory either.

Decompilation would not have been the obstacle in any case: DXVK consumes D3D9
bytecode directly, so recovered bytecode would need no decompiling to run.
Decompiling would only ever be for making it readable again.

### An inline member function defined in a .cpp is a Linux-only link error

Found 2026-08-26 while clearing `Game`'s link, and worth knowing because MSVC
does not report it and GCC does.

`CTerraGen::FindCrag`, `FindCragInfo` and `FindRiver` are declared in
`SceneB2/GenTerrain.h` and were defined `inline` in `CragsBuilder.cpp` and
`RiversBuilder.cpp`. An inline function must be defined in **every** translation
unit that uses it, so `PrecipicesManager.cpp`, which calls `FindCragInfo`, had
nothing to bind to. MSVC emits an out-of-line copy of an inline function it has
the definition for, which is why the Windows build has always linked; GCC emits
nothing and reports three undefined references. Fixed in `3fa976f8d` by deleting
the keyword, which also makes the three agree with `FindRiverInfo` and
`FindRiverGfxInfo` sitting beside them in the same files, neither of which was
ever marked inline.

**The shape is still in the tree, and where it remains it is harmless.** Thirteen
member functions are still defined `inline` in a `.cpp`, almost all of them the
`CBetaSpline3D` helpers in `Misc/Spline.cpp`. They link because every caller is
in that same file. The rule to carry forward: an `inline` member defined in a
`.cpp` is fine only while nothing outside that `.cpp` calls it, and the day
someone adds a caller elsewhere the Windows build will stay green while the Linux
one breaks. Do not read a green MSVC link as proof the definition is reachable.

### The bison/flex parser: what it is, and why it is not regenerated

Investigated 2026-08-24 after `lang.y` turned out to need `<cstdarg>` like everything else.

#### What the grammar is for

`Parser/lang.y` and `Parser/lang.l` are the front end of the **database schema
compiler**. They parse a C-like type definition language - `class`, `struct`, `enum`,
`typedef`, `attribute`, `newtype`, with `#include` and `h_external` / `cpp_external`
directives - into an `NLang::` AST. `libdb`'s `NCompileCLike::Compile` turns that into
type definitions and `NCodeGen::GenerateCode` emits the `DB*.h/.cpp` structs. That is
where the "automatically generated, don't change manually" banner on those files comes
from, and it is the sibling of `types.xml`.

**Its only consumer is `dbcodegen`, which has no CMakeLists.txt and is not built.** No
built module calls `NLang`, `NCompileCLike` or `NCodeGen` at runtime - checked across
`Game`, `GameX`, `Main`, `AILogic`, `SceneB2`, `B2_M1_World` and `Script`. The game loads
the prebuilt `types.xml` and binary database and never parses schema. `Parser` is linked
into `Game.exe` and never invoked.

#### The trap

`Parser/CMakeLists.txt` compiles **`lang.tab.c`**, the committed bison output, and sets
its `LANGUAGE` to `CXX`. Nothing runs bison or flex at build time. So **editing `lang.y`
alone does nothing** - the build will not change and the fix will look like it failed.
The same goes for `lang.l` and `lex.yy.c`. Edit both, as "include cstdarg in the parser
grammar too" did.

#### Regenerating works, if it is ever needed

The natural fear is that the committed output came from bison 1.24 (1993) and flex 2.5,
and that a modern generator would produce something subtly different. Measured, with
bison 3.8.2 and flex 2.6.4:

| | |
|---|---|
| `bison -d -o lang.tab.c lang.y` | clean, zero warnings |
| `flex -o lex.yy.c lang.l` | clean, zero warnings |
| token numbering | **identical** - `CLASS=258`, `IDENTIFIER=259`, `NUMBER=261`, `STRING=265` |
| the `yyerror` prologue | carried through verbatim; bison 3 emits no competing prototype |

The grammar is minimal - 31 `%token`s and a prologue, no `%union`, no `%pure_parser`, no
`%type`, no `YYPARSE_PARAM` - so there is nothing in it that bison 3 removed. The output
grows from 1676 to 2315 lines because the modern skeleton is larger, not because anything
about the language changed.

Also worth knowing: a regenerated parser is *semantically* equivalent to the old one by
construction. Bison guarantees the generated parser accepts the language the grammar
describes; it never guaranteed byte-identical tables. "We would not reproduce the exact
state machine" is true and does not matter.

#### Why it is still not wired into the build

Not reproducibility - payoff. `lang.y` has two commits in its life, the original import
and the `<cstdarg>` one. Its consumer is not built. Nobody regenerates it. Wiring bison
and flex in would put both tools on every contributor's machine and in CI, and on Windows
that means `winflexbison` or MSYS, to automate something that has happened twice in
twenty years.

The same argument answers the modern-parser question. `re2c` and `re-flex` are still
build-time generators, so they trade one tool dependency for another rather than removing
it. Only a header-only approach such as Boost.Spirit X3 removes the tool, and that means
hand-rewriting a 465 line grammar with real semantic actions, for a component that is
currently dead in the build and has no test corpus to validate a rewrite against.

If the schema language ever does need to change, the measurements above say the path is
short: install bison 3 and flex 2.6, regenerate both files, rebuild.

### One GCC message, several unrelated causes

Worth reading before acting on any error count from the build log. Twice now a single
diagnostic has turned out to cover jobs that have nothing to do with each other, and the
raw count was two to four times the real work.

#### `there are no arguments to X that depend on a template parameter`

49 sites when first counted, and four different problems:

| cause | sites | fix |
|---|---|---|
| cascade from `3Dmotor/Cache.h`'s broken typedefs | 27 | fix that file's root defect |
| zlib API called without including `zlib.h` | 8 | the include |
| genuine dependent-base members | 7 | `this->` or a `using` |
| a name in `NDb` used from global scope | 1 | qualify it |

After `Cache.h` and the zlib includes landed, 19 were left.

Those 19 went on 2026-08-26 in `f292aedab`, `AreaMap.h`'s `GetSizeX`/`GetSizeY`,
`StaticObjectsIters.h`'s `GetAreaMap`/`GetCellSize` and `ListsSet.h`'s
`IncreaseQueuesNum`/`GetQueuesNum`/`DelQueue`. GCC reported them at exactly
fourteen file:line pairs in three files and nowhere else, which a static scan of
all 22 class templates in the tree with a dependent base agreed with.

#### `X was not declared in this scope`

292 sites, of which only 74 carried `[-Wtemplate-body]` and were the dependent-base class
at all. The other 230 are ordinary undeclared symbols in non-template code - the Win32
surface, `SplashScreen`, `WinFrame`, the font handling, `MainLoopInternal`, `SysKeys` -
and belong with the HWND work. **Filter on the `-Wtemplate-body` marker** or the number
means nothing. After `Cache.h`, 35 of the 74 remain.

The dependent-base half of that 35 is gone as of `f292aedab`: `cListsSet` and
`currentPos` in `ListsSet.h`. `f875f6357` took `NMath` in
`StaticObjectRotation.h`, which carried the marker but was not the dependent-base
class at all, only a header that named a namespace it did not include.

#### A missing-header fatal is a floor, not a count

Added 2026-08-26, and it is the sharpest version of this whole section. GCC stops
a translation unit at a `#include` it cannot resolve, so **every error below that
line in that file is invisible until the include is fixed.**

Deleting one `#include <crtdbg.h>` from `Game/main.cpp` in `6c73c6ad4` took the
whole-tree count from 36 to 42. Nothing regressed; seven errors that had been
masked all day became visible at once.

So while any `fatal error: ... No such file or directory` remains in a build log,
the error count is a lower bound on the work, not a measure of it. Count the
fatals separately and say which ones are still masking, especially before
telling anyone how much is left.

The last two were `dinput.h` in `Input/Input.cpp` and `Bind.cpp`, hiding 2081
lines. When they went the compile phase ended entirely rather than revealing
more: `Bind.cpp` needed nothing behind the include, and `Input.cpp` needed the
device layer replaced and nothing else. **A fatal is a lower bound, not a
prediction; it can hide a great deal or nothing at all.**

#### The lesson

Grep the *diagnostic text plus its warning flag*, count **distinct file:line** rather than
occurrences - a header included by 300 translation units reports 300 times - and read a
sample before believing a class is uniform. `3Dmotor/Cache.h` alone was inflating both
counts above by roughly half, from three broken lines.

### zconf.h is not a porting bug

129 files include `<zconf.h>` and 10 include `<zlib.h>`, which looks wrong and is not.
"include zlib as a system library" faithfully rewrote the original's
`#include "../zlib/zconf.h"`, and the original Nival tree included that header in 129
files, most of which never touch zlib - `InterfaceCredits`, `MouseTranslator`,
`WeaponMnemonics`. Cargo cult from 2005. Harmless as well: zlib-ng is configured with
`ZLIB_COMPAT`, so `zconf.h` exists and resolves.

The real defect was narrower: seven files *called* the API without including `zlib.h`,
which has the prototypes where `zconf.h` has only types and macros. Fixed in "include
zlib.h in the files that call zlib". Six of the seven compiled only because some other
header in the same translation unit pulled `zlib.h` in first.

**That "works by luck of include order" shape is worth watching for generally.** It is the
same thing behind the `System/GResource.h` and `libdb/Checksum.h` fixes: MSVC's late
lookup plus a fortunate include order hiding a missing declaration. Expect more as further
modules start compiling.

Removing the other 122 `zconf.h` includes is cleanup, not a fix, and wants a build per
file to prove nothing leans on the transitive include - which is the very mistake above.
Not worth doing blind.

### Two shapes the compiler has to find for you

Two classes on this branch cannot be swept by reading the source, and both were found by
reading the build log instead. Worth knowing which those are, because reaching for grep
on them wastes an afternoon.

#### `enum X` naming an enum nothing has declared

Done, in "declare the enums that are only ever named through an elaborated specifier".

`enum EAlphaMode2D` in a parameter list is an MSVC extension when nothing has declared
EAlphaMode2D: MSVC invents the type. C++ allows an unscoped enum to be named before its
definition only with a fixed underlying type, so a bare `enum X;` will not do either -
the declaration has to be `enum X : int;` and the definition has to agree.

**The spelling is not the problem.** `enum X` appears about 180 times in the tree and is
ordinary C++ wherever X is already visible. Only 18 named one that was not, across 10
files, and which those are depends on what each translation unit includes. Grep cannot
tell them apart; `grep "use of enum" ` over a keep-going build can.

The fix is declarations only - once the enum is declared the existing use sites are
legal, so none of the 18 needed touching. Seven enums gained an underlying type and
twelve headers gained a forward declaration, 25 lines added in total.

Two things to carry forward if more of these appear:

- **Name the type MSVC already picks**, so the Windows build cannot move: `int`, unless
  an enumerator does not fit, in which case both compilers were already choosing
  `unsigned int` anyway. `EUnitStatus` is the one that mattered - its `EUS_` values reach
  `0xFFFF0000` and `: int` would not have compiled. `ESoundMixType` is the near miss at
  exactly `0x7fffffff`.
- **Fixing the type also settles a disagreement that was already there.** For an enum
  with no negative enumerators GCC picks `unsigned int` where MSVC picks `int`, so those
  seven types differed between the two builds before this.

Forward declarations rather than includes, deliberately: none of the ten files included
the defining header, and the elaborated spelling is how they avoided it. Includes would
have put `Sound/SoundScene.h` into `B2_M1_World`, `UI/UI.h` into `GameX` and
`Stats_B2_M1` into `B2_M1_World`.

#### `?:` between mutually convertible types

The other one. See
[`?:` between two mutually convertible types](#-between-two-mutually-convertible-types)
for why a source scan produced thirty candidates of which one was real.

### `?:` between two mutually convertible types

Done for the ten sites known today, in "give each ambiguous ?: a single operand
type". This one is **not** finished the way a sweep is finished, and the section
is here to say why.

GCC rejects a conditional whose branches have different types when each converts
to the other: neither conversion is preferable, so there is no composite type.
MSVC picks one silently. Every type involved has both a non-explicit converting
constructor and a conversion operator back:

| type | in | out |
|---|---|---|
| `CPtr<T>` / `CObj<T>` / `CMObj<T>` | `CPtrBase( T * )` | `operator T *()` |
| `CDynamicCast<T>` | `CDynamicCast( TT * )` | `operator T *()` |
| `CDBPtr<T>` | `CDBPtr( const T * )` | `operator const T *()` |
| `SAIAngle` | `SAIAngle( int )` | `operator int()` |
| `CVariant` | `CVariant( int )`, and nine more | `operator int()`, `float`, `bool` |

**What MSVC picks.** Compiling the three shapes against `cl.exe` and reading the
`decltype` back says MSVC takes the **class** type every time - `Variant`, not
`int`; `SAIAngle`, not `int`; the wrapper, not `T *`. Worth knowing before
changing any of these, because the class-type composite is not always harmless:

- It builds a temporary wrapper around the *raw* operand and destroys it at the
  end of the full expression. For `CPtr` that is an `AddRef`/`ReleaseRef` round
  trip, and `ReleaseRef` deletes the object when both counters reach zero. In
  `Parser/LangNodesDefinitions.h` the temporary wrapped `this`.
- For `CDBPtr` the conversion back out calls `LoadObject()` again.
- For `CVariant`, had MSVC picked `int` instead, `libdb/TypeDef.cpp` would have
  turned an empty enum's `VT_NULL` into `VT_INT` 0, because `operator int()`
  maps `VT_NULL` to 0.

**Why this cannot be scanned for.** Two reasons, both fatal to a source-level
sweep:

1. The branch types nearly always come from another file - a member declared in
   a generated `RPGStats.h`, a method whose return type is three headers away.
   Attributing a type to `pFoo->Bar()` by name alone gets it wrong more often
   than right.
2. `wrapper ? x : 0` is **well-formed**: `0` is a null pointer constant, so it
   converts to `T *` and there is no ambiguity. Confirmed against GCC directly.
   Most conditionals involving these wrappers are exactly that shape, so a scan
   that flags them drowns the real hits.

A two-pass scan over the whole tree - collect every name declared with a wrapper
type and every function returning one, then look at each conditional - produced
thirty candidates, of which **one** was real (`ED_B2_M1/XMLExport.cpp:379`, in
the editor, which GCC never compiles). The other nine came from GCC itself.

**So the way to find the rest is to compile.** The keep-going Linux build
attempted 1276 translation units, which is most of what builds, but not the ones
that die earlier for other reasons, and not the editor or the unwired targets.
Every time a module starts compiling further, re-check
`grep "operands to .?:." ` over the build log.

#### `SAIAngle` converts to itself

Noticed while fixing the above and left alone, since it is a different defect:
`Common_RTS_AI/AIAngle.h:25` declares `operator SAIAngle() const`, a conversion
function to the struct's own type. GCC warns
(`-Wclass-conversion`, "converting SAIAngle to the same type will never use a
type conversion operator"); such a function can never be called. Deleting it is
a one-line change with no callers to update, but it is unrelated to the `?:`
work and belongs in its own commit.

### Two-phase lookup: the sweeps are not finished

GCC 14 diagnoses the body of a template that is never instantiated, under
`-Wtemplate-body`. MSVC parses those bodies late and only ever looks at the
ones something actually uses, which is why a decade of these sat invisible and
why they all surface at once now. It also means "the header is included by a
built target" is not the same as "this member is compiled": most of the sites
below are in members nothing instantiates, and they still have to be fixed,
because GCC reads them regardless.

Three `typename` passes were needed, and all three are done. The class-scope
explicit specializations are not started.

#### Pass 2 of the typename sweep: dependent scopes built from a template-id

Done, in "add the typename a dependent type name needs when the scope is a
template-id".

`1cc33a07b add the typename a dependent type name needs` only caught the bare
form, `TArray::const_iterator`, because that is the shape its scanner looked
for. It missed the commoner one, where the scope is a *template-id* that
mentions a parameter: `std::vector<T>::const_iterator`. Sixteen of those were
left, in `AILogic/ListsSet.h`, `B2_M1_Terrain/TerrUtils.h` (2),
`Image/Targa.h`, `libdb/Checksum.h`, `libdb/ObjMan.h`, `Misc/BiMap.h` (2),
`Misc/Sync.h`, `Misc/TypeConvertor.h` (2),
`MapEditorLib/Tools_MnemonicsCollector.h` (3) and
`SceneB2/TerrainGfxObserver.cpp` (2). Eleven are in modules that build; the
other five are in the editor and in `Misc/BiMap.h`, which nothing includes.

Three corrections came out of rerunning the scan, and they are the reason the
list this file used to carry is not the list that was fixed:

- `AILogic/ListsSet.h:361` was missing from it. `CQueuesSet<T>::Push` names
  `CListsSet<T>::tEnumerator`, a *different* template, so it is dependent and
  GCC rejects it.
- `libdb/ObjMan.h:87` was on the list but the scanner did not report it. The
  scope there is `TContainer<TValue>`, where `TContainer` is a
  *template-template* parameter, and the check only asked whether the
  arguments mentioned a parameter. A scope that is itself a parameter is
  dependent too, so the check has to test both.
- `3DLib/Ring.h:125` was on the list and should not have been.
  `CRing<Type>::const_iterator`, written inside `CRing<Type>` itself, names a
  member of the *current instantiation*, which is found without `typename`.
  GCC 15 compiles that line as written, so it was left alone and any rerun
  will report it again.

Four more near misses to leave alone, all dependent *values* rather than
types, so they neither need nor tolerate `typename`:
`System/MetaProg.h:43`, `:44` and `:48`, inside `SUPERSUBCLASS` and
`SUPERSUBCLASS_STRICT`, read `NMeta::CConversion<...>::exists` and
`::sameType`; `Misc/StrProc.cpp:28` compares against
`std::basic_string<T1>::npos`.

The scanner is the widened form of the pass-1 one: collect the type-parameter
names declared in a file, then match `Scope< ... >::Member` in the three
positions only a type can occupy - declarator, pointer or reference
declarator, and parenthesised cast - and skip a match already preceded by
`typename`. Treat a match as dependent if either the arguments mention a
parameter or the leftmost component of the scope is itself one. The lookbehind
has to anchor on the *leftmost* component of the qualified name; anchoring on
a word boundary lets it restart after the `::` in `typename std::vector<T1>`,
which reports nine already-correct sites as missing. Collecting the parameter
names needs a balanced-bracket scan rather than a regex, or a
`template <template <typename TYPE> class TContainer, typename TValue>` header
yields only `TYPE`.

#### Pass 3 of the typename sweep: a dependent member typedef used as a scope

Done, in "add the typename a dependent member typedef needs when it is used as
a scope".

Passes 1 and 2 both keyed on a template *parameter* appearing in the qualified
name. The shape neither looked for is a member of the enclosing class template
used as a scope, whose own definition is dependent:

```cpp
typedef std::unordered_map<CPtr<T>, int, SPtrHash> CStuffHash;
...
CStuffHash::iterator i = t.find( p );
```

The name is found without help, because it is a member of the current
instantiation, but what it names is dependent, so its members are not known
until instantiation and `typename` is still required. GCC says "because
`CSetSyncSrc<T>::CStuffHash` is a dependent scope".

Forty-one sites in ten files. Eighteen are in modules that build - `3Dmotor`
(5), `GameX` (2), `Misc` (11) - and twenty-three are in `MapEditorLib`. GCC
reported seventeen of them itself once the earlier passes stopped masking the
files they were in; the rest are in headers no Linux translation unit gets far
enough to read, or in the editor.

The scanner walks each class template's body rather than the file: a nested
class or struct is dependent outright, and a typedef is dependent when its
right-hand side mentions a parameter or an already-dependent name, to a
fixpoint. Both halves are load-bearing. The transitive step is what reaches
`3Dmotor/GRTShare.cpp`, where `CTexHash` is a hash of `STexInfo` and only
`STexInfo` mentions `T`. Scoping to the class template body is what keeps
`3Dmotor/GfxBuffers.cpp:862`, `:877` and `:884` out: they read
`CCache::SStatePlace` exactly like the three lines fixed above them, but that
`CCache` is a typedef in `CTextureCache`, a plain class, so those names are not
dependent and `typename` would be wrong there. A file-wide collection of
typedef names reports all three.

The scan also has to accept a template argument as a type position, not only a
declarator, or it misses `std::vector<CCache::SStatePlace> places;`.

#### `typename typedef` in 3Dmotor/Cache.h

Three lines in `3Dmotor/Cache.h` put `typename` before `typedef`:

| line | as written | intended |
|---|---|---|
| 25 | `typename typedef uint32_t pointer;` | `typedef uint32_t pointer;` |
| 114 | `typename typedef TAlloc<SElement> Alloc;` | `typedef TAlloc<SElement> Alloc;` |
| 115 | `typename typedef Alloc::pointer pointer;` | `typedef typename Alloc::pointer pointer;` |

`typename` there introduces an elaborated type specifier and needs a qualified
name after it, so none of the three is well-formed; MSVC takes them anyway.
Line 115 is the only one that wants a `typename` at all, and it wants it after
the `typedef`. Line 25 is not in a dependent context in the first place.

Fixing them is three lines, but it does not make the file compile: GCC's
cascade from line 114 hides a second problem underneath, the dependent-base
member lookups that `bb76e044b` fixed for the matrix stack templates
(`pNext`, `pPrev`, `pUp`, `pDown`, `alloc`, `GetSizeX`, `GetSizeY`). Do the
whole file in one commit.

#### `hash_map` is pre-C++11 MSVC, and BiMap.h is dead

`hash_map` is the old MSVC/SGI non-standard container that predates
`std::unordered_map`. It is not declared anywhere in this tree any more, and
`Misc/BiMap.h` has no includers at all, so both of its sites are in a header
nothing compiles. The original code for it lives on `main`. Do not add a shim:
if either header is ever revived, the type it wants is `std::unordered_map`.

#### The remaining 20 class-scope explicit specializations

`755fb8dc1` moved `CBitStream`/`CDataStream`, `74af7b6b1` moved
`IXmlSaver::CallObjectSerialize`, and "move IObjMan's SetValue and GetValue
specializations to namespace scope" moved `libdb/ObjMan.h`'s sixteen, out of
class scope, which is where an explicit specialization is ill-formed and where
MSVC accepts it anyway.

**The count of thirty-nine this file used to carry was wrong.** Three of them
specialize at namespace scope already and are perfectly well-formed:
`SceneB2/SceneData.cpp:15` (`__fill_tga_header`, inside `namespace NImage`) and
`ED_B2_M1/DBMinimap.h:157` and `:171` (`SKnownEnum`, at global scope). The real
total was thirty-six. Twenty are left, and **none of them is in a target that
builds**:

| file | count | built |
|---|---|---|
| `MapEditorLib/ManipulatorManager.h` | 12 | no - editor |
| `MapEditorLib/ObjectBaseController.h` | 4 | no - editor |
| `ShaderCompiler/Streams.h` | 4 | no - no CMake wiring |

`ShaderCompiler/Streams.h` is a second copy of the `CBitStream` and
`CDataStream` that `755fb8dc1` already fixed under `System`, so that one is the
same edit applied to a duplicate rather than fresh work.

These are **not** the same shape as the earlier batches, and should not be moved
by script. `CallObjectSerialize` was one member template specialized thirteen
times over built-in types with identical bodies. These specialize on class
types, and several in `ManipulatorManager.h` are `static`.

The recipe, from the commits that did it: move the specialization to just after
the closing `};` of the class, qualify it with `TheClass::`, and mark it
`inline`. Neither half is cosmetic. Qualifying keeps the function a *member*, so
it still reaches the private helpers and sibling members the body is written in
terms of. `inline` is needed because an explicit specialization of a function
template is **not** implicitly inline the way an in-class definition is, so
without it a header included by more than one translation unit gives the linker
duplicate definitions. A `static` member specialization keeps the `static` on
the in-class declaration and drops it from the out-of-class definition.

What `ObjMan.h` added to the recipe: two of its four primaries take a
`template <typename> class TContainer` parameter and are specialized on
`std::basic_string`, which has three template parameters. That match depends on
the relaxed template-template matching of C++17, so it was not obvious it would
survive the move. Modelling the four primaries in a standalone file and
compiling it under both `cl` and `g++` settled it in a minute; both accept it.
Worth doing again for `ManipulatorManager.h`, where the `static` members are the
unknown.

One thing `ObjMan.h` did **not** deliver: despite the name it is not included
widely. Ten translation units, all inside `libdb`, plus the editor and the
unwired `TestDB`. Moving it unblocks libdb compiling, not a fan-out of headers.
