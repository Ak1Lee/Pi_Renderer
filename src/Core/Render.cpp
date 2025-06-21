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

    Cube.draw(); // 使用 CubeMesh 类来绘制立方体
}

void Renderer::render(const float vp[16], const std::vector<float*>& modelMatrices) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glUniform4f(glGetUniformLocation(shaderProgram,"u_color"),1.f,1.f,1.f,1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "u_lightDir"), 1.0f, 1.0f, 1.0f);

    float mvp[16];
    for (auto model : modelMatrices) {
        multiplyMatrices(vp, model, mvp); // mvp = vp * model
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"u_mvpMatrix"),1,GL_FALSE,mvp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"u_modelMatrix"),1,GL_FALSE,model);
        Cube.draw();
    }
}
void Renderer::shutdown() {
    glDeleteProgram(shaderProgram); // Delete shader program
}
