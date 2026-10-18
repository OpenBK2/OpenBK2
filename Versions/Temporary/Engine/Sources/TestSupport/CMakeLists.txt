# What every test executable carries beyond its own sources. One target so far.
project(TestSupport)

# OBJECT, not a library: the objects go on each test's link line, where the
# installer object in CrashTrace.cpp cannot be dropped for being unreferenced,
# which is what would happen to it inside a static library.
#
# Compiled once for every test rather than once per test, which is the other
# reason for a target here: boost/stacktrace.hpp is not a cheap include.
bk2_add_library(TestCrashTrace OBJECT CrashTrace.cpp)
set_target_properties(TestCrashTrace PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        EXCLUDE_FROM_ALL ON
        FOLDER "unittest")

# PUBLIC: the backend's library and its macro have to reach the executables
# these objects are linked into, not just this compile.
target_boost_stacktrace(TestCrashTrace PUBLIC)
