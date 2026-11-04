# The simulation's transcendental functions (sin, cos, atan2, exp, pow and the
# rest) from a verbatim copy of musl's libm, rather than from the platform's.
# MSVC's CRT and glibc round differently on a fraction of inputs, so a lockstep
# game or a replay computed on one diverges on the other; the same source
# built by each platform's compiler does not. See
# Versions/Temporary/Engine/Sources/vendor/musl-libm/README.md and
# docs/Determinism.md.
#
# After cmake/unittest.cmake, which its test needs.
add_subdirectory(
    "${CMAKE_SOURCE_DIR}/Versions/Temporary/Engine/Sources/vendor/musl-libm"
    "${CMAKE_BINARY_DIR}/vendor/musl-libm")
