#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

IMAGE_NAME="${IMAGE_NAME:-openbk2-dev:ubuntu24.04}"
OPENBK2_BRANCH="${OPENBK2_BRANCH:-linux2}"
DXVK_TAG="${DXVK_TAG:-v2.6.2}"

if ! command -v docker >/dev/null 2>&1; then
  echo "ERROR: docker was not found in PATH." >&2
  exit 1
fi

if ! docker info >/dev/null 2>&1; then
  echo "ERROR: Docker is installed, but the Docker daemon is not accessible." >&2
  echo "Start Docker and/or make sure your user can run 'docker info'." >&2
  exit 1
fi

args=(
  --build-arg "USER_UID=$(id -u)"
  --build-arg "USER_GID=$(id -g)"
  --build-arg "OPENBK2_BRANCH=$OPENBK2_BRANCH"
  --build-arg "DXVK_TAG=$DXVK_TAG"
)

if [[ -n "${SDL_TAG:-}" ]]; then
  args+=(--build-arg "SDL_TAG=$SDL_TAG")
fi

echo "Building $IMAGE_NAME"
echo "  host UID:GID : $(id -u):$(id -g)"
echo "  OpenBK2 branch: $OPENBK2_BRANCH"
echo "  DXVK           : $DXVK_TAG"
[[ -n "${SDL_TAG:-}" ]] && echo "  SDL             : $SDL_TAG" || echo "  SDL             : derive from OpenBK2 pin"
echo

docker build "${args[@]}" -t "$IMAGE_NAME" .

echo
echo "Image built successfully: $IMAGE_NAME"
echo "Next: ./start-dev.sh"
