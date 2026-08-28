# Compare what the built library actually exports against exports.txt.
#
# Run through cmake -P by the libgr2-verify-exports target, which passes
# GR2_LIBRARY, GR2_EXPECTED and GR2_MSVC.
#
# The point is that the replacement is a drop-in: the engine's link against
# granny2 has to resolve, and nothing beyond those 54 names should escape and
# collide with the real DLL when both are loaded for A/B comparison.

if(NOT DEFINED GR2_LIBRARY OR NOT DEFINED GR2_EXPECTED)
    message(FATAL_ERROR "VerifyExports.cmake needs GR2_LIBRARY and GR2_EXPECTED")
endif()

file(STRINGS "${GR2_EXPECTED}" expected)
list(SORT expected)

if(GR2_MSVC)
    find_program(DUMPBIN dumpbin)
    if(NOT DUMPBIN)
        message(FATAL_ERROR "dumpbin not found; run this from a Visual Studio developer prompt")
    endif()
    execute_process(COMMAND ${DUMPBIN} /nologo /exports "${GR2_LIBRARY}"
                    OUTPUT_VARIABLE raw RESULT_VARIABLE status)
else()
    find_program(NM nm)
    if(NOT NM)
        message(FATAL_ERROR "nm not found")
    endif()
    execute_process(COMMAND ${NM} --dynamic --defined-only --format=posix "${GR2_LIBRARY}"
                    OUTPUT_VARIABLE raw RESULT_VARIABLE status)
endif()

if(NOT status EQUAL 0)
    message(FATAL_ERROR "could not read exports from ${GR2_LIBRARY}")
endif()

# One name per line, then keep only the Granny entry points.
#
# On x86 MSVC decorates a __stdcall export as _Name@N, so both the leading
# underscore and the trailing byte count come off before comparing. On x64 there
# is no decoration and the same expressions leave the name alone.
string(REGEX MATCHALL "_?Granny[A-Za-z0-9_]*(@[0-9]+)?" found "${raw}")
set(actual "")
foreach(symbol IN LISTS found)
    string(REGEX REPLACE "^_" "" symbol "${symbol}")
    string(REGEX REPLACE "@[0-9]+$" "" symbol "${symbol}")
    list(APPEND actual "${symbol}")
endforeach()
list(REMOVE_DUPLICATES actual)
list(SORT actual)

set(missing "${expected}")
list(REMOVE_ITEM missing ${actual})
set(unexpected "${actual}")
list(REMOVE_ITEM unexpected ${expected})

list(LENGTH expected expected_count)
list(LENGTH actual actual_count)

if(missing)
    string(REPLACE ";" "\n  " missing "${missing}")
    message(FATAL_ERROR "not exported by ${GR2_LIBRARY}:\n  ${missing}")
endif()
if(unexpected)
    string(REPLACE ";" "\n  " unexpected "${unexpected}")
    message(FATAL_ERROR "exported but not in exports.txt:\n  ${unexpected}")
endif()

message(STATUS "${GR2_LIBRARY}: all ${expected_count} expected exports present, ${actual_count} in total")
