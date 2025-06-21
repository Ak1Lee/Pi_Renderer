#pragma once

#include "Core/Mesh.h" // Inherit from base Mesh class

namespace Core {

class CubeMesh{
public:
    CubeMesh(); // Constructor will define cube's specific data and call Mesh::setupData
    ~CubeMesh();
    void draw();
private:
    GLuint vbo = 0, ebo = 0, vao = 0;
    GLsizei indexCount = 0;
};

} // namespace Core
