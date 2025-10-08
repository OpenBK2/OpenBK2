add_executable(3Dmotor_test EXCLUDE_FROM_ALL
  MultiplyOnColor.cpp)
target_link_libraries(3Dmotor_test PRIVATE GTest::gtest_main 3Dmotor Misc)
gtest_discover_tests(3Dmotor_test)
