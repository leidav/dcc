add_library(test_helpers STATIC)
target_sources(test_helpers 
	PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/test.h"
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/test.c")

target_include_directories(test_helpers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(${CMAKE_C_COMPILER_ID} MATCHES "GNU|Clang")
	target_compile_options(test_helpers PUBLIC -fsanitize=address,undefined)
	target_link_options(test_helpers PUBLIC -fsanitize=address,undefined)
endif()
