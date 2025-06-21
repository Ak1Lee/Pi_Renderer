#include "Core/Renderer.h"
#include <SDL.h>
#include <glad/glad.h>
#include <iostream>

using namespace Core;

static const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvpMatrix;
uniform vec4 u_color;
out vec4 v_color;
void main() {
    gl_Position = u_mvpMatrix * vec4(a_position, 1.0);
    v_color = u_color;
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
in vec4 v_color;
out vec4 FragColor;
void main() {
    FragColor = v_color;
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
    const float verts[] = {
        // 前面
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        // 后面
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
        // 左面
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
        // 右面
         0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
        // 顶面
        -0.5f, 0.5f,-0.5f, -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
        // 底面
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f
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
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glEnable(GL_DEPTH_TEST);
    
    return true;
}

void Renderer::resize(int w, int h) {
    glViewport(0, 0, w, h);
}

void Renderer::render(const float mvp[16]) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"u_mvpMatrix"),1,GL_FALSE,mvp);
    glUniform4f(glGetUniformLocation(shaderProgram,"u_color"),0.7f,0.7f,0.3f,1.0f);
#ifdef USE_DESKTOP_GL
    glBindVertexArray(vao);
#else
    // OpenGL ES 2.0: 手动绑定 VBO/EBO 并设置顶点属性
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
#endif
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
}

void Renderer::shutdown() {
    glDeleteBuffers(1,&vbo);
    glDeleteBuffers(1,&ebo);
    glDeleteVertexArrays(1,&vao);
    glDeleteProgram(shaderProgram);
}