project(B2_MapEditor)
set(SOURCES
    main.cpp
    ModuleHook.cpp
    stdafx.cpp
    VFSDbObserver.cpp
)
add_executable(B2_MapEditor WIN32 ${SOURCES})
target_link_libraries(B2_MapEditor PRIVATE 3DLib 3Dmotor AILogic B2_M1_Terrain B2_M1_World Client Common_RTS_AI DebugTools ED_B2 ED_B2_M1 ED_Common ED_RTS GameX Image Input libdb Main MapEditorLib MapEditor MemoryLib Misc Net Parser SceneB2 Scintilla Script Server_Client_Common Sound Stats_B2_M1 System UISpecificB2 UI zlib)
install(TARGETS B2_MapEditor DESTINATION bin)