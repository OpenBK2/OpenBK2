#!/usr/bin/env bash

set -eux

# DXSDK_ROOT is gone: cmake/dxsdk.cmake is only included under if(WIN32), and
# every dxsdk:: link is guarded the same way, so the stub tree it pointed at
# has nothing left to satisfy. Verified by configuring with it unset.

# Where DXVK installed its pkg-config files. Inert until cmake/dxvk.cmake
# exists to consume them.
export PKG_CONFIG_PATH="$HOME/opt/dxvk/lib/x86_64-linux-gnu/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

pushd linux-build
#cmake .. -GNinja -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX=/home/sse4/bk2 -DCMAKE_BUILD_TYPE=Debug
#cmake --build . -- -j $(nproc) 
cmake --build . -- -j $(nproc) -k 0
cmake --build . --target install
popd
