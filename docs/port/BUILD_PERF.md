# Build performance backlog

Compile and configure time on this tree. Not part of the port; kept here because it is measured against the same checkout.

Part of the Linux port notes; see [LINUX_PORT.md](../../LINUX_PORT.md) for the index.

---

## Build performance backlog (not part of the port)

Raised 2026-08-20 after a header edit rebuilt ~900 files. Nothing here is required for
the Linux port - do it separately, on a branch off `port`, so CI benefits too.

### Compile time - the tree as measured

| fact | value |
|---|---|
| `.cpp` translation units | 1357 |
| TUs that include a `stdafx.h` | **1328 (98%)** |
| `stdafx.h` prelude headers | 43 (one per project) |
| `target_precompile_headers` / `/Yu` | **used nowhere** |
| `UNITY_BUILD` | used nowhere |
| `CMAKE_CXX_COMPILER_LAUNCHER` / ccache / sccache | wired nowhere |

Hub-header fan-out (direct includers): `StrProc.h` 197, `Tools.h` 50, `DB.h` 45,
`BinSaver.h` 43, `Asserts.h` 43, `Basic.h` 41.

**The headline:** the `stdafx.h` files are VS2003-era preludes that the CMake port carried
over as ordinary `#include`s. CMake's PCH support was never wired up, so the prelude is
fully re-parsed 1328 times per clean build - and re-parsed again in every one of those
~900 files when a header changes.

Options, cheapest and safest first:

1. **PCH.** `target_precompile_headers(<tgt> PRIVATE stdafx.h)` per project, reusing the
   preludes that already exist. Lowest risk, likely the biggest single win. Caveat: PCH
   makes missing includes invisible, so a file that compiles with PCH may fail without it
   - which matters because the Linux build will not use the same prelude.
2. **sccache** via `CMAKE_CXX_COMPILER_LAUNCHER`. Caches across the 4 presets and across
   cherry-pick churn where the same files keep reappearing.
   `winget install Mozilla.sccache`. (clcache is dead - Python, MSVC-only, unmaintained.)
3. **Unity / jumbo builds.** Real gains, but this codebase is full of file-static helpers
   and short local names; expect ODR and name-collision fallout. Try per-project.
4. **Decoupling the hub headers.** The actual fix, and a large refactor. `StrProc.h` at
   197 direct includers is the worst offender. Touches determinism-critical code, so it
   is the highest-risk item on this list.
5. **C++20 modules - not viable.** Top-level `CMakeLists.txt` pins
   `CMAKE_CXX_STANDARD 17`, and the DLL-per-subsystem layout with
   `generate_export_header` does not map onto modules without a rewrite.

### Configure time

A `CMakeLists.txt` edit re-runs the *whole* top-level configure. Biggest cause is
`cmake/boost.cmake` not setting `BOOST_INCLUDE_LIBRARIES`, so all 158 boost libraries get
`add_subdirectory`d each time. The engine uses only `math`, `predef`, `sort` (plus
`boost/functional`). Also `-DFETCHCONTENT_FULLY_DISCONNECTED=ON` once deps are populated
- but it silently ignores `GIT_TAG` bumps, so turn it off after any commit touching
`cmake/`. Windows Defender exclusions: done 2026-08-20.

Do not share `FETCHCONTENT_BASE_DIR` between two build trees - it holds `<name>-build`,
used as the `add_subdirectory` binary dir, so two configs collide. `linux-configure.sh`
sets a shared one and is only safe while a single Linux build dir exists.

Measure before changing anything:

```powershell
cmake --preset Windows-x64-Release --profiling-format=google-trace --profiling-output=C:\Temp\cfg.json
# then open in https://ui.perfetto.dev
```

---


