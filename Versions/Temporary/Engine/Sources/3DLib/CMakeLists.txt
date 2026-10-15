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
bk2_add_library(3DLib SHARED ${SOURCES})
target_link_libraries(3DLib PRIVATE MemoryLib Misc System)
if(WIN32)
    target_link_libraries(3DLib PRIVATE winapi)
endif()
