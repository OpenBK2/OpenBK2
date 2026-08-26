# DXVK on Linux

Written 2026-08-25. Part of the Linux port notes; see
[LINUX_PORT.md](../../LINUX_PORT.md) for the index.

This is the setup and the shape of the work. What is still to do lives in
[PORT_ROADMAP.md](PORT_ROADMAP.md); the machine setup that predates DXVK,
including the fake DXSDK stub tree this replaces, is in
[PORT_SETUP.md](PORT_SETUP.md).

## What DXVK covers, and what it does not

[doitsujin/dxvk](https://github.com/doitsujin/dxvk) implements D3D9, D3D10 and
D3D11 over Vulkan. The `dxvk-native` fork that used to carry the non-Wine build
is archived because it was merged upstream: there is no `-Ddxvk_native` option,
the mode is implicit in the target. `meson.build` branches on
`if platform == 'windows'`, so passing a Wine cross-file produces DLLs and
omitting one produces native `.so` files. Native mode additionally does
`subdir('include/native')`, which is the half of this that matters most here.

Covered:

- **D3D9 itself.** `libdxvk_d3d9.so` exports `Direct3DCreate9`, which is what
  `3Dmotor/Gfx.cpp:646` calls.
- **Win32 base types and COM.** `include/native/windows/` ships `windows.h`,
  `windows_base.h`, `unknwn.h`, `objbase.h`, `oaidl.h`, `ocidl.h`, `ole2.h`,
  `rpc.h` and `rpcndr.h`. That is `HWND`, `HRESULT`, `IUnknown`, `GUID` and the
  `STDMETHOD` macros, so it also feeds `Misc/Win32Helper.h`. Expect it to take
  out a large part of the `HWND`/`HRESULT` error class, not just the Gfx layer.
- **The D3D9 headers**, by way of the `include/native/directx` submodule, which
  is Joshua-Ashton/mingw-directx-headers.

Not covered, and each is its own piece of work:

- **D3DX9.** See [What is left after DXVK](#what-is-left-after-dxvk) below.
- **DirectInput.** `Input/Input.cpp`, `Input/Bind.cpp`, `Input/Input.h` and
  `Input/Specific.h`, including the whole `DIK_*` scancode namespace.
- **Win32 windowing.** `SplashScreen`, `WinFrame`, `WinCursor`.

### HWND becomes SDL_Window*

This is the design consequence worth absorbing before writing any code. DXVK
Native does not treat `HWND` as an opaque handle it ignores; it defines it as
the WSI window pointer, so with the SDL3 backend **`HWND` is `SDL_Window*`**.

The windowing work and the DXVK work are therefore the same work. Porting
`WinFrame` to SDL3 and then handing its window to `Direct3DCreate9` is not two
steps that meet in the middle, it is one object that both sides already agree
about. Do not schedule them apart.

## Prerequisites

Measured on the WSL clone's box on 2026-08-25: Ubuntu 26.04 LTS, codename
resolute.

Already present there, nothing to do:

| tool | version |
|---|---|
| ninja-build | 1.13.2 |
| build-essential (g++) | 15.2.0 |
| cmake | 4.2.3 |
| pkg-config | 2.5.1 |
| python3 | 3.14.4 |
| mesa-vulkan-drivers | 26.0.8 |

Missing, all of it in the distro repository:

| package | candidate | why |
|---|---|---|
| `meson` | 1.10.1 | the build system |
| `glslang-tools` | 16.2.0 | provides both `glslang` and `glslangValidator`; `meson.build` does `find_program('glslang', 'glslangValidator')` and targets `vulkan1.3` |
| `libvulkan-dev` | 1.4.341.0 | the Vulkan loader and its `vulkan` pkg-config module |
| `libsdl3-dev` | 3.4.2 | the WSI backend, and since `cmake/sdl.cmake` now uses `find_package(SDL3)` off Windows, a build requirement of this project too |
| `vulkan-tools` | 1.4.341.0 | `vulkaninfo`, only to check the machine before blaming DXVK |

```bash
sudo apt-get install meson glslang-tools libvulkan-dev libsdl3-dev vulkan-tools
```

**On meson specifically**, since it is the one with three plausible answers: use
apt. DXVK's floor is 0.58 and the distro carries 1.10.1, so there is nothing to
gain from pip or from a source build, and a pip meson alongside an apt ninja is
a known way to end up with two toolchains that disagree. If some future DXVK
ever outruns the distro, `pipx install meson` is the escape hatch, deliberately
not `pip install` into the system interpreter, which Ubuntu refuses anyway
under PEP 668.

**On Vulkan headers**, there is no `vulkan-headers` package on Ubuntu and none
is needed: the headers are vendored as DXVK's `include/vulkan` submodule
(KhronosGroup/Vulkan-Headers), alongside `include/spirv`. `libvulkan-dev` is
there for the loader to link against. This is also why the clone has to be
recursive; the same applies to `include/native/directx`, without which there are
no D3D9 headers at all.

**On the WSL specifics**, `mesa-vulkan-drivers` is already installed, which
carries both the Microsoft `dzn` driver and the `lavapipe` software rasterizer.
Check what actually enumerates before assuming a GPU path exists:

```bash
vulkaninfo --summary | head -40
```

Bring-up does not need a hardware ICD. `VK_ICD_FILENAMES` pointed at lavapipe is
slow but correct, and correctness is what the first run is testing.

## Building DXVK

```bash
git clone --recursive https://github.com/doitsujin/dxvk.git ~/src/dxvk
cd ~/src/dxvk
meson setup --buildtype release --prefix ~/opt/dxvk build.native
cd build.native
ninja install
```

No `--cross-file`: that is the whole of what selects native mode.

`meson.build` hard-errors with *"SDL3, SDL2, or GLFW are required to build
dxvk-native"* if none of them is found. The options are `native_sdl3`,
`native_sdl2` and `native_glfw`, all `auto`, so installing `libsdl3-dev` is
enough to select SDL3 and nothing needs passing on the command line.

Configure reporting that it did not find GLFW, or SDL2, is expected and not a
problem. `auto` means take it if present, and the backend is selected at
runtime anyway, so an absent one costs nothing unless `DXVK_WSI_DRIVER` names
it.

Native mode sets `dxvk_name_prefix = 'dxvk_'` and `dxvk_pkg_prefix = 'dxvk-'`,
so the results are `libdxvk_d3d9.so` and a `dxvk-d3d9` pkg-config module. Check
both before going further:

```bash
find ~/opt/dxvk -name "libdxvk_d3d9.so"
PKG_CONFIG_PATH=~/opt/dxvk/lib/x86_64-linux-gnu/pkgconfig pkg-config --cflags --libs dxvk-d3d9
```

Meson's `libdir` defaults to the multiarch path, so on this box the library
lands in `~/opt/dxvk/lib/x86_64-linux-gnu/`. Use `find`, not a `lib*/` glob,
which does not descend that far. Confirmed 2026-08-25: the install produces
`libdxvk_d3d9.so` exporting `Direct3DCreate9`, `Direct3DCreate9Ex`,
`Direct3DCreate9On12` and `Direct3DCreate9On12Ex`, and pkg-config resolves
`dxvk-d3d9` to `-I<prefix>/include/dxvk -L<prefix>/lib/x86_64-linux-gnu
-ldxvk_d3d9`.

Only D3D9 is wanted here, so `-Denable_dxgi=false -Denable_d3d8=false
-Denable_d3d10=false -Denable_d3d11=false` will cut the build down once the
default configuration is known to work. Do that as a second step, not a first;
a failure in a trimmed build is harder to attribute.

### At runtime

**`DXVK_WSI_DRIVER` must be set. There is no default.** For this project:

```bash
export DXVK_WSI_DRIVER=SDL3
```

Accepted values are `SDL3`, `SDL2` and `GLFW`.

## The SDL3 duplication problem, settled

`cmake/sdl.cmake` used to FetchContent libsdl-org/SDL at a pinned commit and
build it as part of this project, while DXVK was compiled against the
distribution's.

The mechanism is worth being exact about, because the obvious guess is wrong.
DXVK does not link SDL3: `libdxvk_d3d9.so` lists only `libm`, `libc` and the
loader in its `NEEDED` entries, and reaches SDL3 through
`dlopen("libSDL3.so.0")` and `dlsym`. The binary carries those strings, along
with `libvulkan.so.1` and `DXVK_WSI_DRIVER environment variable unset`, which
is why the backend is an environment variable rather than a build option.

So there would not have been two SDL3 instances: the loader returns whichever
library with that soname is already mapped, which would be the game's own. The
hazard is version skew instead. DXVK was compiled against 3.4.2 and resolves
every entry point by name at runtime, so handing it a differently-versioned
`libSDL3.so.0` means a `dlsym` can come back null for something it needs.

Settled off Windows in favour of the system copy: `cmake/sdl.cmake` now calls
`find_package(SDL3 REQUIRED CONFIG)` there and keeps the FetchContent build for
Windows, which has no distribution to ask. The package config defines
`SDL3::SDL3-shared`, the same name `cmake/fmod.cmake` and `UI/CMakeLists.txt`
already link, so nothing downstream changed.

**`libsdl3-dev` is therefore a build requirement on Linux and configure fails
without it.**

SDL_mixer stays FetchContent on both platforms: Ubuntu 26.04 has no
`libsdl3-mixer-dev`, only `libsdl3-dev`. `SDLMIXER_VENDORED` is already off, so
SDL_mixer resolves SDL3 through `find_package` and builds against the same one.

## Wiring it into this build

This **replaces the fake DXSDK stub tree** described in
[PORT_SETUP.md](PORT_SETUP.md). That stub exists so `find_library` in
`cmake/dxsdk.cmake` resolves to something, and its empty archives were never
going to link.

Note that the reason PORT_SETUP gives for keeping the workaround outside git no
longer applies. It was there so an uncommitted edit to a tracked CMake file
would not collide with the cherry-pick replay, and the replay is finished. Patch
the tracked files.

The shape that changes the fewest files is a `cmake/dxvk.cmake` that defines the
same target names the engine already links, so no project `CMakeLists.txt` has
to know which backend it got:

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(DXVK_D3D9 REQUIRED IMPORTED_TARGET dxvk-d3d9)

add_library(dxsdk::d3d9 INTERFACE IMPORTED)
target_link_libraries(dxsdk::d3d9 INTERFACE PkgConfig::DXVK_D3D9)
```

with the top-level `CMakeLists.txt` including `cmake/dxsdk.cmake` on `WIN32` and
`cmake/dxvk.cmake` otherwise, and `PKG_CONFIG_PATH` carrying the DXVK prefix.

Two judgement calls worth making deliberately:

- **Keep the `dxsdk::` names or rename them.** Keeping them means zero churn now
  and a misleading name forever. Renaming to something like `d3d9::d3d9` touches
  every `CMakeLists.txt` that links D3D9, which is a small set. Renaming is the
  better end state; doing it as its own commit, before the DXVK work, keeps that
  diff separate from anything that can fail.
- **Do not add a `USE_DXVK` option yet.** One implementation per platform is
  fewer combinations to keep green, and CI builds Windows only, so a
  DXVK-on-Windows configuration would be untested by construction.

The other three stub archives are a separate question. `dxguid` is satisfiable
(the GUIDs are data, and DXVK's headers declare them), `dxerr` has no Linux
equivalent and its uses should be checked rather than stubbed, and `d3dx9` is
the item below.

## What is left after DXVK

### D3DX9 is a second problem wearing the same coat

DXVK implements `d3d9`, not `d3dx9`. `d3dx9_43.dll` is a closed Microsoft
utility library, and there is no native replacement to link against. The
roadmap counts D3D9 as one blocker covering thirteen translation units; it is
really two problems of quite different size.

**It ships the headers anyway, which makes this fail later than expected.**
The install carries `d3dx9.h`, `d3dx9core.h`, `d3dx9effect.h`, `d3dx9math.h`,
`d3dx9mesh.h`, `d3dx9shader.h`, `d3dx9tex.h`, `d3dx9anim.h`, `d3dx9shape.h` and
`d3dx9xof.h`, by way of the mingw-directx-headers submodule, and no library
matching `*d3dx9*` at all. So `GShaderFX.cpp` will compile once DXVK is on the
include path and then fail at link with undefined references to
`D3DXCreateEffect` and friends. Do not read a clean compile there as progress.

Surveyed 2026-08-25, case-insensitively, which matters because
`3Dmotor/GShaderFX.cpp` spells the include `<D3DX9.h>`:

| file | needs | status |
|---|---|---|
| `3Dmotor/GShaderFX.cpp` `.h` `.hpp` | `ID3DXEffect`, `ID3DXEffectCompiler`, `ID3DXEffectStateManager`, `D3DXHANDLE` | live blocker |
| `3Dmotor/GSceneInternal.cpp` | includes `D3DX9.h`, calls `InitShaderFX()` | live blocker |
| `Image/ImageDDSWrite.cpp` | `D3DXLoadSurfaceFromMemory`, `D3DXSaveTextureToFile` | already Windows-only |
| `ShaderCompiler/hlsl.cpp`, `ShaderCompiler.cpp` | `D3DXCompileShaderFromFile`, `D3DXAssembleShader` | not wired into the build |
| `3DLib/GGeometryUtil.cpp` | `D3DXCreateMeshFVF`, `ID3DXMesh` | inside a `/* */` block, dead |

So the entire live dependency is the D3DX Effects framework, inside
`GShaderFX`. Two things make that better than it sounds:

- The seam is narrow. `GShaderFX.h` exposes six entry points - `InitShaderFX`,
  `DoneShaderFX`, `CreatePixelShader`, `CreateVertexShader`,
  `CPixelShader::Begin`/`End`, `CVertexShader::Use` - over 391 lines of
  implementation, and only `GfxRender.cpp` and `GSceneInternal.cpp` include it.
- A second shader path already exists. `GfxRender.cpp:1639` and `:1654` go
  straight at `pDevice->CreatePixelShader` and `pDevice->CreateVertexShader`
  with no D3DX involved.

Treat it as its own project, after something runs.

### Suggested order

1. Build DXVK native against SDL3, install to a prefix, confirm
   `libdxvk_d3d9.so` and the native headers exist.
2. Settle the SDL3 duplication question.
3. Add `cmake/dxvk.cmake`, drop the DXSDK stub, and let the Gfx layer see
   `d3d9.h` and `windows.h` from DXVK. Expect a large error count to fall at
   once, including much of the `HWND`/`HRESULT` class outside 3Dmotor.
4. `SplashScreen`, `WinFrame`, `WinCursor` on SDL3, with `HWND` as
   `SDL_Window*`.
5. `Input.cpp` and `Bind.cpp` on the same event loop.
6. `GShaderFX`, last.
