# A d3d9.dll that creates devices and draws nothing, for running the editor
# where Direct3D 9 will not give it a device: the bk2probe desktop the port's
# probes drive it on, an RDP session, a GPU-less CI runner. Everything behind
# the viewport -- opening a map with objects, the state-driven palettes and
# dialogs -- then runs and can be tested. See src/Device.cpp for what it does
# and src/Trace.h for the log that says what the engine asked of it.
#
# It is never loaded by accident. It is installed to bin/d3d9stub/, not bin/,
# and Windows only finds a d3d9.dll next to the executable before System32's;
# scripts/port/rundesktop.py --d3d9stub copies it next to B2_MapEditor.exe for
# that run and takes it away again for the next run without the flag.
#
# The interface classes are generated from the SDK's d3d9.h and committed --
# scripts/port/gen-d3d9-stub.py -- and the caps from a real card --
# scripts/port/gen-d3d9-caps.py.

add_library(d3d9stub SHARED
    d3d9.def
    src/Caps.inc
    src/Device.cpp
    src/Device.h
    src/Direct3D9.cpp
    src/Formats.h
    src/Guids.cpp
    src/Resources.h
    src/Stubs.h
    src/Trace.cpp
    src/Trace.h
)
set_target_properties(d3d9stub PROPERTIES OUTPUT_NAME d3d9)
# The headers only. dxsdk::d3d9 is the real d3d9.dll's import library, which
# is the one thing this must not link.
target_include_directories(d3d9stub PRIVATE ${DXSDK_INCLUDE})
target_compile_definitions(d3d9stub PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
add_version_info(d3d9stub)

install(FILES $<TARGET_FILE:d3d9stub> DESTINATION bin/d3d9stub)
