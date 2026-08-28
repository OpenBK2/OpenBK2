include(FetchContent)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG 486b55554f11c9cccc913e11a87085b2a91f706f
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(spdlog)

FetchContent_GetProperties(spdlog SOURCE_DIR SPDLOG_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(spdlog_targets ${SPDLOG_SOURCE_DIR})
foreach(target IN LISTS spdlog_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/spdlog")
endforeach()
