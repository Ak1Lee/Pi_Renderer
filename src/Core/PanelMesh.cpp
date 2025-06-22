#include "Core/PanelMesh.h"
#include <vector> //

namespace Core {

PanelMesh::PanelMesh() {
    // Define vertices and indices for a simple panel (2 triangles forming a rectangle)
    const float verts[] = {
        // Vertex positions (x, y, z) Normal Direction (nx, ny, nz)
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // Bottom-left
         0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // Bottom-right
         0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // Top-right
        -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f  // Top-left
    };
    const unsigned short idxs[] = {
        0, 1, 2, // First triangle
        2, 3, 0  // Second triangle
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

    // Define vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // Normal
    glEnableVertexAttribArray(1);

#ifdef USE_DESKTOP_GL
    glBindVertexArray(0);
#endif

}

PanelMesh::~PanelMesh() {

#ifdef USE_DESKTOP_GL
    if (vao) glDeleteVertexArrays(1, &vao);
#endif
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (ebo) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }

}

void PanelMesh::draw() {
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