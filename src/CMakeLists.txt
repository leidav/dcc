add_executable(dcc main.c)
target_sources(dcc PRIVATE 
               "${CMAKE_CURRENT_SOURCE_DIR}/string_set.c" 
               "${CMAKE_CURRENT_SOURCE_DIR}/string_set.h"
               "${CMAKE_CURRENT_SOURCE_DIR}/allocator.c" 
               "${CMAKE_CURRENT_SOURCE_DIR}/allocator.h")
target_compile_features(dcc PRIVATE c_std_11)
