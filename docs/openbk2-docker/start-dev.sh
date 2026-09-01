#!/usr/bin/env bash
set -Eeuo pipefail

IMAGE_NAME="${IMAGE_NAME:-openbk2-dev:ubuntu24.04}"
CONTAINER_NAME="${CONTAINER_NAME:-openbk2-dev}"
OPENBK2_BRANCH="${OPENBK2_BRANCH:-linux2}"
WORKSPACE="${1:-${OPENBK2_WORKSPACE:-$HOME/openbk2-dev}}"

if ! command -v docker >/dev/null 2>&1; then
  echo "ERROR: docker was not found in PATH." >&2
  exit 1
fi

if ! docker info >/dev/null 2>&1; then
  echo "ERROR: Docker daemon is not accessible." >&2
  exit 1
fi

if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
  echo "ERROR: Docker image '$IMAGE_NAME' does not exist." >&2
  echo "Run ./build-image.sh first." >&2
  exit 1
fi

mkdir -p "$WORKSPACE" "$WORKSPACE/build" "$WORKSPACE/install" "$WORKSPACE/.ccache"
WORKSPACE="$(cd "$WORKSPACE" && pwd -P)"

container_exists=false
if docker container inspect "$CONTAINER_NAME" >/dev/null 2>&1; then
  container_exists=true
  old_workspace="$(docker container inspect -f '{{range .Mounts}}{{if eq .Destination "/workspace"}}{{.Source}}{{end}}{{end}}' "$CONTAINER_NAME")"
  old_image="$(docker container inspect -f '{{.Config.Image}}' "$CONTAINER_NAME")"

  if [[ "$old_workspace" != "$WORKSPACE" || "$old_image" != "$IMAGE_NAME" ]]; then
    echo "Recreating '$CONTAINER_NAME' because its workspace/image changed."
    docker rm -f "$CONTAINER_NAME" >/dev/null
    container_exists=false
  fi
fi

if [[ "$container_exists" == false ]]; then
  echo "Creating development container '$CONTAINER_NAME'..."
  docker run -d \
    --name "$CONTAINER_NAME" \
    --hostname openbk2-dev \
    --mount "type=bind,src=$WORKSPACE,dst=/workspace" \
    --workdir /workspace \
    "$IMAGE_NAME" >/dev/null
else
  running="$(docker container inspect -f '{{.State.Running}}' "$CONTAINER_NAME")"
  if [[ "$running" != "true" ]]; then
    echo "Starting existing development container '$CONTAINER_NAME'..."
    docker start "$CONTAINER_NAME" >/dev/null
  else
    echo "Development container '$CONTAINER_NAME' is already running."
  fi
fi

# First-run source setup happens inside the container, so the host only needs Docker.
if ! docker exec "$CONTAINER_NAME" test -d /workspace/OpenBK2/.git; then
  if docker exec "$CONTAINER_NAME" test -e /workspace/OpenBK2; then
    # An empty directory is safe; anything else is ambiguous and should not be destroyed.
    if [[ -n "$(find "$WORKSPACE/OpenBK2" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null || true)" ]]; then
      echo "ERROR: $WORKSPACE/OpenBK2 exists but is not a Git checkout." >&2
      echo "Move/remove it or set OPENBK2_WORKSPACE to a different parent directory." >&2
      exit 1
    fi
  fi

  echo "Cloning OpenBK2 branch '$OPENBK2_BRANCH' into $WORKSPACE/OpenBK2 ..."
  docker exec "$CONTAINER_NAME" bash -lc \
    'git clone --branch "$1" --recurse-submodules https://github.com/SSE4/OpenBK2.git /workspace/OpenBK2' \
    _ "$OPENBK2_BRANCH"
fi

# Generate local VS Code CMake settings only when the checkout does not already
# have its own settings.json. Keep generated files out of git status.
if ! docker exec "$CONTAINER_NAME" test -e /workspace/OpenBK2/.vscode/settings.json; then
  docker exec "$CONTAINER_NAME" bash -lc 'mkdir -p /workspace/OpenBK2/.vscode && cat > /workspace/OpenBK2/.vscode/settings.json <<"JSON"
{
  "cmake.sourceDirectory": "/workspace/OpenBK2",
  "cmake.buildDirectory": "/workspace/build",
  "cmake.generator": "Ninja",
  "cmake.configureOnOpen": false,
  "cmake.configureSettings": {
    "CMAKE_BUILD_TYPE": "RelWithDebInfo",
    "CMAKE_INSTALL_PREFIX": "/workspace/install",
    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
  }
}
JSON
if [[ -d /workspace/OpenBK2/.git ]]; then
  grep -qxF ".vscode/settings.json" /workspace/OpenBK2/.git/info/exclude 2>/dev/null || echo ".vscode/settings.json" >> /workspace/OpenBK2/.git/info/exclude
fi'
  echo "Created container-local CMake Tools settings in OpenBK2/.vscode/settings.json"
else
  echo "Existing OpenBK2/.vscode/settings.json left unchanged."
fi

if ! docker exec "$CONTAINER_NAME" test -e /workspace/OpenBK2/.vscode/extensions.json; then
  docker exec "$CONTAINER_NAME" bash -lc 'cat > /workspace/OpenBK2/.vscode/extensions.json <<"JSON"
{
  "recommendations": [
    "ms-vscode.cmake-tools",
    "ms-vscode.cpptools"
  ]
}
JSON
if [[ -d /workspace/OpenBK2/.git ]]; then
  grep -qxF ".vscode/extensions.json" /workspace/OpenBK2/.git/info/exclude 2>/dev/null || echo ".vscode/extensions.json" >> /workspace/OpenBK2/.git/info/exclude
fi'
fi

# Sanity-check the exact dependency that previously caused configure failures.
docker exec "$CONTAINER_NAME" pkg-config --exists dxvk-d3d9
DXVK_VERSION="$(docker exec "$CONTAINER_NAME" pkg-config --modversion dxvk-d3d9)"

echo
echo "OpenBK2 development container is ready."
echo "  Container : $CONTAINER_NAME"
echo "  Workspace : $WORKSPACE"
echo "  Source    : $WORKSPACE/OpenBK2"
echo "  Build     : $WORKSPACE/build"
echo "  Install   : $WORKSPACE/install"
echo "  DXVK      : $DXVK_VERSION (visible through pkg-config)"
echo
echo "VS Code:"
echo "  1. Open VS Code and install the 'Dev Containers' extension (first time only)."
echo "  2. Ctrl+Shift+P -> 'Dev Containers: Attach to Running Container...'"
echo "  3. Choose '$CONTAINER_NAME'."
echo "  4. File -> Open Folder -> /workspace/OpenBK2"
echo "  5. Install the recommended CMake Tools/C++ extensions in the container if prompted."
echo "  6. Run 'CMake: Configure', then build normally from CMake Tools."
echo "  7. To install, select the CMake build target 'install' and build it."
echo
echo "To stop it later: docker stop $CONTAINER_NAME"
