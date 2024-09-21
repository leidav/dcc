add_executable(test_lexer "${CMAKE_CURRENT_SOURCE_DIR}/test_lexer.c")
target_link_libraries(test_lexer dcc test_helpers)

add_test(NAME "Lex Macros"
         WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data"
         COMMAND test_lexer macro.c)
	 add_test(NAME "Lex Macros 2"
         WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data"
         COMMAND test_lexer macro2.c)
