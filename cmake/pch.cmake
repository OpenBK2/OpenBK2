# Wires a project's existing VS2003-era stdafx.h prelude up to CMake's
# precompiled-header support.
#
# 1328 of the 1357 translation units in the tree include a stdafx.h, and those
# preludes are not small: a typical one pulls in windows.h, a dozen STL headers
# and the engine hub headers (System.h, Basic.h, BinSaver.h, DB.h, Tools.h).
# Without /Yc + /Yu that whole graph is re-parsed once per .cpp. The headers
# were already written to be PCH content, the CMake port just never connected
# them, so this only turns on what the code was structured for.
#
# Note that CMake force-includes the PCH (/FI) into *every* source of the
# target, not only the ones that spell out #include "stdafx.h". That is fine
# for the targets wired up here because all of their sources already include
# the prelude, but it is the thing to check before adding a new target: a
# source that deliberately avoids stdafx.h (to dodge the windows.h min/max
# macros, say) needs the SKIP_PRECOMPILE_HEADERS source property.
#
# Scintilla is the one built target left out: it is third-party, has no
# stdafx.h and none of its 60 sources reference one.
#
# The per-target stdafx.h files are not interchangeable and the compile flags
# are not uniform either (AILogic builds with /fp:strict), so each target gets
# its own PCH rather than a REUSE_FROM chain.
#
# Configure with -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON to build without this,
# which is worth doing after touching includes: a PCH can mask a .cpp that
# forgot an #include of its own and only fails on a compiler that has no PCH.
function(add_stdafx_pch target)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h")
        message(FATAL_ERROR "add_stdafx_pch(${target}): no stdafx.h in ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    # PRIVATE: the PCH is an implementation detail of building this module and
    # must not be pushed onto targets that link it.
    target_precompile_headers(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h")
endfunction()
