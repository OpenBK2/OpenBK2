include(FetchContent)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863 #refs/tags/1.0.1
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(glm)

FetchContent_GetProperties(glm SOURCE_DIR GLM_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(glm_targets ${GLM_SOURCE_DIR})
foreach(target IN LISTS glm_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/GLM")
endforeach()
