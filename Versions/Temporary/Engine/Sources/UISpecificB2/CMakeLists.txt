project(UISpecificB2)
set(SOURCES
    AtomicReactions.cpp
    BackgroundFrameSequence.cpp
    Components.cpp
    DBUISpecificB2.cpp
    EffectorB2Move.cpp
    EffectorPlaySound.cpp
    stdafx.cpp
    Window3DControl.cpp
    WindowFrameSequence.cpp
    WindowMiniMap.cpp
    WindowPotentialLines.cpp
    WindowRoundProgressBar.cpp
    WindowSelection.cpp
)
include(GenerateExportHeader)
add_library(UISpecificB2 SHARED ${SOURCES})
generate_export_header(UISpecificB2 EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/UISpecificB2_export.h)
target_link_libraries(UISpecificB2 PRIVATE 3Dmotor Input libdb MemoryLib Misc Sound Stats_B2_M1 System UI zlib)
install(TARGETS UISpecificB2 DESTINATION bin)
