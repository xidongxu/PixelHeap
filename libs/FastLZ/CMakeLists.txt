# 创建一个静态库 fastlz
add_library(fastlz STATIC fastlz.c)

# 包含头文件目录
target_include_directories(fastlz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 如果需要，链接到其他库
# target_link_libraries(foo some_other_library)
