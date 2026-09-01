# syntax=docker/dockerfile:1
FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG OPENBK2_BRANCH=linux2
ARG DXVK_TAG=v2.6.2
# Leave empty to derive the exact SDL revision from OpenBK2/cmake/sdl.cmake.
ARG SDL_TAG=
ARG USERNAME=developer
ARG USER_UID=1000
ARG USER_GID=1000

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates git cmake ninja-build pkg-config ccache build-essential gdb python3 \
    meson glslang-tools libvulkan-dev vulkan-tools \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev \
    libwayland-dev wayland-protocols libxkbcommon-dev \
    libasound2-dev libpulse-dev libudev-dev libdrm-dev libgbm-dev libgl-dev libegl-dev \
    libcurl4-openssl-dev libssl-dev \
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev \
    sudo bash-completion procps less nano \
 && rm -rf /var/lib/apt/lists/*

ENV OPENBK2_DEPS=/opt/openbk2-deps \
    SDL_PREFIX=/opt/openbk2-deps/sdl3 \
    DXVK_PREFIX=/opt/openbk2-deps/dxvk

# Ubuntu 24.04 has no libsdl3-dev. Build the exact SDL revision pinned by
# OpenBK2, then build DXVK Native against that SDL3 installation.
# Force lib/ rather than a multiarch libdir so pkg-config paths are stable.
RUN set -eux; \
    git clone --depth 1 --branch "${OPENBK2_BRANCH}" https://github.com/SSE4/OpenBK2.git /tmp/OpenBK2-pin; \
    resolved_sdl_tag="${SDL_TAG}"; \
    if [[ -z "${resolved_sdl_tag}" ]]; then \
      resolved_sdl_tag="$(awk '/GIT_REPOSITORY[[:space:]]+https:\/\/github.com\/libsdl-org\/SDL.git/ { found=1; next } found && /GIT_TAG[[:space:]]+/ { print $2; exit }' /tmp/OpenBK2-pin/cmake/sdl.cmake)"; \
    fi; \
    test -n "${resolved_sdl_tag}"; \
    echo "Building SDL pin: ${resolved_sdl_tag}"; \
    git clone https://github.com/libsdl-org/SDL.git /tmp/SDL; \
    git -C /tmp/SDL checkout --detach "${resolved_sdl_tag}"; \
    cmake -S /tmp/SDL -B /tmp/SDL-build -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="${SDL_PREFIX}" \
      -DCMAKE_INSTALL_LIBDIR=lib \
      -DSDL_SHARED=ON \
      -DSDL_STATIC=OFF \
      -DSDL_TEST_LIBRARY=OFF \
      -DSDL_TESTS=OFF \
      -DSDL_EXAMPLES=OFF; \
    cmake --build /tmp/SDL-build --parallel "$(nproc)"; \
    cmake --install /tmp/SDL-build; \
    test -f "${SDL_PREFIX}/lib/pkgconfig/sdl3.pc"; \
    PKG_CONFIG_PATH="${SDL_PREFIX}/lib/pkgconfig" pkg-config --modversion sdl3; \
    rm -rf /tmp/OpenBK2-pin /tmp/SDL /tmp/SDL-build

RUN set -eux; \
    git clone --recursive --shallow-submodules --depth 1 --branch "${DXVK_TAG}" \
      https://github.com/doitsujin/dxvk.git /tmp/dxvk; \
    PKG_CONFIG_PATH="${SDL_PREFIX}/lib/pkgconfig" \
      meson setup \
        --buildtype release \
        --prefix "${DXVK_PREFIX}" \
        --libdir lib \
        -Dnative_sdl3=enabled \
        -Dnative_sdl2=disabled \
        -Dnative_glfw=disabled \
        /tmp/dxvk-build /tmp/dxvk; \
    PKG_CONFIG_PATH="${SDL_PREFIX}/lib/pkgconfig" \
      ninja -C /tmp/dxvk-build install; \
    test -f "${DXVK_PREFIX}/lib/pkgconfig/dxvk-d3d9.pc"; \
    PKG_CONFIG_PATH="${DXVK_PREFIX}/lib/pkgconfig:${SDL_PREFIX}/lib/pkgconfig" \
      pkg-config --cflags --libs dxvk-d3d9; \
    rm -rf /tmp/dxvk /tmp/dxvk-build

# Container-wide environment. VS Code extensions started inside the container
# inherit this, so dxvk-d3d9 does not rely on terminal-only exports.
ENV PKG_CONFIG_PATH=/opt/openbk2-deps/dxvk/lib/pkgconfig:/opt/openbk2-deps/sdl3/lib/pkgconfig \
    CMAKE_PREFIX_PATH=/opt/openbk2-deps/dxvk:/opt/openbk2-deps/sdl3 \
    OPENBK2_ROOT=/workspace \
    OPENBK2_SRC=/workspace/OpenBK2 \
    OPENBK2_BUILD=/workspace/build \
    BK2_PREFIX=/workspace/install \
    CCACHE_DIR=/workspace/.ccache \
    CCACHE_MAXSIZE=10G

# Ubuntu 24.04's base image may already contain an account/group using UID/GID
# 1000 (commonly `ubuntu`).  Reuse/rename those IDs instead of assuming they
# are free.  This also works when build-image.sh passes a different host UID/GID.
RUN set -eux; \
    existing_group="$(getent group "${USER_GID}" | cut -d: -f1 || true)"; \
    if [[ -n "${existing_group}" && "${existing_group}" != "${USERNAME}" ]]; then \
      if getent group "${USERNAME}" >/dev/null; then \
        echo "ERROR: group '${USERNAME}' already exists with a different GID" >&2; \
        exit 1; \
      fi; \
      groupmod --new-name "${USERNAME}" "${existing_group}"; \
    elif [[ -z "${existing_group}" ]]; then \
      groupadd --gid "${USER_GID}" "${USERNAME}"; \
    fi; \
    existing_user="$(getent passwd "${USER_UID}" | cut -d: -f1 || true)"; \
    if [[ -n "${existing_user}" && "${existing_user}" != "${USERNAME}" ]]; then \
      if id "${USERNAME}" >/dev/null 2>&1; then \
        echo "ERROR: user '${USERNAME}' already exists with a different UID" >&2; \
        exit 1; \
      fi; \
      usermod --login "${USERNAME}" "${existing_user}"; \
      usermod --home "/home/${USERNAME}" --move-home "${USERNAME}"; \
    elif [[ -z "${existing_user}" ]]; then \
      useradd --uid "${USER_UID}" --gid "${USER_GID}" -m -s /bin/bash "${USERNAME}"; \
    fi; \
    usermod --gid "${USER_GID}" --shell /bin/bash "${USERNAME}"; \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > "/etc/sudoers.d/${USERNAME}"; \
    chmod 0440 "/etc/sudoers.d/${USERNAME}"; \
    mkdir -p /workspace; \
    chown "${USER_UID}:${USER_GID}" /workspace

WORKDIR /workspace
USER ${USERNAME}

# Development container: keep it alive and attach VS Code to it.
CMD ["sleep", "infinity"]
