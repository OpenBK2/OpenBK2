# Copy wx's shared libraries next to an executable, run as a build step.
#
# A script rather than a copy_if_different in the custom command, because wx's
# DLL name carries the version, the compiler, the architecture and a vendor tag
# -- wxmsw333u_vc_x64_custom.dll today -- and writing that string into a
# CMakeLists is one more thing to get wrong on a version bump. Globbing works
# here and not at configure time, because by the time this runs wx has been
# built.
#
# Invoked as:
#   cmake -DWX_LIB_DIR=<dir> -DWX_DESTINATION=<dir> -P copy_wx_runtime.cmake

if(NOT DEFINED WX_LIB_DIR OR NOT DEFINED WX_DESTINATION)
    message(FATAL_ERROR "copy_wx_runtime.cmake needs WX_LIB_DIR and WX_DESTINATION")
endif()

file(GLOB WX_RUNTIME_FILES "${WX_LIB_DIR}/*.dll")

if(NOT WX_RUNTIME_FILES)
    # Not fatal on purpose: a static wx produces no DLLs and this step should
    # then do nothing rather than fail a build that is otherwise correct.
    message(STATUS "copy_wx_runtime: no DLLs in ${WX_LIB_DIR}, nothing to copy")
    return()
endif()

foreach(WX_RUNTIME_FILE IN LISTS WX_RUNTIME_FILES)
    file(COPY "${WX_RUNTIME_FILE}" DESTINATION "${WX_DESTINATION}")
endforeach()
