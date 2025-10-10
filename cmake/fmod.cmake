# fmod (flessd)
set(FLESSD_ROOT ${CMAKE_SOURCE_DIR}/third_party/flessd)
add_library(fmod STATIC
  ${FLESSD_ROOT}/src/fmusic.cpp
  ${FLESSD_ROOT}/src/fsound.cpp
)
target_include_directories(fmod PUBLIC ${FLESSD_ROOT}/include)

target_link_libraries(fmod PUBLIC SDL3_mixer::SDL3_mixer-shared SDL3::SDL3-shared)

install(TARGETS SDL3-shared SDL3_mixer-shared DESTINATION bin)

set_target_properties(fmod PROPERTIES FOLDER "third_party/fmod")
