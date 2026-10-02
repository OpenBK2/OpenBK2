# Serialize runtime copies into each shared executable directory. Independent
# test links remain parallel, and directories do not block one another.
if(NOT DEFINED RUNTIME_DLLS OR NOT DEFINED RUNTIME_DESTINATION)
    message(FATAL_ERROR "copy_runtime_dlls.cmake needs RUNTIME_DLLS and RUNTIME_DESTINATION")
endif()

file(LOCK "${RUNTIME_DESTINATION}/.runtime-dlls.lock" GUARD PROCESS TIMEOUT 120)
foreach(RUNTIME_DLL IN LISTS RUNTIME_DLLS)
    get_filename_component(RUNTIME_DLL_NAME "${RUNTIME_DLL}" NAME)
    file(COPY_FILE "${RUNTIME_DLL}" "${RUNTIME_DESTINATION}/${RUNTIME_DLL_NAME}" ONLY_IF_DIFFERENT)
endforeach()
