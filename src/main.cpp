// // src/main.cpp
// #include <SDL.h>
// #ifdef USE_DESKTOP_GL
// #include <glad/glad.h>
// #else
// #include <GLES2/gl2.h>
// #endif
// #include <iostream>
// #include <vector>
// #include <cmath>


// #ifndef M_PI
// #define M_PI 3.14159265358979323846f
// #endif

// // --- Shader source code ---
// // Vertex Shader: Transforms vertex positions with MVP matrix and passes color
// // const char* vertexShaderSrc =
// //     "#version 100\n" // GLSL ES version 1.00 for OpenGL ES 2.0
// //     "attribute vec3 a_position;\n" // Vertex position (x, y, z)
// //     "uniform mat4 u_mvpMatrix;\n" // Model-View-Projection matrix
// //     "uniform vec4 u_color;\n"     // Uniform color for the model
// //     "varying vec4 v_color;\n"     // Pass color to fragment shader
// //     "void main() {\n"
// //     "  gl_Position = u_mvpMatrix * vec4(a_position, 1.0);\n"
// //     "  v_color = u_color;\n" // Assign uniform color to varying
// //     "}\n";



// // // Fragment Shader: Assigns the interpolated color to each fragment (pixel)
// // const char* fragmentShaderSrc =
// //     "#version 100\n" // GLSL ES version 1.00
// //     "precision mediump float;\n" // Required for GLES
// //     "varying vec4 v_color;\n"    // Interpolated color from vertex shader
// //     "void main() {\n"
// //     "  gl_FragColor = v_color;\n"
// //     "}\n";


// const char* vertexShaderSrc =
//     "#version 330 core\n"
//     "layout(location = 0) in vec3 a_position;\n"
//     "uniform mat4 u_mvpMatrix;\n"
//     "uniform vec4 u_color;\n"
//     "out vec4 v_color;\n"
//     "void main() {\n"
//     "  gl_Position = u_mvpMatrix * vec4(a_position, 1.0);\n"
//     "  v_color = u_color;\n"
//     "}\n";

// const char* fragmentShaderSrc =
//     "#version 330 core\n"
//     "in vec4 v_color;\n"
//     "out vec4 FragColor;\n"
//     "void main() {\n"
//     "  FragColor = v_color;\n"
//     "}\n";

// // --- Global variables for OpenGL state ---
// GLuint gProgramID;
// GLint gMVPMatrixUniformLocation;
// GLint gModelColorUniformLocation;
// GLuint gVBO; // Vertex Buffer Object
// GLuint gEBO; // Element Buffer Object (for indices)
// GLsizei gNumIndices; // Number of indices to draw


// void createIdentityMatrix(float matrix[16]) {
//     for (int i = 0; i < 16; ++i) {
//         matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f; // Set diagonal to 1, others to 0
//     }
// }

// void multiplyMatrices(const float a[16], const float b[16], float result[16]) {
//     // OpenGL 列主序矩阵乘法 result = a * b
//     for (int i = 0; i < 4; ++i) {         // 行
//         for (int j = 0; j < 4; ++j) {     // 列
//             result[j * 4 + i] = 0.0f;
//             for (int k = 0; k < 4; ++k) {
//                 result[j * 4 + i] += a[k * 4 + i] * b[j * 4 + k];
//             }
//         }
//     }
// }

// void CreatePrespectiveMatrix(float fovY, float aspect, float nearZ, float farZ, float matrix[16]) {
//     float f = 1.0f / tanf(fovY * 0.5f);
//     float nf = 1.0f / (nearZ - farZ);
    
//     // 初始化为0
//     for (int i = 0; i < 16; i++) {
//         matrix[i] = 0.0f;
//     }
    
//     matrix[0] = f / aspect;
//     matrix[5] = f;
//     matrix[10] = (farZ + nearZ) * nf;
//     matrix[11] = -1.0f;
//     matrix[14] = (2.0f * farZ * nearZ) * nf;
// }
// void createLookAtMatrix(const float eye[3], const float center[3], const float up[3], float M[16]) {
//     // 1. forward = normalize(center - eye)
//     float f[3] = {
//         center[0] - eye[0],
//         center[1] - eye[1],
//         center[2] - eye[2]
//     };
//     float fl = sqrtf(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
//     for (int i = 0; i < 3; ++i) f[i] /= fl;

//     // 2. s = normalize(cross(f, up))
//     float s[3] = {
//         f[1]*up[2] - f[2]*up[1],
//         f[2]*up[0] - f[0]*up[2],
//         f[0]*up[1] - f[1]*up[0]
//     };
//     float sl = sqrtf(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
//     for (int i = 0; i < 3; ++i) s[i] /= sl;

//     // 3. u = cross(s, f)
//     float u[3] = {
//         s[1]*f[2] - s[2]*f[1],
//         s[2]*f[0] - s[0]*f[2],
//         s[0]*f[1] - s[1]*f[0]
//     };

//     // 4. Build column-major matrix
//     M[0] = s[0];   M[4] = s[1];   M[8]  = s[2];   M[12] = - (s[0]*eye[0] + s[1]*eye[1] + s[2]*eye[2]);
//     M[1] = u[0];   M[5] = u[1];   M[9]  = u[2];   M[13] = - (u[0]*eye[0] + u[1]*eye[1] + u[2]*eye[2]);
//     M[2] = -f[0];  M[6] = -f[1];  M[10] = -f[2];  M[14] =   (f[0]*eye[0] + f[1]*eye[1] + f[2]*eye[2]);
//     M[3] = 0.0f;   M[7] = 0.0f;   M[11] = 0.0f;   M[15] = 1.0f;
// }
// void CreateViewMatrix(float matrix[16],float Position[3],float center[3],float up[3])
// {
//     float focus_dir[3] = {
//         center[0] - Position[0],
//         center[1] - Position[1],
//         center[2] - Position[2]
//     };
//     float focus_length = sqrtf(focus_dir[0] * focus_dir[0] +
//                                focus_dir[1] * focus_dir[1] +
//                                focus_dir[2] * focus_dir[2]);

//     focus_dir[0] /= focus_length;
//     focus_dir[1] /= focus_length;
//     focus_dir[2] /= focus_length;

//     // calculate right vector
//     float right_dir[3] = {
//         focus_dir[1] * up[2] - focus_dir[2] * up[1],
//         focus_dir[2] * up[0] - focus_dir[0] * up[2],
//         focus_dir[0] * up[1] - focus_dir[1] * up[0]
//     };
//     float right_length = sqrtf(right_dir[0] * right_dir[0] +
//                                right_dir[1] * right_dir[1] +
//                                right_dir[2] * right_dir[2]);
//     right_dir[0] /= right_length;
//     right_dir[1] /= right_length;
//     right_dir[2] /= right_length;
//     // calculate up vector
//     float up_dir[3] = {
//         right_dir[1] * focus_dir[2] - right_dir[2] * focus_dir[1],
//         right_dir[2] * focus_dir[0] - right_dir[0] * focus_dir[2],
//         right_dir[0] * focus_dir[1] - right_dir[1] * focus_dir[0]
//     };

//     matrix[0] = right_dir[0]; matrix[1] = up_dir[0]; matrix[2] = -focus_dir[0]; matrix[3] = 0;
//     matrix[4] = right_dir[1]; matrix[5] = up_dir[1]; matrix[6] = -focus_dir[1]; matrix[7] = 0;
//     matrix[8] = right_dir[2]; matrix[9] = up_dir[2]; matrix[10] = -focus_dir[2]; matrix[11] = 0;
//     matrix[12] = -(right_dir[0] * Position[0] + right_dir[1] * Position[1] + right_dir[2] * Position[2]);
//     matrix[13] = -(up_dir[0] * Position[0] + up_dir[1] * Position[1] + up_dir[2] * Position[2]);
//     matrix[14] = -(focus_dir[0] * Position[0] + focus_dir[1] * Position[1] + focus_dir[2] * Position[2]);
//     matrix[15] = 1.0f; // Homogeneous coordinate
// }
// void createModelMatrix(float matrix[16], float rotate_y_deg, float rotate_x_deg) {
//     createIdentityMatrix(matrix);

//     float angle_rad_y = rotate_y_deg * (M_PI / 180.0f);
//     float c_y = cos(angle_rad_y);
//     float s_y = sin(angle_rad_y);
    
//     float rotY_mat[16]; 
//     createIdentityMatrix(rotY_mat);
//     rotY_mat[0] = c_y; 
//     rotY_mat[2] = s_y;
//     rotY_mat[8] = -s_y; 
//     rotY_mat[10] = c_y;

//     float angle_rad_x = rotate_x_deg * (M_PI / 180.0f);
//     float c_x = cos(angle_rad_x);
//     float s_x = sin(angle_rad_x);
    
//     float rotX_mat[16]; 
//     createIdentityMatrix(rotX_mat);
//     rotX_mat[5] = c_x; 
//     rotX_mat[6] = -s_x;
//     rotX_mat[9] = s_x; 
//     rotX_mat[10] = c_x;

//     float temp_rot[16];
//     multiplyMatrices(rotY_mat, rotX_mat, temp_rot);
    
//     // 复制结果到返回矩阵
//     for (int i = 0; i < 16; i++) {
//         matrix[i] = temp_rot[i];
//     }
// }

// GLuint compileShader(GLenum type, const char* source) {
//     GLuint shader = glCreateShader(type);
//     glShaderSource(shader, 1, &source, nullptr);
//     glCompileShader(shader);

//     GLint success;
//     glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
//     if (!success) {
//         GLint logLength;
//         glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
//         char* log = new char[logLength];
//         glGetShaderInfoLog(shader, logLength, nullptr, log);
//         std::cerr << "Shader compilation error: " << log << std::endl;
//         delete[] log;
//         glDeleteShader(shader);
//         return 0;
//     }
//     return shader;
// }

// GLuint createShaderProgram(){
//     GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
//     if (!vertexShader) return 0;
//     GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
//     if (!fragmentShader) {
//         glDeleteShader(vertexShader);
//         return 0;
//     }

//     GLuint program = glCreateProgram();
//     glAttachShader(program, vertexShader);
//     glAttachShader(program, fragmentShader);
//     glLinkProgram(program);

//     GLint success;
//     glGetProgramiv(program, GL_LINK_STATUS, &success);
//         if (!success) {
//         char infoLog[512];
//         glGetProgramInfoLog(program, 512, NULL, infoLog);
//         std::cerr << "Program linking error:\n" << infoLog << std::endl;
//         glDeleteProgram(program);
//         program = 0;
//     }
    
//     glDeleteShader(vertexShader);
//     glDeleteShader(fragmentShader);
//     return program;
// }


// void createCube(std::vector<float>& vertices,
//                 std::vector<unsigned short>& indices){
//     vertices = {
//         // 前面 (Z = 0.5)
//         -0.5f, -0.5f,  0.5f,
//          0.5f, -0.5f,  0.5f,
//          0.5f,  0.5f,  0.5f,
//         -0.5f,  0.5f,  0.5f,
        
//         // 后面 (Z = -0.5)
//         -0.5f, -0.5f, -0.5f,
//          0.5f, -0.5f, -0.5f,
//          0.5f,  0.5f, -0.5f,
//         -0.5f,  0.5f, -0.5f,
        
//         // 左面 (X = -0.5)
//         -0.5f, -0.5f, -0.5f,
//         -0.5f, -0.5f,  0.5f,
//         -0.5f,  0.5f,  0.5f,
//         -0.5f,  0.5f, -0.5f,
        
//         // 右面 (X = 0.5)
//          0.5f, -0.5f, -0.5f,
//          0.5f, -0.5f,  0.5f,
//          0.5f,  0.5f,  0.5f,
//          0.5f,  0.5f, -0.5f,
        
//         // 顶面 (Y = 0.5)
//         -0.5f,  0.5f, -0.5f,
//         -0.5f,  0.5f,  0.5f,
//          0.5f,  0.5f,  0.5f,
//          0.5f,  0.5f, -0.5f,
        
//         // 底面 (Y = -0.5)
//         -0.5f, -0.5f, -0.5f,
//         -0.5f, -0.5f,  0.5f,
//          0.5f, -0.5f,  0.5f,
//          0.5f, -0.5f, -0.5f
//     };

//     indices = {
//         // 前面
//         0, 1, 2,  0, 2, 3,
//         // 后面
//         4, 5, 6,  4, 6, 7,
//         // 左面
//         8, 9, 10, 8, 10, 11,
//         // 右面
//         12, 13, 14, 12, 14, 15,
//         // 顶面
//         16, 17, 18, 16, 18, 19,
//         // 底面
//         20, 21, 22, 20, 22, 23
//     };
//                 }
// void checkGLError(const char* operation) {
//     GLenum error;
//     while ((error = glGetError()) != GL_NO_ERROR) {
//         std::cout << "OpenGL error after " << operation << ": " << error << std::endl;
//     }
// }
// int main(int argc, char* argv[]) {
//     if (SDL_Init(SDL_INIT_VIDEO) != 0) {
//         std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
//         return -1;
//     }

//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
//     SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
//     SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

//     SDL_Window* window = SDL_CreateWindow("3D Cube",
//         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
//         800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
//     if (!window) {
//         std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
//         SDL_Quit();
//         return -1;
//     }

//     SDL_GLContext context = SDL_GL_CreateContext(window);
//     if (!context) {
//         std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
//         SDL_DestroyWindow(window);
//         SDL_Quit();
//         return -1;
//     }

// #ifdef USE_DESKTOP_GL
//     if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
//         std::cerr << "Failed to initialize GLAD" << std::endl;
//         return -1;
//     }
// #endif

//     gProgramID = createShaderProgram();
//     if (gProgramID == 0) {
//         std::cerr << "Failed to create shader program" << std::endl;
//         SDL_GL_DeleteContext(context);
//         SDL_DestroyWindow(window);
//         SDL_Quit();
//         return -1;
//     }
//     glUseProgram(gProgramID);
//     gMVPMatrixUniformLocation = glGetUniformLocation(gProgramID, "u_mvpMatrix");
//     gModelColorUniformLocation = glGetUniformLocation(gProgramID, "u_color");

//     std::vector<float> vertices;
//     std::vector<unsigned short> indices;
//     createCube(vertices, indices);
//     // std::vector<float> vertices = {
//     // // 简单三角形顶点 (x, y, z)
//     // -0.5f, -0.5f, 0.0f,
//     //  0.5f, -0.5f, 0.0f,
//     //  0.0f,  0.5f, 0.0f
//     // };
//     // std::vector<unsigned short> indices = {
//     //     0, 1, 2  // 一个三角形
//     // };

//     gNumIndices =  indices.size();

//     // ...existing code...
//     GLuint vao;
//     glGenVertexArrays(1, &vao);
//     glBindVertexArray(vao);

//     //vbo
//     glGenBuffers(1, &gVBO);
//     glBindBuffer(GL_ARRAY_BUFFER, gVBO);
//     glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

//     //ebo
//     glGenBuffers(1, &gEBO);
//     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
//     glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

//     GLint posAttrib = glGetAttribLocation(gProgramID, "a_position");
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//     // glEnable(GL_DEPTH_TEST);
//     glDisable(GL_CULL_FACE);


//     // Test: clear screen with changing color
//     bool running = true;
//     SDL_Event event;
//     float t = 0.0f;
//     float rotationY = 0.0f;
//     float rotationX = 0.0f;
//     float aspect = 800.0f / 600.0f;


//     while (running) {
//         while (SDL_PollEvent(&event)) {
//             if (event.type == SDL_QUIT) running = false;
//         }
//         t += 0.01f;
//         float r = (sin(0.1f*t) + 1.0f) * 0.5f;
//         float g = (cos(0.1f*t) + 1.0f) * 0.5f;
//         glViewport(0, 0, 800, 600);
//         glClearColor(r, g, 0.3f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);

//         rotationX += 0.5f; // Rotate around X-axis
//         rotationY += 0.5f; // Rotate around Y-axis

//         if (rotationX >= 360.0f) rotationX -= 360.0f;
//         if (rotationY >= 360.0f) rotationY -= 360.0f;
// // rotationX = 0;
// // rotationY = 0;

//         float projectionMatrix[16],modelMatrix[16],viewMatrix[16],mvMatrix[16],mvpMatrix[16];
//         createModelMatrix(modelMatrix, rotationY, rotationX);
//         CreatePrespectiveMatrix(M_PI / 4.0f, aspect, 0.1f, 100.0f, projectionMatrix);
//         float cameraPosition[3] = { 0.0f, 0.0f, 5.0f };
//         float cameraCenter[3] = { 0.0f, 0.0f, 0.0f };
//         float cameraUp[3] = { 0.0f, 1.0f, 0.0f };
//         createLookAtMatrix(cameraPosition, cameraCenter, cameraUp, viewMatrix);
//         multiplyMatrices(viewMatrix, modelMatrix,mvMatrix);
//         multiplyMatrices(projectionMatrix, mvMatrix, mvpMatrix);


//         //createIdentityMatrix(mvpMatrix);

//         glUniformMatrix4fv(gMVPMatrixUniformLocation, 1, GL_FALSE, mvpMatrix);
//         glUniform4f(gModelColorUniformLocation, 0.7f, 0.7f, 0.3f, 1.0f);

//         glDrawElements(GL_TRIANGLES, gNumIndices, GL_UNSIGNED_SHORT, 0);

//         // 打印顶点数和索引数
//         std::cout << "Vertices: " << vertices.size()/3 << " Indices: " << indices.size() << std::endl;


//         // 检查MVP矩阵
//         std::cout << "MVP Matrix:" << std::endl;
//         for(int i = 0; i < 4; i++) {
//             for(int j = 0; j < 4; j++) {
//                 std::cout << mvpMatrix[i*4 + j] << " ";
//             }
//             std::cout << std::endl;
//         }

//         SDL_GL_SwapWindow(window);
//     }

//     SDL_GL_DeleteContext(context);
//     SDL_DestroyWindow(window);
//     SDL_Quit();
//     return 0;
// }


#include "Core/Platform.h"
#include "Core/Mesh.h"
#include "Core/CubeMesh.h"
#include "Core/Renderer.h"
#include "Math/MathTool.h"
#include <cmath>
#include <vector>
#include <string>


int main() {
    using namespace Platform;
    int window_width = 800;
    int window_height = 600;
    if (!initWindow(window_width, window_height)) return -1;

    Core::Renderer renderer;
    if (!renderer.init()) return -1;

    bool running = true;
    float aspect = 800.0f / 600.0f;
    float proj[16], view[16], model[16], tmp[16], mvp[16];
    while (running) {
        pollEvents(running);

        static float t = 0.0f;
        t += 0.01f;
        // 模型矩阵：旋转
        createModelMatrix(model, t * 50.f, t * 10.f);
        // 投影矩阵：透视
        createPerspectiveMatrix(M_PI / 4.0f, aspect, 0.1f, 100.0f, proj);
        // 视图矩阵：摄像机
        float eye[3] = {0.0f, 0.0f, 5.0f};
        float center[3] = {0.0f, 0.0f, 0.0f};
        float up[3] = {0.0f, 1.0f, 0.0f};
        createLookAtMatrix(eye, center, up, view);

        // MVP = proj * view * model
        multiplyMatrices(view, model, tmp);
        multiplyMatrices(proj, tmp, mvp);

        // 渲染
        renderer.resize(800, 600);
        renderer.render(mvp, model);
        swapBuffers();
    }

    renderer.shutdown();
    shutdown();
    return 0;
}