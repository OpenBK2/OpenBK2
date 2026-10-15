project(Sound)
set(SOURCES
    CellsConglomerateContainer.cpp
    DBMusicSystem.cll
    DBMusicSystem.cpp
    DBSound.cll
    DBSound.cpp
    DBSoundDesc.cll
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
bk2_add_library(Sound SHARED ${SOURCES})
target_link_libraries(Sound PRIVATE libdb MemoryLib Misc System zlib fmod fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Sound PRIVATE winapi)
endif()
