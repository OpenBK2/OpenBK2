function(add_unit_test filename)
    set(target_name ${filename}_test)
    add_executable(${target_name} EXCLUDE_FROM_ALL ${filename}.cpp)
    target_link_libraries(${target_name} PRIVATE GTest::gtest_main 3Dmotor Misc)
    set_target_properties(${target_name} PROPERTIES FOLDER "unittest")
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:GTest::gtest_main>
            $<TARGET_FILE_DIR:${target_name}>)
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:GTest::gtest>
            $<TARGET_FILE_DIR:${target_name}>)
endfunction()

add_unit_test(MultiplyOnColor)
add_unit_test(CalcDirectionalLighting)
add_unit_test(CalcPointLightColors)
add_unit_test(AddColors)
add_unit_test(ScaleColors)
add_unit_test(MMXTransformVector)
add_unit_test(SampleWarFogInt)
add_unit_test(SampleWarFog)
add_unit_test(MMXhelpers)
