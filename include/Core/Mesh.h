#pragma once

#ifdef USE_DESKTOP_GL
#include <glad/glad.h>
#else

#include <GLES2/gl2.h>

#endif
#include <vector>
#include <cstddef> // For std::size_t

namespace Core
{
    class Mesh
    {
    public:
        Mesh();

        // Function for drawing the mesh
        void bind();
        void unbind();
        // Getters for drawing information
        GLsizei getIndexCount() const { return indexCount; }
        GLsizei getVertexStride() const { return vertexStride; } // Stride in bytes (e.g., 6 floats * sizeof(float))
        GLsizei getNumFloatsPerVertex() const { return numFloatsPerVertex; } // Number of floats per vertex (e.g., 3 for pos, 6 for pos+normal)


        virtual ~Mesh();

    protected:
        // This method is called by derived classes to set their specific data
        // singleVertexAttributeCount: e.g., 3 for pos only, 6 for pos+normal
        void setupData(const std::vector<float>& vertices, const std::vector<unsigned short>& indices, GLsizei singleVertexAttributeCount);

        GLuint vbo;
        GLuint ebo;
        GLsizei indexCount;
        GLsizei vertexStride; // Stride in bytes (e.g., 6 floats * sizeof(float) for pos+normal)
        GLsizei numFloatsPerVertex; // Number of floats per vertex (e.g., 3 for pos, 6 for pos+normal)
        #ifdef USE_DESKTOP_GL
            GLuint vao; // Vertex Array Object ID (Only for desktop GL, as GLES 2.0 doesn't have VAOs)
        #endif
    };
} // namespace core
