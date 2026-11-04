# musl-libm's results against hashes recorded from the Windows and the Linux
# build, which agree. The library is vendor/musl-libm; the test lives here, with
# this tree's code, so that it gets the same allocator, hygiene and formatting.
add_unit_test(MuslLibm musl_libm)
set_tests_properties(MuslLibm_test PROPERTIES TIMEOUT 120)
