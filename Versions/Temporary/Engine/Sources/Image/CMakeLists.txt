project(Image)
set(SOURCES
    GUnpackDXT.cpp
    ImageBMP.cpp
    ImageDDS.cpp
    ImageDDSWrite.cpp
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
# squish is the DDS writer's block encoder. The writer is portable now: it
# builds the container itself and needs neither D3DX, which exists only on
# Windows, nor a Direct3D device, which it used to create purely to have
# something to call CreateTexture on.
target_link_libraries(Image PRIVATE MemoryLib Misc System fmt::fmt-header-only squish)
install(TARGETS Image DESTINATION bin)
add_version_info(Image)

add_subdirectory(test)
