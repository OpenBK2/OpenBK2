# cmake -DTOOL=<nm|dumpbin path> -DKIND=<nm|dumpbin> -DLIB=<library>
#       [-DSTRIP_UNDERSCORE=ON] -P check-symbols.cmake
#
# STRIP_UNDERSCORE for 32-bit Windows, where C names carry a leading _.
#
# Fails if musl_libm defines a global symbol without the musl_ prefix, or
# references one of the platform's transcendental functions or an unrenamed
# musl kernel. See CMakeLists.txt.

cmake_minimum_required(VERSION 3.23)

if(KIND STREQUAL "nm")
    execute_process(COMMAND "${TOOL}" -g --defined-only "${LIB}" OUTPUT_VARIABLE defined_out RESULT_VARIABLE rc)
    execute_process(COMMAND "${TOOL}" -u "${LIB}" OUTPUT_VARIABLE undefined_out RESULT_VARIABLE rc2)
    if(rc OR rc2)
        message(FATAL_ERROR "${TOOL} failed on ${LIB}")
    endif()
    # "0000000000000000 T musl_sin" and "                 U sqrt"
    string(REGEX MATCHALL "[ \t][A-Za-z][ \t]+[^ \t\r\n]+" defined_lines "${defined_out}")
    string(REGEX MATCHALL "[ \t]U[ \t]+[^ \t\r\n]+" undefined_lines "${undefined_out}")
else()
    execute_process(COMMAND "${TOOL}" /nologo /symbols "${LIB}" OUTPUT_VARIABLE out RESULT_VARIABLE rc)
    if(rc)
        message(FATAL_ERROR "${TOOL} failed on ${LIB}")
    endif()
    # "008 00000000 SECT3  notype ()    External     | musl_sin"
    # "00A 00000000 UNDEF  notype ()    External     | sqrt"
    string(REGEX MATCHALL "[^\r\n]*External[ \t]+[|][ \t]+[^ \t\r\n]+" lines "${out}")
    set(defined_lines)
    set(undefined_lines)
    foreach(line IN LISTS lines)
        if(line MATCHES "UNDEF")
            list(APPEND undefined_lines "${line}")
        else()
            list(APPEND defined_lines "${line}")
        endif()
    endforeach()
endif()

function(symbol_names out)
    set(names)
    foreach(line IN LISTS ARGN)
        string(REGEX MATCH "[^ \t|]+$" name "${line}")
        # 32-bit Windows decorates C names with a leading underscore.
        if(STRIP_UNDERSCORE AND name MATCHES "^_[A-Za-z_]")
            string(REGEX REPLACE "^_" "" name "${name}")
        endif()
        list(APPEND names "${name}")
    endforeach()
    list(REMOVE_DUPLICATES names)
    set(${out} ${names} PARENT_SCOPE)
endfunction()

symbol_names(defined ${defined_lines})
symbol_names(undefined ${undefined_lines})

set(errors)
foreach(name IN LISTS defined)
    # MSVC emits floating point constants as COMDAT globals (__real@3ff0...),
    # and its math.h defines a few exact functions inline in C (fabsf), which
    # are the CRT's own code, not musl's.
    if(NOT name MATCHES "^musl_" AND NOT name MATCHES "^_?_(real|xmm|ymm|zmm)@"
       AND NOT name MATCHES "^(fabs|floor|ceil|sqrt|fmod|frexp|ldexp|modf)[fl]?$")
        list(APPEND errors "defines ${name}: add it to compat/musl_libm_prefix.h")
    endif()
endforeach()

set(transcendental "sin|cos|tan|asin|acos|atan|atan2|sinh|cosh|tanh|asinh|acosh|atanh|exp|exp2|expm1|log|log2|log10|log1p|pow|hypot|cbrt|erf|erfc|lgamma|tgamma")
foreach(name IN LISTS undefined)
    if(name MATCHES "^(${transcendental})[fl]?$")
        list(APPEND errors "calls the platform's ${name}")
    elseif(name MATCHES "^_?_(sin|cos|tan|rem_pio2|expo2|math_|exp|log|pow|polevl|p1evl|lgamma)")
        list(APPEND errors "references unrenamed ${name}: add it to compat/musl_libm_prefix.h")
    endif()
endforeach()

if(errors)
    list(JOIN errors "\n  " text)
    message(FATAL_ERROR "musl_libm symbol check failed:\n  ${text}")
endif()
list(FILTER defined INCLUDE REGEX "^musl_")
list(LENGTH defined n)
message(STATUS "musl_libm: ${n} musl_* symbols, nothing else of musl's, no platform libm calls")
