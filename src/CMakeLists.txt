add_executable(dcc main.c)
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
  "${CMAKE_CURRENT_SOURCE_DIR}/allocator.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/allocator.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/scratchpad.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/scratchpad.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/error.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/error.h"
)
target_compile_features(dcc PRIVATE c_std_11)

#target_compile_options(dcc PRIVATE -fsanitize=address,undefined)
#target_link_options(dcc PRIVATE -fsanitize=address,undefined)
