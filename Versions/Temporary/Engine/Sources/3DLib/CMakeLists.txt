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
include(GenerateExportHeader)
add_library(3DLib SHARED ${SOURCES})
generate_export_header(3DLib EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/3DLib_export.h)
target_link_libraries(3DLib PRIVATE MemoryLib Misc System winapi)
install(TARGETS 3DLib DESTINATION bin)
add_version_info(3DLib)
