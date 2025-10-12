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
include(GenerateExportHeader)
add_library(Main SHARED ${SOURCES})
generate_export_header(Main EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Main_export.h)
target_link_libraries(Main PRIVATE Image Input libdb MemoryLib Misc Net System UI zlib winapi)
install(TARGETS Main DESTINATION bin)
add_version_info(Main)
