# musl-libm's results against hashes recorded from the Windows and the Linux
# build, which agree. The library is vendor/musl-libm; the test lives here, with
# this tree's code, so that it gets the same allocator, hygiene and formatting.
add_unit_test(MuslLibm musl_libm)
set_tests_properties(MuslLibm_test PROPERTIES TIMEOUT 120)

# Misc/DetMath.h, the engine's wrappers over it: std's overload rules, musl's values.
add_unit_test(DetMath musl_libm)

# Unicode conversion must also work after the main thread's cached converters
# have been destroyed, when static destructors flush persistent console output.
find_package(Threads REQUIRED)
add_unit_test(Unicode Boost::predef Threads::Threads)
