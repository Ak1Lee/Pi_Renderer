#pragma once
// Use macro to include correct OpenGL header
#ifdef USE_DESKTOP_GL
#include <glad/glad.h>
#else
#include <GLES2/gl2.h> // For OpenGL ES (Raspberry Pi)
#endif
#include <memory>
#include <vector>
#include "Mesh.h"
#include "CubeMesh.h" // Include CubeMesh class for cube rendering
#include "PanelMesh.h" // Include PanelMesh class for panel rendering
#include "InstanceBase.h" // Include Instance class for rendering instances

namespace Core {

class Renderer {
public:
    // 初始化着色器与几何体
    bool init();
    // 设置窗口大小以更新视口
    void resize(int width, int height);
    // 渲染，参数为 4x4 MVP 矩阵
    void render(const float mvp[16], const float model[16]);
    
    void render(const float vp[16], const std::vector<float*>& modelMatrices);
    // 清理资源
    void renderStaticInstances(const float vp[16], const std::vector<Core::Instance*>& instances);

    void renderDynamicInstances(const float vp[16], const std::vector<Core::Instance*>& instances);

    void renderPanel(const float vp[16],const float model[16]);

    void shutdown();
private:
    unsigned int shaderProgram;
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    int indexCount;
    bool compileShaders();

    CubeMesh Cube; // 使用 CubeMesh 类来处理立方体网格
    PanelMesh Panel; // 使用 PanelMesh 类来处理面板网格

};

} // namespace Core