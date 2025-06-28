#include "Math/MathTool.h"

void createIdentityMatrix(float matrix[16]) {
    for (int i = 0; i < 16; ++i) {
        matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f; // Set diagonal to 1, others to 0
    }
}

void multiplyMatrices(const float a[16], const float b[16], float result[16]) {
    // OpenGL 列主序矩阵乘法 result = a * b
    for (int i = 0; i < 4; ++i) {         // 行
        for (int j = 0; j < 4; ++j) {     // 列
            result[j * 4 + i] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result[j * 4 + i] += a[k * 4 + i] * b[j * 4 + k];
            }
        }
    }
}

void createPerspectiveMatrix(float fovY, float aspect, float nearZ, float farZ, float matrix[16]) {
    float f = 1.0f / tanf(fovY * 0.5f);
    float nf = 1.0f / (nearZ - farZ);
    
    // 初始化为0
    for (int i = 0; i < 16; i++) {
        matrix[i] = 0.0f;
    }
    
    matrix[0] = f / aspect;
    matrix[5] = f;
    matrix[10] = (farZ + nearZ) * nf;
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * farZ * nearZ) * nf;
}
void createLookAtMatrix(const float eye[3], const float center[3], const float up[3], float M[16]) {
    // 1. forward = normalize(center - eye)
    float f[3] = {
        center[0] - eye[0],
        center[1] - eye[1],
        center[2] - eye[2]
    };
    float fl = sqrtf(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
    for (int i = 0; i < 3; ++i) f[i] /= fl;

    // 2. s = normalize(cross(f, up))
    float s[3] = {
        f[1]*up[2] - f[2]*up[1],
        f[2]*up[0] - f[0]*up[2],
        f[0]*up[1] - f[1]*up[0]
    };
    float sl = sqrtf(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
    for (int i = 0; i < 3; ++i) s[i] /= sl;

    // 3. u = cross(s, f)
    float u[3] = {
        s[1]*f[2] - s[2]*f[1],
        s[2]*f[0] - s[0]*f[2],
        s[0]*f[1] - s[1]*f[0]
    };

    // 4. Build column-major matrix
    M[0] = s[0];   M[4] = s[1];   M[8]  = s[2];   M[12] = - (s[0]*eye[0] + s[1]*eye[1] + s[2]*eye[2]);
    M[1] = u[0];   M[5] = u[1];   M[9]  = u[2];   M[13] = - (u[0]*eye[0] + u[1]*eye[1] + u[2]*eye[2]);
    M[2] = -f[0];  M[6] = -f[1];  M[10] = -f[2];  M[14] =   (f[0]*eye[0] + f[1]*eye[1] + f[2]*eye[2]);
    M[3] = 0.0f;   M[7] = 0.0f;   M[11] = 0.0f;   M[15] = 1.0f;
}
void CreateViewMatrix(float matrix[16],float Position[3],float center[3],float up[3])
{
    float focus_dir[3] = {
        center[0] - Position[0],
        center[1] - Position[1],
        center[2] - Position[2]
    };
    float focus_length = sqrtf(focus_dir[0] * focus_dir[0] +
                               focus_dir[1] * focus_dir[1] +
                               focus_dir[2] * focus_dir[2]);

    focus_dir[0] /= focus_length;
    focus_dir[1] /= focus_length;
    focus_dir[2] /= focus_length;

    // calculate right vector
    float right_dir[3] = {
        focus_dir[1] * up[2] - focus_dir[2] * up[1],
        focus_dir[2] * up[0] - focus_dir[0] * up[2],
        focus_dir[0] * up[1] - focus_dir[1] * up[0]
    };
    float right_length = sqrtf(right_dir[0] * right_dir[0] +
                               right_dir[1] * right_dir[1] +
                               right_dir[2] * right_dir[2]);
    right_dir[0] /= right_length;
    right_dir[1] /= right_length;
    right_dir[2] /= right_length;
    // calculate up vector
    float up_dir[3] = {
        right_dir[1] * focus_dir[2] - right_dir[2] * focus_dir[1],
        right_dir[2] * focus_dir[0] - right_dir[0] * focus_dir[2],
        right_dir[0] * focus_dir[1] - right_dir[1] * focus_dir[0]
    };

    matrix[0] = right_dir[0]; matrix[1] = up_dir[0]; matrix[2] = -focus_dir[0]; matrix[3] = 0;
    matrix[4] = right_dir[1]; matrix[5] = up_dir[1]; matrix[6] = -focus_dir[1]; matrix[7] = 0;
    matrix[8] = right_dir[2]; matrix[9] = up_dir[2]; matrix[10] = -focus_dir[2]; matrix[11] = 0;
    matrix[12] = -(right_dir[0] * Position[0] + right_dir[1] * Position[1] + right_dir[2] * Position[2]);
    matrix[13] = -(up_dir[0] * Position[0] + up_dir[1] * Position[1] + up_dir[2] * Position[2]);
    matrix[14] = -(focus_dir[0] * Position[0] + focus_dir[1] * Position[1] + focus_dir[2] * Position[2]);
    matrix[15] = 1.0f; // Homogeneous coordinate
}
void createModelMatrix(float matrix[16], float rotate_y_deg, float rotate_x_deg) {
    createIdentityMatrix(matrix);

    float angle_rad_y = rotate_y_deg * (M_PI / 180.0f);
    float c_y = cos(angle_rad_y);
    float s_y = sin(angle_rad_y);
    
    float rotY_mat[16]; 
    createIdentityMatrix(rotY_mat);
    rotY_mat[0] = c_y; 
    rotY_mat[2] = s_y;
    rotY_mat[8] = -s_y; 
    rotY_mat[10] = c_y;

    float angle_rad_x = rotate_x_deg * (M_PI / 180.0f);
    float c_x = cos(angle_rad_x);
    float s_x = sin(angle_rad_x);
    
    float rotX_mat[16]; 
    createIdentityMatrix(rotX_mat);
    rotX_mat[5] = c_x; 
    rotX_mat[6] = -s_x;
    rotX_mat[9] = s_x; 
    rotX_mat[10] = c_x;

    float temp_rot[16];
    multiplyMatrices(rotY_mat, rotX_mat, temp_rot);
    
    // 复制结果到返回矩阵
    for (int i = 0; i < 16; i++) {
        matrix[i] = temp_rot[i];
    }
}
void createModelMatrix1(float matrix[16], float offset[3],float rotate_deg[3], float scale[3]) {

    createIdentityMatrix(matrix);
    float angle_rad_y = rotate_deg[1] * (M_PI / 180.0f);
    float c_y = cos(angle_rad_y);
    float s_y = sin(angle_rad_y);
    
    float rotY_mat[16]; 
    createIdentityMatrix(rotY_mat);
    rotY_mat[0] = c_y; 
    rotY_mat[2] = s_y;
    rotY_mat[8] = -s_y; 
    rotY_mat[10] = c_y;

    float angle_rad_x = rotate_deg[2] * (M_PI / 180.0f);
    float c_x = cos(angle_rad_x);
    float s_x = sin(angle_rad_x);
    
    float rotX_mat[16]; 
    createIdentityMatrix(rotX_mat);
    rotX_mat[5] = c_x; 
    rotX_mat[6] = -s_x;
    rotX_mat[9] = s_x; 
    rotX_mat[10] = c_x;

    float temp_rot[16];
    multiplyMatrices(rotY_mat, rotX_mat, temp_rot);
        // 复制结果到返回矩阵
    for (int i = 0; i < 16; i++) {
        matrix[i] = temp_rot[i];
    }

    // 平移
    matrix[12] = offset[0];
    matrix[13] = offset[1];
    matrix[14] = offset[2];
    // 缩放
    matrix[0] *= scale[0];
    matrix[5] *= scale[1];
    matrix[10] *= scale[2];
    // 旋转


}

// 4x4矩阵求逆函数（使用Gauss-Jordan消元法）
bool invertMatrix(const float m[16], float inv[16]) {
    // 创建增广矩阵 [M | I]
    float mat[4][8];
    
    // 初始化增广矩阵
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mat[i][j] = m[i * 4 + j];           // 原矩阵
            mat[i][j + 4] = (i == j) ? 1.0f : 0.0f; // 单位矩阵
        }
    }
    
    // Gauss-Jordan消元
    for (int i = 0; i < 4; i++) {
        // 寻找主元
        int maxRow = i;
        for (int k = i + 1; k < 4; k++) {
            if (fabs(mat[k][i]) > fabs(mat[maxRow][i])) {
                maxRow = k;
            }
        }
        
        // 交换行
        if (maxRow != i) {
            for (int k = 0; k < 8; k++) {
                float temp = mat[i][k];
                mat[i][k] = mat[maxRow][k];
                mat[maxRow][k] = temp;
            }
        }
        
        // 检查是否可逆
        if (fabs(mat[i][i]) < 1e-6f) {
            return false; // 矩阵不可逆
        }
        
        // 归一化主元行
        float pivot = mat[i][i];
        for (int k = 0; k < 8; k++) {
            mat[i][k] /= pivot;
        }
        
        // 消元
        for (int k = 0; k < 4; k++) {
            if (k != i) {
                float factor = mat[k][i];
                for (int j = 0; j < 8; j++) {
                    mat[k][j] -= factor * mat[i][j];
                }
            }
        }
    }
    
    // 提取逆矩阵
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            inv[i * 4 + j] = mat[i][j + 4];
        }
    }
    
    return true;
}
void createOrthographicMatrix(float left, float right, float bottom, float top, float nearZ, float farZ, float matrix[16]) {
    // 初始化为0
    for (int i = 0; i < 16; i++) {
        matrix[i] = 0.0f;
    }
    
    // 正交投影矩阵
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -2.0f / (farZ - nearZ);
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(farZ + nearZ) / (farZ - nearZ);
    matrix[15] = 1.0f;
}