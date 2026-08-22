# common definitions for Windows API
add_library(winapi INTERFACE)

# 0x0501 = Windows XP (NT 5.1)
# WINDOWINFO requires at least Windows 2000
# RtlCaptureContext requires Windows XP
set(WINAPI_VERSION "0x501")

# https://learn.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers

# Exclude rarely-used stuff from Windows headers
target_compile_definitions(winapi INTERFACE "WIN32_LEAN_AND_MEAN=1")


target_compile_definitions(winapi INTERFACE "_WIN32_WINNT=${WINAPI_VERSION}")

target_compile_definitions(winapi INTERFACE "WINVER=${WINAPI_VERSION}")

# precise Windows version including Service Pack
target_compile_definitions(winapi INTERFACE "NTDDI_VERSION=${WINAPI_VERSION}0000")

# DirectInput
# https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416756(v=vs.85)
target_compile_definitions(winapi INTERFACE "DIRECTINPUT_VERSION=0x0800")

# doesn't play well with Boost headers
target_compile_definitions(winapi INTERFACE "NOMINMAX")
