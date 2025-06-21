#pragma once

namespace Core {

class Renderer {
public:
    // 初始化着色器与几何体
    bool init();
    // 设置窗口大小以更新视口
    void resize(int width, int height);
    // 渲染，参数为 4x4 MVP 矩阵
    void render(const float mvp[16]);
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