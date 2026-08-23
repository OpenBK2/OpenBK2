# MMX reference implementations, lifted out of the __asm blocks the tests used to
# carry inline. One shared .asm body per function, reached through a thin
# architecture wrapper, so the same source assembles for x86 and x64 and the
# bit-exactness tests stop being x86-only.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ASM_SUFFIX "_x64")
else()
    set(ASM_SUFFIX "_x86")
endif()

# An OBJECT library, not STATIC: with ASM_MASM as the only language in the target
# CMake picks an ar-style archive rule and hands lib.exe an "cr" argument, which
# fails as LNK1181. Object libraries have no archive step at all.
add_library(OriginalMMX OBJECT EXCLUDE_FROM_ALL
    CalcDirectionalLighting${ASM_SUFFIX}.asm)
set_target_properties(OriginalMMX PROPERTIES FOLDER "unittest")

# The wrappers `include` the shared body by bare filename, so ml/ml64 has to be able
# to find it next to them.
target_include_directories(OriginalMMX PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The sentinel runs the extracted .asm against the __asm it replaced and compares
# byte for byte. x86 only, because that is the only architecture where MSVC still
# accepts the inline form, so it is the only place the two can be compared at all.
# It links no googletest and stays a single small executable, so it is cheap enough to
# rerun on every change to a .asm file.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_executable(OriginalMMXSentinel EXCLUDE_FROM_ALL
        sentinel/Sentinel.cpp
        sentinel/InlineRef.cpp)
    # Misc only, for the CVec2/CVec4 constructors that Geom.h's axis constants call
    # from their dynamic initializers. Comparing the primitives rather than the C++
    # loops keeps SPerVertexLightState, and therefore 3Dmotor, out of the link.
    target_link_libraries(OriginalMMXSentinel PRIVATE OriginalMMX Misc)
    set_target_properties(OriginalMMXSentinel PROPERTIES FOLDER "unittest")
    add_custom_command(TARGET OriginalMMXSentinel POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:OriginalMMXSentinel> $<TARGET_FILE_DIR:OriginalMMXSentinel>
        COMMAND_EXPAND_LISTS
        VERBATIM)
endif()
