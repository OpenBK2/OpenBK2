#!/usr/bin/env bash
#
# Compile a sample of translation units with clang++ instead of the configured
# compiler, reusing the flags ninja already has for them, to see whether a clang
# build would hit anything.
#
# This is a probe rather than a build. It never links, and it writes its objects
# to a temporary directory, so it cannot disturb the real build. Configuring a
# second build tree with CMAKE_CXX_COMPILER=clang++ is the real test; this
# answers the cheaper question of whether that is worth doing.
#
# Two flags have to be dropped rather than passed through: the GCC precompiled
# header, which clang cannot read, and the ccache wrapper, which would cache the
# result under the wrong compiler.
#
# Usage:
#   scripts/port/try-clang.sh [build-dir] [target ...]
#
#   build-dir   a configured Ninja build tree. Defaults to ./linux-build.
#   target      ninja object targets. Defaults to the sample below, which is one
#               translation unit from each of the modules most likely to break:
#               the SIMD code, the renderer against the DXVK headers, the
#               determinism-critical simulation, and the port shims.

set -uo pipefail

BUILD_DIR="${1:-linux-build}"
shift 2>/dev/null || true

DEFAULT_TARGETS=(
	"Versions/Temporary/Engine/Sources/Input/CMakeFiles/Input.dir/Input.cpp.o"
	"Versions/Temporary/Engine/Sources/Game/CMakeFiles/Game.dir/main.cpp.o"
	"Versions/Temporary/Engine/Sources/System/CMakeFiles/System.dir/WinFrame.cpp.o"
	"Versions/Temporary/Engine/Sources/3Dmotor/CMakeFiles/3Dmotor.dir/GCombiner.cpp.o"
	"Versions/Temporary/Engine/Sources/3Dmotor/CMakeFiles/3Dmotor.dir/GfxRender.cpp.o"
	"Versions/Temporary/Engine/Sources/3Dmotor/CMakeFiles/3Dmotor.dir/GShaderFXStub.cpp.o"
	"Versions/Temporary/Engine/Sources/AILogic/CMakeFiles/AILogic.dir/Formation.cpp.o"
	"Versions/Temporary/Engine/Sources/SceneB2/CMakeFiles/SceneB2.dir/Scene.cpp.o"
	"Versions/Temporary/Engine/Sources/UI/CMakeFiles/UI.dir/UIScreen.cpp.o"
	"Versions/Temporary/Engine/Sources/Misc/CMakeFiles/Misc.dir/Geom.cpp.o"
	"CMakeFiles/granny.dir/Versions/Temporary/Engine/Sources/vendor/granny/GrannyStub.cpp.o"
)

if [ "$#" -gt 0 ]; then
	TARGETS=("$@")
else
	TARGETS=("${DEFAULT_TARGETS[@]}")
fi

if [ ! -f "${BUILD_DIR}/build.ninja" ]; then
	echo "error: ${BUILD_DIR} is not a configured Ninja build tree" >&2
	exit 1
fi

if ! command -v clang++ >/dev/null; then
	echo "error: clang++ is not installed" >&2
	exit 1
fi

cd "${BUILD_DIR}"
OUT="$(mktemp -d)"
trap 'rm -rf "${OUT}"' EXIT

clang++ --version | head -1
echo

PASS=0
FAIL=0
SKIP=0

for t in "${TARGETS[@]}"; do
	name="$(basename "${t}" .o)"
	cmd="$(ninja -t commands "${t}" 2>/dev/null | tail -1)"
	if [ -z "${cmd}" ]; then
		echo "SKIP  ${name}  (no such target)"
		SKIP=$((SKIP + 1))
		continue
	fi
	cmd="${cmd//\/usr\/bin\/c++/clang++}"
	cmd="$(printf '%s' "${cmd}" |
		sed -e 's#[^ ]*cmake -E env CCACHE_SLOPPINESS=[^ ]* [^ ]*ccache ##' \
			-e 's#-Winvalid-pch ##' \
			-e 's#-include [^ ]*cmake_pch.hxx ##' \
			-e "s#-o [^ ]*#-o ${OUT}/${name}.o#" \
			-e "s#-MF [^ ]*#-MF ${OUT}/dep.d#")"
	if eval "${cmd}" >"${OUT}/${name}.log" 2>&1; then
		echo "ok    ${name}"
		PASS=$((PASS + 1))
	else
		echo "FAIL  ${name}"
		sed -n '1,6p' "${OUT}/${name}.log" | sed 's/^/        /'
		FAIL=$((FAIL + 1))
	fi
done

echo
echo "passed ${PASS}, failed ${FAIL}, skipped ${SKIP}"
[ "${FAIL}" -eq 0 ]
