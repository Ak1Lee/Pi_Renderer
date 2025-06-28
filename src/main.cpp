#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "Core/Platform.h"
#include "Core/Mesh.h"
#include "Core/CubeMesh.h"
#include "Core/PanelMesh.h"
#include "Core/InstanceBase.h"
#include "Core/Sphere.h"
#include "Core/Renderer.h"
#include "Math/MathTool.h"
#include <cmath>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#ifndef _WIN32
#include <wiringPi.h>
#endif

class Camera
{
    public:
        float position[3];
        float target[3];
        float up[3];

        float aspect = 1.0f;
        float FOVY = M_PI / 4.0f; // 45 degrees in radians
        float NearZ = 0.1f;
        float FarZ = 100.0f;
        
        // 正交投影参数
        bool useOrthographic = false;
        float orthoSize = 10.0f; // 正交视野大小

        float view[16];
        float perspective[16];
        float vp[16];


        Camera(float pos[3], float tar[3], float upVec[3], float aspectRatio = 1.0f, float fovY = M_PI / 4.0f, float nearZ = 0.1f, float farZ = 100.0f, bool ortho = false, float orthoSz = 10.0f) {
            for (int i = 0; i < 3; ++i) {
                position[i] = pos[i];
                target[i] = tar[i];
                up[i] = upVec[i];
            }
            this->aspect = aspectRatio;
            this->FOVY = fovY;
            this->NearZ = nearZ;
            this->FarZ = farZ;
            this->useOrthographic = ortho;
            this->orthoSize = orthoSz;
            updateMatrix();
        }
        
        void updateMatrix() {
            createLookAtMatrix(position, target, up, view);
            
            if (useOrthographic) {
                // 正交投影
                float halfWidth = orthoSize * aspect * 0.5f;
                float halfHeight = orthoSize * 0.5f;
                createOrthographicMatrix(-halfWidth, halfWidth, -halfHeight, halfHeight, NearZ, FarZ, perspective);
            } else {
                // 透视投影
                createPerspectiveMatrix(FOVY, aspect, NearZ, FarZ, perspective);
            }
            
            multiplyMatrices(perspective, view, vp);
        }

};
float mazeCenterX = 0.0f;
float mazeCenterY = 0.0f;
const int mazeWidth = 16;
const int mazeHeight = 16;
// 更大更复杂的迷宫：0=通道，1=墙，2=起点，3=终点
int maze[mazeHeight][mazeWidth] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,0,0,1,0,0,0,0,1,0,0,0,0,0,1}, // 起点在(1,1)
    {1,0,1,0,1,0,1,1,0,1,0,1,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1},
    {1,0,1,1,1,1,0,1,1,1,0,1,0,1,1,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1},
    {1,1,1,0,1,1,1,1,0,1,1,1,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,1},
    {1,1,1,1,0,1,1,0,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,0,1,0,1,1,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,1,0,0,1,0,1},
    {1,0,1,1,1,0,0,0,1,0,0,0,1,0,3,1}, // 终点在(14,14)
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// 起点和终点坐标
const float startX = 1.0f;
const float startY = 1.0f;
const float exitX = 14.0f;
const float exitY = 14.0f;

int main() {
    std::cout << "Program started" << std::endl;
    using namespace Platform;
    int window_width = 800;
    int window_height = 600;
    if (!initWindow(window_width, window_height)) {
        std::cerr << "initWindow failed!" << std::endl;
        return -1;
    }

    Core::Renderer renderer;
    if (!renderer.init()) return -1;

    bool running = true;
    float aspect = 800.0f / 600.0f;
    // float proj[16], view[16], model[16], tmp[16], mvp[16],vp[16];
    float model[16];
    float PanelModel[16];

    // Performance optimization settings
    bool enableGI = true;  // 启用全局光照测试
    bool enablePostProcessing = true; // 后处理开关
    int frameSkip = 0; // 每帧都计算GI以获得更好效果

    // 根据树莓派性能调整设置
    #ifndef _WIN32
    enableGI = true;  // 树莓派也启用GI测试SDF效果
    frameSkip = 2;    // 每3帧计算一次GI（平衡性能和效果）
    std::cout << "Using Raspberry Pi with SDF GI enabled" << std::endl;
    #endif

    const float targetFrameTime = 1000.0f / 60.0f; // 60帧
    Uint32 lastTicks = SDL_GetTicks();
    float totalTime = 0.0f;

    std::cout << "Init :"<< std::endl;

    // float eye[3] = {0, 0, 5};
    // float center[3] = {0, 0, 0.0f};
    // float up[3] = {0, 1, 0}; 

    float eye[3] = {mazeWidth / 2.0f, mazeHeight / 2.0f, 25.0f}; // 提高相机高度适应更大迷宫
    float center[3] = {mazeWidth / 2.0f, mazeHeight / 2.0f, 0.0f};  // 看向中心
    float up[3] = {0, 1, 0};

    std::vector<float*> models;

    // 使用正交投影，设置合适的视野大小以完全包含迷宫
    float orthoSize = std::max(mazeWidth, mazeHeight) + 2.0f; // 稍微大一点确保完全可见
    Camera camera(eye, center, up, aspect, M_PI / 4.0f, 0.1f, 100.0f, true, orthoSize);

    

    Core::CubeMesh cubeMesh;
    Core::PanelMesh panelMesh;
    Core::SphereMesh sphereMesh;
    std::vector<Core::Instance*> StaticInstances;
    std::vector<Core::Instance*> DynamicInstances;
    std::vector<Core::Instance*> BlockInstances;

    Core::Instance* instance = new Core::PanelInstance(&panelMesh);
    float pos[3] = {mazeHeight/2.f, mazeWidth/2.f, -3.0f};
    float rot[3] = {0, 3.14f/2, 0}; // 只绕Y轴旋转
    float scale[3] = {20, 20, 20}; 
    createModelMatrix1(PanelModel, pos, rot, scale);
    instance->setModelMatrix(PanelModel);
    instance->setColor(0.0f, 0.0f, 0.0f, 1.0f); // 黑色地板
    StaticInstances.push_back(instance);



    for (int y = 0; y < mazeHeight; ++y) {
        for (int x = 0; x < mazeWidth; ++x) {
            if (maze[y][x] == 1) { // 是墙
                float pos[3] = { (float)x, (float)y, 0.0f}; // y轴向下
                float rot[3] = {0, 0, 0};
                float scale[3] = {1, 1, 2};
                float* model = new float[16];
                createModelMatrix1(model, pos, rot, scale);
                Core::Instance* instance = new Core::CubeInstance(&cubeMesh);
                instance->setModelMatrix(model);
                instance->setColor(0.0f, 0.0f, 0.0f, 1.0f);
                instance->setEmissive(0.f, 0.f, 0.f, 0.0f);
                StaticInstances.push_back(instance);
                BlockInstances.push_back(instance); // 墙体实例也加入BlockMap
            } else if (maze[y][x] == 2) { // 起点标记
                // 在起点放置一个黑色地板标记，只有微弱绿色发光
                float pos[3] = { (float)x, (float)y, -0.5f}; // 稍微低一点
                float rot[3] = {M_PI/2, 0, 0}; // 旋转90度让面板水平
                float scale[3] = {0.8f, 0.8f, 0.1f};
                float* model = new float[16];
                createModelMatrix1(model, pos, rot, scale);
                Core::Instance* instance = new Core::PanelInstance(&panelMesh);
                instance->setModelMatrix(model);
                instance->setColor(0.0f, 0.0f, 0.0f, 1.0f); // 黑色
                instance->setEmissive(0.05f, 0.2f, 0.05f, 1.0f); // 很微弱的绿色发光
                StaticInstances.push_back(instance);
            } else if (maze[y][x] == 3) { // 终点标记
                // 在终点放置一个黑色地板标记，只有微弱红色发光
                float pos[3] = { (float)x, (float)y, -0.5f}; // 稍微低一点
                float rot[3] = {M_PI/2, 0, 0}; // 旋转90度让面板水平
                float scale[3] = {0.8f, 0.8f, 0.1f};
                float* model = new float[16];
                createModelMatrix1(model, pos, rot, scale);
                Core::Instance* instance = new Core::PanelInstance(&panelMesh);
                instance->setModelMatrix(model);
                instance->setColor(0.0f, 0.0f, 0.0f, 1.0f); // 黑色
                instance->setEmissive(0.2f, 0.05f, 0.05f, 1.0f); // 很微弱的红色发光
                StaticInstances.push_back(instance);
            }
        }
    }


    //Player:
    Core::Instance* playerInstance = new Core::SphereInstance(&sphereMesh);
    DynamicInstances.push_back(playerInstance);
    float playerPos[3] = {startX, startY, 0.0f}; // 玩家从起点开始
    float playerRot[3] = {0, 0, 0};
    float playerScale[3] = {0.3f, 0.3f, 0.3f}; // 球体半径为0.3
    float* playerModel = new float[16];
    float* playerMVP = new float[16];
    createModelMatrix1(playerModel, playerPos, playerRot, playerScale);
    playerInstance->setModelMatrix(playerModel);
    playerInstance->setColor(1.0f, 0.0f, 0.0f, 1.0f); // 红色球体
    playerInstance->setEmissive(1.5f, 1.0f, 0.8f, 1.0f); // 更强的橙红色发光

    //Input:
    #ifndef _WIN32
    wiringPiSetupGpio(); // 使用BCM编号
    pinMode(17, INPUT);
    pinMode(18, INPUT);
    pinMode(27, INPUT);
    pinMode(22, INPUT);
    pullUpDnControl(17, PUD_UP); // 上拉
    pullUpDnControl(18, PUD_UP);
    pullUpDnControl(27, PUD_UP);
    pullUpDnControl(22, PUD_UP);
    #endif

    while (running) {
        models.clear();
        Uint32 frameStart = SDL_GetTicks();

        //pollEvents(running);

        float deltaTime = targetFrameTime / 1000.0f; // 固定步进
        totalTime += deltaTime;

        // 1秒转半圈
        float angle = fmod(totalTime * 180.0f, 360.0f);

    #ifdef _WIN32
        // Windows下用原有的Platform::pollEvents
        Platform::InputState input;
        Platform::pollEvents(running, input);
        
        // 保存当前位置用于碰撞检测
        float newPlayerPos[3] = {playerPos[0], playerPos[1], playerPos[2]};
        
        if (input.up) newPlayerPos[1] += 0.1f;
        else if (input.down) newPlayerPos[1] -= 0.1f;
        else if (input.left) newPlayerPos[0] -= 0.1f;
        else if (input.right) newPlayerPos[0] += 0.1f;
        
        // 碰撞检测：检查新位置是否合法
        int gridX = (int)round(newPlayerPos[0]);
        int gridY = (int)round(newPlayerPos[1]);
        if (gridX >= 0 && gridX < mazeWidth && gridY >= 0 && gridY < mazeHeight && 
            maze[gridY][gridX] != 1) { // 不是墙
            playerPos[0] = newPlayerPos[0];
            playerPos[1] = newPlayerPos[1];
            
            // 检查是否到达终点
            if (gridX == (int)exitX && gridY == (int)exitY) {
                std::cout << "恭喜！到达终点！" << std::endl;
            }
        }
    #else
        // 树莓派下用GPIO
        Platform::pollEvents(running);
        
        // 保存当前位置用于碰撞检测
        float newPlayerPos[3] = {playerPos[0], playerPos[1], playerPos[2]};
        
        if (digitalRead(17) == LOW) newPlayerPos[1] += 0.1f;   // 上
        else if (digitalRead(18) == LOW) newPlayerPos[1] -= 0.1f; // 下
        else if (digitalRead(27) == LOW) newPlayerPos[0] -= 0.1f; // 左
        else if (digitalRead(22) == LOW) newPlayerPos[0] += 0.1f; // 右
        
        // 碰撞检测：检查新位置是否合法
        int gridX = (int)round(newPlayerPos[0]);
        int gridY = (int)round(newPlayerPos[1]);
        if (gridX >= 0 && gridX < mazeWidth && gridY >= 0 && gridY < mazeHeight && 
            maze[gridY][gridX] != 1) { // 不是墙
            playerPos[0] = newPlayerPos[0];
            playerPos[1] = newPlayerPos[1];
            
            // 检查是否到达终点
            if (gridX == (int)exitX && gridY == (int)exitY) {
                std::cout << "恭喜！到达终点！" << std::endl;
            }
        }
    #endif

        // 限制玩家在迷宫范围内
        // playerPos[0] = std::max(0.5f, std::min((float)mazeWidth - 0.5f, playerPos[0]));
        // playerPos[1] = std::max(0.5f, std::min((float)mazeHeight - 0.5f, playerPos[1]));

        createModelMatrix1(playerModel, playerPos, playerRot, playerScale);
        playerInstance->setModelMatrix(playerModel);

        float pos[3] = {mazeHeight/2.f, mazeWidth/2.f, 0.0f};
        float rot[3] = {0, 3.14f/2, 0}; // 只绕Y轴旋转
        float scale[3] = {10, 10, 10};
        createModelMatrix1(model, pos, rot, scale);

        renderer.resize(800, 600);
        
        // 性能优化：根据设置选择性渲染
        static int frameCount = 0;
        frameCount++;
        
        // 坐标转换调试：计算玩家的屏幕坐标
        if (frameCount % 10 == 0) { // 每10帧打印一次，更频繁
            // 将玩家世界坐标转换为屏幕坐标

            multiplyMatrices(camera.vp, playerModel,playerMVP);
            float playerWorldVec4[4] = {0.f, 0.f, 0.f, 1.0f};
            float playerClipSpace[4];
            
            // 
            for (int i = 0; i < 4; i++) {
                playerClipSpace[i] = playerMVP[12+i];
            }
            
            // 透视除法得到NDC坐标
            float playerNDC[3];
            if (playerClipSpace[3] != 0.0f) {
                playerNDC[0] = playerClipSpace[0] / playerClipSpace[3];
                playerNDC[1] = playerClipSpace[1] / playerClipSpace[3];
                playerNDC[2] = playerClipSpace[2] / playerClipSpace[3];
            } else {
                playerNDC[0] = playerNDC[1] = playerNDC[2] = 0.0f;
            }
            
            // 转换为屏幕UV坐标 [0, 1]
            float playerScreenUV[2] = {
                (playerNDC[0] + 1.0f) * 0.5f,
                (playerNDC[1] + 1.0f) * 0.5f
            };
            
            std::cout << "玩家世界: (" << playerPos[0] << ", " << playerPos[1] << ") ";
            std::cout << "屏幕UV: (" << playerScreenUV[0] << ", " << playerScreenUV[1] << ")" << std::endl;
        }
        
        if (enableGI && (frameSkip == 0 || frameCount % (frameSkip + 1) == 0)) {
            // 完整的全局光照渲染
            // if (frameCount % 60 == 0) {
            //     std::cout << "Rendering GI frame " << frameCount << std::endl;
            // }
            renderer.renderEmissiveToRadianceFBO(camera.vp, DynamicInstances);
            renderer.renderBlockMap(camera.vp, BlockInstances);
            
            // 统一使用SDF GI shader，传递VP矩阵和玩家坐标
            renderer.renderDiffuseFBO(camera.vp, DynamicInstances, playerPos, camera.vp);
        }
        
        // 基础渲染（每帧都执行）
        renderer.renderStaticInstances(camera.vp, StaticInstances);
        renderer.renderDynamicInstances(camera.vp, DynamicInstances);

        if (enablePostProcessing) {
            renderer.renderPPGI();
        }
        
        renderer.OneFrameRenderFinish(enablePostProcessing);
        swapBuffers();

        // 性能统计（无帧率限制）
        Uint32 frameEnd = SDL_GetTicks();
        Uint32 frameTime = frameEnd - frameStart;
        
        // 如果需要限制帧率，可以启用以下代码
        // if (frameTime < targetFrameTime) {
        //     SDL_Delay((Uint32)(targetFrameTime - frameTime));
        // }
        
        // Frame info moved to less frequent output
        if (frameCount % 60 == 0) { // 每60帧输出一次
            float fps = frameTime > 0 ? 1000.0f / frameTime : 0.0f;
            std::cout << "FPS: " << fps << " FrameTime: " << frameTime << "ms" 
                      << " PlayerPos: (" << playerPos[0] << ", " << playerPos[1] << ")" << std::endl;
        }
    }

    renderer.shutdown();
    shutdown();
    return 0;
}


