# for now I took granny from https://github.com/uesp/uesp-esoapps
# for the record, it's Windows only and proprietary
# can switch to a fork: https://github.com/arves100/opengr2

set(GRANNY_ROOT "${CMAKE_SOURCE_DIR}/third_party/uesp-esoapps/common/granny")
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

add_library(granny::granny ALIAS granny)
