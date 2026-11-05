project(IconRender)

# Re-renders the unit icons (the pictures in the selection slots, the
# reinforcement lists and the encyclopedia) through the game's own renderer.
#
# The shipped icons were made one at a time in the model editor
# (ED_B2_M1/IconsCreation.cpp): the unit was drawn into a 512x512 target and the
# centre 42x42 cropped out. The camera used for that shot is still in every
# MechUnitRPGStats record, so this replays it for all of them in one run: once
# exactly as the editor did, to check the reproduction against the shipped
# picture, and once at a higher resolution. scripts/iconrender/icon_report.py turns
# the output into a comparison page.
#
# It is the game's start-up without the main loop, so it links what Game links
# and needs a real Direct3D 9 device (DXVK off Windows), like the game.
set(SOURCES
    main.cpp
    stdafx.cpp
)
bk2_add_executable(IconRender ${SOURCES})
target_link_libraries(IconRender PRIVATE 3DLib 3Dmotor AILogic B2_M1_Terrain B2_M1_World Client Common_RTS_AI DebugTools GameX Image Input libdb Main MemoryLib Misc Net Parser SceneB2 Script Server_Client_Common Sound Stats_B2_M1 System UISpecificB2 UI zlib fmt::fmt-header-only)
# Make the skeleton runtime available beside the executable in the build tree too.
target_link_libraries(IconRender PRIVATE gr2)
if(WIN32)
    target_link_libraries(IconRender PRIVATE winapi)
    # The game's UTF-8 code page manifest: narrow strings are UTF-8 in the
    # engine, and without it the files written for a unit named in UTF-8
    # (MOD-18's "Sturmmoerser" with an umlaut) land under a mangled name.
    target_sources(IconRender PRIVATE ../Game/utf8.manifest)
else()
    target_link_libraries(IconRender PRIVATE SDL3::SDL3-shared)
endif()

install_runtime_dlls(IconRender)
