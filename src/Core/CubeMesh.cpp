#include "Core/CubeMesh.h"
#include <vector> // Required for std::vector

namespace Core {

CubeMesh::CubeMesh() {
    // Vertex format: (px, py, pz, nx, ny, nz) = 6 floats per vertex
    // Vertices are explicitly duplicated for each face to ensure correct normal per face.
    const float verts[] = {
        // Front face: +Z face (normal: 0,0,1)
        -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f, // 0
         0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f, // 1
         0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f, // 2
        -0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f, // 3
        // Back face: -Z face (normal: 0,0,-1)
        -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f, // 4
         0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f, // 5
         0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f, // 6
        -0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f, // 7
        // Left face: -X face (normal: -1,0,0)
        -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f, // 8 (same position as 4, but different normal)
        -0.5f,-0.5f, 0.5f,  -1.0f, 0.0f, 0.0f, // 9 (same position as 0)
        -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f, // 10 (same position as 3)
        -0.5f, 0.5f,-0.5f,  -1.0f, 0.0f, 0.0f, // 11 (same position as 7)
        // Right face: +X face (normal: 1,0,0)
         0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f, // 12 (same position as 5)
         0.5f,-0.5f, 0.5f,   1.0f, 0.0f, 0.0f, // 13 (same position as 1)
         0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f, // 14 (same position as 2)
         0.5f, 0.5f,-0.5f,   1.0f, 0.0f, 0.0f, // 15 (same position as 6)
        // Top face: +Y face (normal: 0,1,0)
        -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f, // 16 (same position as 7)
        -0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f, // 17 (same position as 3)
         0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f, // 18 (same position as 2)
         0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f, // 19 (same position as 6)
        // Bottom face: -Y face (normal: 0,-1,0)
        -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f, // 20 (same position as 4)
        -0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f, // 21 (same position as 0)
         0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f, // 22 (same position as 1)
         0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f  // 23 (same position as 5)
    };
    const unsigned short idxs[] = {
        0,1,2, 0,2,3,       // Front
        4,5,6, 4,6,7,       // Back
        8,9,10,8,10,11,     // Left
        12,13,14,12,14,15,  // Right
        16,17,18,16,18,19,  // Top
        20,21,22,20,22,23   // Bottom
    };
    
    indexCount = sizeof(idxs) / sizeof(idxs[0]);
#ifdef USE_DESKTOP_GL
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#endif

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // 位置
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // 法线
    glEnableVertexAttribArray(1);

#ifdef USE_DESKTOP_GL
    glBindVertexArray(0);
#endif

}

CubeMesh::~CubeMesh() {
#ifdef USE_DESKTOP_GL
    if (vao) glDeleteVertexArrays(1, &vao);
#endif
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
}

void CubeMesh::draw() {
#ifdef USE_DESKTOP_GL
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
#else
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
#endif
}

} // namespace Core

