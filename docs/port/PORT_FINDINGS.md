# Technical findings

Why things behave as they do in this codebase. Reference material, not a task list: read when something surprising happens, not to find work.

Part of the Linux port notes; see [LINUX_PORT.md](../../LINUX_PORT.md) for the index.

---

### `typename typedef` - MSVC-only laxity, hard error on GCC/clang

`3Dmotor/Cache.h`, three sites. `typename` and `typedef` are in the wrong order. MSVC accepts
it; GCC and clang reject it at **parse** time, so being inside a template does not defer the
error. This will stop the Linux build of `3Dmotor` as soon as it gets that far.

| line | current | should be |
|---|---|---|
| 25 | `typename typedef uint32_t pointer;` | `typedef uint32_t pointer;` |
| 114 | `typename typedef TAlloc<SElement> Alloc;` | `typedef TAlloc<SElement> Alloc;` |
| 115 | `typename typedef Alloc::pointer pointer;` | `typedef typename Alloc::pointer pointer;` |

Line 115 is a real bug rather than noise: `Alloc::pointer` *is* a dependent name and genuinely
needs `typename` - the two tokens are simply swapped.


### Macro placed after the return type

`3Dmotor/GShadowVolume.cpp:175` reads

```cpp
int BOOST_FORCEINLINE Float2IntScale( const float fpVar, const float fpScale )
```

Valid on both MSVC (`__forceinline`) and GCC (`inline __attribute__((always_inline))`), just
ugly. It was `int __forceinline` before the replay, so commit #3 preserved the original
ordering faithfully. Move the macro in front of `int` when convenient.


### `SPlane` / `SHMatrix` picked the wrong member as storage

Commit #14 (`3d33edfaa`) removed four unions from `Misc/Geom.h`. Three of the four choices
were right; two of them inverted the cheap and expensive sides.

The tell is what the accessors have to do:

```cpp
// rect: returns a reference, just indexes the storage. Free.
const CVec2 & v1() const noexcept { return v[0]; }

// SPlane: has to RECONSTRUCT a CVec3 on every call
CVec3 n() const noexcept { return {vec4.x, vec4.y, vec4.z}; }

// SHMatrix: same
CVec4 x() const noexcept { return CVec4{xx, xy, xz, xw}; }
```

When an accessor rebuilds a value instead of returning a reference to it, the wrong member
was chosen as storage.

**`SPlane`.** `.vec4` is used **4 times in the whole tree** (`3Dmotor/GShadowMap.cpp` and
`Geom.h` itself); `n` and `d` are used everywhere. Keeping them as real members:

```cpp
struct SPlane
{
    CVec3 n;
    float d;
    CVec4 AsVec4() const noexcept { return CVec4( n.x, n.y, n.z, d ); }
};
```

is the same 16 bytes and the same layout, kills the union just as well, puts the
reconstruction cost on the 4 rare uses instead of the common ones, and keeps `n`/`d` as
assignable lvalues so `Set()` and `RecalcDist()` need no change at all. Roughly 4 call
sites instead of ~15 accessor conversions.

**`SHMatrix`.** Keeping `CVec4 x, y, z, w` as members would make *both* faces free: `x()`
becomes a `const CVec4&`, and flat access is just `x.x`. As written, flat `xx..ww` is the
storage and the `CVec4` view reconstructs, costing 41 call-site conversions.

**The rect was done right** and needs no change: the union really was two redundant views
of the same four vectors, and keeping the array with `const CVec2&` accessors is the
natural shape.

**`reserved` is unnecessary.** It is never referenced. `w` already exists at offset 12 via
the `x, y, z, w` struct, so `struct { float u, v, q; };` gives a working `.w` with no dead
placeholder.

**Root cause worth fixing separately:** `CVec2() { }`, `CVec3() { }`, `CVec4() { }` are
*user-provided* default constructors, which makes those types non-trivially-default-
constructible and is what poisons any union containing them. `= default` fixes the
triviality directly. It would not have been sufficient on its own here, since anonymous
structs inside unions are a GNU/MS extension regardless, but it is the correct declaration
and costs nothing.

**Not urgent.** The churn that landed in lockstep simulation code (110 lines in
`AILogic`/`Common_RTS_AI`) is entirely the *rect* conversion, i.e. the one done right, and
`v1()` returns a reference into the same storage so the semantics there are identical. The
reconstructing accessors are confined to `3Dmotor`/`3DLib`, which is presentation. Redoing
`SPlane` now would re-touch those files for style rather than correctness.

---


## Scoping searches to code that is actually built

Roughly a third of the tree is not in the default build, so a plain `rg` over
`Sources/` produces findings nobody wants to act on. `scope.py` (untracked, in the clone
root) generates a whitelist:

```bash
python3 scope.py                       # uses out/build/Windows-x64-Debug/compile_commands.json
python3 scope.py path/to/other/compile_commands.json
```

It writes `built_files.txt`, paths relative to `Sources/`. Test membership with:

```bash
grep -qxF "AILogic/AIUnit.cpp" built_files.txt && echo built
```

### Why a directory blacklist is not enough

`libdb/RenameNode.cpp` **is** built and does:

```cpp
class CString;  // без такой ботвы #include "../../MapEditorLib/Interface_UserData.h" отказывается компилироваться!
#include "MapEditorLib/Interface_UserData.h"
```

so 7 `MapEditorLib` headers are compiled into the default `BUILD_EDITOR=OFF` build:
`ControlSelection.h`, `Interface_Builder.h`, `Interface_Controller.h`,
`Interface_FolderCallback.h`, `Interface_UserData.h`, `StringManager.h`, `Tools_Resources.h`.

That is why `scope.py` works in two layers: translation units from
`compile_commands.json` (authoritative), then the transitive `#include` closure from
those TUs. Quoted includes resolve against the including file's directory first, then the
`Sources/` root - `Sources/CMakeLists.txt:1` does
`include_directories(${CMAKE_CURRENT_SOURCE_DIR})`.

### Current numbers (x64-Debug, BUILD_EDITOR=OFF)

- 1000 translation units, +1108 headers reached = **2108 files in scope**
- Entirely out of scope: `ED_B2_M1` (361), `TestParsing` (209), `MapEditor` (186),
  `ED_Common` (66), `ELK_A7` (53), `Server` (38), `TestClient` (20), `ED_B2` (17),
  `TestDB` (16), `ShaderCompiler` (13), `dbcodegen` (10), `B2_MapEditor` (8),
  `ED_RTS` (7), `FontGen` (6), `librcs` (5), `dbindex` (4), `dbstruct` (4)
- `Stats_B2_M1` has 62 unused files - the `include_*.h` / `prefix_*.h` / `*.cll` leftovers
  of the `dbcodegen` generator. Verified nothing references them.

### Caveats

- The whitelist reflects **whichever configuration produced the database**. Regenerate
  after changing presets or toggling `BUILD_EDITOR`.
- Headers reached only through macro-built or computed include paths will be missed. None
  are known in this tree, but a "NOT BUILT" verdict on a header is weaker evidence than
  one on a `.cpp`.
- `compile_commands.json` lists TUs, not link membership: a `.cpp` compiled into an
  `EXCLUDE_FROM_ALL` target (the `3Dmotor` tests and benchmarks) still appears.

### Cross-check against `Contributing.md`

`Contributing.md` § *looking around the code* (lines 64-133) already documents every project,
grouped **main game / editor / server / other small utilities / test code / miscellaneous**.
That grouping is an accurate mental model - verified against `built_files.txt`:

- all **27 "main game" entries are built**; the group is exactly the default build set
- every other group is **not built**, with **one exception**: `MapEditorLib` sits under
  *editor* but has 7 files built, via the `libdb/RenameNode.cpp` include described above

So the quick rule is: **"main game" is the whitelist**, plus `DebugTools`, `port`, `vendor`,
plus those 7 `MapEditorLib` headers. `built_files.txt` is the file-level version of it.

Drift found while checking (the doc is CRLF, so parse it with `sub(/\r$/,"")`):

| entry | problem |
|---|---|
| `DebugInfo` | listed under main game but **no such directory**; the real one is `DebugTools` (built, 10 files), and the doc's link points at `Common_RTS_AI` |
| `TestParsing` | link points at `TestDB` |
| `DebugTools`, `port`, `vendor` | built but undocumented (`port` is new from this port) |
| `Arch`, `Backup`, `XDBRefsAnswerer`, `XDBWatcher`, `XDBWatcherClient` | not built and undocumented |

Worth a small PR to `port` after the replay lands - not during, since a dirty tracked file
blocks cherry-picks.

---


### String conversion: the policy, and the sites the replay never reaches

Settled 2026-08-21, after measuring the candidates rather than guessing:

| task | use |
| --- | --- |
| int to string | `std::to_string`, or `std::to_chars` where the allocation matters |
| float to string | `std::to_chars` or `fmt::format`; **never** `std::to_string`, whose float overloads are `"%f"`, six decimals, locale-dependent point |
| string to number | `std::from_chars` |
| formatted text | `fmt::format` (arrives tree-wide in commit 25) |
| avoid | `atoi`/`atof` (no error reporting, UB on overflow), `stoi`/`stof` (throws, locale-dependent), `stringstream`, `boost::lexical_cast` |
| defer | the 76 `sscanf` and 62 `sprintf` sites, to commit 25's fmt pass |

The deciding property is that **`from_chars`/`to_chars` are the only standard
conversions specified to ignore `LC_NUMERIC`**. Everything else reads the
decimal point from the process locale. Nothing calls `setlocale` today, so the
C locale holds, but SDL3 and graphics drivers can call it, and in a lockstep
sim a comma-decimal locale silently reparsing every float is the worst kind of
bug to ship.

Measured on g++ 15 and clang 21, `atof` -> `stof` is not value-preserving:

```
input                  (float)atof    std::stof      from_chars
1e-45                  1.40129846e-45 threw -> 0     1.40129846e-45
0.5000000298023224     0.5            0.50000006     0.50000006
3.4028236e38           inf            threw -> 0     out_of_range
```

`atof` parses to `double` then narrows, rounding twice; the other two parse
straight to `float`. libstdc++'s `stof` throws `out_of_range` for subnormal
results as well as for overflow.

**Sites still in the default build that no `linux` commit touches:**

| file | line | uses |
| --- | --- | --- |
| `Misc/BSAssertDialog.cpp` | 52, 131 | `itoa` |
| `Misc/BSDialogFunctions.cpp` | 67 | `itoa` (four more commented out) |
| `Script/scriptCommon.cpp` | 125 | `itoa` |
| `Script/Script.cpp` | 210 | `atof` for a script parameter default |
| `Input/Bind.cpp` | 798 | `_wtof` for a control coefficient |
| `Script/llimits.h` | 48 | `#define lua_str2number(s,p) strtod((s), (p))` |

`itoa` does not exist in glibc at all, so `scriptCommon.cpp` will simply fail
to compile. The two `Misc` dialog files are Windows-only UI and are probably
swept up by commit 27 (`exclude Windows-specific stuff from build`); the rest
are not.

`Script/llimits.h` deserves a commit of its own. It routes every numeric
literal in every Lua scenario script through `strtod`, and scenario scripts
drive the simulation.


### Floating point text has several valid forms, and which one is pinned

The same double can be written many ways, all correct. `to_chars` with no
format argument gives the shortest string that round-trips; with
`chars_format::general` and a precision it gives the printf `%g` form. They
are not the same text:

```
value 100000
  to_chars shortest      : 1e+05
  to_chars general P=16  : 100000
  printf %.16g           : 100000
value 1e-05
  to_chars fixed         : 0.00001
  to_chars general P=16  : 1e-05
```

This is why both conversions use the precision overload rather than the
default. `to_chars(v, chars_format::general, P)` is specified to produce what
`printf("%.Pg")` produces in the C locale, so matching the old output is a
guarantee of the wording, not an accident.

Checked anyway, on libstdc++, over 2 million random double bit patterns plus
the format switchover points plus a 200k sweep of `n/8` (where exact ties are
most likely): **byte-identical to printf for both P=7 and P=16**, exponent
padding included.

Two caveats worth holding on to:

- The standard pins the *shape* ("as if by printf"): where the point goes, when
  it switches to an exponent, how many exponent digits. The *digits* depend on
  the conversion being correctly rounded, which libstdc++ and the MS STL both
  are, but which was only measured here on libstdc++. This still lowers the
  cross-platform risk rather than raising it: before the change Windows went
  through MSVC's `sprintf` and Linux would have gone through glibc's, two
  independent implementations, and now both go through one specification.
- The bounds derived for the buffers are **tight, not generous**. `-DBL_MAX`
  hits exactly 15 bytes at P=7 and exactly 24 at P=16.

**Pre-existing and deliberately unchanged: Lua's precision of 16 does not
round-trip.** `max_digits10` for double is 17. At 16, 45.4% of random doubles
fail `tonumber(tostring(x)) == x`:

```
-1.7650700593638481e+106 -> "-1.765070059363848e+106" -> -1.7650700593638479e+106
```

That is Lua 4's original behavior and the commit keeps it, because raising the
precision to 17 would change the text every scenario script sees from
`tostring`. Worth revisiting on its own if a scenario is ever found sending a
number through a string and back, but it is a gameplay-visible change, not a
portability one.


### What libc++ showed that libstdc++ could not

`libc++-dev` and `libc++abi-dev` 1:21.1.6-71 come straight from Ubuntu's own
repo on resolute, same major version as the installed clang, so no third-party
apt source is needed to test the other implementation:

```bash
sudo apt install -y libc++-dev libc++abi-dev
clang++ -std=c++17 -stdlib=libc++ ...
```

Three findings, all from running the same probes twice.

**1. `to_chars(general, P)` is printf-identical on both implementations.**
2 million random doubles, a 200k `n/8` tie sweep and the extremes, at P=7 and
P=16, on libstdc++ 20260321 and libc++ 210108: byte-identical to
`printf("%.Pg")` every time. The claim that the emitted text is unchanged now
rests on two independent implementations rather than one.

**2. `from_chars` disagrees on out-of-range input, and the difference is
invisible only because the code checks the error code.**

| input | libstdc++ | libc++ |
| --- | --- | --- |
| `float` `"1e39"` | `ec=out_of_range`, **value untouched** | `ec=out_of_range`, **value written = inf** |
| `float` `"1e-46"` | `ec=out_of_range`, value untouched | `ec=out_of_range`, value written = 0 |
| `double` `"1e309"` | `ec=out_of_range`, value untouched | `ec=out_of_range`, value written = inf |

The standard says the value is unmodified when `ec` is set, so libstdc++ is
right and libc++ writes anyway. Both report the same `ec`, which is why every
conversion added here behaves identically on both: they all test
`res.ec != std::errc()` and return 0 before ever reading the value.

**This is the rule to keep.** Written the tempting way:

```cpp
float f = 0;
std::from_chars( p, end, f );   // no check
return f;                        // 0 on libstdc++, inf on libc++
```

the same input silently produces a different number per platform, which for a
lockstep sim is the worst possible shape of bug. Always check `ec`.

**3. `__cpp_lib_to_chars` is not defined by libc++ 21, even though floating
point `from_chars` and `to_chars` both work.** Verified directly: parsing
`"0.1"` into a float and a double, and formatting with
`chars_format::general` and a precision, all succeed. The feature test macro
is simply not advertised, most likely because some corner such as
`long double` is unfinished.

So `#ifdef __cpp_lib_to_chars` is **not** a usable gate for this on libc++,
and none of the code added here uses it. (`conv_probe.cpp` in the scratchpad
does, which is why it reports "no floating point charconv" under libc++; that
is the probe being wrong, not the library.)


### `Sleep( 0 )` is a yield, not a sleep

Commit 19 upstream converts one `Sleep` call. The default build had twenty,
splitting cleanly into two kinds that need different replacements.

Twelve are real waits and become `std::this_thread::sleep_for`. On Windows,
MSVC implements that on top of `Sleep`, so the granularity is unchanged.

Eight are `Sleep( 0 )`, which is **not** a zero length sleep: it gives up the
rest of the timeslice to another ready thread of the same priority. The
portable equivalent is `std::this_thread::yield()`. Translating them to
`sleep_for( milliseconds( 0 ) )` would have been silently wrong, because a
non-positive duration is allowed to return immediately, and does:

```
  empty loop body                         4.1 ns/call     libstdc++
  std::this_thread::yield()             237.3 ns/call
  sleep_for(milliseconds(0))              1.2 ns/call
  std::this_thread::yield()             197.1 ns/call     libc++
  sleep_for(milliseconds(0))              0.0 ns/call
```

At the cost of an empty loop body, eight polite spin loops would have become
hot ones, each burning a core: the GPU fence waits in `3Dmotor`, the resource
wait in `GResource.h`, the video frame wait in `GBinkPlayer.cpp`, and the
short-wait branch of the frame limiter in `MainLoopInternal.cpp`.

One related site left alone: `Sound/SoundEngine.cpp` sleeps a computed
`nSleepTime` clamped to the range 0 to 1000, so it can be 0 and used to yield
in that case. It is the sound-log replay debug path, where a no-op instead of
a yield does not matter.

Not touched, being outside the default build: `Server/ServerApp.cpp`,
`Server/Terminal.cpp`, `MapEditorLib/CommonExporterMethods.cpp`,
`MapEditorLib/InteractiveProcess.cpp`, `ED_Common/ParticleExporter.cpp`.

Unrelated but noticed while editing: `UI/GBinkPlayer.cpp` is CRLF while its
neighbours are LF, so any scripted edit has to match and emit the line ending
the file already uses rather than assuming LF.


### Token pasting: `##` onto a punctuator

The registration macros wrote `name##::New##name`. `##` only means something
when the two tokens have to fuse into one, and `CFoo` and `::` cannot; the
standard leaves it undefined when the paste does not form a valid
preprocessing token. MSVC accepts it and produces the intended text, so the
bug is invisible on Windows. Both other compilers refuse:

```
g++    error: pasting "CFoo" and "::" does not give a valid preprocessing token
clang  error: pasting formed 'CFoo::', an invalid preprocessing token
```

and the same in reverse for `&CCommands::##FuncName`:

```
g++    error: pasting "::" and "ParseX" does not give a valid preprocessing token
clang  error: pasting formed '::ParseX', an invalid preprocessing token
```

The fix is to stop pasting where nothing needs to fuse. `New##name` still
pastes because `NewCFoo` really is one token. The expansion is character for
character what MSVC produced, so no call site changes.

**This is not what Boost.Preprocessor's `BOOST_PP_CAT` is for.** That solves a
different MSVC problem: arguments not being expanded before they are pasted,
which needs the usual two-level indirection, plus the traditional
preprocessor's broken `__VA_ARGS__` handling, which is why
`BOOST_PP_VARIADICS_MSVC` exists. No `CAT` helper can rescue a paste whose
result is not a token; the only fix is not to paste. Boost.Preprocessor would
be the right tool if these macros ever need to expand an argument before
pasting it, which they currently do not.

Sites, all found by grepping for `##` next to a punctuator in either
direction:

| site | paste | status |
| --- | --- | --- |
| `System/System.h` x3 | `name##::` | fixed, in build |
| `B2_M1_World/UpdatableWorld.cpp` | `Name##::` | fixed, in build |
| `Server_Client_Common/Commands.cpp` | `::##FuncName` | fixed, in build, **missed by upstream** |
| `MapEditorLib/DefaultFactoryCpp.h` | `I##TYPE##*` | not in the default build |
| `Server/Server.cpp`, `TestClient/TestClient.cpp` | `::##FuncName` | not in the default build |
| `System/Text.h` x2 | several | inside commented out macro definitions |


### Extra qualification on member

Inside class X a member may not be *declared* as `X::name`. MSVC accepts it;
the others do not:

```
g++    error: extra qualification 'X::' on member 'f' [-fpermissive]
clang  error: extra qualification on member 'f'
```

Note the `[-fpermissive]` tag: GCC will downgrade this to a warning if asked.
That is not worth doing, since the qualifier carries no meaning at all here.

Two sites in the default build, one of which upstream misses:

| site | declaration |
| --- | --- |
| `System/ConsoleBufferInternal.h:30` | `int CConsoleBuffer::operator&( IBinSaver & )` |
| `AILogic/GeneralInternal.h:138` | `virtual int CGeneral::RequestForSupport( ... )` |

The out-of-line definition in `GeneralInternal.cpp:100` keeps its qualifier,
which is where one belongs.

Three related spellings stay legal and were deliberately not touched, all of
which the codebase uses: a qualified **return type** inside the class
(`CCmdLine::EProcessResult AcceptValue(...)` in `System/CmdLine.cpp`, since
deleted by row 52), a
**call to a base member** (`CCommonStaticObject::GetPlayer()` in
`AILogic/StaticObject.h`), and an **explicit destructor call**
(`CNetCompressor::~CNetCompressor()` in `System/Cruncher.h`, used to
reinitialise in place with placement new).

The detector for this lives in `extraqual3.py` in the scratchpad. It keys on
the qualifier matching the nearest enclosing class and on `X::name` being
followed directly by `(`, which is what separates a declarator from a return
type. It does not strip string literals, so `NI_ASSERT( ..., "CFoo::Bar():
..." )` shows up as a false positive; the only remaining real site it finds is
`ED_Common/ParticleExporter.h:15`, outside the default build.


### `GetTickCount` to `std::chrono`: the width is the whole problem

The table warned "epoch AND integer width change". The epoch turned out to be
harmless; the width was not.

**Epoch: no issue.** Every caller measures an interval, and `steady_clock` is
the right choice precisely because `system_clock` steps when the wall clock is
corrected. The standard leaves the epoch unspecified, but nothing depends on
where zero sits. Even the game clock is safe: `MainLoopInternal` feeds
`IGameTimer::Update`, and `CScaleTimer::Update` only uses `time - prevTime`,
with `prevTime == 0` as the "not started" sentinel.

**Width: a real defect in the upstream commit.** It defines the helper as
returning `uint64_t`. Callers store readings in `uint32_t` fields, in
`NTimer::STime` (which is `uint32_t`) and in `float`, then subtract two of
them. Mixing a 64-bit "now" with a stored 32-bit value breaks the subtraction
once uptime passes 2^32 ms, about 49.7 days: the stored value has wrapped, the
live one has not, and a small interval reads as an enormous one. With
`GetTickCount` both sides were 32 bits and unsigned wraparound cancelled.

Affected pairs, all in the default build:

| site | pair |
| --- | --- |
| `GameX/InterfaceMissionInternal.cpp` | `GetCurrentTimeMilliseconds() - NInput::GetLastEventTime()`, the latter an `STime` |
| `3Dmotor/FrameTransition.cpp` | minus `static unsigned int nStartTime` |
| `libdb/GameDatabase.cpp`, `System/WinVFS.cpp`, `System/Profiler.cpp` | minus a `uint32_t dwStartTime` |
| `Client/PlayGameProcessor.cpp` | packet timeout against a stored tick |

The fix belongs in the helper, not in nine call sites: **return `uint32_t`**,
matching `GetTickCount`'s own contract. Every existing difference then keeps
working unchanged, including across the wrap, and no call site needs an
explicit cast that a later reader would have to re-derive.

**Determinism: nothing here reaches the lockstep simulation.**

- the `AILogic` site is the `PathfinderTest` console command printing a duration
- `3Dmotor/RandomGen.cpp` and `GameX/*` seed `NWin32Random`, the non-lockstep
  generator, and `System/RandomGenInternal.cpp` uses it for entropy gathering
  before the deterministic stream is seeded
- the `SceneB2/RiversBuilder.cpp` site is inside a comment, so the "watch
  water" note on this row was a false alarm; the real water animation is in
  `3Dmotor/GMaterial.cpp` and is presentation only

Also worth knowing: `Client/ConnectionEffortInternal.cpp` stores tick values in
**`float`**, which has 24 bits of mantissa, so it loses millisecond resolution
after about 4.6 hours of uptime and gets steadily coarser. That is
pre-existing, unchanged by this commit, and its own bug if connection timing
ever misbehaves on a long-running machine.


### Checking the code no build compiles

`_DO_ASSERT_SLOW` is defined nowhere, and `NI_ASSERT( x, text )` expands to
`((void)0)`, so the preprocessor throws away **both** the condition and the
text. Roughly 620 `fmt::format` calls and every assert condition in the engine
are therefore never compiled by any build that currently exists. That is a
large blind spot, and commit 25 walked straight into it.

**A full assert-enabled build is the wrong tool**: it is a global define, so a
reconfigure plus a complete rebuild. The cheap equivalent reuses the existing
`compile_commands.json`, adds `/D_DO_ASSERT_SLOW`, and runs `cl.exe /Zs`
(syntax check, no code generation) over only the translation units that
actually contain an assert carrying a `fmt::format`. That is 138 of 2062.

```bash
python3 scratchpad/assertcheck.py assertcheck.bat   # regenerates from the DB
cmd /c assertcheck.bat > assertcheck.log 2>&1
```

The script picks up `vcvars64.bat` from VS 18 Community and the fmt include
from the RelWithDebInfo build tree. It is worth re-running after any commit
that touches assert text.

**Two blockers it found immediately**, both invisible to normal builds:

1. *Introduced by commit 25.* `NI_FORCE_ASSERT` passes the text to
   `OutputDebugString( LPCSTR )` and `NBSU::ReportAssert( ..., const char *,
   ... )`. `StrFmt` returned `const char *`; `fmt::format` returns
   `std::string`, so all ~620 conversions failed to compile. Fixed in that
   commit by binding the text to a local `std::string` inside the failure
   branch, which takes both spellings and costs nothing unless the assert
   fires.

2. *Pre-existing.* `ResetCrashHandler()` is called in `Asserts.h` and declared
   nowhere. It belonged to the in-house exception filter that the crashpad
   commit removed. Dropped in its own commit.

**A third class the run is turning up: rot in assert conditions.** The
condition is discarded along with the text, so it has not been compiled in
years and has drifted. `Game/main.cpp:279` reads

```cpp
NI_ASSERT( nLastQuotePos != string::npos, ... );
```

with a bare `string::npos` where every neighbouring line says
`std::string::npos`, and the file has no `using namespace std`. These are not
port problems and not commit 25's doing; they are the cost of code that no
compiler has looked at. They belong in their own cleanup, separate from the
replay.

### The memory mapping rewrite, and why it fought back

Analysis of `3c9c49bd9` before replaying it. The commit is 3 files and looks
small; it is not.

#### What the existing code does

`CDataStream` is not a stream in the usual sense. It holds four raw pointers
into a buffer, `pBuffer`, `pBufferEnd`, `pFileEnd`, `pCurrent`, and reads and
writes are `memcpy` against `pCurrent`. When a write passes `pBufferEnd`,
`FixupBuf` picks a new size, `min(current + 1MB, current * 2)` rounded down to
4K, and calls `AllocBuf`.

For a mapped stream that means, on every growth: unmap the view, close the
mapping, set the file size, create a new mapping, map a new view, and re-derive
all four pointers from the new base address. The base moves each time. Any
pointer a caller kept into the old buffer is dangling. That is the "constant
remapping" the port has to preserve.

#### What boost genuinely simplifies

Offset alignment, and only that. `mapped_region` keeps an `m_page_offset`,
maps from the aligned base, and hands back the shifted address, so the manual
`dwAllocationGranularity` arithmetic in `CMemoryMappedFileFragment` can go.
Worth checking rather than assuming: `mapped_region.hpp:387` returns
`info.dwAllocationGranularity` on Windows, not the 4K page size, which is what
`MapViewOfFile` actually requires.

#### What boost does not simplify

Resizing. Neither `file_mapping` nor `mapped_region` can be resized, so growth
is still destroy, resize, recreate. And it is worse than before in one specific
way: `CreateFileMapping( hFile, ..., 0, nSize, 0 )` **extends the file** to
nSize as a side effect. boost does not, so the port has to resize the file
explicitly, and a file cannot be resized on Windows while a mapping is open.
The ordering that was implicit becomes something the code must get right.

#### The defects in the commit as written

**The file is truncated on every read-write open.**

```cpp
if (access == STREAM_ACCESS_READ_WRITE) {
    std::ofstream{ pszName }.close();
}
```

`std::ofstream`'s default mode is `ios::out`, which truncates. The original
used `OPEN_ALWAYS`, which opens or creates and keeps the contents. So a
read-write `CMemoryMappedFile` starts by destroying the file it is about to
map, `GetFileSize()` then returns 0, and everything downstream reads nothing.
This alone accounts for short reads and for logic that silently sees an empty
file.

**Nothing holds the file open any more.** The original kept `hFile` for the
object's lifetime with `FILE_SHARE_READ`. The rewrite keeps only a filename and
re-resolves it in `GetFileSize`, `SetFileSize` and `MapFile`. Between calls the
file can be replaced or removed, and each call walks the path again. The
`SetFileTime` call that stamped the access and write time on a read-write open
is gone with it, silently.

**The filesystem calls throw.** `std::filesystem::file_size` and `resize_file`
are used without an `error_code`, so a missing file raises where the original
returned a value. `GetFileSize()` also writes to `file_size` as a side effect,
and `MapFile` depends on that write having happened to decide whether to grow.

**The no-mapping path is deleted.** `CMemoryMappedFileFragment::MapFile` had a
branch for when the file is not mapped, reading directly into a heap buffer,
and the rewrite constructs a `mapped_region` from whatever `GetMapping()`
returns. On a default-constructed `file_mapping` that raises
`interprocess_exception`.

That branch is reachable only through `CZipFile`, which maps the archive
`if ( ( GetVersion() & 0x80000000 ) == 0 )`. That is the Win32 `GetVersion`,
and bit 31 marks the Windows 9x family, so on anything NT-based the archive is
always mapped and the fallback is dead code. Removing it is defensible, but it
should be removed deliberately alongside the version check, which is what
`a5f1f029b` in this same series does.

#### If it is replayed

The safe shape, in order:

1. Keep a real handle open for the object's lifetime rather than a filename,
   so the file cannot move underneath and the size queries are cheap.
2. Open read-write without truncating.
3. Use the `error_code` overloads throughout; a missing file is a false
   return here, never an exception.
4. Resize only while nothing is mapped, and keep that invariant asserted the
   way the original asserted `hMapping == 0`.
5. Drop the granularity arithmetic in the fragment, which boost really does
   handle.
6. Leave the no-mapping fallback until the version check goes, then remove
   both together.

This one deserves a standalone harness rather than a syntax check: grow a file
from empty across several remaps, read fragments at unaligned offsets, and
compare the bytes against the same operations done with plain reads. The
failure modes here are silent, and a game that starts proves very little.

### A stale check batch fails in a way that looks like a broken tree

The sweep over 902 translation units reports failures as a count, and a run
that comes back with 185 looks like a serious regression. Every one of them was
the same thing:

```
fatal error C1083: Cannot open include file: 'Misc_export.h'
```

183 for `Misc_export.h`, one each for `libdb_export.h` and `UI_export.h`, and
no other error of any kind.

The first guess, that the generated headers had been deleted, was wrong. They
are never in the source tree: `cmake/exportheader.cmake` writes each one to its
target's binary directory and exposes that directory as a PUBLIC include, so
consumers pick it up through linking. The tree was fine.

What had gone stale was the batch. It is generated from
`compile_commands.json`, and that file had been rewritten by a later configure.
The batch still carried the older command lines, which predate those binary
include directories, so every translation unit reaching `Misc/Asserts.h` failed
before compiling a line. Nothing was wrong with the source at all.

So a failing run is worth classifying before believing. Group the errors first:

```bash
grep -oE 'error C[0-9]+' run.log | sort | uniq -c
grep -oE "Cannot open include file: '[^']+'" run.log | sort | uniq -c | sort -rn
```

If they are all C1083 on a `*_export.h`, the batch is stale rather than the
tree broken. Regenerate it from the current `compile_commands.json` before
reading anything into the result, and compare the batch's timestamp against the
database's when in doubt.
