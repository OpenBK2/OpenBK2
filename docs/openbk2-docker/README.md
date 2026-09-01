# OpenBK2 Ubuntu 24.04 — plug-and-play Docker development environment

This setup deliberately has only **two normal host-side scripts**:

```bash
./build-image.sh     # usually once
./start-dev.sh       # every development session
```

OpenBK2 itself is **not configured, compiled, or installed by the scripts**.
After the container is running, those actions are done from VS Code/CMake Tools.

The Docker image contains the Ubuntu 24.04 compiler/dev packages plus private
SDL3 and DXVK Native installations. `PKG_CONFIG_PATH` is set at image level, so
VS Code CMake Tools sees `dxvk-d3d9` without the terminal-only export workaround.

## Prerequisites

On the host you need:

- Docker Engine / Docker Desktop
- VS Code
- VS Code's **Dev Containers** extension

The host does **not** need the OpenBK2 compiler/development packages.

## 1. Build the development image (normally once)

From this directory:

```bash
./build-image.sh
```

The image is named by default:

```text
openbk2-dev:ubuntu24.04
```

The build uses your host UID/GID, installs the Ubuntu 24.04 build dependencies,
builds the SDL3 revision pinned by OpenBK2, then builds DXVK Native v2.6.2.

Re-run `build-image.sh` only when you intentionally want to rebuild/update the
development image or after changing the Dockerfile/dependency pins.

## 2. Start the development container

Normally:

```bash
./start-dev.sh
```

The default persistent host workspace is:

```text
~/openbk2-dev/
```

On the **first** start, the script also clones the OpenBK2 `linux2` branch and
its submodules. It does not configure or compile it.

The resulting host layout is:

```text
~/openbk2-dev/
├── OpenBK2/       editable source checkout
├── build/         CMake/Ninja build tree
├── install/       CMAKE_INSTALL_PREFIX
└── .ccache/       persistent compiler cache
```

All four directories are visible inside the container at `/workspace`:

```text
/workspace/OpenBK2
/workspace/build
/workspace/install
/workspace/.ccache
```

The container is persistent and named `openbk2-dev`. Running `start-dev.sh`
again simply starts it if it is stopped; it does not recreate it unless the
image or mounted workspace changed.

### Put the workspace somewhere else

Pass the parent directory as the first argument:

```bash
./start-dev.sh /home/marshall/Development/openbk2-linux
```

or set:

```bash
OPENBK2_WORKSPACE=/home/marshall/Development/openbk2-linux ./start-dev.sh
```

## 3. Attach VS Code

After `start-dev.sh` says the container is ready:

1. Open VS Code.
2. Press **Ctrl+Shift+P**.
3. Choose **Dev Containers: Attach to Running Container...**.
4. Choose **openbk2-dev**.
5. In the remote VS Code window choose **File -> Open Folder**.
6. Open:

   ```text
   /workspace/OpenBK2
   ```

The first-start script creates local VS Code settings (only if the repository
doesn't already have them) so CMake Tools uses:

```text
Source:          /workspace/OpenBK2
Build directory: /workspace/build
Generator:       Ninja
Build type:      RelWithDebInfo
Install prefix:  /workspace/install
```

It also recommends these extensions:

```text
ms-vscode.cmake-tools
ms-vscode.cpptools
```

Install them **in the container** when VS Code prompts you.

## 4. Configure/build/install entirely in VS Code

No helper build script is required.

### Configure

Press **Ctrl+Shift+P** and run:

```text
CMake: Configure
```

DXVK should be found automatically because the container has:

```text
PKG_CONFIG_PATH=/opt/openbk2-deps/dxvk/lib/pkgconfig:/opt/openbk2-deps/sdl3/lib/pkgconfig
```

You can verify from the VS Code container terminal:

```bash
pkg-config --modversion dxvk-d3d9
pkg-config --cflags --libs dxvk-d3d9
```

### Compile

Use CMake Tools' **Build** command/button, or:

```text
CMake: Build
```

Artifacts go to `/workspace/build`, which is `~/openbk2-dev/build` on the host
when using the default workspace.

### Install

In CMake Tools select the build target:

```text
install
```

and build that target. CMake installs into:

```text
/workspace/install
```

which is the host directory:

```text
~/openbk2-dev/install
```

The Docker startup script does not run the install target for you.

## 5. Daily workflow

After the image has been built once, the routine is simply:

```bash
./start-dev.sh
```

then attach VS Code to `openbk2-dev` and work normally.

At the end of the session you may stop the container:

```bash
docker stop openbk2-dev
```

Stopping/removing the container does not delete source/build/install files,
because they live in the host workspace.

If you remove it completely:

```bash
docker rm -f openbk2-dev
```

then the next `./start-dev.sh` recreates it against the same host workspace.

## 6. Useful overrides

Different image/container names:

```bash
IMAGE_NAME=my-openbk2-dev:24.04 ./build-image.sh
IMAGE_NAME=my-openbk2-dev:24.04 CONTAINER_NAME=my-openbk2 ./start-dev.sh
```

Different OpenBK2 branch:

```bash
OPENBK2_BRANCH=linux2 ./build-image.sh
OPENBK2_BRANCH=linux2 ./start-dev.sh
```

Pin SDL explicitly when rebuilding the image:

```bash
SDL_TAG=<commit> ./build-image.sh
```

Pin another DXVK version:

```bash
DXVK_TAG=v2.6.2 ./build-image.sh
```

## 7. Existing OpenBK2 checkout

`start-dev.sh` expects the checkout to be:

```text
<workspace>/OpenBK2
```

So if you already have:

```text
/home/marshall/Development/openbk2-linux/OpenBK2/.git
```

just run:

```bash
./start-dev.sh /home/marshall/Development/openbk2-linux
```

The script recognizes the checkout and will not clone over it.

## 8. Why this avoids the previous pkg-config problem

DXVK and SDL3 are installed inside the image under `/opt/openbk2-deps`, and the
Dockerfile defines `PKG_CONFIG_PATH` with `ENV`. This is inherited by VS Code's
remote extension processes as well as terminal shells.

So CMake Tools does not depend on manually running something like:

```bash
PKG_CONFIG_PATH=... cmake ...
```

before every configure.


### UID/GID handling

The Dockerfile reuses an existing base-image UID/GID when it matches the host IDs (including Ubuntu 24.04's common UID/GID 1000 account), then exposes it as `developer`.
