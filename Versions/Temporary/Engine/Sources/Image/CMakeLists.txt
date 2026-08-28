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
target_link_libraries(Image PRIVATE MemoryLib Misc System dxsdk::d3dx9 winapi)
install(TARGETS Image DESTINATION bin)
add_version_info(Image)
