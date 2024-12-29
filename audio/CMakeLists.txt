# 创建一个静态库 audio
add_library(audio STATIC audio.cpp)

# 包含头文件目录
target_include_directories(audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 如果需要，链接到其他库
# target_link_libraries(foo some_other_library)
