#pragma once

#include "Core/Mesh.h"

namespace Core {

class SphereMesh : public Mesh {
public:
    SphereMesh(int sectorCount = 32, int stackCount = 16);
    ~SphereMesh();
    void draw();
private:
    GLuint vbo = 0, ebo = 0, vao = 0;
    GLsizei indexCount = 0;
};

} // namespace Core