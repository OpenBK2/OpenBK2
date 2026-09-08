project(Image)
set(SOURCES
    GUnpackDXT.cpp
    ImageBMP.cpp
    ImageDDS.cpp
    ImageEffect.cpp
    ImageInternal.cpp
    ImageMip.cpp
    ImageProcessor.cpp
    ImagePSD.cpp
    ImageScale.cpp
    ImageTGA.cpp
    stdafx.cpp
)
add_library(Image SHARED ${SOURCES})
add_stdafx_pch(Image)
add_export_header(Image)
target_link_libraries(Image PRIVATE MemoryLib Misc System fmt::fmt-header-only)
if(WIN32)
    # the DDS writer is D3D9/D3DX only and its sole caller is the editor's TextureExporter
    target_sources(Image PRIVATE ImageDDSWrite.cpp)
    target_link_libraries(Image PRIVATE dxsdk::d3d9 dxsdk::d3dx9 winapi) # DDS export can create its own conversion device.
endif()
install(TARGETS Image DESTINATION bin)
add_version_info(Image)
