add_library(test_helpers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/test.h")
target_include_directories(test_helpers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(${CMAKE_C_COMPILER_ID} MATCHES "GNU|Clang")
	target_compile_options(test_helpers INTERFACE -fsanitize=address,undefined)
	target_link_options(test_helpers INTERFACE -fsanitize=address,undefined)
endif()
