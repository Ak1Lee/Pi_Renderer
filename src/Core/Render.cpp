#include "Core/Renderer.h"
#include "Math/MathTool.h"
#include <SDL.h>
#include <glad/glad.h>
#include <iostream>

using namespace Core;

static const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
uniform mat4 u_mvpMatrix;
uniform mat4 u_modelMatrix;
out vec3 v_normal;
void main() {
    gl_Position = u_mvpMatrix * vec4(a_position, 1.0);
    v_normal = mat3(transpose(inverse(u_modelMatrix))) * a_normal;
    //v_normal = a_normal;
    
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 v_normal;
out vec4 FragColor;
uniform vec4 u_color;
uniform vec3 u_lightDir;
void main() {
    float NdotL = dot(normalize(v_normal), normalize(-u_lightDir));
    float diff = NdotL * 0.5 + 0.5; // 半兰伯特公式
    diff = clamp(diff, 0.0, 1.0);
    vec3 diffuse = diff * u_color.rgb;
    FragColor = vec4(diffuse, u_color.a);
    //FragColor = vec4(1,0,0,1);
    //FragColor = vec4(normalize(v_normal) * 0.5 + 0.5, 1.0);
}
)";

bool Renderer::compileShaders() {
    auto compile = [&](unsigned int type, const char* src) {
        unsigned int sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        int ok;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512]; glGetShaderInfoLog(sh, 512, nullptr, buf);
            std::cerr << "Shader error: " << buf << std::endl;
            return 0u;
        }
        return sh;
    };
    unsigned int vs = compile(GL_VERTEX_SHADER, vertexShaderSrc);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    if (!vs || !fs) return false;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int ok; glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
    if (!ok) { char buf[512]; glGetProgramInfoLog(shaderProgram,512,nullptr,buf);
        std::cerr<<"Link error:"<<buf<<std::endl; return false;
    }
    return true;
}

bool Renderer::init() {
    if (!compileShaders()) return false;
        // 立方体顶点与索引
    // 定义立方体顶点和索引（与原 main.cpp 中 createCube 保持一致）
    // 顶点数据：每个顶点6个float（x, y, z, nx, ny, nz）
    const float verts[] = {
    // 前面
    -0.5f,-0.5f, 0.5f,  0,0,1,  0.5f,-0.5f, 0.5f,  0,0,1,  0.5f, 0.5f, 0.5f,  0,0,1,  -0.5f, 0.5f, 0.5f,  0,0,1,
    // 后面
    -0.5f,-0.5f,-0.5f,  0,0,-1,  0.5f,-0.5f,-0.5f,  0,0,-1,  0.5f, 0.5f,-0.5f,  0,0,-1,  -0.5f, 0.5f,-0.5f,  0,0,-1,
    // 左面
    -0.5f,-0.5f,-0.5f,  -1,0,0,  -0.5f,-0.5f, 0.5f,  -1,0,0,  -0.5f, 0.5f, 0.5f,  -1,0,0,  -0.5f, 0.5f,-0.5f,  -1,0,0,
    // 右面
     0.5f,-0.5f,-0.5f,  1,0,0,  0.5f,-0.5f, 0.5f,  1,0,0,  0.5f, 0.5f, 0.5f,  1,0,0,  0.5f, 0.5f,-0.5f,  1,0,0,
    // 顶面
    -0.5f, 0.5f,-0.5f,  0,1,0,  -0.5f, 0.5f, 0.5f,  0,1,0,  0.5f, 0.5f, 0.5f,  0,1,0,  0.5f, 0.5f,-0.5f,  0,1,0,
    // 底面
    -0.5f,-0.5f,-0.5f,  0,-1,0,  -0.5f,-0.5f, 0.5f,  0,-1,0,  0.5f,-0.5f, 0.5f,  0,-1,0,  0.5f,-0.5f,-0.5f,  0,-1,0
    };
    const unsigned short idxs[] = {
        0,1,2, 0,2,3,       // 前面
        4,5,6, 4,6,7,       // 后面
        8,9,10,8,10,11,     // 左面
        12,13,14,12,14,15,  // 右面
        16,17,18,16,18,19,  // 顶面
        20,21,22,20,22,23   // 底面
    };
    indexCount = sizeof(idxs)/sizeof(idxs[0]);
    
#ifdef USE_DESKTOP_GL
    // 使用 VAO 在桌面环境绑定状态
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#endif

    // 绑定并上传 VBO/EBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

    // 设置顶点属性指针（位置和法线）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // 位置
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // 法线
    glEnableVertexAttribArray(1);

#ifdef USE_DESKTOP_GL
    glBindVertexArray(0); // 解绑 VAO，防止后续误操作
#endif
    
    glEnable(GL_DEPTH_TEST);
    
    return true;
}

void Renderer::resize(int w, int h) {
    glViewport(0, 0, w, h);
}

void Renderer::render(const float mvp[16], const float model[16]) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"u_mvpMatrix"),1,GL_FALSE,mvp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"u_modelMatrix"),1,GL_FALSE,model);
    glUniform4f(glGetUniformLocation(shaderProgram,"u_color"),1.f,1.f,1.f,1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "u_lightDir"), 1.0f, 1.0f, 1.0f); // 你想要的光方向

#ifdef USE_DESKTOP_GL
    glBindVertexArray(vao);
#else
    // GLES2.0 需要每帧重新设置属性指针
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
#endif

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
}

void Renderer::shutdown() {
    glDeleteProgram(shaderProgram); // Delete shader program
}
