#include "Core/Sphere.h"
#include <vector>
#include <cmath>

namespace Core {

SphereMesh::SphereMesh(int sectorCount, int stackCount) {
    std::vector<float> verts;
    std::vector<unsigned short> idxs;
    const float PI = 3.1415926f;

    for (int i = 0; i <= stackCount; ++i) {
        float stackAngle = PI / 2 - i * PI / stackCount;
        float xy = cosf(stackAngle);
        float z = sinf(stackAngle);
        for (int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * 2 * PI / sectorCount;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            verts.push_back(x * 0.5f); // 位置
            verts.push_back(y * 0.5f);
            verts.push_back(z * 0.5f);
            verts.push_back(x); // 法线
            verts.push_back(y);
            verts.push_back(z);
        }
    }
    for (int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                idxs.push_back(k1);
                idxs.push_back(k2);
                idxs.push_back(k1 + 1);
            }
            if (i != (stackCount - 1)) {
                idxs.push_back(k1 + 1);
                idxs.push_back(k2);
                idxs.push_back(k2 + 1);
            }
        }
    }
    indexCount = idxs.size();

#ifdef USE_DESKTOP_GL
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#endif

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxs.size() * sizeof(unsigned short), idxs.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

#ifdef USE_DESKTOP_GL
    glBindVertexArray(0);
#endif
}

SphereMesh::~SphereMesh() {
#ifdef USE_DESKTOP_GL
    if (vao) glDeleteVertexArrays(1, &vao);
#endif
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
}

void SphereMesh::draw() {
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