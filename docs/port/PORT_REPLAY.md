# The 44-commit replay

The commit table and what each replayed commit turned out to involve. This is the record of work already done.

Part of the Linux port notes; see [LINUX_PORT.md](../../LINUX_PORT.md) for the index.

---

## The 44 commits

Review with `git show <sha>`, apply with `git cherry-pick <sha>`.

| # | done | sha | files | what | gate | notes |
|---|------|-----|-------|------|------|-------|
| 1 | [x] | `9be30ecc6` | 5 | fix case-sensitive file names in CMakeLists.txt | `none` | Windows FS is case-insensitive; cannot break MSVC |
| 2 | [x] | `c6a1f46ad` | 88 | do not include Windows-specific headers on other platforms | `build` |  |
| 3 | [x] | `b416e7b59` | 17 | replace non-portable __forceinline with BOOST_FORCEINLINE | `build` |  |
| 4 | [x] | `800855c13` | 6 | replace MSVC-specific __stdcall with portable macro | `build` |  |
| 5 | [x] | `b1ef6440e` | 46 | declare STime in a portable manner | `run+sim` | STime feeds the lockstep clock |
| 6 | [x] | `177843659` | 903 | use portable types (cstdint) instead of Windows-specific | `build (x86!)` | pointer/int width - x86 is where this breaks |
| 7 | [x] | `688cc54b0` | 1 | remove ReportAssertHR definition | `none` | **skipped**: already removed in the base by `2f906948f` (crashpad), so the cherry-pick is empty |
| 8 | [x] | `f8ffbfb44` | 15 | replace MSVC-specific __cdecl with portable macro | `build` |  |
| 9 | [x] | `34d9ad599` | 21 | use portable integer types (cstdint) instead of Windows specific | `build` |  |
| 10 | [x] | `a6527f34f` | 4 | type_info -> std::type_info | `build` |  |
| 11 | [x] | `a13479f46` | 1 | replace custom bit manipulation routines with boost::core | `build` |  |
| 12 | [x] | `d06172668` | 1 | correctly delete assignment operator | `build` |  |
| 13 | [x] | `5700a506c` | 2 | move explicit template specializations to the namespace scope | `build` |  |
| 14 | [x] | `3d33edfaa` | 25 | do not use non-trivial types in union | `run` | non-trivial union members change object semantics |
| 15 | [x] | `e14b2770f` | 481 | replace non-portable dllimport/dllexport with CMake export macro | `build` | export macros change linkage on both arches |
| 16 | [x] | `50c17582f` | 1 | include x86intrin.h for __rdtsc on linux | `build` | adds <intrin.h> on the MSVC path |
| 17 | [x] | `5b482f6cd` | 1 | replace non-portable itoa with std::to_string | `run` |  |
| 18 | [x] | `e328e46b4` | 1 | replace non-portable _wtof/atof with std::stof | `run` | atof->stof: locale-sensitive parsing |
| 19 | [x] | `c4b4212e9` | 1 | replace non-portable Sleep with std::this_thread::sleep_for | `run` |  |
| 20 | [x] | `6a087b510` | 1 | replace non-portable __declspec(noinline) with BOOST_INLINE | `build` |  |
| 21 | [x] | `2271d5e5a` | 2 | fix joining preprocessor tokens | `build` |  |
| 22 | [x] | `c78aec19d` | 1 | fix extra qualification on member | `build` |  |
| 23 | [x] | `31dfb331a` | 39 | replace Windows-specific GetTickCount with std::chrono | `run+sim` | epoch AND integer width change; watch water/wind/animated textures |
| 24 | [x] | `095ae3bb9` | 3 | remove GetAddressByClientID as it's not used anywhere | `run` | dead-code removal |
| 25 | [x] | `b73dba7f9` | 291 | replace custom format function StrFmt with fmt::format | `run+sim` | 291 files of format-string translation; a mistranslation is silently wrong text |
| 26 | [x] | `20a674b60` | 1 | update .gitignore | `none` |  |
| 27 | [x] | `79606f7c3` | 35 | exclude Windows-specific stuff from build | `build` |  |
| 28 | [x] | `b17173dac` | 2 | add missing cstdarg header | `build` |  |
| 29 | [x] | `d7b0c232a` | 1 | make DbgTrace portable (write to stderr), debug output is Windows-only concept | `run` | DbgTrace output path |
| 30 | [x] | `a3e92fb35` | 24 | define NOMINMAX, min/max macro from Windows.h conflicts with Boost headers | `build` | NOMINMAX can break code that uses the min/max macros |
| 31 | [x] | `770769d00` | 5 | reformat CMake | `none` |  |
| 32 | [x] | `366eb1f23` | 30 | replace Windows-specific GUI with boost::uuid | `run+sim` | changes IXmlSaver::DataChunk vtable; libdb is in the normal build |
| 33 | [x] | `568e82e59` | 1 | fix pointer truncation | `none` | low 4 bits survive truncation either way - identical behavior |
| 34 | [x] | `36eb5aa52` | 8 | replace custom critical section class with std::mutex | `run+sim` | threading primitive under the lockstep sim |
| 35 | [x] | `7d923c82b` | 6 | replace MSVC-specific __int64 with int64_t (cstdint) | `build (x86!)` | pointer/int width - x86 is where this breaks |
| 36 | [x] | `3b984fa06` | 1 | remove unused function TypeReq | `build` |  |
| 37 | [x] | `5b0709dcc` | 11 | replace Windows-specific WCHAR with wchar_t | `build` |  |
| 38 | [x] | `2e793a47e` | 291 | use portable types (cstdint) instead of Windows-specific | `build (x86!)` | pointer/int width - x86 is where this breaks |
| 39 | [x] | `c916da45f` | 7 | replace FileTime functions with boost::filesystem | `run` |  |
| 40 | [x] | `aab481398` | 11 | replace NFile::CreatePath with std::filesystem::create_directories | `run` |  |
| 41 | [x] | `411de6e46` | 7 | replace __declspec(dllexport) with CMake macro | `build` |  |
| 42 | [x] | `b7a76ff05` | 2 | replace non-portable integer types with cstdint | `build` |  |
| 43 | [x] | `098555bd0` | 12 | replace Windows-specific BOOL with regular bool | `run` | BOOL->bool changes any '== TRUE' comparison |
| 44 | [~] | `accc2c5f9` | 42 | remove redundant ./ for includes | `build` | **absorbed** into the include-case pass, see below; every one of its 42 lines is also case-wrong |
| 45 | [x] | `3c9c49bd9` | 5 | replace custom memory mapping with boost::interprocess | `run+sim` | **rewritten, not cherry-picked**; the original produced out of bound reads, interprocess exceptions and short reads. See below |
| 46 | [x] | `dcbe48b67` | 4 | replace SKillLoaderThread with portable std/boost functions | `run` | **rewritten**: the commit as written busy-waits the loader thread; std::condition_variable used instead of Boost.Thread. See below |
| 47 | [x] | `89da73090` | 6 | replace Windows-specific _controlfp / _control87 with std::fenv | `run+sim` | **determinism**, and **widened**: the commit converts only the helper classes and leaves 12 raw call sites, including every `_RC_CHOP` one. See below |
| 48 | [x] | `31a3c89ad` | 13 | replace Windows-specific IsDebuggerPresent with portable function | `build` | **merged with 49** into one `port/debugging.h`, and both platform paths reimplemented. See below |
| 49 | [~] | `e340d84e9` | 7 | replace Windows-specific __debugbreak with portable function | `build` | **absorbed into 48**: the two are halves of the same paper and edit the same lines |
| 50 | [x] | `fe7fbeeb5` | 2 | replace custom random generator with std::random | `run+sim` | **not applied as written.** The generator is already portable; only its seeding was not. See below |
| 51 | [x] | `1c21a16ef` | 25 | replace Sleep with std::this_thread::sleep_for | `build` | **squashed** into the branch's two existing Sleep commits, split by which one each site belongs to. See below |
| 52 | [x] | `037859f8a` | 3 | remove custom command-line parser as it's not used by game | `build` | true of the game; `dbcodegen` and `dbstruct` do use it, and neither has CMake wiring. See below |
| 53 | [x] | `c9bb3c1e0` | 18 | unify replace Unicode conversion functions with portable | `run` | applied **after 54**, with four defects fixed. Closes 3 overruns and a format-security site. See below |
| 54 | [x] | `116bf57bb` | 2 | make executable UTF-8 aware by default | `run` | **applied before 53**: this is the real behaviour change and the only one of the pair that is verifiable alone. See below |
| 55 | [x] | `bf1b8930a` | 1 | use explicit format specifier | `build` | the last `-Wformat-security` site in `System`; seven left, all in other projects |
| 56 | [x] | `a5f1f029b` | 1 | remove redundant Windows version check | `run` | the last OS version check in `System`; four remain, all in Windows-only code. The mapping fallback stays. See below |
| 57 | [x] | `02aaba7ca` | 2 | use modern portable string conversion functions instead of swprintf | `run` | **last row of the table.** The commit's `fmt::format( "%g", ... )` prints the literal `%g`; fixed. See below |

---

_Update the checkboxes as you go._

---


### Where the replay diverges from the `linux` branch

Commits applied with a deliberate change to what upstream wrote, so the two
branches are not expected to match byte for byte:

| # | upstream | what was applied instead |
| --- | --- | --- |
| 4 | `PORT_STDCALL` keyed on `BOOST_COMP_MSVC` | keyed on `BOOST_OS_WINDOWS`; the compiler macros are 0 under clang-cl |
| 6 | `GetLSB( WORD )` became `GetLSB( uint32_t )` | `uint16_t`, since the `uint32_t` overload already existed and the pair did not compile |
| 8 | `PORT_CDECL` keyed on the compiler | same fix as 4 |
| 11 | not in upstream | new commit: unit tests for the `boost::core` bit helpers |
| 14 | `SPlane` stores the `CVec4`, `n()`/`d()` computed | `n` and `d` are the members, `vec4()` is computed; `SRect` accessors gained non-const overloads |
| 15 | three scratch files rode along | dropped, parked on `linux2-strays` |
| 16 | `__rdtsc` header keyed on `BOOST_COMP_MSVC` / `BOOST_COMP_GNUC` | keyed on `BOOST_OS_WINDOWS`; both macros are 0 under clang-cl, and `BOOST_COMP_GNUC` is 0 for clang anywhere |
| 17 | fixes `itoa` in `Streams.h` only | plus a follow-up commit replacing `gcvt` on the adjacent line |
| 18 | `std::stof` inside a `try`/`catch` | `std::from_chars` in a static helper, no exceptions, locale-independent |
| 19 | fixes one `Sleep` in `HPTimer.cpp` | all twelve timed sleeps in built code, plus a follow-up commit turning the eight `Sleep( 0 )` into `yield()` |
| 20 | subject says `BOOST_INLINE` | subject corrected to `BOOST_NOINLINE`, which is what the diff uses and what Boost.Config actually defines |
| 21 | fixes 4 macros in 2 files | plus `REGISTER_PARSE` in `Server_Client_Common/Commands.cpp`, which is in the default build and pastes in the other direction |
| 22 | fixes `CConsoleBuffer::operator&` | plus the virtual `CGeneral::RequestForSupport` in `AILogic/GeneralInternal.h` |
| 23 | `GetCurrentTimeMilliseconds` returns `uint64_t` | returns `uint32_t`, matching `GetTickCount`, so every difference against a stored 32-bit value stays correct; plus one site upstream missed in `Input/Input.cpp` |
| 25 | 28 mistranslated format strings, 54 `StrFmt` calls left live in built code | all repaired, `%f` kept as `{:f}` to preserve output, integer precision mapped to zero-padded width |
| 25 | 28 mistranslated format strings, 54 `StrFmt` calls left live in built code | all repaired, `%f` kept as `{:f}` to preserve output, integer precision mapped to zero-padded width |
| 17+ | upstream fixed one `itoa` site | all four in built code, plus three follow-up commits: `NStr` conversions, the remaining `atof`/`_wtof`, and Lua |


### Done already

- `port/stdcall.h` keyed on `BOOST_COMP_MSVC`, which is 0 under clang-cl - fixed during #4
  by keying on `BOOST_OS_WINDOWS` instead.
- `Misc/Tools.h` `GetLSB( WORD n )` converted to `uint32_t` instead of `uint16_t`, producing
  two identical overloads - fixed during #6.


### `boost::core` bit helpers - made bit-exact (done)

Commit #11 (`a13479f46`) replaced the hand-rolled bit routines in `Misc/Tools.h` with
`boost::core`, and in doing so changed results for degenerate inputs. Rather than carry
that, the helpers were amended into #11 to be bit-exact with what they replaced.

What had differed, and is now preserved:

| call | old / restored | naive boost version |
|---|---|---|
| `GetNextPow2(0)` | `0` (wraps through `0xFFFFFFFF`) | `1` |
| `GetNextPow2(n > 2^31)` | `0` | **undefined** - `bit_ceil` result not representable |
| `GetMSB(0)` | `0` | `-1` |
| `GetMSB(-1)` as `int` | `31` | `63` (widened via `uint64_t`) |
| `GetLSB(0)` | `31` / `15` / `7` by width | `-1` |

The mechanism is `std::make_unsigned_t<T>`, which reproduces the original 32/16/8-bit
domains instead of widening everything to `uint64_t`. That also fixes two side problems:
a negative `int` no longer becomes a huge value, and signed callers compile rather than
failing inside `countr_zero`/`popcount`, which require unsigned types.

**Performance is not a concern.** The old routines were 5 dependent compare/shift steps
(~10-20 instructions, poorly predicted); `countr_zero`/`bit_width`/`bit_ceil` lower to
`BSF`/`BSR`/`TZCNT`/`LZCNT`. The added zero test is one compare that usually folds into a
`cmov`, and is frequently already present because `BSF`/`BSR` leave the destination
undefined for zero input.

Verified by `3Dmotor/test/BitOps.cpp` (`add_unit_test(BitOps)`): exhaustive over all
`uint8_t` and `uint16_t` inputs, single-bit and boundary sweeps plus randomised comparison
over `uint32_t`, and explicit cases for negative `int` and above-half-range values. A
standalone g++ run of the same comparison reported **8,197,793 checks, 0 mismatches**.

```powershell
cmake --build out/build/Windows-x64-Release --target BitOps_test
./out/build/Windows-x64-Release/Versions/Temporary/Engine/Sources/3Dmotor/test/BitOps_test.exe
```

Call-site audit that made this safe either way: `GetLSB` has one call site
(`Stats_B2_M1/Commands_Actions.h:102`, guarded by `if ( actions[i] )`, `uint32_t[4]`),
`GetNumBits` has none, and every `GetMSB`/`GetNextPow2` call site is presentation-only -
`3Dmotor`, `Image`, `UI`, `UISpecificB2`, `SceneB2`, `GameX`. No simulation path, so there
was never an ASYNC exposure.


### String conversion: what was actually changed

Done 2026-08-21, on top of commits 17 and 18. Every `itoa`, `atof`, `_wtof`,
`gcvt` and `strtod` in code that the default build compiles is gone.

| commit | what |
| --- | --- |
| `replace non-portable itoa with std::to_string` | all four call sites, not just `Streams.h` |
| `replace non-portable gcvt with std::to_chars` | `to_chars(general, 7)` is byte-identical to `gcvt(n, 7)` |
| `replace non-portable _wtof/atof with std::from_chars` | `NGlobal::CValue`, upstream commit 18 |
| `make NStr string to number conversions locale-independent` | `ToInt`, `ToULong`, `ToFloat`, `ToDouble`, 75 call sites |
| `replace the remaining atof and _wtof calls with NStr::ToFloat` | `Script.cpp`, `Bind.cpp` |
| `make Lua number conversion locale-independent` | `lua_str2number`, `lua_number2str` |

Behavior differences that survive, all deliberate and all in the commit
messages:

- overflow (`"1e309"`) gives 0 rather than `inf`, because `from_chars` reports
  out of range instead of saturating. In Lua it reads as no conversion, so
  `tonumber` returns nil.
- `NStr::ToULong` no longer accepts a negative and wraps it: `"-42"` gave
  4294967254, now gives 0.
- `NGlobal::CValue` no longer double-rounds. `atof` parsed to double then
  narrowed; a midpoint input like `0.5000000298023224` now gives 0.50000006
  rather than 0.5.

`NStr::ToInt` is unchanged across decimal, signed, hex, octal, whitespace and
trailing-junk input.

**Latent bug fixed on the way:** `NStr::ToULong` used the format `"%ul"`,
which is `%u` followed by a literal `l`, so it parsed an `unsigned int` into
an `unsigned long`. Harmless on Windows where `long` is 32 bits; on LP64 it
left the upper half untouched, so `"123456789012"` came back as 3197704724.

Still outstanding: the 76 `sscanf` and 62 `sprintf` sites, deferred to commit
25's fmt pass, and `luaO_verror` / `lundump.cpp`, which still format through
the C library.


### Commit 24 removes a function that was also broken

`GetAddressByClientID` really is unused: the only occurrences were the
`IDriver` default in `NetDriver.h`, the override in `NetA4.h` and the
definition in `NetA4.cpp`. `CNetDriver` is the sole implementation of
`IDriver`, and nothing in `Server` or `TestClient` implements it either, so
the vtable change reaches nothing.

Worth recording because it is a pattern to watch for elsewhere: the function
returned a dangling pointer.

```cpp
return iter->second.currentAddr.GetFastName().c_str();
```

`GetFastName()` returns `std::string` **by value**, so `.c_str()` points into a
temporary that dies at the end of the return statement. Any caller would have
read freed memory. The other branch returns `StrFmt`, which is
`MISC_EXPORT const char * StrFmt( const char *, ... )` handing back a pointer
into a shared static buffer, valid only until the next `StrFmt` call anywhere
in the process.

The safe neighbours are `GetIP` and `GetPort`, which return `std::string` and
`int` by value.

**Follow-up worth doing:** grep for other `.c_str()` returns from functions
declared to return `const char*`, and for `StrFmt` results stored rather than
used immediately. Both are the same bug shape and this codebase clearly has
the habit.


### Commit 25: what the format-string audit found

The biggest commit in the replay, and the one the compiler can check least.

**Why the compiler cannot help much.** `_DO_ASSERT_SLOW` is not defined in any
build, so `NI_ASSERT( x, text )` expands to `((void)0)` and the text argument
is never compiled. Of the 889 lines the commit converts, **498 sit inside
assert macros**. fmt 12 checks format literals at compile time, but only for
the 391 that are actually compiled.

**Defects found and repaired**, none of which the build would have reported:

| class | count | effect |
| --- | --- | --- |
| printf specifier left in a fmt string | 28 | 8 would fail to build, 20 latent |
| `{.2f}`, a field missing its colon | 1 | would fail to build |
| `%f` flattened to `{}` | 7 | `3.140000` becomes `3.14` |
| `{:.8x}`, precision on an integer | 6 | fmt rejects precision for integers |
| **live `StrFmt` calls left in built code** | **54** | the commit deletes `StrFmt`, so these do not build |

The 54 are the interesting ones: they are almost entirely the
`MatchPacketTrace` logging and MP manager code added **after** the `linux`
branch forked, so upstream never saw them, plus
`MapEditorLib/Interface_UserData.h`, which is not an editor target but is
dragged into the build through `libdb/RenameNode.cpp`. A replay of an old
branch onto a moved tree produces exactly this shape of gap, and only a
build-scope-aware sweep finds it.

**Translation rules used**, chosen to keep output byte for byte identical:
`%d %i %u %s` to `{}`; `%f` to `{:f}` because printf gives six decimals where
fmt defaults to shortest round-trip; `%g %e` keep their letter; `%x %X` to
`{:x} {:X}`, since a bare `{}` silently turns hex into decimal; width and
flags carry over, `%02d` to `{:02}` and `%-10s` to `{:<10}`; precision on an
integer becomes a zero padded width, `%.8X` to `{:08X}`; `%%` to `%`; literal
braces doubled.

**Left as the commit had it, deliberately:** `Parser/ParseOperations.cpp` had
`"%type s redifinition, see %s(%d)"`, where `%t` is not a conversion and
printf's behaviour was undefined. The commit's `"type {} redifinition, see
{}({})"` is a fix, so a faithful translation would have preserved a bug.

**Still outstanding after this commit:**

- 967 live `StrFmt` calls in targets that are not built (`MapEditor`,
  `MapEditorLib`, `Server`, `TestClient`, `ED_*`). Those targets already do not
  build; this adds one more reason.
- 123 `StrFmt` calls inside assert macros. Invisible until `_DO_ASSERT_SLOW`
  is defined, at which point they are compile errors.
- `DbgTrc` survives as the remaining printf-style helper, and
  `luaO_verror` / `lundump.cpp` still format through the C library.

**Tooling** left in the scratchpad and worth keeping for the rest of the
replay: `p2f.py` is a printf-to-fmt translator with a 31 case self test,
`fixfmt2.py` pairs removed and added lines by diff position and rewrites any
that differ from a faithful translation, `fmtcheck_tree.py` scans the tree for
bad format strings, and `strfmt_live.py` reports which `StrFmt` calls the
build would actually compile, stripping comments and assert macros properly.

**Process note:** several intermediate checks in this session were run against
the Windows clone while the cherry-pick lived in WSL, which produced counts
that looked alarming and were simply measuring the wrong tree. Tree-state
checks belong on the side that holds the work in progress.


### The assert check has to be run to a fixed point

MSVC abandons a translation unit after the first `type_is_unformattable_for`,
so one pass surfaces at most **one** bad site per file. `libdb/Variant.cpp`
alone had eight `NI_ASSERT( false, fmt::format( "... {}", m_eType ) )`, which
would have been eight compile rounds. Chasing the reports one at a time
converged only after five passes:

| pass | fmt sites reported |
|---|---|
| 1 | 25 |
| 3 | 5 |
| 4 | 2 |
| 5 | 0 |

**Two lessons.**

*Grep the file, not the report.* Once a site is found, sweep its whole file for
the same shape instead of recompiling. Hungarian naming makes that cheap: an
argument spelled `eSomething`, `m_eSomething`, or ending in `Type` is almost
always an enum. `scratchpad/enumscan.py` does the sweep tree-wide and
over-reports by design, so every hit still gets eyeballed, but it turns an
unknown number of compile rounds into one list.

*The checker had a blind spot.* `assertcheck.py` picked translation units by
looking for an assert macro and a `fmt::format` **on the same line**. Asserts
wrap often enough that this missed files entirely, for example

```cpp
NI_VERIFY( eUType >= 0 && eUType < NDb::DB_RPG_TYPE_COUNT,
	fmt::format( "Invalid unit type {}", eUType ), eUType = NDb::DB_RPG_TYPE_SOLDIER );
```

in `AILogic/GeneralRearManager.cpp`. The selector now matches across the whole
call, up to the semicolon that ends the statement, which raises the set from
138 translation units to 140.

Final tally folded into commit 25: **44 casts across 30 call sites**. The only
remaining failure is `Game/main.cpp:279`, the pre-existing condition rot from
[Checking the code no build compiles](PORT_FINDINGS.md#checking-the-code-no-build-compiles), which is not a port problem.

`TestDB/TestIterator.cpp:489` had the same defect and was fixed with it, but no
build covers it: `TestDB` has no CMake wiring, so it is not in
`compile_commands.json` and the check cannot reach it.


### Input now compiles against the DirectX SDK, not the Windows SDK

Commit 27 (`build Windows-only sources and libraries only on Windows`) is a
no-op for a Windows build in every target but one. A script that reconstructs
each `CMakeLists.txt`'s link set and source list with `WIN32` true and diffs it
against the pre-commit file reported exactly one difference:

```
Input:  lost   dinput8  dxguid
        added  dxsdk::dinput8  dxsdk::dxguid
```

`Input` used to name those libraries bare, so the linker resolved them against
whichever library directory came first, and `compile_commands.json` confirms
that nothing put the DirectX SDK on `Input`'s include path at all. `Bind.cpp`
and `Input.cpp` were therefore compiling `#include <dinput.h>` out of the
**Windows SDK** while the rest of the engine used the June 2010 DXSDK. The
imported targets carry `INTERFACE_INCLUDE_DIRECTORIES`, so after this commit
header and library both come from the pinned SDK, which is what the globally
set `DIRECTINPUT_VERSION=0x0800` implies was intended all along.

**Two things to watch.** The DXSDK include directory now lands on `Input`'s
compile line, and it ships its own copies of many Windows headers (`d3d9.h`,
`dsound.h`, and so on), so anything else those translation units include can be
shadowed by the older SDK copy. And this is a link change under the input path,
so a build that succeeds proves less than a run: check that keyboard and mouse
still respond in game.

The sweep for other targets naming a DirectX library bare came back clean:
`Input` was the last one. Every remaining reference is prefixed, and only three
targets have any.

```
3Dmotor  dxsdk::d3d9 dxsdk::d3dx9 dxsdk::dxerr dxsdk::dxguid
Image    dxsdk::d3dx9
Input    dxsdk::dinput8 dxsdk::dxguid
```


### Commit 29 was rewritten, not cherry-picked

The upstream commit did two things: it guarded `OutputDebugString`, which is
right, and it replaced

```cpp
_vsnprintf( charBuff, BUF_SIZE - 1, pszFormat, va );
```

with

```cpp
const auto formatted = fmt::vformat( pszFormat, fmt::make_format_args( va ) );
```

which is wrong twice over.

**`make_format_args` does not take a `va_list`.** It takes the arguments
themselves. On MSVC a `va_list` *is* a `char *`, so this compiles and hands fmt
a pointer to the raw argument area to format as a string. With GCC and clang on
x86-64 a `va_list` is an array type that decays to `__va_list_tag *`, which has
no formatter, so it does not compile at all. The one platform where it builds
is the one where it is undefined behaviour.

**Every caller passes a printf format string.** `%` means nothing to fmt, so
`fmt::vformat( "save_size %d", ... )` does not fail, it quietly emits the
literal text `save_size %d` and drops the value. `DbgTrc` has 24 direct
callers, and `Tools.h` defines `DebugTrace` as an alias for it, which accounts
for several hundred more. Taken as written, this commit would have silently
stripped the arguments from every trace line in the Windows build.

So only the output half was taken. `stderr` rather than `stdout` keeps the
trace apart from anything the game prints, and `std::fputs` rather than
`std::cerr` avoids pulling an iostream static initializer into `Misc` on every
platform, which the upstream `#include <iostream>` would have done
unconditionally.

`_vsnprintf` stays for now, per [_vsnprintf is a gap the replay does not close](PORT_ROADMAP.md#_vsnprintf-is-a-gap-the-replay-does-not-close).


### Commit 30: what the NOMINMAX sweep actually covered

822 translation units syntax-checked with /DNOMINMAX (cl /Zs over
compile_commands.json, restricted to the 32 projects that link winapi).
809 clean. The 13 failures are 3Dmotor/test and 3Dmotor/benchmark, and
they fail identically without the define: original.h holds the x86 __asm
MMX reference implementations, 228 x error C4235 "__asm keyword not
supported on this architecture". Those targets are EXCLUDE_FROM_ALL and
have never built for x64. Not a NOMINMAX problem, and not something this
replay introduced.

B2_M1_World, the one project the pre-check flagged as an open question,
passed all 23 of its TUs.

ED_B2_M1 and MapEditorLib have bare min/max and link winapi but contribute
no TUs to compile_commands.json, so the sweep says nothing about them.
They are behind -DBUILD_EDITOR=ON and will need the same treatment if that
is ever turned on.


### SceneB2 min/max is an inherited gap, not something commit 30 closed

Commit 30 only touches the winapi closure. SceneB2 does not link winapi, so it
never saw NOMINMAX and nothing in it changed. But nothing in this tree defines
min or max either, and SceneB2 has no `using namespace std`, so its 491 bare
`min(` / `max(` call sites are binding to the Windows.h macros arriving
transitively through some other project's header. Take Windows.h away and every
one of them is an undeclared identifier.

CLOSED by `stop SceneB2 depending on the Windows.h min/max macros`, an extra
commit that is not part of the replay. See
[Closing the SceneB2 gap](#closing-the-sceneb2-gap-not-a-replayed-commit). The rest of this section is kept for why it was needed.

The linux branch did not fix this. At its tip SceneB2 still has 490 of them,
and SceneB2 is in that branch's add_subdirectory list, so its Linux build never
got this far. The gap is inherited, not introduced by the replay.

Whoever closes it: 31 files, worst offenders TileMasks.cpp 58,
TerraObjectsBuilder.cpp 32, RiversBuilder.cpp 31, RoadsBuilder.cpp 30. The
mechanical fix is the same as commit 30, (std::min) / (std::max) plus
<algorithm>, with the caveat that these are mostly float and CVec2i work, so
mixed-type literals need the same 0 to 0.f treatment.

Everything else in the tree is benign or gated:

- GameX/ScenarioTracker.cpp:1535 and B2_M1_World/UpdatableWorld.cpp:568 are
  inside comments, the latter a block comment.
- Misc/Geom.h:499-514 are members of the CSpan<T> class template, compiled only
  on instantiation, and nothing in the build instantiates operator&= or
  operator|=. Latent on both platforms, which is why the NOMINMAX sweep passed
  all 15 Misc TUs.
- ED_B2_M1 (4) and MapEditorLib (2) are real but behind -DBUILD_EDITOR=ON.


### Closing the SceneB2 gap (not a replayed commit)

364 live call sites in 30 files rewritten to (std::min) / (std::max), plus
<algorithm> in SceneB2/stdafx.h. 150 further occurrences sit in commented-out
code and were deliberately left alone: the rewrite walks each file tracking
line comments, block comments, string and char literals, and only touches live
code, so the diff stays readable.

<algorithm> went in the prelude rather than in 30 files. Half these sources
have no plain <...> include block at all, and TerraHeight.h has no includes
whatsoever and is written to be included after stdafx.h, so per-file
include-what-you-use was not actually available. stdafx.h already carries
<list>, <vector>, <set>, <unordered_map>, <cstdint>, so it is the consistent
home. Neither changed header is included from outside SceneB2, so the blast
radius is the project.

Five sites needed a type, because std::min/std::max deduce one type from both
arguments and the macros did not:

- PrecipicesRender.cpp:520, RoadsBuilder.cpp:499, SceneVisObjs.cpp:117 all
  compare a small non-negative int against container.size() - 1. The macro
  converted the int to the unsigned size type. That is load-bearing: on an
  empty container size() - 1 wraps to its maximum and the comparison picks
  the int operand. std::min<int> would make it -1 and hand a negative index
  to the loop below, so these use std::min<size_t> and keep the unsigned
  comparison. Note stdafx.h disables C4244 and C4267, so the narrowing back
  to int is silent, as it was before.
- RiversBuilder.cpp:233, int * float, so 1 becomes 1.f.
- TerraHeight.cpp:30, float + float returning float, so 0 becomes 0.f.

One residual difference that no spelling removes: the macros are
a < b ? a : b, std::min is b < a ? b : a and std::max is a < b ? b : a. Same
result for equal values and for every ordinary float; only a NaN operand
selects the other argument. Recorded in the commit message because SceneB2
terrain output does reach the simulation.

Verified with cl /Zs over all 83 SceneB2 TUs plus /DNOMINMAX. The define
matters: without it a missed bare min( would still have compiled on Windows
and the check would have proved nothing. First pass found exactly the five
mixed-type sites above, second pass clean.

Still open after this: ED_B2_M1 (4) and MapEditorLib (2) behind
-DBUILD_EDITOR=ON. The editor is explicitly not a target, so these stay
open on purpose. CSpan was dealt with separately, see
[CSpan deleted](#cspan-deleted-not-a-replayed-commit).


### CSpan deleted (not a replayed commit)

CSpan<T> in Misc/Geom.h was the last min/max site outside the editor. It turned
out to be dead: the only references anywhere in the repository are inside its
own definition, plus two of Nival's static analysis logs under Arch/
(IgnoreOldProblems.txt, LogWhichIsOk.txt) pointing at the same operator| lines
at essentially their present position. Unused since the original codebase.

Deleted rather than fixed, 99 lines. Fixing four call sites in code with no
callers is not worth the lines.

Worth remembering as a pattern: because it was a class template it was never
instantiated, so it was never compiled, so no build on any platform could ever
have told us those macros were still in there. Templates are where this kind of
rot hides, and no amount of /Zs sweeping will surface it. If more turns up,
looking for uninstantiated templates directly beats waiting for a compiler.

Checked with 90 TUs spread across all 26 projects (every Misc TU plus the first
three of each other project), /Zs with /DNOMINMAX. git grep already proved
nothing could fail to resolve; the point of the sweep was that Geom.h is in
most stdafx.h files, so an unbalanced brace from the cut would have taken out
the tree. Clean.

Two mechanics worth keeping:

- /Zs still honours /Zi, and a TU whose command carries no /Fd will try to open
  a default vc140.pdb in the working directory and fail with C1041. Strip /Z[i7]
  along with /Fo and /Fd when generating these batches.
- Anchor a deletion script on surrounding text, not line numbers, and assert on
  what the cut range contains before writing. The first guard here counted
  "class " and tripped over "template <class T>", which is exactly the kind of
  loud failure that is wanted.


### Commit 32, GUID to boost::uuid: the byte order has to be preserved

**This section was wrong when first written, and the error broke loading any
save made before the commit.** It claimed no uid ever reaches a binary. Uids
reach every save game. What follows the correction is kept because the rest of
the reasoning still holds.

The claim was that the only producer of a non-nil SResKey uid is the
two-argument CResourceLoader::SetKey, which has no callers outside the editor.
The real producer is CGrannyBaseStuffLoader::SetKey in 3Dmotor/GAnimFormat.h,
which builds `SResKey( pResource->uid, ... )` from the NDb record's uid. That
key is serialized by SResKey::operator& and by CGrannyBaseStuffLoader, so save
games and replays are full of uids.

IBinSaver has no operator& for the type, so it writes the object whole: sixteen
raw bytes, the same count before and after, under the same chunk id. Only the
meaning changed. GUID holds its first three fields as integers, so on a little
endian machine its bytes are transposed against the string it prints, while
boost::uuids::uuid is a flat array in written order. Measured on a real uid:

| | |
|---|---|
| bytes an old build wrote | `9646709D525F8042A25600D8DEF284D7` |
| read as a uuid, no swap | `9646709d-525f-8042-a256-00d8def284d7` |
| the uid the resource file actually has | `9D704696-5F52-4280-A256-00D8DEF284D7` |

The transposed name matches no file, so CFileRequest::Read finds nothing, the
buffer stays empty, GrannyReadEntireFileFromMemory returns null, and
CGrannyMeshLoader::Recalc dereferences it. A new game is unaffected, because
its uid comes from .xdb text through string_generator and never round-trips
through the old bytes; that is why the game ran and only loading a save failed.

The fix is AddUuidChunk in System/UuidChunk.h: transpose the first eight bytes
on the way in and out, keeping the on-disk order exactly what it always was.
The permutation is its own inverse, and a round trip reproduces the original
bytes. It is applied to SResKey and to the uid field of every NDb record that
has one, in DBScene and DBTerrain.

Two lessons worth keeping. A type swap that preserves size and alignment can
still change a file format, because the format is the bytes and not the size.
And "nothing writes this to disk" is a claim about the whole call graph, so
grepping for the setter is not enough; the field was reached through a
constructor argument, which the search for SetKey never saw.

#### The original reasoning, for the parts that still hold

This is the one that looked like it could corrupt data, so the reasoning is
worth keeping.

GUID and boost::uuids::uuid are both 16 bytes and disagree about what those
bytes mean. GUID's Data1 is a 32-bit integer, so on x86 the raw bytes are
byte-swapped relative to the string form, and GUIDToString printed uppercase.
boost::uuids::uuid is a flat byte array printed in order, lowercase. Feed the
same 16 bytes to both and you get different strings. That string is a resource
file name, so a uid arriving from a binary would break resource loading
outright.

None does. The decisive checks:

- index.bin holds SFullTypeHeader, which is STypeObjectHeader (szClassTypeName,
  nObjectID) plus szFileName. No uid.
- Records load from .xdb, where the uid is text, through
  CXMLChunkSaver::DataChunk. So the path is string to uuid to string, identity
  for old and new alike.
- Confirmed against shipped data:
  Bridges/Africa/summer_center_aigeometry.xdb has
  <uid>9D704696-5F52-4280-A256-00D8DEF284D7</uid> and
  Data/bin/AIGeometries/9D704696-5F52-4280-A256-00D8DEF284D7 exists. Byte for
  byte the same string.

The residual case change is harmless. ConvertChar in FilePath.cpp folds case
and the path separator together, branchlessly, and backs both
CFilePath::operator== and CFilePath::MakeHashKey. The VFS index is therefore
case-insensitive in the comparator and in the hash, on any platform, so a
lowercase name still finds an uppercase file even on a case-sensitive
filesystem.

The binary path that does exist is SResKey, whose operator& does
f.Add( 2, &uidKey ) into IBinSaver, which is the save game and replay format.
Its uid is always nil in the game: the only producer is the two-argument
CResourceLoader::SetKey, and that has no callers outside
ED_Common/ParticleExporter.cpp. Sixteen zero bytes are the same in either
representation, so old saves are unaffected.

Two incidental improvements: boost::uuids::uuid has `data_type data = {}`, so
it zero-initialises where `GUID uid;` left stack garbage (this is why is_nil()
is now meaningful on an unloaded record); and GObjectInfo builds the geometry
file name with fmt::format instead of sprintf into char[1024].

Boost 1.89 detail worth knowing: uuid's storage is a union carrying a
std::uint64_t align member unless BOOST_UUID_DISABLE_ALIGNMENT, so its
alignment is 8, not 16 and not GUID's 4. Structs embedding it gain alignment on
x86. Harmless here because BinSaver writes these records field by field under
chunk ids rather than dumping the struct, and ReportMetaInfo computes offsets
at runtime.

**Editor left broken, deliberately.** The commit removes NStr::GUID2String,
NStr::String2GUID, NBinResources::GUIDToString and NBinResources::IsEmptyGUID
but only updates MapEditorLib/ManipulatorManager.h. Still calling them:
ED_B2_M1/MapInfoExporter.cpp:200, ED_B2_M1/XMLExport.cpp:124,
MapEditor/PC_GUIDEditor.cpp:20 and :42, MapEditor/PC_MainTreeControl.cpp:2282,
MapEditorLib/CommonExporterMethods.cpp:311. All behind -DBUILD_EDITOR=ON, which
is not a target.

Verified with all 902 default-build TUs, /Zs, clean. The stale
compile_commands.json has no boost/uuid include path, so the checker adds all
159 Boost library include dirs by hand rather than reconfiguring; Boost here is
the full 1.89 with no BOOST_INCLUDE_LIBRARIES filter, so Boost::uuid resolves.
184 warnings, none mentioning uuid, GUID or boost: 136 C4838 narrowing, 25
C5033 register, 16 C4273 dllimport, 3 C4477 format mismatch, 4 pointer
truncation. All pre-existing.


### Commit 34, CCriticalSection to std::mutex: why no recursive_mutex

Win32 CRITICAL_SECTION is recursive, std::mutex is not, and re-locking a
std::mutex is undefined behaviour that shows up as a hang. So the swap is only
safe if nothing acquires the same section twice on one thread. Nothing does.

CTLock even shipped the mechanism for it: __declspec(deprecated) Enter() and
Leave() on the lock object itself, commented "DANGEROUS!". Zero call sites, so
there is no explicit re-entry to worry about.

Call-chain re-entry, per mutex, by working out which functions lock it and
whether any of those calls another:

| mutex | file | lock sites | re-entry |
|---|---|---|---|
| netDriverCriticalSection | Net/NetA4.cpp | 17 | none |
| reqQueue | System/GResource.cpp | 5 | none |
| g_WinVFSCriticalSection | System/WinVFS.cpp | 2 | none |
| msgs | System/WinFrame.cpp | 2 | none |
| readResource, pendingCheck, directRead | System/* | 1 each | none |
| csClientSocketReading/Writing | Server/Terminal.cpp | 2 each | none, and Server is unbuilt |

The two that hold a lock across real work are both clean. CFileRequest::Read
holds readResource across DoesFileExist and a CFileStream read, neither of
which loops back. CWinVFS::UpdateFileEntry holds its lock across only a Win32
stat, map operations and a trivial CWinFileEntry constructor; and
GetAllFileNames calls NFile::EnumerateFiles with a CWinFileAdder that touches
pVFS->streamEntriesMap directly rather than going through UpdateFileEntry,
which is deliberate, it is written to run under the caller's lock.

One apparent hit was a false positive worth remembering: CTerminal::MTSegment
does call OutString and both touch csClientSocketWriting, but MTSegment's lock
is in an inner scope that closes on the line before the call. A scanner working
at function granularity cannot see that; check the braces before believing it.

Lock ordering is untouched by this commit. The loader thread still takes
pendingCheck then readResource then g_WinVFSCriticalSection, exactly as before.

Incidental: CCriticalSection ran InitializeCriticalSection during dynamic
initialisation, so every namespace-scope instance carried a static
initialisation order dependency. std::mutex is constexpr-constructible and so
constant-initialised. That hazard is gone.

**Follow-up 1, remove the dead lock.** Sound/SoundEngine.cpp:23 declares
critSection and never locks it. Dead before this commit and dead after; the
commit converted it rather than deleting it. Delete it.

**Follow-up 2, better primitives.** Several of these are hand-rolled versions
of things the standard library or Boost already provide, and replacing them
would remove code rather than translate it:

- The bIsFileReading / bIsReady / bExit / bActive flags are `volatile bool`
  shared across threads, which is not a synchronisation primitive on any
  compiler. These want std::atomic<bool>.
- GResource's request pipeline is a producer/consumer queue built out of two
  mutexes, a std::list, a manual CEvent and a hand-managed loader thread. That
  is boost::sync_queue or boost::lockfree::spsc_queue, and it would delete the
  event, the queue mutexes and most of LoaderThread.
- WinFrame's msgList is the same shape: one producer (the window procedure),
  one consumer (the main loop). SPSC.
- NetA4 serialises 17 entry points on a single mutex. Worth measuring before
  touching, but MPMC or a finer split is the obvious direction.

Order this after the replay; each is a behaviour change under threads, not a
mechanical translation, and the queue rework in particular deserves its own
commit and its own testing.

### Commit 45, memory mapping to boost::interprocess: rewritten, not cherry-picked

The original commit was abandoned after it produced out-of-bound reads,
interprocess exceptions and short reads. Every one of those has the same root:
`CreateFileMapping` and `mapped_region` disagree about who is allowed to change
the size of a file, and the Win32 code leaned on behaviour boost does not
reproduce.

**What `CreateFileMapping( hFile, ..., 0, nSize, 0 )` did that boost does not.**
It extends the file to `nSize` as a side effect of creating the mapping object.
The stream never asked for a file of a given size; it asked for a mapping, and
the file grew to match. `boost::interprocess::file_mapping` maps whatever the
file already holds, so a `mapped_region` of `nSize` over a shorter file fails
outright. Growth has to become an explicit `resize_file` **before** the mapping
is constructed, which is what `CMMFile::MapFile` now does.

**Why the file cannot be resized while it is mapped.** `SetEndOfFile` acted on
the same `HANDLE` the mapping was made from, so Win32 was willing. Resizing by
name is not: on Windows the open handle inside `file_mapping` blocks it, and
that is the "bip disallowed to grow the mapping while a view is active" failure.
The ordering `CMappedStream::ReleaseBuf` already had - unmap, then set the file
size - is exactly right, and `CMMFile::SetFileSize` now asserts `!bMapped` so
any other caller is caught rather than silently failing.

**Why there is no handle any more.** Keeping one open for the object's lifetime
is the obvious translation of the old class and it is the wrong one, because it
is the thing that makes `resize_file` fail. The file name is the handle now.
Destroying the `file_mapping` closes both the section and the file, which is
what makes the next resize possible.

**The granularity arithmetic is gone.** `CMemoryMappedFileFragment` used to
round `nOffset` down to a multiple of `dwAllocationGranularity`, map from there
and return a pointer shifted forward by the difference. `mapped_region` does the
same rounding internally and hands back an address inside the region, so doing
it again shifts twice and reads real bytes from the wrong place - the
out-of-bound reads. `MappedFragment.WindowsAtManyOffsetsReadTheRightBytes`
covers offsets either side of both the 4K page and the 64K granularity.

**Two behaviours preserved deliberately.** Opening for writing must not
truncate, because the VFS opens existing files read-write and reads them; the
default `std::ofstream` mode would empty them, and only the file is created when
it is missing, matching `OPEN_ALWAYS`. And the Win32 constructor stamped the
file times on opening for writing whether or not anything was written, so
`last_write_time` is still set; the access time has no portable setter and is
dropped.

**One behaviour improved.** The direct-read fallback used to seek a shared
`HANDLE` under a `std::mutex`, because every fragment shared one file pointer.
Each call opens its own `std::ifstream` now and carries its own position, so the
lock is gone. That path is only reachable on the Windows 9x family (see the
`GetVersion() & 0x80000000` check in `ZipArchieve.cpp`, row 56), so it had no
test coverage and now has two.

**A mapping that fails is still a broken stream, not an exception.** Both boost
constructors throw; `TryMake` turns that back into the null return
`CMappedStream::AllocBufImpl` expects. The old code's quiet fallback - if
`CreateFileMapping` failed, fragments read the file directly - is preserved,
since a failed `MapFile` leaves `IsMapped()` false.

### Commit 46, the resource loader thread: the commit as written busy-waits

The loader thread waited on a manual-reset `CEvent`, then drained the queue in
an inner loop that `break`s when it is empty. The commit deletes the wait and
keeps the `empty()` check, so nothing blocks any more: the outer `for(;;)`
spins at 100% CPU for the life of the process. `sync_queue::pull` is the
blocking call meant to replace the event, but the `empty()` check in front of
it means it is never reached with an empty queue. The loop had to be
restructured either way.

**`std::condition_variable`, not `boost::sync_queue`.** Boost.Thread is a
compiled library and nothing else in the tree uses it, so a queue of about ten
lines would have added a build dependency and a `boost_thread` DLL to the
install. `wait` with a predicate is also strictly better than the event it
replaces: the event was signalled on every push and cleared by the waiter, so
an arrival between the wait returning and the reset could be missed. Waiting on
the queue's own state cannot lose a wakeup.

**LIFO is preserved.** `AddFileRequest` uses `push_front`, so the newest
request is served first, and `~SKillLoaderThread` pushes its stop sentinel to
the front as well so shutdown does not wait for the queue to drain. A FIFO
queue reverses both, which changes texture load order and makes exit block on
every pending load.

**`pendingCheck` was deleted, not translated.** The loader held it across the
pop and the read and no other code ever locked it, so it serialised one thread
against itself. It looks like it was meant to close the
`ReleaseFileRequestHolder` race, but it never could, because that function
takes a different mutex. The race is real and pre-existing; it is written up in
[PORT_ROADMAP.md](PORT_ROADMAP.md#the-resource-loaders-holder-race) rather than
fixed here.

**Two things `std::thread` changes on its own.** Assigning over a running
thread calls `std::terminate` where `CreateThread` only leaked the old handle,
so `SFLB3_RunResourceLoadingThread` now returns early if the thread is already
running; the game and the editor each call it once, but from different
executables, and nothing enforced that. And `CreateThread` was given a 100 KB
stack, where `std::thread` takes the platform default (1 MB on Windows), which
has no portable control.

**`Threads::Threads` is now linked.** `std::thread` needs an explicit threading
library on everything but Windows and nothing in the build wired one, because
until this commit nothing had spawned a thread through the standard library.

### Commit 47, the FP control word: the commit does not reach the sites that matter

`89da73090` rewrites the three helper classes in `Misc/Win32Helper.h` and
nothing else, so after it lands the tree still has twelve raw
`_control87`/`_controlfp` calls, including all three `_RC_CHOP` sites the
comments describe as *"here we must set special rounding and precision state to
be sync during multiplayer"*. Its subject would not have been true of the tree
it produced. The replay converts every site that has a standard equivalent.

**What was converted.** `CControl87Guard` to `fegetenv`/`fesetenv`,
`CRoundingControl` to `fesetround`, the three `_RC_CHOP | _PC_24` database sites
(`GlobalVars.cpp` twice, `GameDatabase.cpp` once) to a `CRoundingControl` plus a
`CPrecisionControl`, `GroupLogic.cpp`'s `_RC_NEAR`, and the three
`NI_ASSERT( (_MCW_RC & _control87( 0, 0 )) == 0 )` guards to
`std::fegetround() == FE_TONEAREST`. `_RC_NEAR` is zero, so the old assertion
said exactly that.

**What was not, and why.** The four `_MCW_EM` sites in `AILogicInternal.cpp` and
`MPTransceiver.cpp` mask every FP exception at simulation init. C++ has no
standard way to mask or unmask them - glibc's `feenableexcept` and
`fedisableexcept` are extensions - so those need a platform helper of their own.
They also carry `_PC_24` on x86, which is load-bearing for sync, so they are not
something to convert casually.

**The x87 precision field has no portable equivalent and does not get one.**
`_PC_24` sets the width x87 arithmetic rounds to irrespective of operand type.
SSE has no such control, which is why the existing x64 branch already dropped
it. `CPrecisionControl` is therefore Windows-x86-only and a no-op elsewhere,
which is what it already was in practice.

**This is a real x86/x64 divergence, not one the port introduces.** On x86 the
database floats are parsed with chop rounding *and* 24-bit precision; on x64
with chop rounding alone. The two builds have therefore never parsed the
database identically, which matters for any cross-architecture multiplayer or
for a replay recorded on the other build.

**The one behaviour change to watch.** `CRoundingControl::Set` did nothing at all
on x64 - the whole body was inside `#ifdef _M_AMD64` with only a comment - so
its two users, `CAILogic::InitAfterMapLoad` and `TerraAIObserver`, were no-ops
on the 64-bit build. They now set the mode. Both ask for `RCM_NEAR`, which is
already the default, so this should be invisible unless something else moved the
rounding mode first, in which case the new code corrects it and the old one did
not. That is the same defensive purpose the `NI_ASSERT` guards serve.

**`fenv_access` is not turned on, deliberately.** MSVC only guarantees that FP
operations respect a changed environment under `#pragma fenv_access(on)`, which
`/fp:strict` implies for `AILogic` but not for `System`, `libdb` or
`Common_RTS_AI`. That was equally true of the `_control87` calls being replaced,
so nothing regresses, but turning the pragma on would change codegen in exactly
the parsing code that has to stay bit-stable, and is not something to do in
passing.

### Commits 48 and 49 are one commit: they are halves of the same paper

`IsDebuggerPresent` and `__debugbreak` become `std::is_debugger_present` and
`std::breakpoint` in C++26, from a single paper (P2546) that also adds
`std::breakpoint_if_debugging`. The two commits edit the same lines of
`Misc/Asserts.h` and `System/BinChunkSaver.cpp`, the second rewriting what the
first just wrote, so they are replayed as one `port/debugging.h` that mirrors
the `<debugging>` header it will eventually be replaced by. `BinChunkSaver.cpp`
had `if ( IsDebuggerPresent() ) __debugbreak();` written out, which is the
definition of `breakpoint_if_debugging`, and now says so.

**`is_debugger_present` off Windows returns a real answer, not `false`.** The
original returned a flat `false` on every other platform, which compiles but
means an assert never breaks into gdb on the one platform this whole exercise
exists to reach. Linux publishes the answer in `/proc/self/status`, on a
`TracerPid:` line holding the pid of whatever is tracing the process or 0 when
nothing is. Catch2 and Boost.Test both read exactly that, and neither exports
it, which is why this is hand-written rather than pulled from a dependency.

It is deliberately not cached. A debugger can attach part way through a run and
`IsDebuggerPresent` reports that, so caching would make the two platforms
disagree in precisely the case where someone is trying to debug something.

**`breakpoint` no longer assumes x86.** The original used
`__asm__ __volatile__( "int3" )` for every non-MSVC compiler, which does not
compile on ARM and would have been a second entry in the same category as
`__rdtsc`. Clang has `__builtin_debugtrap()`, GCC on x86 keeps `int3`, and
everything else raises `SIGTRAP`, which is what `int3` raises anyway, minus the
architecture assumption.

Note that `breakpoint()` with no debugger attached terminates the process on
every platform here, exactly as `__debugbreak()` already did on Windows. That
is the whole reason `breakpoint_if_debugging` is a separate function, and why
the two bare `breakpoint()` calls left in `ChecksumSaver.cpp` are worth a
second look: they fire on a checksum mismatch, unattended, in a release build.

**`OutputDebugString` is still Win32** and sits inside the same assert macro,
two lines from the call this commit converted. It is a separate problem, listed
with the rest of the Windows API surface.

### Commit 50: the generator was never the problem, the seeding was

The premise of `fe7fbeeb5` is that a hand-rolled generator is less portable
than `<random>`. It is the other way round here. The generator is **ISAAC**,
implemented inline in `RandomGenInternal.cpp` in plain `uint32_t` arithmetic
with no platform dependency at all, so it already produces identical output on
every compiler, architecture and OS. What is not portable is how it gets its
seed: `FillRandRsl` enumerated logical drives with `GetLogicalDriveStrings`,
walked the filesystem for a file larger than 1 KB and read 1024 bytes out of it
at an `rand()`-chosen offset.

So the replay keeps ISAAC and replaces only `Init`, with `std::random_device`
filling `randrsl`. `RecFindFile` and `FillRandRsl` go, along with the
`FileUtils.h`, `<thread>`, `<chrono>` and `port/time.h` includes that existed
only for them, and `FileUtils.h` is itself a `FindFirstFile` wrapper. The
sequence, the save format and the chunk ids are untouched.

**Engines are portable, distributions are not, and the commit used
distributions.** `std::mt19937` and `mt19937_64` are specified exactly, down to
a mandated conformance value for the 10000th draw, so an engine's output is
bit-identical across libstdc++, libc++ and MSVC. `uniform_int_distribution`,
`uniform_real_distribution` and friends specify only the statistical
distribution, not the algorithm, and all three implementations differ. The
commit drew every value through `uniform_int_distribution`, which would have
made the simulation *less* reproducible across platforms than it already is.

**Three other things it would have broken.**

- `Random( uMax )` was `Random() % uMax`, giving `[0, uMax-1]`. The commit made
  it `uniform_int_distribution( 0, uMax )`, which is inclusive at both ends.
  That is an off-by-one across 189 call sites, several of which index arrays
  directly - `Random( anims.size() )`, `Random( GetNEntrancePoints() )` - so it
  is an out-of-bounds read, not a statistical wobble.
- Saves and replays. The engine state was to be written as a *text* dump via
  `operator<<`, roughly 6 KB of ASCII against today's 2 KB binary, reusing
  chunk ids 1 and 2 which currently hold `randcnt` and `randrsl`. Existing
  saves would have misparsed rather than failed.
- `float Random( fMin, fMax )` was a fixed formula over the raw 32-bit draw.
  `uniform_real_distribution` is implementation-defined, and this one is called
  from `/fp:strict` code in `AILogic`.

**One caveat on `random_device`.** libstdc++ on MinGW historically returned a
deterministic sequence from it, which would silently give every fresh game the
same seed. It is a real CSPRNG under MSVC and under libstdc++ on Linux, which
is every target here, but it is worth knowing before anyone tries a MinGW
build.

### Commit 51 was squashed into the two existing Sleep commits, and `Sleep( 0 )` is not `sleep_for( 0 )`

This branch had already done the work, in two commits split by meaning rather
than by file:

- `replace non-portable Sleep with std::this_thread::sleep_for` - the real waits
- `replace Sleep( 0 ) with std::this_thread::yield()` - the spin-waits

Only seven sites were left when row 51 came up, and they were squashed back into
those two rather than kept as a third commit. Which one each site went to
follows the same split: `ParticleExporter.cpp` is a spin-wait and joined the
yield commit, the other six are real waits and joined the sleep_for commit.
Keeping that boundary matters, because the sleep_for commit's message says
`Sleep( 0 )` is deliberately excluded from it, and putting six `sleep_for` sites
in the yield commit would have made both messages false.

Those two commits also made the better call.

`Sleep( 0 )` on Windows yields the rest of the timeslice to another ready
thread of the same priority. `std::this_thread::sleep_for( milliseconds( 0 ) )`
is only required to block for *at least* the given duration, so it is entitled
to return immediately without yielding anything, which turns

```cpp
while ( !pObjInfo->GetValue() )
	Sleep( 0 );
```

into a pure spin. `std::this_thread::yield()` is the actual equivalent, and
that is what the tree already uses in all eight of those loops. Row 51's own
version would have written `sleep_for( milliseconds( 0 ) )` at every one of
them.

**What was actually left.** Six live calls and two commented-out ones, none in
a module the default build compiles: `ED_Common` and `MapEditorLib` are behind
`-DBUILD_EDITOR=ON`, and `Server` has no CMake wiring at all. The two comments
were updated so that a grep for `Sleep(` now comes back empty.

That last point is why this was worth finishing rather than skipping: the
remaining calls were invisible to the build, so nothing would have flagged them
until someone turned the editor on.

**The commit would not have compiled.** Its `ParticleExporter.cpp` hunk reads
`Sstd::this_thread::sleep_for( ... )`. `ED_Common` is editor-only, so nothing
ever built that file and the typo survived - which is the same reason these
seven sites were still here at all.

### Commit 52 deletes a parser two unbuilt utilities still call

`NCmdLine::CCmdLine` has no user in the game or in any module the build
compiles. It does have two: `dbcodegen/main.cpp` and `dbstruct/main.cpp`.
Neither is wired into CMake, so neither can be built today, and deleting the
parser leaves both referencing a header that is not there.

That is the trade the commit makes and it is the right one here. The generated
`DB*.h/.cpp` files are edited by hand alongside `types.xml` in practice, so
`dbcodegen` is not part of anyone's workflow, and reviving either utility means
wiring it into CMake anyway. `boost::program_options` is already available and
is what they should use when that happens.

**It also closes three roadmap entries by deletion.** `CmdLine.cpp` held one of
the two remaining `_vsnprintf` calls, one of the ten `-Wformat-security` sites,
and a `printf( charBuff )` where `charBuff` is a buffer the function itself
formatted - a format string bug of the kind the rest of that list is about.
Nine `-Wformat-security` sites remain, and `Misc/Tools.cpp` is now the only
`_vsnprintf` in the tree.

### Commits 53 and 54 are a pair, and the table has them in the wrong order

Row 54 is a manifest with `<activeCodePage>UTF-8</activeCodePage>`, which makes
`GetACP()` return 65001. Row 53 rewrites the conversion helpers to hardcode
`CP_UTF8` where they previously asked `GetACP()`.

`NStr` reads the code page dynamically, through
`static int gs_nCodePage = GetACP()`, so **row 54 on its own moves the whole
engine to UTF-8 with no code change at all**, and row 53 is then mechanical:
say UTF-8 instead of asking for it, and delete `UTF8ToMBCS` and `MBCSToUTF8`,
which have become identity functions.

Applied in table order, row 53 hardcodes UTF-8 while the process is still on
the system code page, so every non-ASCII string is mangled for exactly one
commit. That is unverifiable on its own and a bisect landing between the two
finds a bug that does not exist. 54 goes first.

Two things about the manifest: it needs **Windows 10 1903 or later**, and it is
**per executable**. `Game` is the only one built, but if the editor is wired up
it needs its own or its code page will disagree with the DLLs it shares. The
`.manifest` entries in `Game/CMakeLists.txt` are also unguarded by `WIN32`,
which was already true of `dpi_aware.manifest`.

### Commit 53 closed four items, and had four defects

**What it gets right.** It removes the fixed buffers rather than patching their
bounds, which takes out three of the overruns listed in the roadmap and one of
the `-Wformat-security` sites at the same time:

- `Logger.cpp` `CFileDumper` - the overrun and `Logger.cpp:32` both go, the
  write becoming `fprintf( f, "%s", szBuffer.c_str() )`
- `Logger.cpp` `CDebugDumper` - converts nothing now, calls `OutputDebugStringW`
- `LogStream.cpp:69` - gone with the `wchar_t[1024]`

Log lines also stop being silently truncated at 1024 characters.

**What was fixed on the way in.**

1. `WideToUTF8Converter` and `UTF8ToWideConverter` are held in function-local
   `static`s, and `iconv_t` is a stateful descriptor that cannot be used from
   two threads at once. The logger is reached from the loader, net and terminal
   threads. Wants `thread_local`.
2. Three editor call sites do not compile: `pstrText = WideToUTF8( wszText )`
   assigns a `std::string` to a `CString*` in `ObjectCollector.cpp` and
   `Tools_Resources.cpp`, and `pwszText = UTF8ToWide( rstrText )` assigns a
   `std::wstring` to a `wstring*`. `MapEditor` and `MapEditorLib` are behind
   `BUILD_EDITOR`, which is why it was never noticed.
3. `convert()` in both converters falls off the end with no return when the
   target is neither Windows nor Linux - the two `#if` blocks have no `#else`.
4. `CDebugDumper` loses its "append a newline if the string does not end in
   one" behaviour.

Also worth noting: `port/unicode.h` includes `<stdexcept>` and `<string>` twice,
and `IconvConverter`'s constructor throws, from a path the logger calls.

**The encoding question, settled by looking at the data rather than at strings.**
The worry with either commit was that the game's text is in the system code
page and would be mangled by a move to UTF-8. It is not:

| what | encoding |
|---|---|
| `texts.pak`, 4665 entries | 4589 UTF-16LE with BOM, 38 UTF-16 without, 38 ASCII |
| `patch1_texts.pak`, 79 entries | all UTF-16LE with BOM |
| `types.xml` | UTF-8, as it declares |
| engine sources, 3340 files | 2737 pure ASCII, 603 UTF-8, **zero** in any 8-bit code page |

Localised text is wide from end to end and never touches a narrow conversion,
which is why nothing looked wrong after the manifest landed. And because the
sources are UTF-8, the six live non-ASCII narrow literals - Russian assert
messages in `Entrenchment.cpp`, `UnitCreation.cpp`, `Shell.cpp` and
`Scripts.cpp` - were previously being converted to wide *as if* they were
CP1251, so the manifest fixes them rather than breaking them.

One of those literals is already damaged in the source and this does not fix
it: `Scripts.cpp` has `Give ACTION_COMMAND_LEAVE ...ommand`, where the `c` has
been replaced by a U+FFFD that has itself been through another round trip. That
is old corruption, not something the port introduced.

**Two consequences worth stating.** Hardcoding UTF-8 rather than asking
`GetACP()` means the game now **requires Windows 10 1903 or later**; on older
versions the manifest is ignored, `GetACP()` stays at the system code page, and
the hardcoded UTF-8 would disagree with it. And on Linux this needs `iconv`,
which glibc provides in libc; musl needs `-liconv` added.

### Commit 56: the fallback is not dead code after all

Row 45 left `CMemoryMappedFileFragment`'s direct-read path in place on the
grounds that only the Windows 9x branch of `CZipFile` reached it, and that this
commit would decide its fate. Removing the version check does not make it
unreachable, it changes what reaches it.

`mmf.MapFile( nTotalSize, false )` is now unconditional, and `IsMapped()` is
false whenever that fails. So the fallback stops being a 9x path and becomes
the handling for a mapping that could not be created - an archive too large to
map, or address space too fragmented on the 32-bit build. `data.pak` in a full
install is about 954 MB, which is not something to assume always maps on x86.

Note that mapping a file that size does not itself consume address space:
`file_mapping` creates the section object and each fragment maps its own small
window, so the 954 MB never has to be addressable at once. The failure this
guards against is real but not routine.

It is kept, therefore, and the two tests added in row 45 -
`UnmappedFileIsReadDirectly` and `UnmapFileReturnsToDirectReads` - are the only
coverage it has, since nothing in a normal run takes that branch.

**Four OS version checks remain**, none of them in code the Linux build
reaches: `Game/SysKeys.cpp:66` (`GetVersion() & 0x80000000`, again the 9x
family, around the low-level keyboard hook), `Input/Input.cpp:310`
(`GetVersionEx`, deciding whether to release the mouse under a debugger on
Windows 2000 and earlier), `GameX/FontInfo.cpp:117` and `FontGen/FontGen.cpp:294`
(`GetVersionEx`, NT versus 9x font handling). All four are dead in the sense
that the condition can no longer be true, and `GetVersionEx` is worse than dead:
it is deprecated and, without a compatibility manifest, reports 6.2 on anything
from Windows 8.1 onward, so those checks do not even measure what they think.

### Commit 57, the last row: fmt does not take printf specifiers

`02aaba7ca` replaces `swprintf` into a `wchar_t[1024]` with `std::to_wstring`
for the integer overloads, which is right, and with
`UTF8ToWide( fmt::format( "%g", dVal ) )` for the two `double` ones, which is
not. fmt takes `{}` replacement fields, not `%` conversion specifiers. A format
string of `"%g"` contains no replacement field at all, so fmt copies it through
verbatim and never looks at the argument: every double logged would have come
out as the two characters `%g`. It compiles, because fmt permits arguments a
format string does not consume.

The equivalent is `{:g}` - fmt's `g` presentation type is defined to match
printf's `%g`. `std::to_wstring` is not an option here either: it formats a
double as `%f`, so `0.5` becomes `0.500000` and every log line carries six
decimals.

Formatting wide directly, `fmt::format( L"{:g}", dVal )`, also drops the
`UTF8ToWide` round trip the commit had. It needs `<fmt/xchar.h>`, which is
where fmt keeps its wide-character support; the digits are always ASCII, so
converting them through UTF-8 was work that could not change the answer.

**Two smaller things.** The `swprintf` being replaced is the two-argument MSVC
one, which has no size parameter and is not the standard function of that name;
that alone made these lines unportable regardless of the buffer. And
`CDebugDumper` gains a non-Windows branch writing to stderr, which is where
`DbgTrc` already goes off Windows.

**That finishes the table.** What remains is in
[PORT_ROADMAP.md](PORT_ROADMAP.md), and `linux2` has still never been pushed:
CI has not run on any of it, so the x86 build in particular is unverified.

### The first CI run: what x86 is most likely to catch

`linux2` was pushed for the first time on 2026-08-22, after the whole table was
applied and verified on x64. CI builds **x64-Release and x86-Release** on every
push, so this is the first time any of it has been compiled 32-bit. Nothing
below is a known failure - it is where to look first if the x86 job goes red.

**Ranked by how likely they are to break, and why.**

1. **`System/FileReaders.cpp`, the mapping rewrite.** It casts
   `std::uintmax_t` to `int` in three places and `int` to `std::size_t` in two.
   Those are fine while a file is under 2 GB and are exactly the shape that
   warns, or worse, on a 32-bit target. It is also the only new code holding a
   `boost::interprocess::mapped_region`, whose `size_t` is 32-bit there.
2. **`port/unicode.h`.** `WideCharToMultiByte` takes `int` lengths and gets
   `static_cast< int >( value.size() )` from a `size_t`. Correct, but the cast
   is what x86 will scrutinise.
3. **The `_PC_24` path in `Misc/Win32Helper.h`.** `CPrecisionControl` is
   `#if BOOST_OS_WINDOWS && BOOST_ARCH_X86_32`, so **x86 is the only
   configuration that compiles that branch at all**. It has never been built.
   If anything in row 47 is wrong, this is where it shows.
4. **`Threads::Threads` and `Boost::interprocess`** were added to `System`'s
   link line. Both should be architecture-neutral, but they are new inputs to a
   job that has not run.
5. **`fmt::format( L"{:g}", ... )`** needs `<fmt/xchar.h>` to be present in the
   pinned fmt; it is, but only the x64 build has proved it compiles.

**What CI cannot tell us**, and what still has no coverage after a green run:

- The Linux build. CI is Windows-only; the WSL clone is the only thing that
  compiles GCC, and it has not been run against the finished branch.
- Anything behind `-DBUILD_EDITOR=ON`. Rows 51, 53 and 57 all touched editor
  files, and several of the defects found in the original commits were *only*
  possible because those files are never compiled.
- `Server`, `dbcodegen`, `dbstruct`, `ShaderCompiler`, `FontGen`: no CMake
  wiring at all. Row 52 knowingly left `dbcodegen` and `dbstruct` referencing a
  deleted header.
- Determinism. Nothing in CI plays a game, so rows 47 and 50 are covered only
  by the user having run them.

### Commits whose provenance was moved out of the message

The self-contained-history rule means a few things that were worth recording
cannot live in the commit text. They are here instead.

**Commit 38, UINT to unsigned.** Re-run on the current tree rather than
cherry-picked: 1972 occurrences across 291 files is one regular substitution,
and reconciling year-old hunks for it would be all risk. The re-run was checked
against the original commit's output rather than trusted:

| | |
|---|---|
| files touched | 291, exactly the original's set |
| byte-identical to it | 198 |
| differ in whitespace only | 87 |
| genuine textual differences | 0 |
| not comparable, file changed since | 6 |

The 87 are the old tool's doing: it stripped carriage returns and trailing
whitespace as a side effect. This pass is byte-faithful, confirmed with
`git ls-files --eol` showing index and working tree agreeing for all 291.

**Commit 37, WCHAR to wchar_t.** The original missed one occurrence, in
GText.cpp, the same file it edited. Included here, so none remains.

**Commit 35, __int64 to int64_t.** The original predates the ASYNC
diagnostics and left the RNG call counter untouched. The 18 further spellings
in System/RandomGen.h, System/RandomGenInternal.cpp and AILogic's AIUnit.cpp,
Shell.cpp, Shell.h and Soldier.cpp are included here.

**Messages rewritten 2026-08-22.** Four commits referred to the old branch and
were reworded in place with `git filter-branch --msg-filter`, rewriting 16
commits from the ResetCrashHandler one onward. Content was untouched: every
commit's tree hash is identical to before, verified pairwise. Safe because
linux2 has never been pushed; there is no origin/linux2.

### The table is not the whole branch, and its hashes are pre-rebase

Two corrections to how the replay was scoped, found while diagnosing a Linux
build failure on 2026-08-22.

**The hashes in the table above are stale.** The linux branch was rebased at
some point, so the same work exists there under different hashes: the table's
`accc2c5f9 remove redundant ./ for includes` is `6f5272b03` on the branch
today. Compare by subject, not by hash. Note also that several replayed
commits were reworded on the way in, so a subject comparison has false
positives too; the honest check is to read the diff.

**Fourteen commits follow the table's last row.** In branch order after
`6f5272b03`:

| | commit | note |
|---|---|---|
| 1 | `f50f043a5` make includes portable on case sensitive file system | 422 files, see below |
| 2 | `3c9c49bd9` replace custom memory mapping with boost::interprocess | |
| 3 | `dcbe48b67` replace SKillLoaderThread with portable std/boost functions | |
| 4 | `89da73090` replace _controlfp / _control87 with std::fenv | **determinism** |
| 5 | `31a3c89ad` replace IsDebuggerPresent with portable function | |
| 6 | `e340d84e9` replace __debugbreak with portable function | |
| 7 | `fe7fbeeb5` replace custom random generator with std::random | **determinism, the dangerous one** |
| 8 | `1c21a16ef` replace Sleep with std::this_thread::sleep_for | |
| 9 | `037859f8a` remove custom command-line parser as it's not used by game | |
| 10 | `c9bb3c1e0` unify replace Unicode conversion functions with portable | |
| 11 | `116bf57bb` make executable UTF-8 aware by default | |
| 12 | `bf1b8930a` use explicit format specifier | |
| 13 | `a5f1f029b` remove redundant Windows version check | |
| 14 | `02aaba7ca` use modern portable string conversion functions instead of swprintf | |

Number 7 deserves reading before it is reached. Every client in a lockstep
game must draw the same values in the same order from the same seed, and the
current generator is a specific algorithm whose state is saved and compared
(`NRandom::RecordCall`, `GetDebugState`, the checksum log). Swapping it for
std::random changes the sequence, and std::random's distributions are not
specified to produce identical output across implementations even from the
same engine and seed, which would make Windows and Linux clients disagree by
construction. Number 4 is the same territory: the FP control word is set
deliberately for multiplayer sync, `_RC_CHOP` while parsing database floats
and `_RC_NEAR` in GroupLogic.

### Include case is broken on a case-sensitive filesystem

Measured 2026-08-22 on linux2: **950 quoted includes** out of 11982 resolve
only when case is ignored, across 28 projects. Roughly 494 are in code the
default build compiles; `GameX` 91, `Stats_B2_M1` 83, `UI` 83, `AILogic` 80
lead, and `System` has exactly one, `XMLChunkSaver.cpp` asking for
`XmlReader.h` when the file is `XMLReader.h`.

This is not a regression. That include traces to `3726f2d9d Add engine
sources`, and `9f84eea1f normalize case for includes for portability` is
already an ancestor of linux2 with 329 commits after it, so the earlier pass
either was partial or was undone by later work. Windows resolves every one of
these regardless of case, which is why it stayed invisible.

`f50f043a5` on the linux branch addresses it in 422 files and 839 lines, which
is fewer sites than the 950 measured here, so when it is replayed the checker
should be re-run rather than assumed to have finished the job. The two passes
target the same goal and are worth combining.

The fix direction is always to change the include to match the file on disk,
never to rename files: the names appear in each project's CMakeLists SOURCES
list, which would then need to change too.

### Commit 44 is absorbed into the include-case pass, not applied

Measured on linux2 before deciding: of the 42 includes written with a leading
`./`, **all 42 are also case-wrong**, and none needs only the `./` removed.

| | |
|---|---|
| includes with a leading `./` | 42 |
| of those, also case-wrong | 42 |
| needing only the `./` stripped | 0 |

They share one shape: a file including its own header, lowercased.
`AILogic/BridgeCreation.cpp` asks for `"./bridgecreation.h"` when the file is
`BridgeCreation.h`; `Sound/MusicSystem.cpp` asks for `"./musicsystem.hpp"`.

Applying it on its own would be a commit that changes nothing on Windows,
where both spellings resolve, and fixes nothing on a case-sensitive
filesystem, where stripping the `./` still leaves `bridgecreation.h` not
matching `BridgeCreation.h`. It would also rewrite 42 lines that the case pass
then has to rewrite again, and the two would conflict.

So the `./` removal happens as part of the case pass: one edit takes
`"./bridgecreation.h"` straight to `"BridgeCreation.h"`. Each line is touched
once and every touched line resolves afterwards.

That closes the 44-row table. The work that follows it is listed under "The
table is not the whole branch".
