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
out vec2 TexCoord;
void main() {
    gl_Position = u_mvpMatrix * vec4(a_position, 1.0);
    v_normal = mat3(transpose(inverse(u_modelMatrix))) * a_normal;
    TexCoord = (gl_Position.xy / gl_Position.w);
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
uniform sampler2D radianceTex;
in vec2 TexCoord;
void main() {
    float NdotL = dot(normalize(v_normal), normalize(-u_lightDir));
    float diff = NdotL * 0.5 + 0.5;
    diff = clamp(diff, 0.0, 1.0);
    vec3 diffuse = diff * u_color.rgb;
    vec3 emissive = u_emissive.rgb; 

    vec2 uv = gl_FragCoord.xy / vec2(800,600);

    fragColor = vec4(diffuse, u_color.a);

    vec3 color = diffuse + emissive;
    vec3 radiance = texture(radianceTex, uv).rgb;
    color += radiance; // 叠加全局光照
    fragColor = vec4(color, u_color.a);
}
)";


static const char* blockShaderSrc = R"(
#version 300 es
precision mediump float;
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvpMatrix;
void main() {
    gl_Position = u_mvpMatrix * vec4(a_position, 1.0);
}
)";

static const char* blockfragmentShaderSrc = R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
void main() {
    FragColor = vec4(1,1,1,1); // 纯白色
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

// 新建一个简单的shader，只输出u_emissive
const char* radianceVertexShaderSrc = R"(
#version 300 es
precision mediump float;
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvpMatrix;
void main() {
    gl_Position = u_mvpMatrix * vec4(a_position, 1.0);
}
)";
const char* radianceFragmentShaderSrc = R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
uniform vec4 u_emissive;
void main() {
    FragColor = u_emissive;
    //FragColor.a = 0.0; // 透明度为0
    //FragColor = vec4(1,1,1,1);
}
)";

const char* radianceDiffuseFragmentShaderSrc = R"(
#version 300 es
precision mediump float;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D radianceTex;
uniform sampler2D blockMapTex;
uniform vec2 texelSize; // (1.0/width, 1.0/height)

void main() {
    vec4 accum = vec4(0.0);
    // 四个方向： up, down, left, right
    vec2 dirs[4] = vec2[4](
        vec2( 1.0, 1.0),
        vec2( 1.0,-1.0),
        vec2(-1.0, -1.0),
        vec2(-1.0, 1.0)
    );
    
    int bSample[4] = int[4](1, 1, 1, 1);
    int maxSteps1 = 16;
    int i = 0;
    int j = 0;
    float factor = 1.0;
    for(i = 0; i < maxSteps1; ++i) {
        factor *= 0.98;
        for(j = 0; j < 4; ++j) {
            if(bSample[j]<1)
            {
                continue;
            }
            vec2 offset = vec2(texelSize.x * dirs[j].x, texelSize.y * dirs[j].y);
            vec4 SampleColor = texture(radianceTex, TexCoord + offset * float(i));
            vec4 BlockColor = texture(blockMapTex, TexCoord + offset * float(i));
            if(length(SampleColor.rgb) > 0.1) {
                accum += factor * SampleColor * 0.25;
                bSample[j] = 0;
            }
            if(length(BlockColor.rgb) > 0.8) {
                bSample[j] = 0; 
            }
            
        }
        // vec2 offset = vec2(texelSize.x * dirs[0].x, texelSize.y * dirs[0].y);
        // vec4 SampleColor = texture(radianceTex, TexCoord + offset* float(i));
        // if(length(SampleColor) > 0.1) {
        //     accum += factor * SampleColor;
        //     //break;
        // }
        // offset = vec2(texelSize.x * dirs[1].x, texelSize.y * dirs[1].y);
        // SampleColor = texture(radianceTex, TexCoord + offset* float(i));
        // if(length(SampleColor) > 0.1) {
        //     accum += factor * SampleColor;
        //     //break;
        // }
        // offset = vec2(texelSize.x * dirs[2].x, texelSize.y * dirs[2].y);
        // SampleColor = texture(radianceTex, TexCoord + offset* float(i));
        // if(length(SampleColor) > 0.1) {
        //     accum += factor * SampleColor;
        //     //break;
        // }
        // offset = vec2(texelSize.x * dirs[3].x, texelSize.y * dirs[3].y);
        // SampleColor = texture(radianceTex, TexCoord + offset* float(i));
        // if(length(SampleColor) > 0.1) {
        //     accum += factor * SampleColor;
        //     //break;
        // }

    }




    vec4 c1 = texture(radianceTex, TexCoord);
    //outCol = c1
    FragColor = vec4(accum.rgb,1.0);
}
)";

const char* radianceDiffuseCascadeFragmentShaderSrc = R"(
#version 300 es
precision mediump float;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D radianceTex;
uniform sampler2D blockMapTex;
uniform vec2    texelSize;

const int   STEPS1 = 16;
const float DECAY1 = 0.98;
const int   STEPS2 = 16;
const float DECAY2 = 0.98;

// π 常量
const float PI = 3.141592653589793;

// 把角度转弧度
float rad(float deg) {
    return deg * PI / 180.0;
}

void main(){
    vec3 accum = vec3(0.0);
    bool   done1[4] = bool[4](false,false,false,false);
    float  maxD1    = 0.0;
    
    // —— 第一级：4 条对角线（45°,135°,225°,315°） ——
    float w = 1.0;
    for(int i=1; i<=STEPS1; ++i){
        w *= DECAY1;
        float dist = float(i);
        for(int d=0; d<4; ++d){
            if(done1[d]) continue;
            // 计算角度：起始 45°，每次 +90°
            float angle = rad(45.0 + float(d)*90.0);
            vec2 dir   = vec2(cos(angle), sin(angle));
            vec2 uv    = TexCoord + dir * texelSize * dist;
            // 越界或遮挡
            if(uv.x<0.||uv.x>1.||uv.y<0.||uv.y>1. ||
               texture(blockMapTex,uv).r>0.5) {
                done1[d] = true; 
                continue;
            }
            vec3 col = texture(radianceTex,uv).rgb;
            if(length(col)>0.01){
                accum   += w * col * 0.5;
                done1[d] = true;
                maxD1    = max(maxD1, dist);
            }
        }
        if(done1[0]&&done1[1]&&done1[2]&&done1[3]) break;
    }

    float startDist = max(maxD1, float(STEPS1));
    bool  done2[8] = bool[8](false,false,false,false,false,false,false,false);
    w = 1.0;

    //—— 第二级：8 条径向（0°,45°,90°……315°） ——
    for(int i=int(startDist)+1; i<=STEPS2; ++i){
        w *= DECAY2;
        float dist = float(i);
        for(int d=0; d<8; ++d){
            if(done2[d]) continue;
            float angle = rad(float(d) * 45.0);  // 360/8 = 45°
            vec2 dir   = vec2(cos(angle), sin(angle));
            vec2 uv    = TexCoord + dir * texelSize * dist;
            if(uv.x<0.||uv.x>1.||uv.y<0.||uv.y>1. ||
               texture(blockMapTex,uv).r>0.5) {
                done2[d] = true;
                continue;
            }
            vec3 col = texture(radianceTex,uv).rgb;
            if(length(col)>0.01){
                accum   += w * col * 0.5;
                done2[d] = true;
            }
        }
        bool all2 = true;
        for(int d=0; d<8; ++d) all2 = all2 && done2[d];
        if(all2) break;
    }

    FragColor = vec4(accum,1.0);
}
)";

const char* ppgiVertexShaderSrc = R"(
#version 300 es
precision mediump float;
// 顶点属性：
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";
const char* ppgiFragmentShaderSrc = R"(
#version 300 es
precision mediump float;
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D u_scene;     // 主渲染颜色
uniform sampler2D u_radiance;  // 自发光贴图
uniform float u_intensity;     // 发光强度
void main() {
    vec4 sceneCol = texture(u_scene, vUV);
    vec4 glowCol  = texture(u_radiance, vUV) * u_intensity;
    // 线性叠加，可根据需求改为其它混合模式
    FragColor = sceneCol + glowCol;
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


    // compile radiance shader
    {
            // 顶点
        unsigned int rvs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(rvs, 1, &radianceVertexShaderSrc, nullptr);
        glCompileShader(rvs);
        // 检查编译
        int ok;
        glGetShaderiv(rvs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetShaderInfoLog(rvs, 512, nullptr, buf);
            std::cerr << "Radiance VS error: " << buf << std::endl;
        }

        unsigned int rfs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(rfs, 1, &radianceFragmentShaderSrc, nullptr);
        glCompileShader(rfs);
        // 检查编译...

        radianceShaderProgram = glCreateProgram();
        glAttachShader(radianceShaderProgram, rvs);
        glAttachShader(radianceShaderProgram, rfs);
        glLinkProgram(radianceShaderProgram);
        // 检查链接...

        glDeleteShader(rvs);
        glDeleteShader(rfs);
    }
    //block
    {
        unsigned int bvs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(bvs, 1, &blockShaderSrc, nullptr);
        glCompileShader(bvs);
        // 检查编译...

        unsigned int bfs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(bfs, 1, &blockfragmentShaderSrc, nullptr);
        glCompileShader(bfs);
        // 检查编译...

        blockMapShaderProgram = glCreateProgram();
        glAttachShader(blockMapShaderProgram, bvs);
        glAttachShader(blockMapShaderProgram, bfs);
        glLinkProgram(blockMapShaderProgram);
        // 检查链接...

        glDeleteShader(bvs);
        glDeleteShader(bfs);
    }
    //
    {
        unsigned int dvs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(dvs, 1, &quadVertexShaderSrc, nullptr); // 用全屏quad的vs
        glCompileShader(dvs);
        // 检查编译...

        unsigned int dfs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(dfs, 1, &radianceDiffuseFragmentShaderSrc, nullptr);
        glCompileShader(dfs);
        // 检查编译...

        radianceDiffuseShaderProgram = glCreateProgram();
        glAttachShader(radianceDiffuseShaderProgram, dvs);
        glAttachShader(radianceDiffuseShaderProgram, dfs);
        glLinkProgram(radianceDiffuseShaderProgram);
        // 检查链接...

        glDeleteShader(dvs);
        glDeleteShader(dfs);
    }

    {
        unsigned int ppgivs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(ppgivs, 1, &ppgiVertexShaderSrc, nullptr);
        glCompileShader(ppgivs);
        unsigned int ppgifs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(ppgifs, 1, &ppgiFragmentShaderSrc, nullptr);
        glCompileShader(ppgifs);

        ppgiShaderProgram = glCreateProgram();
        glAttachShader(ppgiShaderProgram, ppgivs);
        glAttachShader(ppgiShaderProgram, ppgifs);
        glLinkProgram(ppgiShaderProgram);

        glDeleteShader(ppgivs);
        glDeleteShader(ppgifs);
    }


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
        // 生成并绑定 FBO
        glGenFramebuffers(1, &blurFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);

        // 生成纹理
        glGenTextures(1, &blurTex[i]);
        glBindTexture(GL_TEXTURE_2D, blurTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 将纹理附加到 FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, blurTex[i], 0);

        // 检查 FBO 完整性
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Blur FBO " << i << " not complete!" << std::endl;
        }
    }
    // 恢复到默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Init Radiance FBO
    glGenFramebuffers(1, &radianceFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, radianceFBO);

    glGenTextures(1, &radianceTex);
    glBindTexture(GL_TEXTURE_2D, radianceTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, radianceTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Radiance FBO not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Init BlockMap FBO
    glGenFramebuffers(1, &blockMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, blockMapFBO);
    glGenTextures(1, &blockMapTex);
    glBindTexture(GL_TEXTURE_2D, blockMapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blockMapTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "BlockMap FBO not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    //ppgi
    glGenFramebuffers(1, &postprocessingFBO_GI);
    glBindFramebuffer(GL_FRAMEBUFFER, postprocessingFBO_GI);
    glGenTextures(1, &postprocessingTex_GI);
    glBindTexture(GL_TEXTURE_2D, postprocessingTex_GI);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postprocessingTex_GI, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ppgi FBO not complete!" << std::endl;
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

void Renderer::renderEmissiveToRadianceFBO(const float vp[16], const std::vector<Instance*>& instances) {
    glBindFramebuffer(GL_FRAMEBUFFER, radianceFBO);
    glViewport(0, 0, 800, 600);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glDisable(GL_DEPTH_TEST); 


    glUseProgram(radianceShaderProgram);

    GLint locMVP = glGetUniformLocation(radianceShaderProgram, "u_mvpMatrix");
    GLint locEmissive = glGetUniformLocation(radianceShaderProgram, "u_emissive");



    //glBindVertexArray(vao);
    for (const auto& inst : instances) {
        float mvp[16];
        multiplyMatrices(vp, inst->modelMatrix, mvp);
        glUniformMatrix4fv(glGetUniformLocation(radianceShaderProgram, "u_mvpMatrix"), 1, GL_FALSE, mvp);
        glUniform4fv(glGetUniformLocation(radianceShaderProgram, "u_emissive"), 1, inst->emissive);
        inst->mesh->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderBlockMap(const float vp[16], const std::vector<Instance*>& instances) {
    glBindFramebuffer(GL_FRAMEBUFFER, blockMapFBO);
    glViewport(0, 0, 800, 600);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(blockMapShaderProgram);

    GLint locMVP = glGetUniformLocation(blockMapShaderProgram, "u_mvpMatrix");

    for (const auto& inst : instances) {
        float mvp[16];
        multiplyMatrices(vp, inst->modelMatrix, mvp);
        glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp);
        inst->mesh->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Core::Renderer::renderDiffuseFBO(const float vp[16], const std::vector<Instance *> &instances)
{
    // 1. 绑定 ping-pong FBO
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[0]);
    glViewport(0, 0, 800, 600);
    glClear(GL_COLOR_BUFFER_BIT);


    // 2. 用扩散shader
    glUseProgram(radianceDiffuseShaderProgram);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) std::cerr << "VAO bind error: " << err << std::endl;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, radianceTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blockMapTex); // 绑定blockMap贴图
    glUniform1i(glGetUniformLocation(radianceDiffuseShaderProgram, "radianceTex"), 0);
    glUniform1i(glGetUniformLocation(radianceDiffuseShaderProgram, "blockMapTex"), 1);
    glUniform2f(glGetUniformLocation(radianceDiffuseShaderProgram, "texelSize"), 1.0f/800.0f, 1.0f/600.0f);

    // 3. 绘制全屏quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Core::Renderer::renderPPGI()
{
    glBindFramebuffer(GL_FRAMEBUFFER, postprocessingFBO_GI);
    glViewport(0, 0, 800, 600);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(ppgiShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex); // 主渲染颜色
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTex[0]); // 自发光贴图
    glUniform1i(glGetUniformLocation(ppgiShaderProgram, "u_scene"), 0);
    glUniform1i(glGetUniformLocation(ppgiShaderProgram, "u_radiance"), 1);

    float intensity = 1.0f;
    glUniform1f(glGetUniformLocation(ppgiShaderProgram, "u_intensity"), intensity);

    // 渲染全屏 Quad（已绑定 VAO）
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 恢复默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
void Renderer::shutdown() {
    glDeleteProgram(shaderProgram); // Delete shader program
}
void Renderer::renderStaticInstances(const float vp[16], const std::vector<Instance*>& instances) {

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTex[0]); // 扩散后的radiance贴图
    glUniform1i(glGetUniformLocation(shaderProgram, "radianceTex"), 1);


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
        glUniform3f(glGetUniformLocation(shaderProgram, "u_lightDir"), 1.0f, 1.0f, 1.0f); // 你想要的光方向


        inst->mesh->draw();
    }
}

void Core::Renderer::renderDynamicInstances(const float vp[16], const std::vector<Core::Instance *> &instances)
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTex[0]); // 扩散后的radiance贴图
    glUniform1i(glGetUniformLocation(shaderProgram, "radianceTex"), 1);


    glUseProgram(shaderProgram);
    for (const auto& inst : instances) {
        float mvp[16];
        multiplyMatrices(vp, inst->modelMatrix, mvp);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_mvpMatrix"), 1, GL_FALSE, mvp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_modelMatrix"), 1, GL_FALSE, inst->modelMatrix);
        glUniform4fv(glGetUniformLocation(shaderProgram, "u_color"), 1, inst->color);
        // 如有自发光
        glUniform4fv(glGetUniformLocation(shaderProgram, "u_emissive"), 1, inst->emissive);
        glUniform3f(glGetUniformLocation(shaderProgram, "u_lightDir"), 1.0f, 1.0f, 1.0f); // 你想要的光方向
        
        inst->mesh->draw();
    }
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
    // glBindTexture(GL_TEXTURE_2D, blurTex[0]);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    //glBindTexture(GL_TEXTURE_2D, radianceTex);
    // glBindTexture(GL_TEXTURE_2D, postprocessingTex_GI);
    //glBindTexture(GL_TEXTURE_2D, blockMapTex); // 如果需要显示radiance贴图


    glUniform1i(glGetUniformLocation(quadShaderProgram, "screenTex"), 0);

    // 3. 绘制全屏Quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
