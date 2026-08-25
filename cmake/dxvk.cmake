# DXVK Native stands in for the DirectX SDK off Windows: it implements D3D9 over
# Vulkan and, just as usefully here, installs a windows.h shim next to its D3D9
# headers, so HWND, HRESULT, IUnknown and the STDMETHOD macros come from the same
# place. See cmake/dxsdk.cmake for the Windows side.
#
# Point PKG_CONFIG_PATH at the DXVK install prefix, for example
# $HOME/opt/dxvk/lib/x86_64-linux-gnu/pkgconfig.
#
# There is no dxvk::d3dx9 and there will not be. DXVK ships the d3dx9 headers by
# way of its mingw-directx-headers submodule but implements none of them, so code
# including d3dx9.h compiles and then fails to link. 3Dmotor/GShaderFX.cpp is the
# only live user.
find_package(PkgConfig REQUIRED)
pkg_check_modules(DXVK_D3D9 REQUIRED IMPORTED_TARGET dxvk-d3d9)

add_library(dxvk::d3d9 INTERFACE IMPORTED)
target_link_libraries(dxvk::d3d9 INTERFACE PkgConfig::DXVK_D3D9)

# The library lives outside any default search path, so record where it was found
# rather than making every run depend on LD_LIBRARY_PATH. Appended rather than
# assigned, since these are directory-scoped and inherited by every subdirectory.
list(APPEND CMAKE_BUILD_RPATH ${DXVK_D3D9_LIBRARY_DIRS})
list(APPEND CMAKE_INSTALL_RPATH ${DXVK_D3D9_LIBRARY_DIRS})

# Off Windows these headers are the platform SDK: windows.h there is DXVK's own
# four-line shim over windows_base.h, which is where HWND, HRESULT, LONG and S_OK
# come from. Every module that includes 3Dmotor/Gfx.h needs them, and that reaches
# SceneB2, UISpecificB2, ED_B2_M1 and Game, so the include directory is global the
# way the Windows SDK is on Windows. Only 3Dmotor links the library.
include_directories(${DXVK_D3D9_INCLUDE_DIRS})

message(STATUS "using DXVK ${DXVK_D3D9_VERSION} from ${DXVK_D3D9_LIBRARY_DIRS}")
