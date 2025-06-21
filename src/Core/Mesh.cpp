#include "Core/Mesh.h"
#include <iostream>

namespace Core {
Mesh::Mesh() : vbo(0), ebo(0), indexCount(0), vertexStride(0), numFloatsPerVertex(0) {
#ifdef USE_DESKTOP_GL
    vao = 0; // Initialize VAO ID for desktop GL
#endif
}

Mesh::~Mesh() {
    // Delete OpenGL buffer resources when Mesh object is destroyed
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ebo != 0) glDeleteBuffers(1, &ebo);
#ifdef USE_DESKTOP_GL
    if (vao != 0) glDeleteVertexArrays(1, &vao); // Delete VAO for desktop GL
#endif
};

// Protected method for derived classes to set up their specific vertex and index data
void Mesh::setupData(const std::vector<float>& vertices, const std::vector<unsigned short>& indices, GLsizei singleVertexAttributeCount) {
    // Delete any existing buffers before creating new ones
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ebo != 0) glDeleteBuffers(1, &ebo);
#ifdef USE_DESKTOP_GL
    if (vao != 0) glDeleteVertexArrays(1, &vao);
#endif

    indexCount = static_cast<GLsizei>(indices.size());
    numFloatsPerVertex = singleVertexAttributeCount; // e.g., 6 for (pos + normal)
    vertexStride = numFloatsPerVertex * sizeof(float); // Total size of one vertex in bytes

#ifdef USE_DESKTOP_GL
    glGenVertexArrays(1, &vao); // Generate VAO for desktop GL
    glBindVertexArray(vao);     // Bind VAO to capture VBO and attribute configurations
#endif

    // Generate and bind Vertex Buffer Object (VBO)
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Generate and bind Element Buffer Object (EBO)
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

    // Set vertex attribute pointers ONCE if VAO is used (desktop GL)
#ifdef USE_DESKTOP_GL
    // Assuming position is at location 0 (3 floats)
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)0);

    // Assuming normal is at location 1 (3 floats, after position)
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)(3 * sizeof(float)));

    glBindVertexArray(0); // Unbind VAO after configuration
#endif
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);       // Unbind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind EBO
}

// Binds the Mesh's buffers (or VAO for desktop GL) for drawing
void Mesh::bind() {
#ifdef USE_DESKTOP_GL
    glBindVertexArray(vao); // For desktop GL, just bind the VAO
#else
    // For OpenGL ES 2.0, bind VBO and EBO manually
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
#endif
}

// Unbinds the Mesh's buffers (or VAO for desktop GL)
void Mesh::unbind() {
#ifdef USE_DESKTOP_GL
    glBindVertexArray(0); // Unbind VAO for desktop GL
#else
    // For OpenGL ES 2.0, unbind VBO and EBO manually
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
}

}