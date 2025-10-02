# fmod (flessd)
set(FLESSD_ROOT ${CMAKE_SOURCE_DIR}/third_party/flessd)
add_library(fmod STATIC
  ${FLESSD_ROOT}/src/fmusic.cpp
  ${FLESSD_ROOT}/src/fsound.cpp
)
target_include_directories(fmod PUBLIC ${FLESSD_ROOT}/include)

target_include_directories(fmod PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include/SDL3
    ${CMAKE_SOURCE_DIR}/third_party/SDL_mixer/include/SDL3_mixer)
target_link_libraries(fmod PUBLIC SDL3_mixer::SDL3_mixer-shared SDL3::SDL3-shared)

install(TARGETS SDL3-shared SDL3_mixer-shared DESTINATION bin)
