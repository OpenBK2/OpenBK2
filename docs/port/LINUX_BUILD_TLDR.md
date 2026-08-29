# Building on Linux - TLDR

Every command needed to go from a bare machine to a running `Game`, in order.
Part of the Linux port notes; see [LINUX_PORT.md](../../LINUX_PORT.md) for the
index. The reasoning behind each step lives in [DXVK.md](DXVK.md) and
[PORT_SETUP.md](PORT_SETUP.md); this file is only the commands.

Two things have to be built: **DXVK Native** (D3D9 over Vulkan, plus the
`windows.h` shim the whole tree compiles against) and then **the game**, which
finds DXVK through `pkg-config`. DXVK first, always: without it configure fails
in `cmake/dxvk.cmake`.

Written against Ubuntu 26.04, gcc 15.2, cmake 4.2, ninja 1.13.

---

## 0. Distro packages

```bash
sudo apt update && sudo apt install -y \
  git cmake ninja-build pkg-config ccache build-essential gdb \
  meson glslang-tools libvulkan-dev libsdl3-dev vulkan-tools \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev \
  libasound2-dev libpulse-dev libudev-dev libdrm-dev libgbm-dev libgl-dev libegl-dev \
  libcurl4-openssl-dev libssl-dev \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
```

- `meson` + `glslang-tools` + `libvulkan-dev` + `libsdl3-dev` are for DXVK.
- The `libav*` group is FFmpeg, taken from the system through pkg-config off
  Windows (`cmake/ffmpeg.cmake`), so it is required, not optional.
- Everything else that is not a distro package - SDL3, SDL_mixer, boost, zlib-ng,
  fmt, spdlog, glm, googletest, googlebenchmark, mimalloc, crashpad - comes in
  through `FetchContent` at configure time. **The first configure needs network.**

Check the machine has a working Vulkan ICD before blaming anything later:

```bash
vulkaninfo --summary | head -40
```

## 1. DXVK Native

```bash
git clone --recursive https://github.com/doitsujin/dxvk.git ~/src/dxvk
cd ~/src/dxvk
meson setup --buildtype release --prefix ~/opt/dxvk build.native
ninja -C build.native install
```

No `--cross-file`: passing one produces Wine DLLs, omitting one is what selects
the native `.so` build. The clone must be `--recursive` or there are no D3D9 or
Vulkan headers at all.

Verify before moving on - the library lands under the multiarch libdir, so use
`find` rather than a `lib*/` glob:

```bash
find ~/opt/dxvk -name "libdxvk_d3d9.so"
PKG_CONFIG_PATH=~/opt/dxvk/lib/x86_64-linux-gnu/pkgconfig pkg-config --cflags --libs dxvk-d3d9
```

Both must succeed. `pkg-config` should print
`-I<prefix>/include/dxvk -L<prefix>/lib/x86_64-linux-gnu -ldxvk_d3d9`.

## 2. The repository

```bash
git clone https://github.com/SSE4/OpenBK2.git ~/src/OpenBK2
cd ~/src/OpenBK2
git checkout linux2
git submodule update --init --recursive
```

Submodule shas **differ per branch**, so re-run the submodule line after every
`git checkout` between branches.

The `commit-msg` hook that strips `Co-Authored-By` trailers is not cloned.
Reinstall it by hand after a fresh clone if you intend to commit; see
[LINUX_PORT.md](../../LINUX_PORT.md).

## 3. Configure, build, install

```bash
export PKG_CONFIG_PATH="$HOME/opt/dxvk/lib/x86_64-linux-gnu/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export BK2_PREFIX="$HOME/bk2"

cmake -S . -B linux-build -GNinja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="$BK2_PREFIX" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build linux-build -- -j "$(nproc)" -k 0
cmake --build linux-build --target install
```

`-k 0` tells Ninja to keep going past failures, which is what `build.sh` does
too. **A run that ends "successfully" can still contain errors** - scroll back,
or check that `Game` was actually relinked.

`./build.sh` does exactly the three commands above, but with the prefix
hardcoded to `/home/sse4/bk2` and expecting `linux-build/` to already exist
(`mkdir -p linux-build` first). Use it once the paths match yours.

Options worth knowing: `BUILD_EDITOR` (OFF), `USE_MIMALLOC` (ON),
`ENABLE_ASAN` (OFF, and it forces mimalloc off), `USE_CCACHE` (ON).

## 4. Game data

CMake installs binaries only. The data is **not** installed and must sit beside
the install prefix, not inside `bin/`: the game runs from `<prefix>/bin` and
resolves data as `../Data`, `../Profiles`.

```bash
ln -s "$PWD/Versions/Current/Data" "$BK2_PREFIX/Data"
cp -r Versions/Current/Profiles "$BK2_PREFIX/Profiles"
cp Versions/Current/splash.bmp "$BK2_PREFIX/splash.bmp"
```

`Data` is symlinked because it is large and read-only; `Profiles` is copied
because the game writes into it. Copying `Data` instead works fine if you would
rather not have the link.

Result:

```
$BK2_PREFIX/
  bin/        Game, crashpad_handler, ~40 .so modules, libdxvk_d3d9.so.0
  Data/       -> Versions/Current/Data
  Profiles/
  splash.bmp
```

## 5. Run

```bash
./launch.sh                 # honours BK2_PREFIX, defaults to $HOME/bk2
```

or by hand, which is all `launch.sh` does:

```bash
cd "$BK2_PREFIX/bin"
DXVK_WSI_DRIVER=SDL3 LD_LIBRARY_PATH="$BK2_PREFIX/bin" ./Game
```

**`DXVK_WSI_DRIVER` has no default and must be set**, to one of `SDL3`, `SDL2`,
`GLFW`. This tree wants `SDL3`. The `cd` is not optional - every path the game
resolves is relative to the working directory.

## 6. Tests, optionally

Tests are `EXCLUDE_FROM_ALL`, so a normal build never compiles them and `ctest`
cannot build them itself. Build first, then run by label:

```bash
cmake --build linux-build --target run-unittests
ctest --test-dir linux-build -L obk2-test --output-on-failure
```

The label must be `obk2-test`, never bare `obk2`: `-L` is a regex and the
fetched dependencies register roughly 150 tests of their own.

---

## When it goes wrong

| symptom | cause |
|---|---|
| configure dies in `cmake/dxvk.cmake`, `dxvk-d3d9` not found | `PKG_CONFIG_PATH` not exported, or DXVK not installed. Step 1. |
| `DXVK_WSI_DRIVER environment variable unset` | step 5, it has no default |
| splash or config "missing, empty, or unreadable" | data is in the wrong place, or `Game` was not launched from `<prefix>/bin`. Step 4. |
| link errors naming `D3DXCreateEffect` and friends | d3dx9 is headers-only under DXVK and has no implementation. `3Dmotor/GShaderFX.cpp` is Windows-only for this reason. |
| `dlsym` failures out of DXVK at startup | the SDL3 this build pins is older than the one DXVK was compiled against. Keep the pin in `cmake/sdl.cmake` ahead of the distro's. |
| configure hangs or fails on first run | `FetchContent` needs network |

Diagnostics that work on both platforms and are the right first move for any
Linux-only divergence:

```bash
OPENBK2_FILE_TRACE=1 ./launch.sh     # every file opened, in order
OPENBK2_CMD_TRACE=1 ./launch.sh      # every interface command that runs
scripts/port/diff-platform-trace.py <linux.log> <windows.log>
```

Crashes land in `$BK2_PREFIX/bin/crashpad_db/reports`. DXVK is loud with
`DXVK_LOG_LEVEL=debug`.
