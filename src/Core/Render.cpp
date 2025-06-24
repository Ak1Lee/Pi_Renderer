#include "Core/Renderer.h"
#include "Math/MathTool.h"
#include <SDL.h>
#ifdef USE_DESKTOP_GL
#include <glad/glad.h>
#else
#include <GLES2/gl2.h>
#endif
#include <iostream>


using namespace Core;



static const char* vertexShaderSrc = R"(
#version 300 es
precision mediump float;
in vec3 a_position;
in vec3 a_normal;
uniform mat4 u_mvpMatrix;
uniform mat4 u_modelMatrix;
out vec3 v_normal;
void main() {
    gl_Position = u_mvpMatrix * vec4(a_position, 1.0);
    v_normal = mat3(transpose(inverse(u_modelMatrix))) * a_normal;
}
)";

static const char* fragmentShaderSrc = R"(
#version 300 es
precision mediump float;
out vec4 fragColor;
in vec3 v_normal;
uniform vec4 u_color;
uniform vec3 u_lightDir;
uniform vec4 u_emissive; // 新增自发光
void main() {
    float NdotL = dot(normalize(v_normal), normalize(-u_lightDir));
    float diff = NdotL * 0.5 + 0.5;
    diff = clamp(diff, 0.0, 1.0);
    vec3 diffuse = diff * u_color.rgb;
    vec3 emissive = u_emissive.rgb; 

    fragColor = vec4(diffuse + emissive, u_color.a);
}
)";


// Quad Shader
const char* quadVertexShaderSrc = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;
out vec2 TexCoord;
void main() {
    TexCoord = aTex;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";
const char* quadFragmentShaderSrc = R"(
#version 300 es
precision mediump float;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D screenTex;
void main() {
    FragColor = texture(screenTex, TexCoord);
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



    unsigned int qvs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(qvs, 1, &quadVertexShaderSrc, nullptr);
    glCompileShader(qvs);
    // 检查编译...

    unsigned int qfs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(qfs, 1, &quadFragmentShaderSrc, nullptr);
    glCompileShader(qfs);
    // 检查编译...

    quadShaderProgram = glCreateProgram();
    glAttachShader(quadShaderProgram, qvs);
    glAttachShader(quadShaderProgram, qfs);
    glLinkProgram(quadShaderProgram);
    // 检查链接...

    glDeleteShader(qvs);
    glDeleteShader(qfs);

    return true;
}

bool Renderer::init() {
    if (!compileShaders()) return false;

    
    glEnable(GL_DEPTH_TEST);

    //FBO
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneColorTex);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex, 0);

    glGenRenderbuffers(1, &sceneDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 800, 600);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "FBO not complete!" << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    float quadVertices[] = {
        // positions   // texcoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);


    // Blur FBO
    int width = 800, height = 600;
    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &blurFBO[i]);
        unsigned int blurTextmp;
        glGenTextures(1, &blurTextmp);
        glBindTexture(GL_TEXTURE_2D, blurTextmp);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTextmp, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Blur FBO " << i << " not complete!" << std::endl;
        }
        // 存到成员变量
        blurTex[i] = blurTextmp;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


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
        // Panel.draw(); // 如果需要绘制面板，可以在这里调用
    }
}
void Core::Renderer::renderPanel(const float vp[16], const float model[16])
{
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glUniform4f(glGetUniformLocation(shaderProgram,"u_color"),1.f,1.f,1.f,1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "u_lightDir"), 0.2f, 0.6f, 0.8f);

    float mvp[16];
    multiplyMatrices(vp, model, mvp); // mvp = vp * model

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"u_mvpMatrix"),1,GL_FALSE,mvp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"u_modelMatrix"),1,GL_FALSE,model);

    Panel.draw(); // 使用 PanelMesh 类来绘制面板
}

void Core::Renderer::OneFrameRenderFinish()
{
        // 1. 解绑FBO，回到屏幕
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 800, 600);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 使用全屏Quad着色器
    glUseProgram(quadShaderProgram);
    glBindVertexArray(quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glUniform1i(glGetUniformLocation(quadShaderProgram, "screenTex"), 0);

    // 3. 绘制全屏Quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::shutdown() {
    glDeleteProgram(shaderProgram); // Delete shader program
}
void Renderer::renderStaticInstances(const float vp[16], const std::vector<Instance*>& instances) {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, 800, 600); // FBO尺寸
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    for (const auto& inst : instances) {
        float mvp[16];
        multiplyMatrices(vp, inst->modelMatrix, mvp);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_mvpMatrix"), 1, GL_FALSE, mvp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_modelMatrix"), 1, GL_FALSE, inst->modelMatrix);
        glUniform4fv(glGetUniformLocation(shaderProgram, "u_color"), 1, inst->color);
        // 如有自发光
        glUniform4fv(glGetUniformLocation(shaderProgram, "u_emissive"), 1, inst->emissive);

        inst->mesh->draw();
    }
}

void Core::Renderer::renderDynamicInstances(const float vp[16], const std::vector<Core::Instance *> &instances)
{
    glUseProgram(shaderProgram);
    for (const auto& inst : instances) {
        float mvp[16];
        multiplyMatrices(vp, inst->modelMatrix, mvp);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_mvpMatrix"), 1, GL_FALSE, mvp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_modelMatrix"), 1, GL_FALSE, inst->modelMatrix);
        glUniform4fv(glGetUniformLocation(shaderProgram, "u_color"), 1, inst->color);
        // 如有自发光
        glUniform4fv(glGetUniformLocation(shaderProgram, "u_emissive"), 1, inst->emissive);

        inst->mesh->draw();
    }
}
