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
add_library(UISpecificB2 SHARED ${SOURCES})
add_export_header(UISpecificB2)
target_link_libraries(UISpecificB2 PRIVATE 3Dmotor Input libdb MemoryLib Misc Sound Stats_B2_M1 System UI zlib winapi)
install(TARGETS UISpecificB2 DESTINATION bin)
add_version_info(UISpecificB2)
