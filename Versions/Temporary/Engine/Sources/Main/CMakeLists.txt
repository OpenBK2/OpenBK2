project(Main)
set(SOURCES
    AICmdsAutoMagicInterface.cpp
    CommonAI.cpp
    DBNetConsts.cpp
    GameTimerInternal.cpp
    Initialization.cpp
    MainLoopCommands.cpp
    MainLoopInternal.cpp
    MODs.cpp
    MultiplayerInternal.cpp
    Profiles.cpp
    stdafx.cpp
)
add_library(Main SHARED ${SOURCES})
add_stdafx_pch(Main)
add_export_header(Main)
target_link_libraries(Main PRIVATE
    Image
    Input
    libdb
    MemoryLib
    Misc
    Net
    System
    UI
    zlib
)
if(WIN32)
    target_link_libraries(Main PRIVATE winapi)
endif()
install(TARGETS Main DESTINATION bin)
add_version_info(Main)
