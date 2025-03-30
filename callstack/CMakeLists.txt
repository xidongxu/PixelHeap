# 创建一个静态库 debug
add_library(callstack STATIC callstack.cpp)

# 包含头文件目录
target_include_directories(callstack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

