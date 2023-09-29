add_executable(test_block_allocator "${CMAKE_CURRENT_SOURCE_DIR}/test_block_allocator.c")
target_sources(test_block_allocator PRIVATE 
	"${CMAKE_SOURCE_DIR}/src/memory/allocator.c"
	"${CMAKE_SOURCE_DIR}/src/memory/block_allocator.c"
	)
target_include_directories(test_block_allocator PRIVATE "${CMAKE_SOURCE_DIR}/src")
