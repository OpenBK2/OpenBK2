set(DXSDK_ROOT "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)")

set(DXSDK_INCLUDE ${DXSDK_ROOT}/Include)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(DXSDK_LIB ${DXSDK_ROOT}/Lib/x64)
else()
  set(DXSDK_LIB ${DXSDK_ROOT}/Lib/x86)
endif()

function(add_dxsdk_library target_name lib_name)
  set(LIB_VAR "${target_name}_LIB_PATH")
  find_library(${LIB_VAR} NAMES ${lib_name} PATHS ${DXSDK_LIB} NO_DEFAULT_PATH)
  add_library(${target_name} STATIC IMPORTED)
  set_target_properties(${target_name} PROPERTIES
    IMPORTED_LOCATION "${${LIB_VAR}}"
    INTERFACE_INCLUDE_DIRECTORIES "${DXSDK_INCLUDE}"
  )
  message(STATUS "${target_name} = ${${LIB_VAR}}")
endfunction()

add_dxsdk_library(dxsdk::d3d9 d3d9)
add_dxsdk_library(dxsdk::d3dx9 d3dx9)
add_dxsdk_library(dxsdk::dxerr dxerr)
add_dxsdk_library(dxsdk::dxguid dxguid)
