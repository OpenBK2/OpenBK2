project(dbindex)

# The database index builder. It walks a data root through a VFS, registers
# every .xdb it finds and writes index.bin, which the game loads to find
# records without opening each file. Versions/Current/Data/dbindex.bat is the
# shipped workflow: delete index.bin, run this, done. Anyone who adds or edits
# an .xdb record needs it, which is why it is built rather than left to the
# prebuilt copy that ships in the game directory.
#
# A plain executable, not the SHARED library with an export header that every
# engine subsystem is: nothing links a tool, so it exports nothing.
set(SOURCES
    main.cpp
    stdafx.cpp
)
add_executable(dbindex ${SOURCES})
add_stdafx_pch(dbindex)

# libdb is what it actually drives; the rest come in through stdafx.h, which
# pulls System, Misc and the serialization headers the way every project here
# does.
target_link_libraries(dbindex PRIVATE libdb MemoryLib Misc System zlib fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(dbindex PRIVATE winapi)
endif()

# This matters more here than for the other executables, because the tool is
# run from the data directory it indexes rather than from bin/.
install_runtime_dlls(dbindex)

install(TARGETS dbindex DESTINATION bin)

# -show-version reads REVISION_NUMBER_STR and BUILD_DATE_TIME_STR, which this
# defines. Without it the tool does not compile.
add_version_info(dbindex)
