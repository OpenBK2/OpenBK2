#!/usr/bin/env bash

# Where the game was installed, which is whatever CMAKE_INSTALL_PREFIX said.
# Overridable so the same script runs a second tree, for instance
# BK2_PREFIX=$HOME/bk2-asan for an ENABLE_ASAN build.
BK2_PREFIX="${BK2_PREFIX:-$HOME/bk2}"

pushd "$BK2_PREFIX/bin"
#DXVK_WSI_DRIVER=SDL3 DXVK_LOG_LEVEL=debug LD_LIBRARY_PATH="$BK2_PREFIX/bin" gdb --args "$BK2_PREFIX/bin/Game"
DXVK_WSI_DRIVER=SDL3 DXVK_LOG_LEVEL=debug LD_LIBRARY_PATH="$BK2_PREFIX/bin" "$BK2_PREFIX/bin/Game"
popd
