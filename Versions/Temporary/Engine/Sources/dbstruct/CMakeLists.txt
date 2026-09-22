project(dbstruct)

# The database schema migrator. -update-struct opens the database in editor
# mode, loads every object of every class, marks each changed and saves, which
# rewrites the .xdb files in the current types.xml shape. That is the step after
# a field changes type or goes away; a field merely added takes its default and
# needs nothing.
#
# -make-bin is advertised by its own usage text but prints "not realized" and
# returns 0xDEAD, which is how it was left in 2005.
#
# A plain executable, not the SHARED library with an export header that every
# engine subsystem is: nothing links a tool, so it exports nothing.
set(SOURCES
    main.cpp
    stdafx.cpp
)
add_executable(dbstruct ${SOURCES})
add_stdafx_pch(dbstruct)

# Boost::program_options replaced System/CmdLine.h, which the port deleted as
# unused by the game.
target_link_libraries(dbstruct PRIVATE libdb MemoryLib Misc System zlib
                                       Boost::program_options fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(dbstruct PRIVATE winapi)
endif()

# As with dbindex, this is run from the data directory it operates on rather
# than from bin/.
install_runtime_dlls(dbstruct)

install(TARGETS dbstruct DESTINATION bin)

# -show-version reads REVISION_NUMBER_STR and BUILD_DATE_TIME_STR, which this
# defines. Without it the tool does not compile.
add_version_info(dbstruct)
