add_unit_test(ParserCases Parser System Misc)
# one directory per case, named NNN.success or NNN.fail by the expected result
target_compile_definitions(ParserCases_test PRIVATE
    PARSER_CASES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/cases")
