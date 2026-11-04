# musl-libm

The transcendental functions the engine uses, `sin`, `cos`, `tan`, `asin`,
`acos`, `atan`, `atan2`, `exp`, `expm1`, `log` and `pow` in float and double, taken from
[musl](https://musl.libc.org/)'s libm and built by each platform's own compiler:
MSVC on Windows, gcc or clang on Linux. They are called `musl_sinf`, `musl_sin`
and so on; `include/musl_libm.h` declares them.

## Why

Lockstep multiplayer and replays need every client to compute the same bits.
The platform's libm does not give them: MSVC's CRT and glibc are both accurate
to within an ulp, but not the same ulp. Measured on 70 million float inputs,
each differs from the correctly rounded `cosf` on tens or hundreds of thousands,
and a campaign replay recorded on Windows diverged on Linux 400 ms in for one
such `cosf`. glibc's x86-64 build also selects FMA variants of several
functions by CPU at run time, so two Linux machines need not agree either.

The same C source, compiled under the rules below, gives the same bits from
both compilers. musl was chosen over CORE-MATH (correctly rounded, so it would
agree with anything) because CORE-MATH cannot be built by MSVC: it needs
`__int128` and GCC builtins throughout, and would bring clang-cl into the
Windows build. musl builds with MSVC as it is. See docs/Determinism.md.

## What is here

| Path | What it is |
|---|---|
| `src/` | musl's files, copied verbatim, at their upstream paths. Never edited. |
| `COPYRIGHT` | musl's licence (MIT), copied verbatim. |
| `UPSTREAM` | The repository and commit the copy was taken from. Written by `update.sh`. |
| `MANIFEST` | The list of upstream files copied. The only place the file set is named: `update.sh` copies it and `CMakeLists.txt` compiles the `.c` files in it. |
| `compat/` | Ours. Stand-ins for the musl-internal headers libm.h includes (`features.h`, `endian.h`, `fp_arch.h`), and `musl_libm_prefix.h`, force-included ahead of every file. |
| `include/musl_libm.h` | Ours. The public declarations. |
| `check-symbols.cmake` | Ours. The `musl_libm_symbols` test, see below. |
| `update.sh` | Ours. Refreshes `src/` from a musl checkout. |

The agreement test is `Misc/test/MuslLibm.cpp`, with this tree's code rather
than here, so that it gets the same allocator, hygiene and formatting as every
other test.

## How the files compile unmodified

`compat/musl_libm_prefix.h` is force-included (`/FI` on MSVC, `-include` on gcc
and clang) and does what would otherwise be patches:

- **Renames every symbol musl defines** to `musl_*`, public and internal alike,
  by macro. The system `math.h` is included first, under its real names, and
  only then are the names redefined, so the macros reach musl's code and none
  of the platform's. Without this the names collide: glibc's `math.h` declares
  `__sin`, `__cos` and `__tan`, which musl uses for its kernels. Names no
  compiler knows also cannot be treated as builtins and evaluated at compile
  time with a different library.
- **Refuses any floating point model but one**: `FLT_EVAL_METHOD` must be 0,
  which rules out x87 excess precision (32-bit Linux needs `-msse2
  -mfpmath=sse`) and `/fp:fast`.
- **Undefines `__FP_FAST_FMA`**, which gcc and clang define on any target with
  an FMA instruction and which switches `log`, `pow` and their tables to a
  different algorithm. MSVC never defines it. Measured: without the `#undef`, a
  `-march=haswell` build changes the `log` and `pow` hashes.

`CMakeLists.txt` adds `/fp:precise` on MSVC and `-ffp-contract=off` elsewhere,
so that neither compiler fuses a multiply and an add.

## The checks

- **`musl_libm_symbols`** (ctest, `obk2-test` label) lists the built library's
  symbols with `nm` or `dumpbin` and fails if musl defines anything not
  prefixed `musl_`, or calls the platform's own transcendental functions. What
  the library may still call from the CRT is `sqrt`, `fabs`, `floor` and
  `scalbn`, which IEEE 754 defines exactly.
- **`MuslLibm_test`** hashes each function's results over a fixed set of inputs
  (a stride through all 2^32 floats, pseudo-random doubles) and compares them
  with hashes recorded from the MSVC and the gcc build, which agree.

## Updating

Only for a reason: a bug that reaches the simulation, or a build fix. musl's
libm is pure functions with no allocation, I/O or parsing, so security fixes
are not expected to apply. Any change in results is a gameplay change.

1. Check out the musl commit you want, then run `sh update.sh /path/to/musl`.
   It copies everything in `MANIFEST` and records the commit in `UPSTREAM`.
2. Read the diff of `src/`. If a file now includes or calls something new, add
   it to `MANIFEST` and run `update.sh` again.
3. Build, then run `ctest -L obk2-test -R "musl_libm|MuslLibm"`, on Windows and
   on Linux.
   - `musl_libm_symbols` failing names a symbol to add to
     `compat/musl_libm_prefix.h`.
   - `MuslLibm_test` passing on both: the results did not change, and the
     update is a plain update.
   - `MuslLibm_test` failing on both with the same new hashes: the results
     changed, which changes the simulation. Record the new hashes (the test
     prints them), and in the same change bump the multiplayer net version so
     that old and new builds refuse each other, re-record any replays kept for
     verification (old ones will no longer verify), and say so in the commit
     message.
   - Failing on one platform only, or with different hashes on the two: the
     platforms no longer agree, and the update cannot go in as it is.
