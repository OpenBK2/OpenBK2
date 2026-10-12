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

**3. Most of the harness machinery already exists, but not an entry point.**
`CCommandsHistory` already stores the start RNG seed, per-segment commands, and a
per-segment and per-entry checksum log whose `AddChecksumLog()` returns false on
divergence. `CCheckSumSaver` (`Common_RTS_AI`) is an `IBinSaver` that CRC32s the
whole serializable simulation state. `ITransceiver` (`GameX/Transceiver.h`) is an
eight-method seam with `DoSegments()` as the tick pump, and
`CreateSinglePlayerTransceiver` already replays from a recorded history. The
"hash of the game state" is implemented and already wired into lockstep.

**Corrected by the Track G spike.** An earlier version of this finding said the
simulation was cleanly separable from the renderer because `AILogic` links
neither `3Dmotor` nor `SceneB2`, and concluded that a d3d9 stub was "the wrong
layer and unnecessary". The link graph is right but the conclusion was wrong:
mission bring-up feeds the simulation its terrain through `SceneB2`, so running
the real engine headless with a software device is in fact the cheap option and a
renderer-free binary is the expensive one. See `docs/TestHarness.md`.

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

Status: **done and committed.**

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

Done since: `ccache max_size` raised from 5 GiB to 100 GB, which it needed with
8800 cleanups recorded against a 28% hit rate across six configurations; the
`.gitignore` entries for `linux-build*`, `CMakeUserPresets.json` and `.out*`; and
the local trees wiped so the layout starts from the presets.

Remaining in this track: the old `build-*.sh`, `editor.sh` and `launch-tree.sh`
are untracked and now redundant.

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

**The spike is done. `docs/TestHarness.md` has the result, the options, the
control design, the fuzzing plan and the invariant work.** This is the ordering
and the conclusions that changed the plan.

The question the spike asked was whether mission bring-up is entangled with the
interface layer. **It is not.** `CreateAI()` is already exported, `IAILogic` is a
complete simulation API, the tick pump is ten lines, and the whole init sequence
lives in a 146-line file rather than in the 4380-line
`InterfaceMissionInternal.cpp`. A replay even carries its own map, so there is no
menu navigation to simulate.

**The blocker is terrain.** The simulation's height and passability data are
populated as a side effect of the renderer loading terrain, pushed through
`ITerraAIObserver` by `SceneB2`, and the AI's terrain is derived rather than
parsed: rivers carve the heights the AI reads. `CTerraGen` is about 10,936 lines
across 26 files, not the 703 an earlier estimate claimed, so extracting it is
weeks rather than days.

So the cost is not where the earlier roadmap put it. "Days if the interface is
clean, weeks if not" was the wrong axis entirely.

**The plan is therefore a flag on the real game, not a new binary.** Run `Game`
headless with a software Vulkan device (lavapipe is installed) driven by
`--replay`, which is purely additive: no simulation code is modified, so there is
no ASYNC risk to carry. The terrain extraction becomes a later optimisation,
triggered by fuzzing throughput rather than by correctness.

That also resolves the apparent chicken-and-egg problem, which was an artefact of
insisting the harness be a lean renderer-free binary. And the extraction itself
does not need the harness: its oracle is a byte comparison of the data crossing
`ITerraAIObserver`, which is exhaustive where a replay only samples.

Oracles, in the order they are worth building:

1. **Relational, no golden file, never rots.** Run twice, Windows against Linux,
   libstdc++ against libc++, one compiler at two optimisation levels,
   before-change against after-change. These answer the question the cleanup
   actually asks.
2. **Crash, hang and sanitizer reports** over a recorded corpus. Highest bug
   yield available, since a replay of real gameplay reaches far more code than
   any unit test.
3. **Lua scenario tests** for coverage of units and abilities no playthrough
   reached. 152 script commands already exist including orders, damage and
   observation; only three assertion commands are missing.
4. **Invariant checks**, starting from the eleven postconditions already
   enumerated by `CAIUnit::PrepareToDelete`, and generalised by an `IBinSaver`
   walk that detects any dangling reference to a reaped unit.
5. **Differential fuzzing**, with the randomness outside the simulation so each
   failing case is itself a replay file and therefore a regression test.

Explicitly not doing: simulated keyboard and mouse input, Python or ctypes
driving the live simulation, a parallel headless terrain reader, or a manual
coverage matrix. `docs/TestHarness.md` gives the reasons.

One correction to Track A's earlier assumption: a headless real engine makes the
`aarch64`-under-qemu and Fil-C ideas harder, since both assumed a renderer-free
binary. They need the terrain extraction first.

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
