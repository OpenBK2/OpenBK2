# wxWidgets, the toolkit the editor is drawn with.
#
# Included with BUILD_EDITOR: the editor's frame and views are wx's and there
# is no MFC front end to fall back to any more. The cost is real: a wx build is
# about 18 minutes of a CI job. CI keeps the finished install in its cache,
# through WX_ROOT below, so it pays that only when what wx is built with
# changes; a build that wants none of it turns BUILD_EDITOR off.
#
# ExternalProject rather than FetchContent, which is what every other dependency
# here uses, and the difference is deliberate:
#
#   FetchContent_MakeAvailable adds the dependency's CMakeLists as a
#   subdirectory of this build. That is fine for fmt and spdlog. wxWidgets is a
#   large, opinionated build that sets compiler flags, defines and cache
#   variables of its own, and it would inherit ours in turn -- the global
#   /arch: flag from cmake/arch.cmake, the stdafx precompiled headers, the
#   ccache launcher. Two build systems sharing one variable scope is how you
#   get a dependency that builds differently depending on what included it.
#
#   ExternalProject configures and builds it in its own tree with an argument
#   list stated in full, so what wx is built with is visible here rather than
#   inherited by accident.
#
# The cost is that ExternalProject runs at build time, so its targets do not
# exist at configure time and the paths below have to be named rather than
# discovered. Every one of them was read out of a completed build rather than
# guessed; the first version of this file guessed and got two of them wrong.

include(ExternalProject)

set(WX_PREFIX  ${CMAKE_BINARY_DIR}/_deps/wxwidgets)

# Where wx is installed, and where a finished install is looked for. Empty means
# inside this build directory, as before. Pointing it elsewhere lets a wx build
# outlive the build directory: CI restores it from its cache, and two local
# build directories of the same configuration can share one. An install there
# is reused only if its build id matches (see below), so a stale one is rebuilt
# rather than linked. One directory per configuration: Debug and Release have
# different ids and would keep replacing each other's install.
set(WX_ROOT "" CACHE PATH "Install prefix for wxWidgets, reused when its build id matches")

if(WX_ROOT)
    # Forward slashes: a Windows path given on the command line keeps its
    # backslashes, which read as escapes once the path is written into the
    # scripts ExternalProject generates.
    file(TO_CMAKE_PATH "${WX_ROOT}" WX_INSTALL)
else()
    set(WX_INSTALL ${WX_PREFIX}/install)
endif()

# wx keeps its own library layout under the install prefix rather than the usual
# lib/ and bin/: everything lands in lib/<toolchain tag>/, the import library,
# the DLL and the generated setup.h together. The tag is "vc" plus an
# architecture suffix plus the linkage.
#
# wx leaves the suffix off for 32-bit. Both spellings have been built and
# checked: an x64 and an x86 editor linked against the library found here, with
# the DLL copied beside them.
#
# Off Windows wx installs the Unix way instead: the libraries straight in lib/,
# the headers under include/wx-3.3, and setup.h under lib/wx/include/<config>.
# The tag there is only the toolkit, and it goes into the build id like the
# MSVC one does.
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(WX_TOOLCHAIN_TAG vc_x64_dll)
    else()
        set(WX_TOOLCHAIN_TAG vc_dll)
    endif()
    set(WX_LIB_DIR ${WX_INSTALL}/lib/${WX_TOOLCHAIN_TAG})
else()
    set(WX_TOOLCHAIN_TAG gtk3)
    set(WX_LIB_DIR ${WX_INSTALL}/lib)
endif()

# Shared, and not static, on purpose. wx keeps process-global state -- the
# application object, the RTTI and event-table registries, the module list -- and
# a static wx linked into more than one of the editor's DLLs would give each of
# them a private copy of all of it. The editor is a dozen shared libraries and
# more than one of them will end up using wx, so the shared build is the shape
# that survives; getting there later would mean discovering it through a bug.
#
# Monolithic because one library is enough here and it keeps the link line, and
# the BUILD_BYPRODUCTS list, down to a single name.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(WX_LIB_SUFFIX d)
else()
    set(WX_LIB_SUFFIX "")
endif()

if(WIN32)
    set(WX_IMPORT_LIB ${WX_LIB_DIR}/wxmsw33u${WX_LIB_SUFFIX}.lib)
    # The directory holding the generated setup.h, next to the binaries.
    set(WX_SETUP_DIR ${WX_LIB_DIR}/mswu)
else()
    # The shared object itself: ELF has no import library, the linker takes
    # the .so. wx's Unix names carry no debug suffix.
    set(WX_IMPORT_LIB ${WX_LIB_DIR}/libwx_gtk3u-3.3.so)
    set(WX_SETUP_DIR ${WX_LIB_DIR}/wx/include/gtk3-unicode-3.3)
endif()

set(WX_GIT_TAG v3.3.3)

# Everything wx is configured with, bar the install prefix. Kept in a list,
# rather than written inline in ExternalProject_Add, because it is also what
# the build id below is hashed from.
set(WX_CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        # Has to match the rest of the build, which uses the shared CRT
        # (CMake's default, /MD); a wx built against the static CRT would put
        # two heaps in one process.
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DwxBUILD_SHARED=ON
        -DwxBUILD_MONOLITHIC=ON
        -DwxBUILD_SAMPLES=OFF
        -DwxBUILD_TESTS=OFF
        -DwxBUILD_DEMOS=OFF
        -DwxBUILD_BENCHMARKS=OFF
        # Nothing here wants an embedded browser, and it is one of the more
        # expensive parts of a wx build.
        -DwxUSE_WEBVIEW=OFF
        # wxSTC is kept, and it is the expensive part of this build: it brings
        # Lexilla with it and the two are roughly 40% of the translation units
        # (1097 targets with, 680 without).
        #
        # It is kept because it is a candidate replacement for the Scintilla
        # this tree vendors. Sources/Scintilla is the 2005 one: a flat directory
        # with 37 Lex*.cxx compiled straight into the control, and
        # DocumentAccessor, WindowAccessor and PropSet still in it, all of which
        # upstream removed long ago. wx 3.3.3 carries Scintilla 5.0 and Lexilla
        # 5.4.6. More to the point, wx's copy has a wx platform layer where ours
        # has a Win32 one -- ScintillaWin.cxx and PlatWin.cxx, which is where
        # both of this year's clean-exit crashes were, and which would have to
        # be ported for a non-Windows editor.
        #
        # The wx text editors (MapEditor/TextEditorViewWx.cpp) are built on it,
        # so -DwxUSE_STC=OFF no longer builds the editor. A build directory
        # that turned it off earlier keeps OFF in the wx sub-build's own cache;
        # set it back there with cmake -DwxUSE_STC=ON <that directory>.
)

# Off Windows the toolkit is named rather than left to wx's default (also gtk3
# today), so what the libraries are built on is stated here and in the build
# id. Appended only there, so the Windows argument list, and with it the
# Windows build id and every cached install, stays exactly as it was.
if(NOT WIN32)
    list(APPEND WX_CMAKE_ARGS -DwxBUILD_TOOLKIT=gtk3)
endif()

# Identifies what an install was built from: the wx release, the toolchain tag
# and every argument above. The compiler paths are among those arguments, so an
# MSVC update changes the id too, which is deliberately conservative. The
# install prefix is left out so that an install can be restored to a different
# path and still match.
string(SHA256 WX_BUILD_ID "${WX_GIT_TAG};${WX_TOOLCHAIN_TAG};${WX_CMAKE_ARGS}")
set(WX_BUILD_ID_FILE ${WX_INSTALL}/wx-build-id.txt)

# Also written into the build directory, where CI reads it to name the cache
# entry the install is saved under.
file(WRITE ${CMAKE_BINARY_DIR}/wx-build-id.txt "${WX_BUILD_ID}")

set(WX_INSTALLED_ID "")
if(EXISTS ${WX_BUILD_ID_FILE} AND EXISTS ${WX_IMPORT_LIB})
    file(READ ${WX_BUILD_ID_FILE} WX_INSTALLED_ID)
    string(STRIP "${WX_INSTALLED_ID}" WX_INSTALLED_ID)
endif()

if(WX_INSTALLED_ID STREQUAL WX_BUILD_ID)
    # A finished install built from exactly these arguments: nothing to clone,
    # configure or build, which in CI is about 18 minutes per job. The editor
    # targets still name wxwidgets_external in add_dependencies, so it stays,
    # as a target that does nothing.
    message(STATUS "wxWidgets: reusing ${WX_INSTALL}")
    add_custom_target(wxwidgets_external)
else()
    message(STATUS "wxWidgets: building ${WX_GIT_TAG} into ${WX_INSTALL}")

    # Removed first, so an install that is being replaced, or one interrupted
    # half way through, is never mistaken for a match by the next configure.
    file(REMOVE ${WX_BUILD_ID_FILE})
    file(WRITE ${WX_PREFIX}/wx-build-id.txt "${WX_BUILD_ID}")

    ExternalProject_Add(wxwidgets_external
        GIT_REPOSITORY  https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG         ${WX_GIT_TAG}
        GIT_SHALLOW     TRUE
        GIT_SUBMODULES_RECURSE TRUE
        GIT_PROGRESS    TRUE
        PREFIX          ${WX_PREFIX}
        INSTALL_DIR     ${WX_INSTALL}
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${WX_INSTALL}
            ${WX_CMAKE_ARGS}
        BUILD_BYPRODUCTS ${WX_IMPORT_LIB}
        USES_TERMINAL_DOWNLOAD TRUE
        USES_TERMINAL_BUILD    TRUE
    )

    # The id goes in only once the install has finished, which is what makes
    # its presence mean "complete".
    ExternalProject_Add_Step(wxwidgets_external record_build_id
        COMMAND ${CMAKE_COMMAND} -E copy ${WX_PREFIX}/wx-build-id.txt ${WX_BUILD_ID_FILE}
        DEPENDEES install
    )
endif()

# The imported target the front-end links. IMPORTED_IMPLIB names a file that
# does not exist until wxwidgets_external has run, which is legal as long as
# whatever links it depends on that step -- see wx::wx below.
add_library(wxwidgets_monolithic SHARED IMPORTED GLOBAL)
if(WIN32)
    set(WX_HEADER_DIR ${WX_INSTALL}/include)
    set_target_properties(wxwidgets_monolithic PROPERTIES
        IMPORTED_IMPLIB ${WX_IMPORT_LIB})
else()
    # Unix installs put the headers one level down, under the version.
    set(WX_HEADER_DIR ${WX_INSTALL}/include/wx-3.3)
    # No import library on ELF: the .so is both what is linked and what runs.
    set_target_properties(wxwidgets_monolithic PROPERTIES
        IMPORTED_LOCATION ${WX_IMPORT_LIB})
endif()
set_target_properties(wxwidgets_monolithic PROPERTIES
    # Two include directories, not one: the generated setup.h is written into
    # the library directory next to the binaries rather than into include/.
    INTERFACE_INCLUDE_DIRECTORIES "${WX_HEADER_DIR};${WX_SETUP_DIR}"
)

# CMake refuses an imported target whose include directories do not exist, and
# wx's build has not run at configure time.
file(MAKE_DIRECTORY ${WX_HEADER_DIR} ${WX_SETUP_DIR})

# What consumers link. The interface library carries the dependency on the
# external build, so linking wx::wx is enough to get wx built first.
add_library(wx::wx INTERFACE IMPORTED GLOBAL)
set_target_properties(wx::wx PROPERTIES
    INTERFACE_LINK_LIBRARIES wxwidgets_monolithic
    # WXUSINGDLL is what tells wx's headers that wx is the shared build, so its
    # exported *data* is declared __declspec(dllimport). Leaving it out links
    # every function fine -- they come from the import library either way -- and
    # fails on the dozen globals: wxDefaultSize, wxDefaultPosition,
    # wxString::npos, wxAppConsoleBase::ms_appInstance, the control name
    # strings. A link error listing only data symbols and no functions is this
    # define missing.
    #
    # And *only* that one. This carried UNICODE and _UNICODE at first, on the
    # assumption that a wide-character wx needs them. It does not, and there are
    # three separate switches here that are easy to run together:
    #
    #   wxUSE_UNICODE  is wx's own, out of its setup.h. It decides what wxString
    #                  and wxChar are. It is 1 in this build and nothing on this
    #                  side can change it.
    #   UNICODE        is the Win32 one, read by <windows.h>. It decides whether
    #                  GetMessage means GetMessageA or GetMessageW, and what
    #                  LPTSTR is. Either can still be called explicitly; this is
    #                  only the default.
    #   _UNICODE       is the CRT one, read by <tchar.h>. It decides TCHAR and
    #                  the _t* functions -- _tcscpy and that family.
    #
    # wx needs none of the three from a consumer, which was verified rather than
    # reasoned: the skeleton builds and runs identically either way, still
    # reporting wxUSE_UNICODE=1 and sizeof(wxChar)=2. So the editor stays MBCS
    # throughout, like the rest of the engine, and none of this spreads. wxString
    # still has to be converted at the boundary, which is a conversion and not a
    # compilation mode.
    INTERFACE_COMPILE_DEFINITIONS "WXUSINGDLL"
)

# Off Windows the headers also have to be told the port, which wx-config
# passes as __WXGTK__ and __WXGTK3__: on MSW wx works it out from _WIN32, on
# GTK nothing can. _FILE_OFFSET_BITS=64 is the other define wx-config emits
# there; it has to match the library, since it changes the size of off_t in
# wx's own headers. The list is `wx-config --cxxflags` of the 3.3.3 GTK3
# install, less its two include directories.
if(NOT WIN32)
    set_property(TARGET wx::wx APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS "__WXGTK3__;__WXGTK__;_FILE_OFFSET_BITS=64")
endif()
