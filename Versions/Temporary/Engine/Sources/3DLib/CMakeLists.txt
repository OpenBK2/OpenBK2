project(3DLib)
set(SOURCES
    GGeometry.cpp
    GGeometryUtil.cpp
    GLMGeometry.cpp
    GMemBuilder.cpp
    GMemFormat.cpp
    MemObject.cpp
    PolyUtils.cpp
    RectPacker.cpp
    stdafx.cpp
    Transform.cpp
)
add_library(3DLib SHARED ${SOURCES})
add_export_header(3DLib)
target_link_libraries(3DLib PRIVATE MemoryLib Misc System winapi)
install(TARGETS 3DLib DESTINATION bin)
add_version_info(3DLib)
