include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# These sources are UTF-8, as .editorconfig declares. Say so to the compiler,
# which otherwise guesses.
#
# MSVC reads a file without a byte order mark in whatever code page the build
# machine runs, so what reaches the binary depends on the machine rather than on
# the source. Narrow literals survive that either way, since the source and
# execution charsets both default to that same code page and the bytes make a
# round trip unchanged; the process code page is UTF-8 at run time regardless
# (Game/utf8.manifest). The nine wide literals are what moves: Russian test
# values in TestDB/TestType.h and libdb/test, whose UTF-8 bytes MSVC has been
# reading as single-byte characters. GCC has always read them as UTF-8, so this
# is also what makes the two compilers agree on them.
#
# Here rather than at the top level, because the top level adds its options
# before the dependencies are brought in, and a dependency source file that is
# not valid UTF-8 would then stop building. This covers the engine's own code
# and nothing else.
#
# Scoped to C and C++ for the same reason the /wd flags in the top-level file
# are: ml and ml64 reject an unknown switch with A4018.
if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>")
else()
    # Already the default on GCC and Clang, on every locale. Stating it keeps a
    # changed default from quietly changing the binaries.
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-finput-charset=UTF-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-fexec-charset=UTF-8>")
endif()

add_subdirectory("Misc")
add_subdirectory("System")
add_subdirectory("Game")
add_subdirectory("GameX")
add_subdirectory("Main")
add_subdirectory("Input")
add_subdirectory("Image")
add_subdirectory("AILogic")
add_subdirectory("Script")
add_subdirectory("3Dmotor")
add_subdirectory("MemoryLib")
add_subdirectory("3DLib")
add_subdirectory("SceneB2")
add_subdirectory("UI")
add_subdirectory("Sound")
add_subdirectory("UISpecificB2")
add_subdirectory("Net")
add_subdirectory("Stats_B2_M1")
add_subdirectory("Common_RTS_AI")
add_subdirectory("Client")
add_subdirectory("Server_Client_Common")
add_subdirectory("B2_M1_World")
add_subdirectory("DebugTools")
add_subdirectory("B2_M1_Terrain")
add_subdirectory("libdb")
add_subdirectory("Parser")
# The database types and records libdb/test opens the editor database with.
# Nothing in ALL depends on it.
add_subdirectory("TestDB")

# The database tools, console programs rather than engine modules. dbindex
# regenerates Data/index.bin after .xdb records change, which the shipped
# Data/dbindex.bat has always expected to find in bin/; dbstruct migrates the
# .xdb files themselves when types.xml changes shape.
add_subdirectory("dbindex")
add_subdirectory("dbstruct")

# The code generator that turns the .cll type descriptions into types.xml and
# the DB*.h/.cpp sources. Run by hand after a .cll change; the build does not
# run it.
add_subdirectory("dbcodegen")

# FreeType glyph rasterisation behind a small interface of its own, used by
# FontGen to bake atlases and meant for the game to render text with later.
add_subdirectory("FontRaster")

# The font baker, which rasterises with FontRaster.
add_subdirectory("FontGen")

# The shader assembler ShaderCompiler uses in place of D3DX.
add_subdirectory("ShaderAsm")

# The shader compiler, which turns 3Dmotor/GfxShaders.txt into the committed
# GfxShaders.cpp with ShaderAsm, and on Windows checks the result against D3DX.
add_subdirectory("ShaderCompiler")

# The dedicated lobby server and the test client that drives it. Off by default
# while they are mid-port.
#
# TestClient is under the same option rather than one of its own: it exists only
# to exercise the server, the two were deployed and supervised together, and it
# is how the server gets tested without a full game build.
if(BUILD_SERVER)
    add_subdirectory("Server")
    add_subdirectory("TestClient")
endif()

if(BUILD_EDITOR)
    # A d3d9.dll that draws nothing, so the editor can be driven where D3D9
    # gives it no device. Installed beside bin/, never into it; see its
    # CMakeLists for how a run opts in. Windows only: it is a replacement for
    # the system DLL, built from the DirectX SDK's headers and a .def file, and
    # off Windows d3d9 is DXVK Native, linked rather than found at load time.
    if(WIN32)
        add_subdirectory("vendor/d3d9stub")
    endif()

    add_subdirectory("MapEditorLib")
    add_subdirectory("ED_Common")
    add_subdirectory("ED_B2_M1")
    add_subdirectory("ED_B2")
    add_subdirectory("MapEditor")
    add_subdirectory("B2_MapEditor")
endif()

# Every module is added by now, so the .cll files they list can be gathered
# into the list dbcodegen reads with --file-list, written beside dbcodegen.
# See cmake/dbcodegen.cmake, which also defines regenerate-db, the target that
# runs dbcodegen over that list.
write_type_descriptions_list("$<TARGET_FILE_DIR:dbcodegen>/type-descriptions.txt")
add_regenerate_db_target("$<TARGET_FILE_DIR:dbcodegen>/type-descriptions.txt")

# Explicitly built crash probes verify real dumps without launching the game.
add_subdirectory("port/test")

# Both aggregates exist by now, so the "run everything of ours" targets can be
# defined. See cmake/benchmarks.cmake.
if(TARGET unittests)
    add_suite_runner(run-unittests obk2-test unittests)
endif()
if(TARGET benchmarks)
    add_suite_runner(run-benchmarks obk2-benchmark benchmarks)
endif()
