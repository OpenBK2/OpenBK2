# for now I took granny from https://github.com/uesp/uesp-esoapps
# for the record, it's Windows only and proprietary
# can switch to a fork: https://github.com/arves100/opengr2

set(GRANNY_ROOT "${CMAKE_SOURCE_DIR}/third_party/uesp-esoapps/common/granny")

if(WIN32)
    add_library(granny SHARED IMPORTED)
    target_include_directories(granny INTERFACE ${GRANNY_ROOT})
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(IMP ${GRANNY_ROOT}/win64/granny2_x64.lib)
        set(DLL ${GRANNY_ROOT}/win64/granny2_x64.dll)
    else()
        set(IMP ${GRANNY_ROOT}/win32/granny2.lib)
        set(DLL ${GRANNY_ROOT}/win32/granny2.dll)
    endif()

    set_target_properties(granny PROPERTIES IMPORTED_IMPLIB ${IMP} IMPORTED_LOCATION ${DLL})
    install(FILES "${DLL}" DESTINATION bin)
else()
    # There is no Granny to link off Windows: it ships as a .lib and a .dll and
    # nothing else, and the only fork is a partial reimplementation. Until that
    # is a project of its own, the 54 entry points this tree references are
    # stubbed, and each one records that it was called, in what order and with
    # what arguments.
    #
    # The point is triage. Linking narrowed several hundred declarations to 54,
    # but a reference is not a call: some of those sit on paths the shipped data
    # cannot reach, and the linker cannot tell. Running the game against these
    # stubs says which are real and in what order they are first needed, which
    # is the order to port them in.
    #
    # Nothing here does any work, so the game will not animate and will very
    # likely stop during loading. The log up to that point is the deliverable.
    add_library(granny STATIC
        ${CMAKE_SOURCE_DIR}/Versions/Temporary/Engine/Sources/vendor/granny/GrannyStub.cpp)
    target_include_directories(granny PUBLIC
        ${GRANNY_ROOT}
        ${CMAKE_SOURCE_DIR}/Versions/Temporary/Engine/Sources)
    target_link_libraries(granny PRIVATE fmt::fmt-header-only)
    # a static library linked into a shared one
    set_target_properties(granny PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(granny PROPERTIES FOLDER "third_party/granny")
endif()

add_library(granny::granny ALIAS granny)
