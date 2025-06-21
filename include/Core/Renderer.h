#pragma once
// Use macro to include correct OpenGL header
#ifdef USE_DESKTOP_GL
#include <glad/glad.h>
#else
#include <GLES2/gl2.h> // For OpenGL ES (Raspberry Pi)
#endif
#include <memory>
#include "Mesh.h"

namespace Core {

class Renderer {
public:
    // 初始化着色器与几何体
    bool init();
    // 设置窗口大小以更新视口
    void resize(int width, int height);
    // 渲染，参数为 4x4 MVP 矩阵
    void render(const float mvp[16], const float model[16]);
    // 清理资源
    void shutdown();
private:
    unsigned int shaderProgram;
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    int indexCount;
    bool compileShaders();
};

} // namespace Core