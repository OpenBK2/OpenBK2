project(WxEditor)

# Console subsystem, unlike every other GUI target here, and deliberately: see
# the comment at the top of main.cpp. It prints what it linked against, which is
# the whole point of a skeleton, and `--no-gui` makes that scriptable.
add_executable(WxEditor main.cpp)

# wx::wx carries the include directories, the import library and the UNICODE
# defines. The explicit dependency is because ExternalProject builds at build
# time: without it Ninja has no reason to run wx's build before this link.
target_link_libraries(WxEditor PRIVATE wx::wx)
add_dependencies(WxEditor wxwidgets_external)

# wx's DLL has to sit beside the executable. Done through a script that globs
# rather than by name, because the name carries wx's version, the compiler and
# the architecture -- see cmake/copy_wx_runtime.cmake.
add_custom_command(TARGET WxEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DWX_LIB_DIR=${WX_LIB_DIR}
            -DWX_DESTINATION=$<TARGET_FILE_DIR:WxEditor>
            -P ${CMAKE_SOURCE_DIR}/cmake/copy_wx_runtime.cmake
    COMMENT "Copying the wxWidgets runtime beside WxEditor"
    VERBATIM)

# Not installed. This is a rehearsal, not a deliverable, and an install rule
# would put it in the editor's install tree where someone would eventually
# wonder what it was for.
