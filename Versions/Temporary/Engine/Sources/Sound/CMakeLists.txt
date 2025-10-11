project(Sound)
set(SOURCES
    CellsConglomerateContainer.cpp
    DBMusicSystem.cpp
    DBSound.cpp
    DBSoundDesc.cpp
    Fade.cpp
    MapSounds.cpp
    MusicSystem.cpp
    Pause.cpp
    PlayList.cpp
    PlayTime.cpp
    Sound.cpp
    Sound2D.cpp
    SoundCell.cpp
    SoundEngine.cpp
    SoundManager.cpp
    SoundSample.cpp
    SoundSceneConsts.cpp
    SoundSceneInternal.cpp
    SoundSceneSerialize.cpp
    SoundSceneUtills.cpp
    stdafx.cpp
    SubstSound.cpp
    TerrainSounds.cpp
    Track.cpp
)
include(GenerateExportHeader)
add_library(Sound SHARED ${SOURCES})
generate_export_header(Sound EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Sound_export.h)
target_link_libraries(Sound PRIVATE libdb MemoryLib Misc System zlib fmod winapi)
install(TARGETS Sound DESTINATION bin)
