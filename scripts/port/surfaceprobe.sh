#!/usr/bin/env bash
# Build and run surfaceprobe.c against the tree's own SDL3, not the system one:
# cmake/sdl.cmake explains why that pin exists and why DXVK resolves against it.
set -eu

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDL_BUILD=$HOME/src/OpenBK2/linux-build/_deps/sdl-build
SDL_SRC=$HOME/src/OpenBK2/linux-build/_deps/sdl-src
OUT=/tmp/surfaceprobe

gcc -o "$OUT" "$HERE/surfaceprobe.c" \
    $(pkg-config --cflags gtk+-3.0 x11) \
    -I"$SDL_SRC/include" -I/usr/include \
    $(pkg-config --libs gtk+-3.0 x11) \
    -L"$SDL_BUILD" -lSDL3 -lvulkan \
    -Wl,-rpath,"$SDL_BUILD"

echo "built $OUT"
# The X11 backend: an XID is the point, and WSLg defaults GTK to Wayland.
GDK_BACKEND=x11 SDL_VIDEO_DRIVER=x11 "$OUT"
