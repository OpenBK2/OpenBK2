# The one place this tree's diagnostics are decided.
#
# Every number quoted below was measured on this tree, GCC 14.2 and clang 22.1
# over all 1389 translation units; docs/DiagnosticsRuleSet.md has the method,
# the full census and the reasoning for each tier. Do not add a flag here
# without a count: half the flags in the published recommendation lists either
# cannot fire in C++17 (-Wtrampolines needs GNU nested functions,
# -Wwrite-strings warns about something that is already an error), do nothing
# on one of our two compilers (clang accepts -Wredundant-decls and ignores it,
# where GCC finds 31 sites), or crash the compiler (-Wmismatched-tags ICEs
# GCC 14.2 on three of our units).
#
# Three levels, because the tree is at zero on the first one and nowhere near
# zero on the second:
#
#   default            the flags that report zero sites here. Keeping them on
#                      costs nothing and stops the clean state regressing,
#                      which is the whole value of the 47-commit sweep that
#                      got here.
#   OBK2_WARNINGS_STRICT  the work in progress: -Wall -Wextra without its four
#                      bulk classes, plus the high-signal classes with tens of
#                      sites. About 700 sites on GCC and 930 on clang. Turn it
#                      on to work through a class, not to build.
#   OBK2_WERROR        promote this tree's warnings to errors. CI turns this on.
#                      Never on by default and never global: a newer compiler
#                      invents a diagnostic nobody could have pre-fixed, and a
#                      packager or a distribution should not get a hard failure
#                      out of that. Google Benchmark has broken this build
#                      twice in exactly that way, with its own -Werror and then
#                      with -pedantic-errors.

option(OBK2_WERROR "promote this tree's own warnings to errors (CI turns this on)" OFF)
option(OBK2_WARNINGS_STRICT "add the diagnostic classes still being cleaned up" OFF)

include(CheckCXXCompilerFlag)
# Every module here includes this itself rather than relying on the root having
# done it, which is the idiom the other cmake/*.cmake files already follow.
include(${CMAKE_CURRENT_LIST_DIR}/get_all_targets.cmake)

# Flags are feature-tested rather than version-gated. GCC makes an unknown
# -Wfoo a hard error (only -Wno-foo is forgiven), and this list spans GCC 6 to
# 14 additions: -Wduplicated-cond is GCC 6, -Wredundant-move 9, -Wbidi-chars 12,
# -Winvalid-utf8 and -Wself-move 13, -Wchanges-meaning 14. A version gate would
# have to be rewritten for every compiler the tree ever meets; a probe would
# not. CMake caches each probe, so this costs one configure, once.
#
# What a probe cannot tell us is whether the flag does anything, only that it
# is accepted. That is what the positive controls in docs/DiagnosticsRuleSet.md
# are for, and it is why these lists are per compiler rather than shared.
function(bk2_supported_warnings out_var)
    set(supported)
    foreach(flag IN LISTS ARGN)
        # The cache key has to be a valid identifier and unique per flag.
        string(MAKE_C_IDENTIFIER "OBK2_HAVE${flag}" probe)
        check_cxx_compiler_flag(${flag} ${probe})
        if(${probe})
            list(APPEND supported ${flag})
        endif()
    endforeach()
    set(${out_var} ${supported} PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Default level: measured at zero sites in our code.
# ---------------------------------------------------------------------------

# Two of these have no positive control yet and so rest on the weaker evidence
# of the two, a zero count: -Wsized-deallocation and -Wstrict-null-sentinel.
# Every other entry has been seen to fire on a snippet written for it.
set(OBK2_WARNINGS_GCC
    -Wduplicated-cond           # if/else-if chain with a repeated condition
    -Wextra-semi                # clang's namesake is 1123 sites here, GCC's is 0
    -Wmultichar
    -Wplacement-new=2           # placement new into too small a buffer
    -Wvla
    -Winvalid-utf8              # the sources are UTF-8 with Russian identifiers
    -Wbidi-chars=any            # Trojan Source, recommended by OpenSSF
    -Wself-move
    -Wvexing-parse
    -Wchanges-meaning
    -Wredundant-move
    -Wdelete-incomplete         # was a real defect here once, libdb/StructMetaInfo
    -Wclass-conversion
    -Wconversion-null
    -Wliteral-suffix
    -Wterminate
    -Winvalid-offsetof          # this tree uses offsetof on non-POD freely
    -Winaccessible-base
    -Wmissing-include-dirs
    -Wsized-deallocation
    -Wstrict-null-sentinel
    -Wconditionally-supported
    -Wstringop-overflow=4       # only fires with -O, see the codegen sweep mode
    # Cleared 2026-10-16 and promoted out of the strict list. Was 6 sites: four
    # hand-rolled float puns, now routed through a bit_cast that no longer lies
    # (Misc/Tools.h), and Lua's luaG_symbexec indexing code[] with an
    # uninitialised stack slot. The remaining two are the refcount rescue in
    # System/Basic.h's object macros, suppressed at the two expansion sites with
    # the reasoning written out there; that is parked UB, not a clean bill.
    # Only fires with -O, so a syntax-only sweep reports it as zero.
    -Wuninitialized
    # Cleared 2026-10-16 in the same pass. Was 19, and all but the four
    # 3Dmotor/Gfx.cpp ones were the same shape: a struct of raw PODs behind a
    # "SFoo() {}" default constructor, copied into a container as a temporary, so
    # default member initialisers fixed the class. SAnimWaterMaterialKey was the
    # one that mattered, being a lookup key compared member by member. Gfx.cpp
    # was ignoring GetClientRect's BOOL and sizing the back buffer from the
    # untouched RECT. -Wuninitialized turns this on with it anyway; listing it is
    # what makes that visible to a reader.
    -Wmaybe-uninitialized
    # Cleared 2026-10-16. Was 2, not the 3 the staging list claimed: a dead
    # IsBinDigit in Misc/StrProc.h that tested a char against '0' and '1' at
    # once, dropped rather than fixed, and CMOBuilding::Create testing the
    # main-storage type twice instead of the temporary one, which had kept the
    # client's idea of a storage out of step with AILogic's.
    -Wlogical-op
    # Cleared 2026-10-16. Was 3, and none of them was a typo to correct: two in
    # CFormationEngineer's reload animations where the "a mortar crew passes the
    # shell sitting" test had the same action on both arms, and one in
    # CUpdatableWorld's chat handler where the centre-of-screen variant was
    # never implemented and its TODO still says so. Both collapsed to the single
    # action with the open question recorded at the site, so behaviour is
    # unchanged. -Wduplicated-cond is in the default list above already.
    -Wduplicated-branches
)

# Shorter than GCC's, and the asymmetry is the point rather than an oversight:
# a flag is listed for the compiler it was measured on. -Wextra-semi is 0 on
# GCC and 1123 here on clang, so it is in the list above and not this one.
set(OBK2_WARNINGS_CLANG
    -Winconsistent-missing-override  # 318 sites once; the sweep cleared them
    -Wundefined-reinterpret-cast     # the strict-aliasing family's narrow case
    -Wover-aligned
    -Wbitfield-enum-conversion
    -Wformat-type-confusion
    -Wvla
    -Wassign-enum
    # Cleared 2026-10-16: was 2, both the same uninitialised diplomacy range in
    # the one CUnitsIter variant that had been missed. GCC has no equivalent at
    # any level, so this is the only compiler that guards the class.
    -Wsometimes-uninitialized
    # Cleared 2026-10-16. Was 3, and each was a different thing: CMOBuilding's
    # ammo total broke out of its own summation, CWorldClient's camera anchor
    # accumulated positions it never divided, and ActionLeaveOneSquad's loop
    # over the selection could only ever address one container. GCC has no
    # equivalent, so clang is the only compiler guarding this.
    -Wunreachable-code-loop-increment
    # The umbrella, and all three of its leaves were measured at 0 on
    # 2026-10-16 rather than inferred, which is the only thing that makes an
    # umbrella safe to adopt here:
    #
    #   -Wfor-loop-analysis           was 3. CArtillery's install check
    #       incremented its turret index twice and so walked only the even
    #       ones, a live simulation defect, plus two unused functions whose
    #       loops tested or incremented the wrong variable and were deleted.
    #   -Wrange-loop-bind-reference   was 2, both in 3Dmotor's glTF import,
    #       where a const reference bound the value fastgltf's accessor
    #       returns per step. Defined behaviour, since that extends the
    #       temporary's lifetime, but it avoided no copy.
    #   -Wrange-loop-construct        0, and it has never appeared in a sweep
    #       of this umbrella, which is how it was measured rather than assumed.
    #
    # GCC has no equivalent of any of them at any level, so clang is the only
    # compiler guarding the class. That is how the artillery defect survived.
    -Wloop-analysis
    # Cleared 2026-10-17. Was 1: AILogic/Executor.h pinned _EID_COUNT, the
    # sentinel that counts the executor event ids, to 1, which is
    # EID_ABILITY_DEACTIVATE. Nothing reads it, so the collision was a trap
    # rather than a defect, and the sentinel now counts. GCC has no equivalent.
    -Wduplicate-enum
    # Cleared without a pass of its own: the single site was IsBinDigit in
    # Misc/StrProc.h, which tested a character against '0' and '1' at the same
    # time, and the -Wlogical-op pass dropped the function. Measured at 0 on
    # 2026-10-17, with the positive control (`c == '0' && c == '1'`) still
    # firing, so the zero means clean rather than accepted and ignored.
    -Wtautological-overlap-compare
    # Cleared 2026-10-17. Was 2, and the same expression both times: an opaque
    # alpha written as 255 << 24, which is the one term of an ARGB word that
    # does not fit in an int. The BMP palette loader and the editor's
    # vec3-to-colour conversion both spell the constant 0xFF000000 now.
    -Wshift-sign-overflow
    # Cleared 2026-10-17, as a pair: both flags report a comparison against
    # zero that an unsigned type decides in advance, and the two together were
    # 11 sites. One was a defect, the editor's window title testing the room
    # left for a truncation after subtracting it in unsigned arithmetic, where
    # a negative result wraps instead. One was dead simulation code, the turn
    # tolerance of 0 that no turn could be within. The other nine were guards
    # whose upper half was doing all the work.
    -Wtautological-unsigned-zero-compare
    -Wtautological-unsigned-enum-zero-compare
    # Cleared 2026-10-17. Was 6, and both causes were a value a reader has to
    # reconstruct: five bool flags in the editor's VisObj builder written only
    # through a pointer by PrepareResource, and a crag height assigned inside a
    # loop condition and read in its body. Neither was a live uninitialised
    # read. This is clang's aggressive variant of -Wmaybe-uninitialized, which
    # reports a use uninitialised on any path it can see rather than on all of
    # them, so it is the one that objects to "initialised by a callee".
    -Wconditional-uninitialized
)

# MSVC is deliberately empty. No Windows toolchain was reachable when this was
# written, so the equivalent census does not exist, and docs/DiagnosticsRuleSet.md
# says so rather than guessing. The mapping is worked out there (C5204 for
# -Wnon-virtual-dtor, C5262 for -Wimplicit-fallthrough, C4302/C4311/C4312/C4319
# /C4826 for the x86-to-x64 truncation family that has already produced defects
# here, C4165 for the HRESULT-to-bool class) and wants measuring before it is
# switched on. /W4 and /permissive- belong with it and are the same decision:
# both change what compiles, so neither is a free adoption.
set(OBK2_WARNINGS_MSVC)

# ---------------------------------------------------------------------------
# Strict level: the work in progress. OBK2_WARNINGS_STRICT.
# ---------------------------------------------------------------------------

# -Wall -Wextra cost about 10,270 sites on GCC and 9,220 on clang in our code.
# Four classes are 9,569 and 8,288 of that, and none of them is a defect class
# here: -Wunused-parameter is interface noise, -Wignored-qualifiers is the
# `const int Get() const` house idiom, -Wsign-compare and -Wreorder are real
# but are 2,300 and 426 sites of their own project. Subtracting them leaves
# about 700 and 930, which is the number worth working through.
set(OBK2_WARNINGS_STRICT_COMMON
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-ignored-qualifiers
    -Wno-sign-compare
)

set(OBK2_WARNINGS_STRICT_GCC
    ${OBK2_WARNINGS_STRICT_COMMON}
    -Wno-reorder
    # Tier 2: tens of sites each, high signal. Counts as of 2026-10-15.
    -Wstrict-aliasing=3             # 12, the class that miscompiled the DXT decoder
    -Wnon-virtual-dtor              # 198
    -Wimplicit-fallthrough=5        # 31, and silent without -O
    -Wcast-align=strict             # 110, inert on x86 and real on ARM
    -Wshadow=compatible-local       # 149; plain -Wshadow is 637 on clang and noisy
    -Wdate-time                     # 2, reproducible builds; Debian passes it
    -Walloc-zero                    # 2
    -Warray-bounds=2                # 1
    -Wformat-overflow=2             # 2
    -Wformat-truncation=2           # 1
    -Wcatch-value=3                 # 1, in vendored Lua
    # Not here on purpose: -Wnull-dereference (143 sites, and nobody has read
    # 141 of them), -Wstrict-overflow (458 and documented as noisy),
    # -Wmismatched-tags (ICEs this compiler).
)

set(OBK2_WARNINGS_STRICT_CLANG
    ${OBK2_WARNINGS_STRICT_COMMON}
    -Wno-reorder-ctor
    # What is left of the clang-only classes. The ones that have reached zero
    # are in OBK2_WARNINGS_CLANG above, each with what it found written next to
    # it, and one of those found a live simulation defect: AILogic/Artillery.cpp
    # advanced its loop index twice and so waited for every second turret.
    #
    # -Wbitwise-instead-of-logical is NOT here and is not in the gated list
    # either, because it cannot reach zero in this tree. Measured 2026-10-16 as
    # 23 sites in two idioms, both deliberate:
    #
    #   15 in 3Dmotor combined CDGPtr::Refresh results with a bitwise |, so that
    #      every pointer is refreshed rather than short-circuited. Those now go
    #      through AnyRefreshed in System/Dg.h, which forces the same evaluation
    #      by passing the results as arguments and says so in its name. One more,
    #      GParts.cpp, was a bitwise & over four pure queries with nothing to
    #      force and is now &&.
    #   8 in AILogic write "( roll < threshold ) & RecordRandomCall()". The
    #      record must happen whichever way the comparison came out, since it is
    #      what records that the Random call on its left consumed a number, and
    #      RecordRandomCall has to stay a macro so __FILE__ and __func__ name the
    #      call site. There is no spelling of that which this flag accepts, so
    #      the eight stay and the flag stays out. The reasoning is at the macro
    #      in System/RandomGen.h.
    -Wheader-hygiene                # 9, `using namespace` in a header
    -Wenum-enum-conversion          # 9, includes the EFC_/EFUT_ mix-up
    -Wnon-virtual-dtor              # 90
    -Wdelete-non-abstract-non-virtual-dtor  # 11
    -Wimplicit-fallthrough          # 39
    -Wcast-align                    # 57
)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------

# -Wformat -Werror=format-security is unconditional rather than behind
# OBK2_WERROR, and it is the one exception to the no-unconditional-errors rule
# above. Debian, Fedora, Arch, Gentoo and OpenSSF all set it; it is twenty
# years stable, so it cannot be the diagnostic a future compiler invents; and
# this tree has already shipped two defects of exactly its shape, PrintTabs
# using runtime text as a format string and the ASYNC dump printing %I64u. It
# is 0 sites in our code on both compilers, and the pair was checked against a
# `printf( s )` to confirm it still catches one.
#
# **The -Wformat half is load-bearing.** Alone, -Werror=format-security makes
# GCC fail *every* translation unit with "'-Wformat-security' ignored without
# '-Wformat'", because -Wformat is not on without -Wall and the complaint about
# the ignored flag is itself promoted to an error. This is why every distro
# passes the two together. clang does not care either way. Probed as a pair for
# the same reason: probing -Werror=format-security on its own fails on GCC, and
# the flag would then be dropped silently and the protection lost.
set(OBK2_FORMAT_SECURITY -Wformat -Werror=format-security)
function(bk2_target_warnings target)
    get_target_property(kind ${target} TYPE)
    if(kind STREQUAL "INTERFACE_LIBRARY" OR kind STREQUAL "UTILITY")
        return()
    endif()

    if(MSVC)
        set(wanted ${OBK2_WARNINGS_MSVC})
        if(OBK2_WARNINGS_STRICT)
            list(APPEND wanted /W4)
        endif()
    else()
        set(wanted ${OBK2_WARNINGS_GCC})
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            set(wanted ${OBK2_WARNINGS_CLANG})
        endif()
        if(OBK2_WARNINGS_STRICT)
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                list(APPEND wanted ${OBK2_WARNINGS_STRICT_CLANG})
            else()
                list(APPEND wanted ${OBK2_WARNINGS_STRICT_GCC})
            endif()
        endif()
    endif()

    bk2_supported_warnings(flags ${wanted})

    # Probed together, never individually. See the note above OBK2_FORMAT_SECURITY.
    if(NOT MSVC)
        string(REPLACE ";" " " format_security_probe "${OBK2_FORMAT_SECURITY}")
        check_cxx_compiler_flag("${format_security_probe}" OBK2_HAVE_FORMAT_SECURITY)
        if(OBK2_HAVE_FORMAT_SECURITY)
            list(APPEND flags ${OBK2_FORMAT_SECURITY})
        endif()
    endif()

    if(OBK2_WERROR)
        if(MSVC)
            list(APPEND flags /WX)
        else()
            list(APPEND flags -Werror)
        endif()
    endif()

    # PRIVATE, and per target rather than in CMAKE_CXX_FLAGS: a fetched
    # dependency's warnings are not ours to fix, and a global flag would also
    # reach the generated parser and vendored Lua.
    #
    # The .cll files in some SOURCES lists are database schema inputs, not
    # translation units, so these go on through the compile-language genex to
    # keep them off anything that is not C or C++.
    target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${flags}>)
    set_target_properties(${target} PROPERTIES OBK2_WARNINGS_APPLIED ON)
endfunction()

# Applies the rule set to every target of this tree that has not had it already,
# which during the migration to bk2_add_library/bk2_add_executable is most of
# them. Converted targets carry OBK2_WARNINGS_APPLIED and are skipped, so the
# two mechanisms cannot double up. Shaped after enable_project_mimalloc() in
# cmake/project_allocator.cmake, which solves the same "every target of ours,
# and nothing else" problem.
function(bk2_enable_warnings_on_project_targets)
    get_all_targets(engine_targets ${CMAKE_SOURCE_DIR}/Versions/Temporary/Engine/Sources)
    foreach(target IN LISTS engine_targets)
        get_target_property(source_dir ${target} SOURCE_DIR)
        get_target_property(done ${target} OBK2_WARNINGS_APPLIED)
        # /vendor/ is Scintilla and friends: built here, not written here.
        if(NOT source_dir MATCHES "/vendor/" AND NOT done)
            bk2_target_warnings(${target})
        endif()
    endforeach()
endfunction()

define_property(TARGET PROPERTY OBK2_WARNINGS_APPLIED
    BRIEF_DOCS "bk2_target_warnings has already run on this target"
    FULL_DOCS "Set by bk2_target_warnings so the sweep over all engine targets \
does not apply the rule set a second time to a target created by \
bk2_add_library or bk2_add_executable.")
