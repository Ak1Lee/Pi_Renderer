#pragma once
#include <cmath>


#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 声明自写矩阵和乘法函数：
void createIdentityMatrix(float m[16]);
void createPerspectiveMatrix(float fovY, float aspect, float nearZ, float farZ, float m[16]);
void createOrthographicMatrix(float left, float right, float bottom, float top, float nearZ, float farZ, float m[16]);
void createLookAtMatrix(const float eye[3], const float center[3], const float up[3], float m[16]);
void createModelMatrix(float m[16], float ry, float rx);
void createModelMatrix1(float m[16], float offset[3], float rotate_deg[3], float scale[3]);
void multiplyMatrices(const float a[16], const float b[16], float result[16]);
bool invertMatrix(const float m[16], float inv[16]); // 4x4矩阵求逆
