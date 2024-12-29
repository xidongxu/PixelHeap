# 创建一个静态库 player
add_library(player STATIC player.cpp)

# 包含头文件目录
target_include_directories(player PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 如果需要，链接到其他库
# target_link_libraries(foo some_other_library)

# 将整个目录的文件拷贝到构建目录
file(GLOB ASSETS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
foreach(file ${ASSETS_FILES})
    configure_file(${file} ${CMAKE_BINARY_DIR}/assets/ COPYONLY)
endforeach()
