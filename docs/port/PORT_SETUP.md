# Linux port - environment and build

How to get a Linux build running, how the two clones stay in sync, and how much verification each replayed commit gets.

Part of the Linux port notes; see [LINUX_PORT.md](../../LINUX_PORT.md) for the index.

---

## Where things are

| | |
|---|---|
| Windows repo | `C:\projects\OpenBK2` |
| WSL clone | `~/src/OpenBK2` (ext4 - **do not build from `/mnt/c`**, 9p makes it ~10x slower) |
| Reach WSL from Windows | `\\wsl.localhost\Ubuntu\home\sse4\src\OpenBK2` |

Branches:

- `linux` - the old port work, 44 commits, frozen reference. Don't commit here.
- `linux2` - the replay target. Started at `1501f1606` (netcode_bugfixes tip).
- `linux2-verified` - moves forward only after a Windows build **and** a clean game run.
  `git log linux2-verified..linux2` is always exactly the unverified surface.
  This is the thing that keeps a bisect range at one or two commits.


## Linux prerequisites

Already present on this box: gcc/g++ 15.2, cmake 4.2.3, git 2.53, python 3.14 (Ubuntu 26.04).

```bash
sudo apt update && sudo apt install -y \
  ninja-build pkg-config ccache mold clang lld gdb \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev \
  libasound2-dev libpulse-dev libudev-dev libdrm-dev libgbm-dev libgl-dev libegl-dev \
  libcurl4-openssl-dev libssl-dev \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
```

The `libav*` group is only needed if the ffmpeg video player gets ported; drop it otherwise.
Everything else - SDL3, boost 1.89, zlib-ng, fmt, glm, googletest - is pulled by `FetchContent`
at configure time, so the first configure needs network and takes a while.

Submodules: the recorded sha **differs per branch**, so re-run this after switching.

```bash
git submodule update --init --recursive
```

(`third_party/flessd` is `bea416b` on netcode_bugfixes but `725fe34` on `linux`.)


## Building on Linux

> **Not yet validated** - no Linux baseline has been established on gcc 15 / cmake 4.2.
> There are no Linux presets in `CMakePresets.json` yet.

Use the local helper (untracked):

```bash
./linux-configure.sh [builddir] [buildtype]     # defaults: linux-build Debug
cmake --build linux-build --parallel
```

### The d3d9 stub - and why it touches no tracked file

> **Superseded on 2026-08-25.** DXVK provides the real thing now: `cmake/dxvk.cmake`
> is included in place of `cmake/dxsdk.cmake` off Windows, `build.sh` sets
> `PKG_CONFIG_PATH` rather than `DXSDK_ROOT`, and the stub tree at
> `~/src/bk2-linux-stubs` can be deleted. See [DXVK.md](DXVK.md). The reason this
> was kept outside git is also spent: it was to avoid colliding with the
> cherry-pick replay, and the replay is finished. What follows is kept because it
> explains what the configure error looked like.

Configure dies in `cmake/dxsdk.cmake`: `find_library` cannot find d3d9/d3dx9/dxerr/dxguid, so the
four `IMPORTED` targets get an empty `IMPORTED_LOCATION`, which is a hard error at generate time
for anything that links them.

The fix uses an extension point the build system **already has** - `dxsdk.cmake` honours
`$ENV{DXSDK_ROOT}`. `~/src/bk2-linux-stubs/dxsdk` holds a fake SDK layout:

```
dxsdk/Include/.keep            <- must exist; CMake errors on a non-existent
                                  INTERFACE_INCLUDE_DIRECTORIES on an IMPORTED target
dxsdk/Lib/{x64,x86}/libd3d9.a  <- real (empty) ar archives, not zero-byte files,
              libd3dx9.a          so IMPORTED_LOCATION resolves to something real
              libdxerr.a
              libdxguid.a
```

Rebuild it with:

```bash
S=~/src/bk2-linux-stubs; mkdir -p $S/dxsdk/Include $S/dxsdk/Lib/x64 $S/dxsdk/Lib/x86
cd $S && printf 'static int m;\nint bk2_dxsdk_stub(void){return m;}\n' > stub.c && gcc -c stub.c -o stub.o
for l in d3d9 d3dx9 dxerr dxguid; do for a in x64 x86; do ar rcs $S/dxsdk/Lib/$a/lib$l.a stub.o; done; done
touch $S/dxsdk/Include/.keep
```

**Why not just patch `cmake/dxsdk.cmake`?** Because an uncommitted edit to a tracked file collides
with the replay - `git cherry-pick` refuses to run against a dirty tree for files it touches, and
several remaining commits (#27, #31, and the top-level `CMakeLists.txt`) touch CMake files. A stub
tree plus an env var is invisible to git, so it survives all 43 remaining cherry-picks untouched.

This moves the failure from configure time to **link** time, which is the honest signal: tier 1
does not link any dxsdk target, so it is unaffected. `3Dmotor` and `Image` will fail at link, and
they are supposed to until there is a real backend.

The real fix is **DXVK-native** (plain DXVK targets Wine and ships a `d3d9.dll`; the native variant
is the one that links into a Linux binary), or writing a GL/Vulkan backend. Both are large.


## Sync Windows <-> Linux

Two clones and remotes. **Not** git worktrees - a worktree stores an absolute path back to its
parent `.git`, which is `C:\...` from Windows and `/mnt/c/...` from Linux, and git resolves only
one of them. The index also caches `stat` data that means different things on NTFS-via-9p vs ext4.

| side | remote | points at |
|---|---|---|
| WSL clone | `windows` | `/mnt/c/projects/OpenBK2` |
| WSL clone | `github` | `https://github.com/SSE4/OpenBK2.git` |
| Windows repo | `wsl` | `\\wsl.localhost\Ubuntu\home\sse4\src\OpenBK2` |
| Windows repo | `origin` | `https://github.com/SSE4/OpenBK2.git` |

Both repos have `receive.denyCurrentBranch = updateInstead`: pushing to a branch the other side
has checked out updates its working tree in place, as long as that tree is clean. (`out/` is
gitignored so builds don't make it dirty.)

Windows needed this once, for the UNC path:

```
git config --global --add safe.directory //wsl.localhost/Ubuntu/home/sse4/src/OpenBK2/.git
```

**The loop, per commit:**

```bash
# WSL
git cherry-pick <sha>              # resolve conflicts, compile-check tier 1
git push windows linux2
git push github linux2             # CI builds x64-Release AND x86-Release for free

# Windows
git checkout linux2                # or it auto-updated via updateInstead
cmake --build --preset Windows-x64-Release --parallel
# run Game.exe: load a mission, save, load, quit

# after it passes, back in WSL:
git branch -f linux2-verified linux2
```

A fix made on the Windows side goes back with `git push wsl linux2`.

### Git credentials in WSL

**You mostly do not need any.** `windows` is a plain filesystem path, so WSL -> Windows pushes
never authenticate, and Windows already has Git Credential Manager set up for GitHub. Routing
GitHub pushes through the Windows side needs zero setup.

For pushing to GitHub *directly from WSL*, WSL borrows the Windows credential store instead of
keeping its own - no key copying, no ssh-agent, no keychain:

```bash
git config --global credential.helper '/mnt/c/Program\ Files/Git/mingw64/bin/git-credential-manager.exe'
```

WSL git then shells out to the Windows GCM (2.5.1), which reads the same Windows Credential
Manager entry the Windows repo uses. Prompts, when they happen, appear as normal Windows dialogs.

Also set globally in WSL, because a fresh clone has no identity and the failure mode is a
cherry-pick that applies but cannot commit (`fatal: empty ident name`):

```bash
git config --global user.name "SSE4"
git config --global user.email "tomskside@gmail.com"
```

SSH is deliberately not used here: the remotes are HTTPS, and sharing the Windows
`~/.ssh/id_ed25519` across the boundary is the ugly path - files under `/mnt/c` present as
world-readable, which OpenSSH refuses, and forwarding the Windows agent needs
`npiperelay` + `socat` plumbing.


## Verification budget

44 commits x 4 configs = 176 Windows builds. You will not do that. Instead:

- **Local, per commit:** x64 Release build + game run.
- **CI, per push:** `.github/workflows/build-windows.yml` already builds x64-Release *and*
  x86-Release on every push and PR. That is your x86 coverage, free.
- **Debug configs:** at checkpoints only - after each of the four big sweeps (#6, #15, #25, #38)
  and at the end.

x86 matters *specifically* for #6, #35 and #38 - the pointer-width and integer-type commits are
the ones that break 32-bit and nothing else.

### Gate meanings

| gate | what it costs you |
|---|---|
| `none` | cannot change Windows behavior. Push it, let CI compile it, move on. |
| `build` | if it compiles on x64 and x86, it is correct. No run needed. |
| `run` | build + launch, load a mission, play a little. |
| `run+sim` | build + run + save/load + a replay or MP check. Determinism-critical. |


