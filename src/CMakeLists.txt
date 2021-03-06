add_executable(dcc main.c)
#target_sources(executable_name PRIVATE 
#               "${CMAKE_CURRENT_SOURCE_DIR}/file.cpp" 
#               "${CMAKE_CURRENT_SOURCE_DIR}/file.h")
target_compile_features(dcc PRIVATE c_std_11)
