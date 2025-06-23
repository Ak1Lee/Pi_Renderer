#pragma once
#include "Mesh.h"

namespace Core {

class Instance {
public:
    Mesh* mesh;           // 指向网格对象
    float modelMatrix[16];
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 默认白色
    float emissive[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // 默认无自发光

    Instance(Mesh* m) : mesh(m) {
        // 默认初始化
        for (int i = 0; i < 16; ++i) modelMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        color[0] = color[1] = color[2] = 1.0f; color[3] = 1.0f;
    }

    virtual ~Instance() {}

    void setModelMatrix(const float* mat) {
        for (int i = 0; i < 16; ++i) modelMatrix[i] = mat[i];
    }
    void setColor(float r, float g, float b, float a = 1.0f) {
        color[0] = r; color[1] = g; color[2] = b; color[3] = a;
    }
    void setEmissive(float r, float g, float b, float a = 1.0f) {
        emissive[0] = r; emissive[1] = g; emissive[2] = b; emissive[3] = a;
    }
};
class CubeInstance : public Instance {
public:
    CubeInstance(Mesh* mesh) : Instance(mesh) {}
    // 可添加Cube专属属性
};
class PanelInstance : public Instance {
public:
    PanelInstance(Mesh* mesh) : Instance(mesh) {}
    // 可添加Panel专属属性
};


}