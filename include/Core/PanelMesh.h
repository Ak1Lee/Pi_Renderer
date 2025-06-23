#pragma once
#include "Core/Mesh.h" // Inherit from base Mesh class

namespace Core {
class PanelMesh : public Mesh {

public:
    PanelMesh(); // Constructor will define panel's specific data and call Mesh::setupData
    ~PanelMesh();
    void draw();
private:
    GLuint vbo = 0, ebo = 0, vao = 0;
    GLsizei indexCount = 0;
};



}
