# Cleanup and stabilization roadmap

Written 2026-10-11, at the point where `editor`, `linux` and `server` have all
landed on `port` and the porting work is largely done. The purpose is to have
one ordered list instead of a dozen good ideas competing for attention, and to
record the measurements and findings behind the ordering so they do not have to
be rediscovered.

Ordering principle: **anything that tells you whether a change was safe comes
before the changes.** Within that, cheap and reusable before expensive and
narrow.

---

## Findings that set the priorities

These came out of surveying the tree and are the reason the order below is what
it is.

**1. `AILogic` has no float-determinism flags on GCC or clang.**
`Versions/Temporary/Engine/Sources/AILogic/CMakeLists.txt:4` wraps
`add_compile_options("/fp:strict")` in `if(MSVC)`, and `cmake/arch.cmake` is
MSVC-only too. At the default x86-64 baseline there is no FMA to contract so
this is probably harmless today, but GCC defaults to `-ffp-contract=fast` and
clang to `=on`, so the moment `ARCHITECTURE` is wired up for non-MSVC, or a
second compiler is added, two builds will produce two different simulations.
The counterpart to `/fp:strict` is `-ffp-contract=off`. Cheap, and a
prerequisite for adding clang.

**2. Windows and Linux cannot agree on simulation checksums today.**
`AILogic` calls `exp` (9 sites), `tan` (6), `sin` (3), `log`, `acos` and
`atan2f`, plus `atan2f` and `exp` in `B2_M1_World` and `Common_RTS_AI`. `sqrt`
is IEEE-exact and fine; the transcendentals are **not** bit-identical between
the MSVC CRT and glibc. So cross-platform checksum agreement is unreachable
regardless of compiler flags until those ~20 call sites go through one shared
implementation. If cross-platform multiplayer is meant to work, this is a
shipping bug and not just a test-harness limitation. **Open question: is
cross-platform MP a goal, or is Windows/Linux agreement only wanted as a test
oracle?** The answer decides whether this is a fix or a documented limitation.

**3. The headless verification harness is mostly already built.**
`AILogic` does not link `3Dmotor`, `3DLib`, `SceneB2`, `Sound`, `UI` or
`Image` - the renderer lives one layer up in `GameX`, so the simulation is
already architecturally separable and **a d3d9 stub DLL is the wrong layer and
unnecessary.** `ITransceiver` (`GameX/Transceiver.h`) is an eight-method seam
with `DoSegments()` as the tick pump, and `CreateSinglePlayerTransceiver` already
replays from a recorded history. `CCommandsHistory` already stores the start RNG
seed, per-segment commands, and a per-segment and per-entry checksum log whose
`AddChecksumLog()` returns false on divergence. `CCheckSumSaver`
(`Common_RTS_AI`) is an `IBinSaver` that CRC32s the whole serializable
simulation state. The "hash of the game state" is implemented and already wired
into lockstep; what is missing is an entry point.

**4. `.gitattributes` is tracked and zero bytes.** No `.editorconfig`, no
`core.autocrlf`, no `core.eol`. Git has never normalized anything, so the repo
stores whatever each contributor's editor emitted since 2005. That is the root
cause of the line-ending churn, not a symptom.

**5. `CLAUDE.md` is stale on tests.** It says no `add_test()` is registered and
ctest finds nothing. `cmake/unittest.cmake` registers every test under label
`obk2-test`, `cmake/benchmarks.cmake` provides `add_suite_runner`, and
`Versions/Temporary/Engine/Sources/CMakeLists.txt:107` already defines a
`run-unittests` target that builds the suite and runs ctest. 51 tests across 19
test directories. Fix the file.

---

## Track A - local build and test ergonomics

Status: **done, uncommitted.**

`CMakePresets.json` is the single source of truth. The IDE, ctest, CI and the
three scripts all read it; nothing about a configuration is defined anywhere
else. There is no `CMakeUserPresets.json` - add one only to override something
for one machine, which is what that file is for.

- **Configure presets.** `Linux-x64-Debug` and `Linux-x64-Release` unchanged, as
  CI uses them, plus `Linux-x64-ASan+UBSan`, `Linux-x64-TSan`,
  `Linux-x64-Clang-Debug` and `Linux-x64-Clang-Release`. Hidden `base-linux` and
  `base-linux-clang` so each setting appears once. Build trees are
  `out/build/<preset name>`; install prefixes are `~/bk2`, `~/bk2-asan`,
  `~/bk2-tsan`, `~/bk2-clang`.
- **Build and test presets** for every configuration, the test ones filtering on
  `obk2-test` with `noTestsAction: error`, and carrying the sanitizer runtime
  options so the IDE's test runner gets them too.
- **`build.sh`, `test.sh`, `launch.sh`** - short names and sequencing only,
  replacing `build-release.sh`, `build-asan.sh`, `build-tsan.sh`,
  `build-ubsan.sh`, `editor.sh` and `launch-tree.sh`. `scripts/configs.sh`
  holds the one short-name-to-preset mapping all three share, and reads install
  prefixes back out of the configured tree's `CMakeCache.txt` rather than
  repeating them.
- **`launch.sh --data NAME`** absorbs `launch-tree.sh`: the configuration
  argument picks the code, `--data` picks the game data, so
  `./launch.sh asan+ubsan --data retail` works without remembering `BK2_BIN`.
- `docs/Sanitizers.md` - where the per-sanitizer comments went.

Two things found while doing it, both worth remembering:

- **ccache was already wired**, by `cmake/ccache.cmake`, with
  `CCACHE_SLOPPINESS=pch_defines,time_macros` that is *mandatory* with
  precompiled headers. Setting `CMAKE_CXX_COMPILER_LAUNCHER` from a preset
  shadows that launcher and silently makes every PCH compilation uncacheable.
  Use `-DUSE_CCACHE=OFF` (`./build.sh --no-ccache`) to turn it off, which is
  what an object-file comparison needs.
- **The DXVK `PKG_CONFIG_PATH` is required, not inert.** `cmake/dxvk.cmake`
  exists now and `pkg_check_modules(... REQUIRED)` it; the comment in the old
  `build.sh` predated that file. It is in `base-linux`'s environment with
  `$penv{PKG_CONFIG_PATH}` appended, so CI's own value - exported through
  `GITHUB_ENV` by `.github/actions/setup-linux-build` - still survives.

Remaining in this track:

- `ccache max_size` is 5 GiB with 8800 cleanups recorded and a 28% hit rate: it
  is thrashing, far too small for six configurations of this tree. `ccache -M
  100G` or similar.
- `.gitignore` covers `linux-build/` and `out/build` but not `linux-build-*`,
  `build-linux/`, `.out*`. Add them, or just rely on the wipe.
- The old `build-*.sh`, `editor.sh` and `launch-tree.sh` are untracked and now
  redundant.

## Track B - `ENABLE_COVERAGE`

Next, because coverage is what tells you a test corpus is weak instead of you
guessing, and it steers every later investment. Design notes are in
`docs/Sanitizers.md`. Use clang source-based coverage, not gcov. Do not verify
determinism from a coverage build.

## Track C - determinism correctness

Cross-platform multiplayer is the stated goal, so this track grew. The hazards,
the detection matrix, the concrete sites found so far and the rules for new code
are in **`docs/Determinism.md`**; that file is the working list and this is just
the ordering.

1. Put `NRandom::GetRandomCallsCounter()` into the per-segment checksum. Cheapest
   item on the whole roadmap and it makes a class of divergence loud instead of
   latent, so it comes first.
2. `-ffp-contract=off` for `AILogic` on GCC and clang (finding 1). Small, blocks
   Track D.
3. The 22 `std::sort` comparators in simulation modules, which are a present
   cross-platform bug wherever ties are reachable.
4. The two live unspecified-evaluation-order sites in `AILogic/Scripts.cpp`.
5. libm: vendor openlibm or CORE-MATH for the ten functions that are not
   IEEE-exact. `sqrt` and `fmod` are exempt.
6. Container iteration residue outside `AILogic`, found by differential replay
   rather than by reading 123 declarations.

Items 3 to 6 each change simulation output on at least one platform by
definition, so they are gated on the replay harness. Items 1 and 2 are not.

Undefined behaviour is part of this track, not just of hygiene: the compiler
preserves observable behaviour only where the program has none, so UB is where
two compilers legitimately disagree, and where one compiler disagrees with itself
across optimisation levels. That promotes the sanitizer work from crash-hunting
to determinism work.

Detection splits into four tiers, and which tier a hazard falls into decides
whether the answer is to run a tool, write a tool, or make the construct
unrepresentable. `docs/Determinism.md` has the full matrix; the planning
consequences are:

- **Tier 1, off-the-shelf dynamic.** Already built: ASan, UBSan, TSan. Two
  additions are nearly free: the UBSan `integer` and `implicit-conversion` groups,
  and a TySan tree for strict aliasing, which no other tool here covers and which
  matters because the build sets no `-fno-strict-aliasing`. TySan has to name
  `clang-21`, cannot share a tree with ASan or TSan, and costs 8.35x memory.
  MSan stays shelved: it needs an instrumented standard library. HWASan and
  tagged pointers are **not available on x86-64** at all.
- **Tier 2, off-the-shelf static.** Warnings from both compilers, plus a
  `.clang-tidy` that does not exist yet.
- **Tier 3, build your own.** The `isalnum` class has no instrument. Prefer
  eliminating the construct over detecting its misuse: wrap it so the bug cannot
  be written. One compile definition, `_GLIBCXX_ASSERTIONS`, covers the container
  half of the same category and is not enabled today. Three custom rules are
  worth writing, the cheapest being a guard against two RNG draws in one
  expression, which a plain regex already found six instances of.
- **Tier 4, undetectable.** Implementation-defined choices get pinned with
  `static_assert` rather than detected. Unknown UB and unspecified behaviour get
  found only by differential execution, which is the strongest argument for the
  harness: there is no authoritative list of UB in C++ to work through, so some
  residue is guaranteed and only running the same game under different conditions
  will surface it. The cheap axes are nearly free once the harness exists:
  `-funsigned-char`, libstdc++ against libc++, one compiler at two optimisation
  levels, `USE_MIMALLOC` on and off. A hostile conforming implementation of the
  DeathStation 9000 kind is not buildable for C++; the achievable version is an
  `aarch64` cross-build of `ReplayRunner` under qemu-user, which varies `char`
  signedness, alignment and libm while keeping layout comparable, and which is
  the natural acceptance test for the libm work. Big-endian is ruled out: raw
  memory serialisation would have to be rewritten first.

On the wider family of hardening proposals (SAL and GSL annotations, C++26
contracts, `-fhardened` and `/sdl`, Safe Buffers, the sanitizers, MTE and CHERI,
Safe C++ and Profiles, Cpp2, Carbon, Rust): `docs/Determinism.md` places them on
a single axis of invasiveness, since that is what their effort and their
guarantee both scale with. Two conclusions drive the plan.

**Nothing on that axis delivers determinism.** All of it targets memory safety.
No borrow checker or shadow runtime says anything about FP contraction,
evaluation order, unordered iteration or libm. So the hardening work is worth
doing for itself and must not be allowed to displace the harness.

**Two items are cheap enough to take without further analysis.** `-fhardened`
on the gcc presets, which brings `_GLIBCXX_ASSERTIONS` and `_FORTIFY_SOURCE=3`,
is ABI-safe against the prebuilt dependencies and changes no arithmetic; clang
needs the pieces spelled out and `/sdl` is the MSVC counterpart. And `/analyze`
in its own Windows CI job, which checks this code's Win32 calls against SAL
contracts that already exist in the SDK headers, with nothing to annotate first.

**Readily available tooling before anything needing a toolchain of our own.** A
compiler from an experimental branch is a second toolchain to keep green, a
reproducibility problem for CI and for anyone else building the tree, and a
dependency on a moving target, so it is last rather than first. The order is:
free flags, then warnings on new code only, then separate sanitizer trees, then
bounded source projects with a measured scope, and only then the exotic options
(Fil-C over `ReplayRunner`, an `aarch64` cross-build, a custom compiler for
profiles or contracts). `docs/Determinism.md` has the list.

Profiles themselves are not waitable: not in C++26, C++29 at the earliest, and
the lifetime profile is contested. The usable subset already ships under other
names, chiefly clang Safe Buffers as the bounds profile. Measured cost of
adopting it here: 102 diagnostics per `AILogic` translation unit, all of them in
headers and none in the `.cpp`, 44% of them third-party boost::uuid to suppress
rather than fix, and the project's own share concentrated in five headers. That
is a bounded project, not a sweep, but it is gated on C++20 for `std::span`,
since the tree is C++17 with no GSL.

Modular extraction, when it starts, goes in this order and not with `AILogic` as
a whole: `System/RandomGen` first, because it is both the most
determinism-critical component and small enough to prove bit-exact with golden
vectors; then `AIGeometry` and `PlanePathMath`; then the libm layer; then
`det_map` and `det_set`.

Two findings from surveying this changed what the track has to worry about.
Strings are excluded from the per-segment checksum, both `DataChunkString`
bodies being commented out, which removes the `wchar_t` width problem from the
cross-platform goal and at the same time means a passing checksum comparison
proves less than it looks. And `Common_RTS_AI/Checksums.h` has union type
punning inside the checksum routine itself.

## Track D - clang as a secondary compiler

Presets exist already. Pin it as secondary and never a gate: clang 22 here is a
development snapshot and will throw unrelated frontend noise. Stay on
libstdc++ - swapping in libc++ would change unordered-container iteration
order, which is an ASYNC risk in simulation code. (It is, for the same reason,
an interesting *later* audit: if the simulation still agrees under libc++, the
unordered-container sweep is clean.) The payoff is a different warning set
feeding `docs/CompilerWarnings.md`, plus clang-tidy, since
`compile_commands.json` is already exported.

## Track E - file hygiene

Branch consolidation removed the expensive part of this: there are no port
branches left to conflict with, so the churn is an ordinary commit and
`merge.renormalize` is a nice-to-have.

Measured over the in-scope set (`Versions/Temporary/Engine/Sources/**` sources
plus CMake files, 3452 files):

| property | count |
| --- | --- |
| LF only | 2505 |
| CRLF only | 106 |
| mixed within one file | 841 |
| trailing whitespace somewhere | 1593 |
| no final newline | 4 |

Encoding is in better shape than expected: 2894 ASCII, 543 UTF-8 without BOM,
1 with a BOM (`UI/ffmpeg_facade.h`), **zero cp1251 sources**. The only
non-UTF-8 in-scope files are the 8 `.rc`s, each non-ASCII solely because of one
`©` byte.

Scope: normalize `Sources/`, CMake files, `cmake/`, `scripts/`, `docs/` and the
root `*.sh`/`*.md`. Declare but do not touch `Complete/` (124,543 `.xdb`, which
are UTF-8 XML and which git would happily treat as text), `Versions/Current/Data`,
`Design/`, and `Versions/Temporary/Engine/Disk_J` (71,654 files, zero sources).
Lua `[[ long strings ]]` preserve newlines, so an EOL change inside one changes
the string value - two mixed-EOL Lua files and 7 cp1251 Lua scripts live under
`Versions/Current/Data`, which is one concrete reason to leave data alone.

Phases, one commit per property, each independently revertible:

1. Config only, no content change: `.gitattributes` (with `* -text` as the
   default so git never guesses, then explicit per-extension opt-in),
   `.editorconfig`, `scripts/hygiene/check.py` with a `--fix` mode, a
   `pre-commit` hook beside the existing `commit-msg`, an empty
   `.git-blame-ignore-revs`. Zero risk; land it early so new files are born
   correct.
2. EOL normalization, ~947 files.
3. Final newline (4) and BOM removal (1).
4. Trailing whitespace, 1593 files. One caveat file:
   `ED_B2_M1/test/SkeletonExporter.cpp` is the only source using `R"(` raw
   strings, where trailing whitespace is semantically live.
5. Enforce in CI; append phases 2-4 to `.git-blame-ignore-revs`.

Verification, in descending order of strength:

- **Prove the change carries no content.** For every touched file assert
  `normalize_eol(decode(old)) == normalize_eol(decode(new))` - decode both to
  Unicode, collapse CRLF and CR to LF, compare exactly. For phase 4 the
  predicate loosens to per-line `rstrip` equality, which is why it is a separate
  commit. Second invariant: **line count unchanged in every file**, which
  protects `__LINE__`, assertion text and anything keyed on line numbers.
  `git diff -w` is not a substitute; it would also hide a real change inside a
  string literal.
- **Byte-identical object files** before and after. Measure whether the build is
  reproducible at all first, by building twice from an identical tree. Compare
  objects, not DLLs: MSVC embeds a per-TU source hash in `/Zi` records and the
  PE and PDB carry timestamps and GUIDs. Linux is the cheaper lab.
- `regenerate-db` and `scripts/check_db_schema.py` clean. `.cll` files are in
  scope and dbcodegen takes the last `//` line as a tooltip, so trailing
  whitespace could in principle leak into generated `DB*.h`.
- Find out what regenerates `Versions/Current/Data/index.bin` before starting,
  so its churn can be told apart from the hygiene churn.

## Track F - `/utf-8`

Separate from Track E, because it is a **behaviour change, not hygiene.** There
is no `/utf-8`, `/source-charset` or `/execution-charset` anywhere in the build,
so MSVC reads the 543 UTF-8 sources as the system ANSI codepage. For the ~535
where the non-ASCII is only in Russian comments that is cosmetic. Eight files
have non-ASCII inside literals and need triage first:

- `libdb/test/EditorDatabase.cpp:327`, `TestDB/TestType.h:174`,
  `TestDB/TestType.cll:90` - `L"Очень клёвый wstring"` and friends. These cannot
  be decoding correctly under MSVC today, and GCC and clang read the same bytes
  as UTF-8, so the two toolchains already disagree. The fixtures presumably
  encode whatever MSVC produces.
- `AILogic/{Entrenchment,UnitCreation,Shell,Scripts}.cpp` - Russian `NI_ASSERT`
  and `CHECK_ERROR` text. `Scripts.cpp:627` looks pre-corrupted, with Cyrillic
  spliced mid-word into an English message.

## Track G - headless verification harness

See finding 3: this is an entry point, not a new system.

Before quoting it, **spike one question**: how entangled is mission bring-up
with the interface layer? `GameX/InterfaceMissionInternal.cpp` is 4380 lines and
the real `StartMission( pMap, Singleton<IAILogic>() )` is at line 4102. If the
simulation can be brought up from map plus slots without a
`CInterfaceMissionInternal` alive, this is days; if the interface owns
simulation-relevant state, it needs an extraction first.

Oracles split into two kinds, and the distinction matters:

- **Relational** - compare two runs against each other: run-twice determinism,
  Windows vs Linux, before-cleanup vs after-cleanup. No golden file, so it never
  rots, and it answers the actual question ("did my change alter behaviour?").
- **Absolute** - compare against a recorded expected value. Needed for
  correctness, but every intentional gameplay change invalidates every golden
  file.

Prefer relational. Then, in order:

1. `ReplayRunner`: links `AILogic`, `Common_RTS_AI`, `libdb`, `Script`,
   `Stats_B2_M1`, `B2_M1_World`/`Terrain`, `System`. `--replay X`, pump
   `DoSegments()` to exhaustion, print per-segment checksums, exit nonzero on
   the first mismatch. Requires relocating `CSinglePlayerTransceiver` (a ~50-line
   self-contained class) out of `GameX`.
2. A golden replay corpus recorded once per product, which is what turns the
   manual testing from per-change into one-time.
3. The corpus under ASan and UBSan. Highest bug yield available: a 20-minute
   replay of real gameplay reaches far more code than any unit test.
4. Lua-authored test scenarios - the right answer to the coverage problem, and
   better than simulated input. A tiny map plus a script that spawns unit X,
   orders the ability, asserts the outcome. Authorable, diffable, one ctest case
   each, and it does not rot when the UI changes. Needs assertion commands
   (`Test.Assert`, `Test.ExpectDestroyed`, `Test.Finish`) registered at the
   existing site in `Script/Script.cpp`.
5. Headless multiplayer: two `ReplayRunner`s over `MPTransceiver` on loopback,
   asserting checksum agreement. Note `TestClient` is already headless but is a
   *lobby* load generator talking to the server through `CServerClient`, so it
   gives no simulation coverage as it stands.
6. Cross-platform differential fuzzing: random command streams, no golden file,
   oracle is "both platforms agreed, nothing crashed, no sanitizer report".
   Blocked on Track C finding 2.

Explicitly **not** doing: a d3d9 stub DLL (wrong layer, see finding 3);
simulated keyboard and mouse input (most fragile possible layer, and the replay
system already records at the command level); a manual coverage matrix in a
spreadsheet (that is the labour being eliminated - use coverage data instead).
Valgrind and heaptrack are low priority next to ASan, which catches a superset
of the memory errors far faster; heaptrack is worth one footprint audit, not a
per-change gate.

## Track H - the actual code cleanup

Everything in `docs/*-estimate.md` (engine replacement, graphics API, logging,
Lua, XML parser, zip archive), `docs/nih-inventory.md`, the
`docs/CompilerWarnings.md` campaign, dead code removal, and the
`det_map`/`det_set` audit. Gated on Tracks A, B, E and G, because none of it is
verifiable without them.

---

## Suggested order

1. Track A - done; review and commit.
2. Track B - `ENABLE_COVERAGE`.
3. Track C item 1 - `-ffp-contract=off`.
4. Track D - clang secondary, triage its warnings.
5. Track E phase 1 - hygiene config, zero content change.
6. Track G spike - can `AILogic` be stepped without `GameX`?
7. Track E phases 2-5 - the normalization churn.
8. Track G items 1-3 - `ReplayRunner`, corpus, corpus under sanitizers.
9. Everything else, in whatever order the harness makes safe.

## Open questions

- Is cross-platform multiplayer a goal, or is Windows/Linux agreement only
  wanted as a test oracle? (Decides Track C item 2.)
- Can current builds load replays recorded by retail BK2? Chunk ids are stable
  by the serialization rules, but `SMultiplayerReplayInfo` has clearly grown
  fields. If old replays will not load, the corpus must be recorded against this
  build and re-recorded on every intentional gameplay change, which changes the
  economics of Track G item 2.
- Which products gate CI - all four of retail, FoTR, Liberation and Total
  Conversion, or one baseline with the rest nightly?
- `.rc` files: currently LF and CI-green, so `rc.exe` is evidently fine with LF.
  Force CRLF anyway, or pin as-is? Same question for the 6 `.bat` files under
  `Sources`, where CRLF is genuinely safer for `cmd`.
- Trailing whitespace in the same PR as the EOL change, or a follow-up?
