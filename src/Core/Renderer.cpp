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
uniform vec2 u_screenSize; // 屏幕分辨率
in vec2 TexCoord;
void main() {
    float NdotL = dot(normalize(v_normal), normalize(-u_lightDir));
    float diff = NdotL * 0.5 + 0.5;
    diff = clamp(diff, 0.0, 1.0);
    vec3 diffuse = diff * u_color.rgb;
    vec3 emissive = u_emissive.rgb; 

    vec2 uv = gl_FragCoord.xy / u_screenSize;

    // 降低环境光，为GI效果留出空间
    vec3 ambient = vec3(0.05, 0.05, 0.08);
    // 降低基础漫反射强度
    vec3 color = diffuse * 0.6 + emissive + ambient;
    
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

uniform float att_c;  // 通常取 1.0
uniform float att_l;  // 0.7 ~ 1.8
uniform float att_q;  // 1.8 ~ 2.5

const int   STEPS1      = 16;
const float DECAY1      = 0.95;
const int   STEPS2      = 16;
const float DECAY2      = 0.90;
const float SPLIT_ANGLE = 22.5;        // 子射线偏转角度
const float PI          = 3.141592653589793;

// 预先定义的四个对角线角度
const float baseAngles[4] = float[4](45.0, 135.0, 225.0, 315.0);

// 角度转弧度
float rad(float deg) {
    return deg * PI / 180.0;
}

void main(){
    vec3 accum = vec3(0.0);
    if(texture(blockMapTex, TexCoord).r > 0.5)
    {
        FragColor = vec4(accum, 1.0);
        return;
    }
    

    // 第一级命中标记与距离
    bool  hit1[4]    = bool[4](false, false, false, false);
    float hitDist[4] = float[4](0.0,  0.0,   0.0,   0.0);

    // —— 第一级对角线采样 —— 
    float w1 = 1.0;
    for(int i = 1; i <= STEPS1; ++i){
        //w1 *= DECAY1;
        float dist = float(i);
        for(int d = 0; d < 4; ++d){
            if(hit1[d]) continue;
            float ang = rad(baseAngles[d]);
            vec2 dir = vec2(cos(ang), sin(ang));
            vec2 uv  = TexCoord + dir * texelSize * dist;

            // 出屏或阻挡
            if(uv.x < 0.0 || uv.x > 1.0 ||
               uv.y < 0.0 || uv.y > 1.0 ||
               texture(blockMapTex, uv).r > 0.5) {
                hit1[d]    = true;
                hitDist[d] = dist;
                continue;
            }
            vec3 col = texture(radianceTex, uv).rgb;
            if(length(col) > 0.01) {
                float att = 1.0/(att_c + att_l*dist + att_q*dist*dist);
                accum      += w1 * col * att;
                hit1[d]     = true;
                hitDist[d]  = dist;
            }
        }
    }
    for (int d = 0; d < 4; ++d) {
        //hit1[d] = true;
        hitDist[d] = 12.0; // 模拟第一级命中
    }


    // —— 第二级：在每个命中方向上分叉两条子射线 —— 
    for(int d = 0; d < 4; ++d) {
        //if(hit1[d]) continue;  // 只有真正命中一级才分叉
        float baseAng = baseAngles[d];

        for(int sgn = -1; sgn <= 1; sgn += 2) {
            float childAng = baseAng + float(sgn)*SPLIT_ANGLE;
            vec2 dir = vec2(cos(rad(childAng)), sin(rad(childAng)));

            float w2 = 0.5;
            int startI = int(hitDist[d]) + 1;
            int endI   = startI + STEPS2;
            for(int i = startI; i <= endI; ++i) {
                float dist = float(i);
                // 计算屏幕步进
                float stepSize = mix(1.0, 3.0, dist/float(endI)); 
                vec2 uv = TexCoord + dir * texelSize * dist * stepSize;
                // 出界或被阻挡，才停止
                if(uv.x<0.||uv.x>1.||uv.y<0.||uv.y>1. ||
                texture(blockMapTex,uv).r>0.5) {
                    break;
                }

                vec3 col = texture(radianceTex,uv).rgb;
                if(length(col) > 0.01) {
                    float att = 1.0/(att_c + att_l*dist + att_q*dist*dist);
                    accum += w2 * col * att;
                    // **不再 break**，让它再往后采，给你一道“光尾”
                    break;
                }
                
            }
        }
    }

    FragColor = vec4(accum, 1.0);
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

// 简化版高性能GI着色器
const char* simpleGIFragmentShaderSrc = R"(
#version 300 es
precision mediump float;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D radianceTex;
uniform sampler2D blockMapTex;
uniform vec2 texelSize;

void main() {
    vec3 accum = vec3(0.0);
    
    // 检查当前像素是否被阻挡
    if(texture(blockMapTex, TexCoord).r > 0.5) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // 简化版：只使用4个主要方向，每个方向只采样4步
    vec2 dirs[4] = vec2[4](
        vec2(1.0, 0.0),   // 右
        vec2(0.0, 1.0),   // 上  
        vec2(-1.0, 0.0),  // 左
        vec2(0.0, -1.0)   // 下
    );
    
    const int MAX_STEPS = 4;
    const float STEP_SIZE = 2.0;
    
    for(int d = 0; d < 4; d++) {
        for(int i = 1; i <= MAX_STEPS; i++) {
            vec2 samplePos = TexCoord + dirs[d] * texelSize * float(i) * STEP_SIZE;
            
            // 边界检查
            if(samplePos.x < 0.0 || samplePos.x > 1.0 || 
               samplePos.y < 0.0 || samplePos.y > 1.0) {
                break;
            }
            
            // 阻挡检查
            if(texture(blockMapTex, samplePos).r > 0.5) {
                break;
            }
            
            // 采样radiance
            vec3 radiance = texture(radianceTex, samplePos).rgb;
            if(length(radiance) > 0.01) {
                float falloff = 1.0 / (1.0 + float(i) * 0.5);
                accum += radiance * falloff * 0.25; // 每个方向贡献25%
                break; // 找到光源就停止这个方向的采样
            }
        }
    }
    
    FragColor = vec4(accum, 1.0);
}
)";

// 简化的SDF GI着色器 - 使用预计算的屏幕坐标
const char* sdfGIFragmentShaderSrc = R"(
#version 300 es
precision mediump float;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D blockMapTex;
uniform vec2 texelSize;
uniform vec2 u_playerScreenPos;    // 预计算的玩家屏幕坐标
uniform float u_lightRange;       // 光照范围（屏幕空间）

void main() {
    // 检查当前像素是否在墙壁中
    if (texture(blockMapTex, TexCoord).r > 0.5) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // 计算到玩家的屏幕空间距离
    vec2 toPlayer = u_playerScreenPos - TexCoord;
    float screenDistance = length(toPlayer);
    
    // 如果玩家不在屏幕内或距离太远，则不产生光照
    if (u_playerScreenPos.x < 0.0 || u_playerScreenPos.x > 1.0 || 
        u_playerScreenPos.y < 0.0 || u_playerScreenPos.y > 1.0 ||
        screenDistance > u_lightRange) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // 使用光线步进进行遮挡检测 - 减少步数提高性能
    vec2 lightDirection = normalize(toPlayer);
    vec2 currentPos = TexCoord;
    const int maxSteps = 16; // 减少从32到16
    float stepSize = screenDistance / float(maxSteps);
    bool occluded = false;
    
    for(int i = 1; i < maxSteps; i++) {
        currentPos += lightDirection * stepSize;
        
        // 检查是否到达光源位置
        if(length(currentPos - u_playerScreenPos) < stepSize * 2.0) {
            break;
        }
        
        // 检查是否被阻挡
        if(currentPos.x < 0.0 || currentPos.x > 1.0 || 
           currentPos.y < 0.0 || currentPos.y > 1.0 ||
           texture(blockMapTex, currentPos).r > 0.5) {
            occluded = true;
            break;
        }
    }
    
    if (occluded) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // 计算光照衰减（基于屏幕距离） - 更平滑的衰减
    float attenuation = 1.0 - (screenDistance / u_lightRange);
    attenuation = smoothstep(0.0, 1.0, attenuation); // 使用smoothstep让衰减更平滑
    attenuation = attenuation * attenuation; // 二次衰减，让光照更集中
    
    // 适度的光照强度，让玩家能够看清周围
    vec3 lightColor = vec3(1.2, 0.6, 0.3) * attenuation; // 稍微提高一点让区分更明显
    
    // 极低的环境光，营造黑暗氛围
    vec3 ambient = vec3(0.005, 0.005, 0.01); // 极低的环境光，几乎全黑
    
    FragColor = vec4(lightColor + ambient, 1.0);
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
        
        // 强制使用SDF GI shader进行测试
        glShaderSource(dfs, 1, &sdfGIFragmentShaderSrc, nullptr);
        std::cout << "Using SDF GI shader for testing" << std::endl;
        
        glCompileShader(dfs);
        
        // 检查编译错误
        int ok;
        glGetShaderiv(dfs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetShaderInfoLog(dfs, 512, nullptr, buf);
            std::cerr << "Diffuse FS error: " << buf << std::endl;
        }

        radianceDiffuseShaderProgram = glCreateProgram();
        glAttachShader(radianceDiffuseShaderProgram, dvs);
        glAttachShader(radianceDiffuseShaderProgram, dfs);
        glLinkProgram(radianceDiffuseShaderProgram);
        
        // 检查链接状态
        int linkOk;
        glGetProgramiv(radianceDiffuseShaderProgram, GL_LINK_STATUS, &linkOk);
        if (!linkOk) {
            char buf[512];
            glGetProgramInfoLog(radianceDiffuseShaderProgram, 512, nullptr, buf);
            std::cerr << "Radiance Diffuse Shader link error: " << buf << std::endl;
        }
        
        // 缓存SDF GI相关的uniform位置
        loc_playerScreenPos = glGetUniformLocation(radianceDiffuseShaderProgram, "u_playerScreenPos");
        loc_texelSize = glGetUniformLocation(radianceDiffuseShaderProgram, "texelSize");
        loc_lightRange = glGetUniformLocation(radianceDiffuseShaderProgram, "u_lightRange");

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

    // Cache uniform locations for performance
    loc_mvpMatrix = glGetUniformLocation(shaderProgram, "u_mvpMatrix");
    loc_modelMatrix = glGetUniformLocation(shaderProgram, "u_modelMatrix");
    loc_color = glGetUniformLocation(shaderProgram, "u_color");
    loc_emissive = glGetUniformLocation(shaderProgram, "u_emissive");
    loc_lightDir = glGetUniformLocation(shaderProgram, "u_lightDir");
    loc_radianceTex = glGetUniformLocation(shaderProgram, "radianceTex");
    loc_screenSize = glGetUniformLocation(shaderProgram, "u_screenSize");
    
    // 缓存SDF GI相关的uniform位置 - 注意这里应该等radianceDiffuseShaderProgram创建后再设置
    // 暂时先注释掉，稍后在radianceDiffuseShaderProgram创建后设置

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
#ifdef USE_GLES2
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 800, 600);
#else
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 800, 600);
#endif
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
#ifdef USE_GLES2
    // GLES2 doesn't have VAO, just create VBO
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
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
#endif


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
    screenWidth = w;
    screenHeight = h;
    glViewport(0, 0, w, h);
}

void Renderer::reinitializeFBOs(int width, int height) {
    // 删除现有的FBO和纹理
    if (sceneFBO) {
        glDeleteFramebuffers(1, &sceneFBO);
        glDeleteTextures(1, &sceneColorTex);
        glDeleteRenderbuffers(1, &sceneDepthRBO);
    }
    if (radianceFBO) {
        glDeleteFramebuffers(1, &radianceFBO);
        glDeleteTextures(1, &radianceTex);
    }
    if (blockMapFBO) {
        glDeleteFramebuffers(1, &blockMapFBO);
        glDeleteTextures(1, &blockMapTex);
    }
    if (postprocessingFBO_GI) {
        glDeleteFramebuffers(1, &postprocessingFBO_GI);
        glDeleteTextures(1, &postprocessingTex_GI);
    }
    if (blurFBO[0]) {
        glDeleteFramebuffers(2, blurFBO);
        glDeleteTextures(2, blurTex);
    }

    // 重新创建所有FBO和纹理，使用新的分辨率
    // Scene FBO
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneColorTex);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex, 0);

    glGenRenderbuffers(1, &sceneDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRBO);
#ifdef USE_GLES2
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
#else
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
#endif
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Scene FBO not complete!" << std::endl;
    }

    // Blur FBO
    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &blurFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);

        glGenTextures(1, &blurTex[i]);
        glBindTexture(GL_TEXTURE_2D, blurTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Blur FBO " << i << " not complete!" << std::endl;
        }
    }

    // Radiance FBO
    glGenFramebuffers(1, &radianceFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, radianceFBO);

    glGenTextures(1, &radianceTex);
    glBindTexture(GL_TEXTURE_2D, radianceTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, radianceTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Radiance FBO not complete!" << std::endl;
    }

    // BlockMap FBO
    glGenFramebuffers(1, &blockMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, blockMapFBO);
    glGenTextures(1, &blockMapTex);
    glBindTexture(GL_TEXTURE_2D, blockMapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blockMapTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "BlockMap FBO not complete!" << std::endl;
    }

    // PostProcessing FBO
    glGenFramebuffers(1, &postprocessingFBO_GI);
    glBindFramebuffer(GL_FRAMEBUFFER, postprocessingFBO_GI);
    glGenTextures(1, &postprocessingTex_GI);
    glBindTexture(GL_TEXTURE_2D, postprocessingTex_GI);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postprocessingTex_GI, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ppgi FBO not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "FBOs reinitialized for resolution: " << width << "x" << height << std::endl;
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
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glDisable(GL_DEPTH_TEST); 

    std::cout << "Rendering " << instances.size() << " emissive instances" << std::endl;

    glUseProgram(radianceShaderProgram);

    GLint locMVP = glGetUniformLocation(radianceShaderProgram, "u_mvpMatrix");
    GLint locEmissive = glGetUniformLocation(radianceShaderProgram, "u_emissive");

    //glBindVertexArray(vao);
    for (const auto& inst : instances) {
        float mvp[16];
        multiplyMatrices(vp, inst->modelMatrix, mvp);
        glUniformMatrix4fv(glGetUniformLocation(radianceShaderProgram, "u_mvpMatrix"), 1, GL_FALSE, mvp);
        glUniform4fv(glGetUniformLocation(radianceShaderProgram, "u_emissive"), 1, inst->emissive);
        std::cout << "Emissive: " << inst->emissive[0] << ", " << inst->emissive[1] << ", " << inst->emissive[2] << ", " << inst->emissive[3] << std::endl;
        inst->mesh->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderBlockMap(const float vp[16], const std::vector<Instance*>& instances) {
    glBindFramebuffer(GL_FRAMEBUFFER, blockMapFBO);
    glViewport(0, 0, screenWidth, screenHeight);
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
    // std::cout << "Rendering diffuse FBO..." << std::endl; // 移除调试输出
    while (glGetError() != GL_NO_ERROR);

    // 1) 绑定 FBO & 清屏
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[0]);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    // 2) 用扩散 Shader
    glUseProgram(radianceDiffuseShaderProgram);

    // 3) 绑定 Quad VAO
#ifdef USE_GLES2
    bindQuadVertexAttributes();
#else
    glBindVertexArray(quadVAO);
#endif

    // 4) 绑定纹理 & 设置 uniform
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, radianceTex);
    glUniform1i(glGetUniformLocation(radianceDiffuseShaderProgram, "radianceTex"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blockMapTex);
    glUniform1i(glGetUniformLocation(radianceDiffuseShaderProgram, "blockMapTex"), 1);

    glUniform2f(glGetUniformLocation(radianceDiffuseShaderProgram, "texelSize"),
                1.0f/(float)screenWidth, 1.0f/(float)screenHeight);

#ifndef USE_GLES2
    // 只在桌面版设置复杂衰减参数
    GLint loc_c  = glGetUniformLocation(radianceDiffuseShaderProgram, "att_c");
    GLint loc_l  = glGetUniformLocation(radianceDiffuseShaderProgram, "att_l");
    GLint loc_q  = glGetUniformLocation(radianceDiffuseShaderProgram, "att_q");

    float kc = 0.5f;   // 常数衰减
    float kl = 0.2f;   // 线性衰减
    float kq = 0.00f;   // 二次衰减
    glUniform1f(loc_c, kc);
    glUniform1f(loc_l, kl);
    glUniform1f(loc_q, kq);
#endif

    // 5) 绘制 Quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 6) 恢复默认 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 7) 检查错误
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cerr << "renderDiffuseFBO error: 0x" 
                  << std::hex << err << std::dec << std::endl;
}

// SDF GI版本的renderDiffuseFBO函数 - 使用简化的坐标转换
void Core::Renderer::renderDiffuseFBO(const float vp[16], const std::vector<Instance *> &instances,
                                     const float playerWorldPos[3], const float viewProjectionMatrix[16])
{
    while (glGetError() != GL_NO_ERROR);

    // 1) 使用简化的坐标转换方法：直接从MVP矩阵获取变换后位置
    float playerModel[16];
    float playerMVP[16];
    
    // 创建玩家的模型矩阵（只有位移，无旋转缩放）
    for (int i = 0; i < 16; i++) playerModel[i] = 0.0f;
    playerModel[0] = playerModel[5] = playerModel[10] = playerModel[15] = 1.0f; // 单位矩阵
    playerModel[12] = playerWorldPos[0]; // X位移
    playerModel[13] = playerWorldPos[1]; // Y位移
    playerModel[14] = playerWorldPos[2]; // Z位移
    
    // MVP变换
    multiplyMatrices(viewProjectionMatrix, playerModel, playerMVP);
    
    // 直接取MVP矩阵的位移部分（第4列）作为裁剪空间坐标
    float playerClipSpace[4];
    for (int i = 0; i < 4; i++) {
        playerClipSpace[i] = playerMVP[12 + i];
    }
    
    // 透视除法得到NDC坐标
    float playerNDC[2];
    if (playerClipSpace[3] != 0.0f) {
        playerNDC[0] = playerClipSpace[0] / playerClipSpace[3];
        playerNDC[1] = playerClipSpace[1] / playerClipSpace[3];
    } else {
        playerNDC[0] = playerNDC[1] = 0.0f;
    }
    
    // 转换为屏幕UV坐标 [0, 1]
    float playerScreenUV[2] = {
        (playerNDC[0] + 1.0f) * 0.5f,
        (playerNDC[1] + 1.0f) * 0.5f
    };

    // 2) 绑定 FBO & 清屏
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[0]);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    // 3) 用SDF扩散 Shader
    glUseProgram(radianceDiffuseShaderProgram);

    // 4) 绑定 Quad VAO
#ifdef USE_GLES2
    bindQuadVertexAttributes();
#else
    glBindVertexArray(quadVAO);
#endif

    // 5) 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blockMapTex);
    glUniform1i(glGetUniformLocation(radianceDiffuseShaderProgram, "blockMapTex"), 0);

    // 6) 设置SDF GI相关的uniform变量
    if (loc_playerScreenPos != -1) {
        glUniform2f(loc_playerScreenPos, playerScreenUV[0], playerScreenUV[1]);
    }
    if (loc_texelSize != -1) {
        glUniform2f(loc_texelSize, 1.0f/(float)screenWidth, 1.0f/(float)screenHeight);
    }
    if (loc_lightRange != -1) {
        // 极大幅缩小光照范围，让效果更加微妙和局部化
        float lightRange = 0.08f; // 从0.15缩小到0.08，让光照范围更小
        
        // 根据玩家位置调整光照范围，实现平滑过渡
        if (playerScreenUV[0] >= 0.0f && playerScreenUV[0] <= 1.0f && 
            playerScreenUV[1] >= 0.0f && playerScreenUV[1] <= 1.0f) {
            // 玩家在屏幕内，使用基础光照范围
            lightRange = 0.08f;
        } else {
            // 玩家在屏幕外，快速衰减光照
            float distFromScreen = 0.0f;
            if (playerScreenUV[0] < 0.0f) distFromScreen = std::max(distFromScreen, -playerScreenUV[0]);
            if (playerScreenUV[0] > 1.0f) distFromScreen = std::max(distFromScreen, playerScreenUV[0] - 1.0f);
            if (playerScreenUV[1] < 0.0f) distFromScreen = std::max(distFromScreen, -playerScreenUV[1]);
            if (playerScreenUV[1] > 1.0f) distFromScreen = std::max(distFromScreen, playerScreenUV[1] - 1.0f);
            
            // 更快的衰减：距离一点就快速减弱
            lightRange = 0.08f * std::max(0.0f, 1.0f - distFromScreen * 8.0f); // 增加衰减速度
        }
        lightRange = 0.2;
        glUniform1f(loc_lightRange, lightRange);
    }

    // 7) 绘制 Quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 8) 恢复默认 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 9) 检查错误
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cerr << "renderDiffuseFBO (SDF GI) error: 0x" 
                  << std::hex << err << std::dec << std::endl;
}

#ifdef USE_GLES2
void Core::Renderer::bindQuadVertexAttributes() {
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}
#endif

void Core::Renderer::renderStaticInstances(const float vp[16], const std::vector<Core::Instance*>& instances) {
    // 渲染到场景FBO
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(shaderProgram);
    
    // 设置光照方向
    float lightDir[3] = {-1.0f, -1.0f, -1.0f};
    glUniform3fv(loc_lightDir, 1, lightDir);
    
    // 绑定radiance纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, radianceTex);
    glUniform1i(loc_radianceTex, 0);
    
    // 设置屏幕尺寸
    glUniform2f(loc_screenSize, (float)screenWidth, (float)screenHeight);
    
    for (const auto& inst : instances) {
        float mvp[16];
        multiplyMatrices(vp, inst->getModelMatrix(), mvp);
        
        glUniformMatrix4fv(loc_mvpMatrix, 1, GL_FALSE, mvp);
        glUniformMatrix4fv(loc_modelMatrix, 1, GL_FALSE, inst->getModelMatrix());
        
        const float* color = inst->getColor();
        glUniform4fv(loc_color, 1, color);
        
        const float* emissive = inst->getEmissive();
        glUniform4fv(loc_emissive, 1, emissive);
        
        inst->mesh->draw();
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Core::Renderer::renderDynamicInstances(const float vp[16], const std::vector<Core::Instance*>& instances) {
    // 渲染到场景FBO（不清空，在静态对象之上叠加动态对象）
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, screenWidth, screenHeight);
    // 不清空缓冲区，继续在sceneFBO上绘制
    
    glUseProgram(shaderProgram);
    
    // 设置光照方向
    float lightDir[3] = {-1.0f, -1.0f, -1.0f};
    glUniform3fv(loc_lightDir, 1, lightDir);
    
    // 绑定radiance纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, radianceTex);
    glUniform1i(loc_radianceTex, 0);
    
    // 设置屏幕尺寸
    glUniform2f(loc_screenSize, (float)screenWidth, (float)screenHeight);
    
    for (const auto& inst : instances) {
        float mvp[16];
        multiplyMatrices(vp, inst->getModelMatrix(), mvp);
        
        glUniformMatrix4fv(loc_mvpMatrix, 1, GL_FALSE, mvp);
        glUniformMatrix4fv(loc_modelMatrix, 1, GL_FALSE, inst->getModelMatrix());
        
        const float* color = inst->getColor();
        glUniform4fv(loc_color, 1, color);
        
        const float* emissive = inst->getEmissive();
        glUniform4fv(loc_emissive, 1, emissive);
        
        inst->mesh->draw();
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Core::Renderer::renderPPGI() {
    glBindFramebuffer(GL_FRAMEBUFFER, postprocessingFBO_GI);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(ppgiShaderProgram);

#ifdef USE_GLES2
    bindQuadVertexAttributes();
#else
    glBindVertexArray(quadVAO);
#endif

    // 绑定场景纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glUniform1i(glGetUniformLocation(ppgiShaderProgram, "u_scene"), 0);

    // 绑定radiance纹理
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTex[0]);
    glUniform1i(glGetUniformLocation(ppgiShaderProgram, "u_radiance"), 1);

    // 设置强度
    glUniform1f(glGetUniformLocation(ppgiShaderProgram, "u_intensity"), 1.0f);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Core::Renderer::OneFrameRenderFinish(bool usePostProcessing) {
    // 将最终结果渲染到屏幕
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(quadShaderProgram);
    
#ifdef USE_GLES2
    bindQuadVertexAttributes();
#else
    glBindVertexArray(quadVAO);
#endif

    glActiveTexture(GL_TEXTURE0);
    // 根据是否使用后处理选择纹理
    if (usePostProcessing) {
        glBindTexture(GL_TEXTURE_2D, postprocessingTex_GI);
    } else {
        glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    }
    glUniform1i(glGetUniformLocation(quadShaderProgram, "screenTex"), 0);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Core::Renderer::shutdown() {
    // 清理资源
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (quadShaderProgram) glDeleteProgram(quadShaderProgram);
    if (radianceShaderProgram) glDeleteProgram(radianceShaderProgram);
    if (radianceDiffuseShaderProgram) glDeleteProgram(radianceDiffuseShaderProgram);
    if (blockMapShaderProgram) glDeleteProgram(blockMapShaderProgram);
    if (ppgiShaderProgram) glDeleteProgram(ppgiShaderProgram);
    
    if (sceneFBO) glDeleteFramebuffers(1, &sceneFBO);
    if (sceneColorTex) glDeleteTextures(1, &sceneColorTex);
    if (sceneDepthRBO) glDeleteRenderbuffers(1, &sceneDepthRBO);
    
    if (radianceFBO) glDeleteFramebuffers(1, &radianceFBO);
    if (radianceTex) glDeleteTextures(1, &radianceTex);
    
    if (blockMapFBO) glDeleteFramebuffers(1, &blockMapFBO);
    if (blockMapTex) glDeleteTextures(1, &blockMapTex);
    
    if (postprocessingFBO_GI) glDeleteFramebuffers(1, &postprocessingFBO_GI);
    if (postprocessingTex_GI) glDeleteTextures(1, &postprocessingTex_GI);
    
    glDeleteFramebuffers(2, blurFBO);
    glDeleteTextures(2, blurTex);
    
#ifndef USE_GLES2
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
#endif
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
}
