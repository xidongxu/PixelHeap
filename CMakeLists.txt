# CMakeList.txt: audio-player 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.10)
# 设置C++标准
set(CMAKE_CXX_STANDARD 11)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("audio-player")
# 源代码
add_executable (audio-player "audio-player.cpp" "audio-player.h")

# 拷贝资源文件
file(COPY ${CMAKE_SOURCE_DIR}/player/assets DESTINATION ${CMAKE_BINARY_DIR})

# 设置 SDL2 路径
set(SDL2_DIR ${CMAKE_SOURCE_DIR}/libs/SDL2-2.30.10/cmake/)
set(SDL2_image_DIR ${CMAKE_SOURCE_DIR}/libs/SDL2_image-2.8.3/cmake/)
set(SDL2_ttf_DIR ${CMAKE_SOURCE_DIR}/libs/SDL2_ttf-2.22.0/cmake/)
# 找到 SDL2 库
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
# 包含 SDL2 头文件
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})
# 链接 SDL2 库
target_link_libraries(audio-player SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)
# 避免 SDL2 替换 main 函数
add_definitions(-DSDL_MAIN_HANDLED)

# 定义 SDL2 的 DLL 文件路径
set(SDL2_DLLS
    ${SDL2_LIBDIR}/SDL2.dll
    ${SDL2_IMAGE_LIBDIR}/SDL2_image.dll
    ${SDL2_TTF_LIBDIR}/SDL2_ttf.dll
)

# 添加自定义命令，将 DLL 复制到目标目录
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${SDL2_DLLS}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# 包含 minimp3 头文件
include_directories(${CMAKE_SOURCE_DIR}/libs/minimp3/)

# 子目录
add_subdirectory(audio)
# 链接 audio 库
target_link_libraries(audio-player audio)
# 包含 audio 头文件
include_directories(${CMAKE_SOURCE_DIR}/audio/)

# 子目录
add_subdirectory(player)
# 链接 player 库
target_link_libraries(audio-player player)
# 包含 player 头文件
include_directories(${CMAKE_SOURCE_DIR}/player/)

# 子目录
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/FastLZ)
# 链接 audio 库
target_link_libraries(audio-player  fastlz)
# 包含 audio 头文件
include_directories(${CMAKE_SOURCE_DIR}/libs/FastLZ/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET audio-player PROPERTY CXX_STANDARD 20)
endif()
