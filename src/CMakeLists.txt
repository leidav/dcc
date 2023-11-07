add_executable(dcc-bin "${CMAKE_CURRENT_SOURCE_DIR}/main.c")

add_library(dcc STATIC)

target_sources( 
	dcc PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/helper.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/helper.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/keyword_hashes.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/token_print.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/lexer.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/lexer.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/input_file.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/input_file.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/string_set.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/string_set.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/allocator.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/allocator.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/linear_allocator.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/linear_allocator.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/block_allocator.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/block_allocator.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/scratchpad.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/scratchpad.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/error.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/error.h"
)
target_include_directories(dcc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(dcc PUBLIC c_std_11)

target_link_libraries(dcc-bin dcc)
set_target_properties(dcc-bin PROPERTIES RUNTIME_OUTPUT_NAME dcc)

#target_compile_options(dcc PRIVATE -fsanitize=address,undefined)
#target_link_options(dcc PRIVATE -fsanitize=address,undefined)
