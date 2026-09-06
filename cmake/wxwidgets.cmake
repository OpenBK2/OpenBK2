# wxWidgets, the toolkit the editor is being ported to.
#
# Gated behind BUILD_WX_EDITOR, which is OFF, and it should stay OFF until the wx
# front-end does something. CI builds both presets from scratch on every push and
# caches only the DirectX SDK -- no _deps cache, no ccache -- so an unconditional
# wx would put a large library on every push twice over before it earns it.
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
set(WX_INSTALL ${WX_PREFIX}/install)

# wx keeps its own library layout under the install prefix rather than the usual
# lib/ and bin/: everything lands in lib/<toolchain tag>/, the import library,
# the DLL and the generated setup.h together. The tag is "vc" plus an
# architecture suffix plus the linkage.
#
# The x64 spelling is the one that has been built and checked. wx leaves the
# suffix off for 32-bit, so the other branch is what its convention says rather
# than something observed -- if an x86 wx build ever fails to link, this is the
# first line to look at.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WX_TOOLCHAIN_TAG vc_x64_dll)
else()
    set(WX_TOOLCHAIN_TAG vc_dll)
endif()

set(WX_LIB_DIR ${WX_INSTALL}/lib/${WX_TOOLCHAIN_TAG})

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

set(WX_IMPORT_LIB ${WX_LIB_DIR}/wxmsw33u${WX_LIB_SUFFIX}.lib)

ExternalProject_Add(wxwidgets_external
    GIT_REPOSITORY  https://github.com/wxWidgets/wxWidgets.git
    GIT_TAG         v3.3.3
    GIT_SHALLOW     TRUE
    GIT_SUBMODULES_RECURSE TRUE
    GIT_PROGRESS    TRUE
    PREFIX          ${WX_PREFIX}
    INSTALL_DIR     ${WX_INSTALL}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${WX_INSTALL}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        # Has to match the rest of the build. MFC comes in as _AFXDLL, the
        # shared MFC DLL, which means the shared CRT; a wx built against the
        # static CRT would put two heaps in one process.
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
        # Turn it off with -DwxUSE_STC=OFF locally if a wx build is in the way
        # of something; nothing depends on it yet.
    BUILD_BYPRODUCTS ${WX_IMPORT_LIB}
    USES_TERMINAL_DOWNLOAD TRUE
    USES_TERMINAL_BUILD    TRUE
)

# The imported target the front-end links. IMPORTED_IMPLIB names a file that
# does not exist until wxwidgets_external has run, which is legal as long as
# whatever links it depends on that step -- see wx::wx below.
add_library(wxwidgets_monolithic SHARED IMPORTED GLOBAL)
set_target_properties(wxwidgets_monolithic PROPERTIES
    IMPORTED_IMPLIB ${WX_IMPORT_LIB}
    # Two include directories, not one: the generated setup.h is written into
    # the library directory next to the binaries rather than into include/.
    INTERFACE_INCLUDE_DIRECTORIES "${WX_INSTALL}/include;${WX_LIB_DIR}/mswu"
)

# CMake refuses an imported target whose include directories do not exist, and
# wx's build has not run at configure time.
file(MAKE_DIRECTORY ${WX_INSTALL}/include ${WX_LIB_DIR}/mswu)

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
    # assumption that a wide-character wx needs them. It does not, and getting
    # this wrong in the other direction would have been expensive:
    #
    #   wxUSE_UNICODE lives in wx's own setup.h and decides what wxString and
    #   wxChar are. It is 1 in this build and nothing here can change it.
    #   _UNICODE decides Win32 TCHAR mapping in the *consumer's* code and, on
    #   this project, which MFC is linked -- MFC ships separate MBCS and Unicode
    #   runtimes and two of them in one process is not a thing that works.
    #
    # The two axes are independent, which was verified rather than reasoned: the
    # skeleton builds and runs the same either way, still reporting
    # wxUSE_UNICODE=1 and sizeof(wxChar)=2. So the editor stays MBCS throughout,
    # a translation unit can include afxwin.h and wx/wx.h together, and none of
    # this spreads. wxString still has to be converted at the boundary, which is
    # a conversion and not a compilation mode.
    INTERFACE_COMPILE_DEFINITIONS "WXUSINGDLL"
)
