include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory("Misc")
add_subdirectory("System")
add_subdirectory("Game")
add_subdirectory("GameX")
add_subdirectory("Main")
add_subdirectory("Input")
add_subdirectory("Image")
add_subdirectory("AILogic")
add_subdirectory("Script")
add_subdirectory("3Dmotor")
add_subdirectory("MemoryLib")
add_subdirectory("3DLib")
add_subdirectory("SceneB2")
add_subdirectory("UI")
add_subdirectory("Sound")
add_subdirectory("UISpecificB2")
add_subdirectory("Net")
add_subdirectory("Stats_B2_M1")
add_subdirectory("Common_RTS_AI")
add_subdirectory("Client")
add_subdirectory("Server_Client_Common")
add_subdirectory("B2_M1_World")
add_subdirectory("DebugTools")
add_subdirectory("B2_M1_Terrain")
add_subdirectory("libdb")
add_subdirectory("Parser")

if(BUILD_EDITOR)
    add_subdirectory("vendor/stingray")

    # A d3d9.dll that draws nothing, so the editor can be driven where D3D9
    # gives it no device. Installed beside bin/, never into it; see its
    # CMakeLists for how a run opts in.
    add_subdirectory("vendor/d3d9stub")

    # Scintilla is the map editor's script text control. Nothing outside this
    # block links it: not Game, and no source in a shipped module names it.
    add_subdirectory("Scintilla")

    add_subdirectory("MapEditorLib")
    add_subdirectory("ED_Common")
    add_subdirectory("ED_B2_M1")
    add_subdirectory("ED_B2")
    add_subdirectory("ED_RTS")
    add_subdirectory("MapEditor")
    add_subdirectory("B2_MapEditor")
endif()

# The wxWidgets skeleton, which is a build-and-link rehearsal rather than
# part of the editor. Off by default with the option that guards it.
if(BUILD_WX_EDITOR)
    add_subdirectory("WxEditor")
endif()

# Both aggregates exist by now, so the "run everything of ours" targets can be
# defined. See cmake/benchmarks.cmake.
if(TARGET unittests)
    add_suite_runner(run-unittests obk2-test unittests)
endif()
if(TARGET benchmarks)
    add_suite_runner(run-benchmarks obk2-benchmark benchmarks)
endif()
