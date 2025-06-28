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

    void renderEmissiveToRadianceFBO(const float vp[16], const std::vector<Instance*>& instances);

    void renderDiffuseFBO(const float vp[16],const std::vector<Instance*>& instances);
    
    // 添加支持SDF GI的新渲染函数
    void renderDiffuseFBO(const float vp[16], const std::vector<Instance*>& instances, 
                         const float playerWorldPos[3], const float viewProjectionMatrix[16]);

    void renderBlockMap(const float vp[16], const std::vector<Instance*>& instances);

    void renderPPGI();

    void OneFrameRenderFinish(bool usePostProcessing = true);

    void shutdown();
private:
    unsigned int shaderProgram;
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;

    //FBO
    unsigned int sceneFBO = 0;
    unsigned int sceneColorTex = 0;
    unsigned int sceneDepthRBO = 0;

    unsigned int quadVAO = 0, quadVBO = 0;
    unsigned int quadShaderProgram = 0;

    //BlockMapFBO
    unsigned int blockMapFBO = 0;
    unsigned int blockMapTex = 0;
    unsigned int blockMapShaderProgram = 0;
    unsigned int blockMapVAO = 0, blockMapVBO = 0;




    //Blur FBO
    unsigned int blurFBO[2] = {0,0};
    unsigned int blurTex[2] = {0,0};

    unsigned int blurVAO = 0, blurVBO = 0;
    unsigned int blurShaderProgram = 0;


    // radianceFBO
    unsigned int radianceFBO = 0;
    unsigned int radianceTex = 0;
    unsigned int radianceShaderProgram = 0;
    unsigned int radianceDiffuseShaderProgram = 0;

    //PostProcessing
    unsigned int postprocessingFBO_GI = 0;
    unsigned int postprocessingTex_GI = 0;
    unsigned int ppgiShaderProgram = 0;
    


    // Uniform locations cache for performance
    GLint loc_mvpMatrix = -1;
    GLint loc_modelMatrix = -1;
    GLint loc_color = -1;
    GLint loc_emissive = -1;
    GLint loc_lightDir = -1;
    GLint loc_radianceTex = -1;
    
    // SDF GI相关uniform变量
    GLint loc_playerScreenPos = -1;  // 玩家屏幕坐标
    GLint loc_texelSize = -1;        // 纹素大小
    GLint loc_lightRange = -1;       // 光照范围

    int indexCount;
    bool compileShaders();

#ifdef USE_GLES2
    void bindQuadVertexAttributes();
#endif

    CubeMesh Cube; // 使用 CubeMesh 类来处理立方体网格
    PanelMesh Panel; // 使用 PanelMesh 类来处理面板网格
    

};

} // namespace Core