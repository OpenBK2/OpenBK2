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
add_library(Sound SHARED ${SOURCES})
add_stdafx_pch(Sound)
add_export_header(Sound)
target_link_libraries(Sound PRIVATE libdb MemoryLib Misc System zlib fmod winapi)
install(TARGETS Sound DESTINATION bin)
add_version_info(Sound)
