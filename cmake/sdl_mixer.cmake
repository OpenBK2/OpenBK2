include(FetchContent)

FetchContent_Declare(
        SDL_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG cedfeef30e93db35eee6b25759117da63f8e5a4f
        #GIT_TAG 93685a9006952fdc49c58fa0f95306a9cff1ed83 (old version)
        GIT_PROGRESS TRUE
)

# Only the decoders SDL_mixer carries in its own source. Every option below
# makes it look for a system library instead (libopus, libvorbisfile,
# libFLAC, game-music-emu, libxmp, mpg123, FluidSynth, WavPack), and that
# search ran again on every reconfigure whether it found anything or not.
#
# The game needs none of them. Everything it ships is WAV, PCM and MS and IMA
# ADPCM, which SDL_mixer's own WAVE decoder reads, or Ogg Vorbis music, which
# stb_vorbis reads; both are compiled in. The engine never calls FMUSIC, so no
# tracker formats either (the .mod files in the data are 3D models).
#
# On Windows none of these libraries was ever found, so nothing changes there.
# On Linux a system libvorbisfile could be picked up for Ogg; now stb_vorbis
# is used everywhere, which is the same rule cmake/sdl.cmake gives for SDL
# itself: one source, one library, whatever the machine has installed.
#
# The other self-contained decoders (dr_mp3, dr_flac, AIFF, VOC, AU,
# Timidity) stay on: they cost no search, and keep audio in those formats
# playable in user-made content.
set(SDLMIXER_OPUS              OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_FLAC_LIBFLAC      OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME               OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD               OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3_MPG123        OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI_FLUIDSYNTH   OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK           OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SDL_mixer)

FetchContent_GetProperties(SDL_mixer SOURCE_DIR SDL_MIXER_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(sdl_mixer_targets ${SDL_MIXER_SOURCE_DIR})
foreach(target IN LISTS sdl_mixer_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/SDL_mixer")
endforeach()
