cmake_minimum_required(VERSION 3.10)
project(WinSDLGLTest C CXX)
set(CMAKE_CXX_STANDARD 11)

# 平台区分：Windows 下使用桌面 OpenGL
if(WIN32)
  add_definitions(-DUSE_DESKTOP_GL)
  include_directories(
    ${CMAKE_SOURCE_DIR}/external/SDL2/include
    ${CMAKE_SOURCE_DIR}/include
    )
    link_directories(${CMAKE_SOURCE_DIR}/external/SDL2/lib/x64)  # 根据目标平台改为 x86 或 x64
else()
    add_definitions(-DUSE_GLES2)
    find_package(SDL2 REQUIRED)
    include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIRS}
    )
endif()

# SDL2：手动指定 include 和 lib 目录
# include_directories(
#     ${CMAKE_SOURCE_DIR}/external/SDL2/include
#     ${CMAKE_SOURCE_DIR}/include
#     )

# glad loader
include_directories(${CMAKE_SOURCE_DIR}/external/glad/include)

# 源文件列表
file(GLOB SRC_FILES
    src/main.cpp
    src/Core/Platform.cpp
    src/Core/Render.cpp
    ${CMAKE_SOURCE_DIR}/external/glad/src/glad.c
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
    SDL2
    SDL2main
    opengl32
)