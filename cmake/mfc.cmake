# MFC, as the editor uses it: the shared MFC DLL.
#
# These definitions used to be PUBLIC properties of the Stingray shim
# (vendor/stingray), and every editor module got MFC-as-DLL by linking the shim.
# That made "uses MFC" and "uses Stingray" the same link, which is wrong in
# exactly the direction the migration goes: a module that has shed Stingray but
# still uses MFC would, on dropping the shim, quietly compile against *static*
# MFC while the rest of the process uses the DLL. So MFC is its own target now.
# The shim links it PUBLIC, which keeps every module that links the shim exactly
# as it was, and a module that no longer needs Stingray links this instead.
#
# _AFXDLL is the part that matters: it selects the shared MFC DLL, and with it
# the shared CRT that wx is also built against -- see cmake/wxwidgets.cmake.
add_library(mfc INTERFACE)

if(MSVC)
    target_compile_definitions(mfc INTERFACE __AFX__ _AFXDLL)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(mfc INTERFACE _AFX_FULL_DEBUG)
    endif()
endif()
