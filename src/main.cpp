

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

        float view[16];
        float perspective[16];
        float vp[16];


        Camera(float pos[3], float tar[3], float upVec[3], float aspectRatio = 1.0f, float fovY = M_PI / 4.0f, float nearZ = 0.1f, float farZ = 100.0f) {
            for (int i = 0; i < 3; ++i) {
                position[i] = pos[i];
                target[i] = tar[i];
                up[i] = upVec[i];
            }
            this->aspect = aspectRatio;
            this->FOVY = fovY;
            this->NearZ = nearZ;
            this->FarZ = farZ;
            createLookAtMatrix(position, target, up, view);
            createPerspectiveMatrix(FOVY, aspect, NearZ, FarZ, perspective);
            multiplyMatrices(perspective, view, vp);
        }
        void updateMatrix() {
            createLookAtMatrix(position, target, up, view);
            createPerspectiveMatrix(FOVY, aspect, NearZ, FarZ, perspective);
            multiplyMatrices(perspective, view, vp);

        }

};
float mazeCenterX = 0.0f;
float mazeCenterY = 0.0f;
const int mazeWidth = 10;
const int mazeHeight = 10;
int maze[mazeHeight][mazeWidth] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,1,0,1,1,0,1,0,1},
    {1,0,0,0,1,0,0,1,0,1},
    {1,1,1,0,1,0,1,1,0,1},
    {1,0,0,0,0,0,1,0,0,1},
    {1,0,1,1,1,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};

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

    const float targetFrameTime = 1000.0f / 60.0f; // 60帧
    Uint32 lastTicks = SDL_GetTicks();
    float totalTime = 0.0f;

    std::cout << "Init :"<< std::endl;

    // float eye[3] = {0, 0, 5};
    // float center[3] = {0, 0, 0.0f};
    // float up[3] = {0, 1, 0}; 

    float eye[3] = {mazeWidth / 2.0f, mazeHeight / 2.0f, 20.0f};
    float center[3] = {mazeWidth / 2.0f, mazeHeight / 2.0f, 0.0f};  // 看向中心
    float up[3] = {0, 1, 0};

    std::vector<float*> models;


    Camera camera(eye, center, up, aspect, M_PI / 4.0f, 0.1f, 100.0f);

    

    Core::CubeMesh cubeMesh;
    Core::PanelMesh panelMesh;
    Core::SphereMesh sphereMesh;
    std::vector<Core::Instance*> StaticInstances;
    std::vector<Core::Instance*> DynamicInstances;
    std::vector<Core::Instance*> BlockInstances;

    Core::Instance* instance = new Core::PanelInstance(&panelMesh);
    float pos[3] = {mazeHeight/2.f, mazeWidth/2.f, 0.0f};
    float rot[3] = {0, 3.14f/2, 0}; // 只绕Y轴旋转
    float scale[3] = {20, 20, 20}; 
    createModelMatrix1(PanelModel, pos, rot, scale);
    instance->setModelMatrix(PanelModel);
    instance->setColor(0.4f, 0.4f, 0.5f, 1.0f);
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
                instance->setColor(0.7f, 0.7f, 0.3f, 1.0f);
                instance->setEmissive(0.4f, 0.2f, 0.1f, 1.0f);
                instance->setEmissive(0.f, 0.f, 0.f, 0.0f);
                StaticInstances.push_back(instance);
                BlockInstances.push_back(instance); // 墙体实例也加入BlockMap

            }
        }
    }


    //Player:
    Core::Instance* playerInstance = new Core::SphereInstance(&sphereMesh);
    DynamicInstances.push_back(playerInstance);
    float playerPos[3] = {1, 1, 0.0f};
    float playerRot[3] = {0, 0, 0};
    float playerScale[3] = {0.5f, 0.5f, 0.5f}; // 球体半径为0.5
    float* playerModel = new float[16];
    createModelMatrix1(playerModel, playerPos, playerRot, playerScale);
    playerInstance->setModelMatrix(playerModel);
    playerInstance->setColor(1.0f, 0.0f, 0.0f, 1.0f); // 红色球体
    playerInstance->setEmissive(0.7f, 0.4f, 0.2f, 1.0f); // 红色发光

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
        if (input.up) playerPos[1] += 0.1f;
        else if (input.down) playerPos[1] -= 0.1f;
        else if (input.left) playerPos[0] -= 0.1f;
        else if (input.right) playerPos[0] += 0.1f;
    #else
        // 树莓派下用GPIO
        Platform::pollEvents(running);
        if (digitalRead(17) == LOW) playerPos[1] += 0.1f;   // 上
        else if (digitalRead(18) == LOW) playerPos[1] -= 0.1f; // 下
        else if (digitalRead(27) == LOW) playerPos[0] -= 0.1f; // 左
        else if (digitalRead(22) == LOW) playerPos[0] += 0.1f; // 右
    #endif


        


        createModelMatrix1(playerModel, playerPos, playerRot, playerScale);
        playerInstance->setModelMatrix(playerModel);

        // for (int y = 0; y < mazeHeight; ++y) {
        //     for (int x = 0; x < mazeWidth; ++x) {
        //         if (maze[y][x] == 1) { // 是墙
        //             float pos[3] = { (float)x, (float)y, 0.0f}; // y轴向下
        //             float rot[3] = {0, 0, 0};
        //             float scale[3] = {1, 1, 2};
        //             float* model = new float[16];
        //             createModelMatrix1(model, pos, rot, scale);
        //             models.push_back(model);
        //         }
        //     }
        // }
    
        float pos[3] = {mazeHeight/2.f, mazeWidth/2.f, 0.0f};
        float rot[3] = {0, 3.14f/2, 0}; // 只绕Y轴旋转
        float scale[3] = {10, 10, 10};
        createModelMatrix1(model, pos, rot, scale);


        renderer.resize(800, 600);
        //renderer.render(mvp, model);
        //renderer.render(camera.vp, models);
        // 1. 渲染自发光到radianceFBO
        renderer.renderEmissiveToRadianceFBO(camera.vp, DynamicInstances);
        //renderer.renderBlockMap(camera.vp, BlockInstances);
        // 2. radiance扩散（radianceFBO -> blurFBO[0]）
        renderer.renderDiffuseFBO(camera.vp, DynamicInstances);
        renderer.renderStaticInstances(camera.vp, StaticInstances);
        renderer.renderDynamicInstances(camera.vp, DynamicInstances);

        renderer.renderPPGI();
        // renderer.renderPanel(camera.vp, model);
        //renderer.renderEmissiveToRadianceFBO(camera.vp, StaticInstances);
        renderer.OneFrameRenderFinish();
        swapBuffers();

        // 固定帧率
        Uint32 frameEnd = SDL_GetTicks();
        Uint32 frameTime = frameEnd - frameStart;
        if (frameTime < targetFrameTime) {
            SDL_Delay((Uint32)(targetFrameTime - frameTime));
        }
        std::cout << "Frame: " << totalTime << std::endl;
        std::cout << "playerPos: " << playerPos[0] << ", " << playerPos[1] << std::endl;
    }

    renderer.shutdown();
    shutdown();
    return 0;
}


