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
if(WIN32)
  # glad 只在桌面GL下用
  include_directories(${CMAKE_SOURCE_DIR}/external/glad/include)
  set(SRC_FILES
      src/main.cpp
      src/Core/Platform.cpp
      src/Core/Render.cpp
      src/Core/Mesh.cpp
      src/Core/CubeMesh.cpp
      src/Core/PanelMesh.cpp
      src/Math/MathTool.cpp
      src/Core/InstanceBase.cpp
      
      ${CMAKE_SOURCE_DIR}/external/glad/src/glad.c
  )
else()
  # 树莓派/ARM下不用 glad
  set(SRC_FILES
      src/main.cpp
      src/Core/Platform.cpp
      src/Core/Render.cpp
      src/Core/Mesh.cpp
      src/Core/CubeMesh.cpp
      src/Math/MathTool.cpp
      src/Core/PanelMesh.cpp
      src/Core/InstanceBase.cpp
  )
endif()

# 可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
    SDL2
    opengl32
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME}
        SDL2
        opengl32
    )
else()
    target_link_libraries(${PROJECT_NAME}
        SDL2
        GLESv2
    )
endif()