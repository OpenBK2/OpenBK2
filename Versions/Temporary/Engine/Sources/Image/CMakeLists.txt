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
include(GenerateExportHeader)
add_library(Image SHARED ${SOURCES})
generate_export_header(Image EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Image_export.h)
target_link_libraries(Image PRIVATE MemoryLib Misc System dxsdk::d3dx9)
install(TARGETS Image DESTINATION bin)
