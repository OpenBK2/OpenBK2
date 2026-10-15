project(Main)
set(SOURCES
    AICmdsAutoMagicInterface.cpp
    CommonAI.cpp
    DBNetConsts.cll
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
bk2_add_library(Main SHARED ${SOURCES})
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
    Boost::uuid
)
if(WIN32)
    target_link_libraries(Main PRIVATE winapi)
endif()
