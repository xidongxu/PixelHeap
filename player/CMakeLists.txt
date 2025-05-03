# 创建一个静态库 player
add_library(player STATIC player.cpp)

# 包含头文件目录
target_include_directories(player PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 如果需要，链接到其他库
target_link_libraries(player freertos_kernel freertos_config)