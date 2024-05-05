add_executable(test_block_allocator "${CMAKE_CURRENT_SOURCE_DIR}/test_block_allocator.c")
target_link_libraries(test_block_allocator dcc test_helpers)

add_test(NAME BlockAllocatorTest COMMAND test_block_allocator)
