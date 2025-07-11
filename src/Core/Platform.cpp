#include "Core/Platform.h"
#include <SDL.h>
#include <iostream>
#ifdef USE_DESKTOP_GL
#include <glad/glad.h>
#else
#include <GLES2/gl2.h>
#endif

namespace Platform {

static SDL_Window* window = nullptr;
static SDL_GLContext glContext = nullptr;



bool initWindow(int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_StartTextInput();

    // 设置 OpenGL / OpenGL ES 上下文属性
#ifdef USE_DESKTOP_GL
    // Windows: 桌面 OpenGL 3.3 Core Profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#else
    // Raspberry Pi: OpenGL ES 3.0 上下文
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow("Pi Renderer",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        return false;
    }

    // 初始化 GLAD（仅桌面 OpenGL）
#ifdef USE_DESKTOP_GL
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
#endif

    return true;
}

void pollEvents(bool &running, InputState &in) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        std::cout << "Event: " << ev.type << std::endl;
        if (ev.type == SDL_QUIT) {
            running = false;
        }
    }
    // 连续查询键盘状态
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    in.up    = state[SDL_SCANCODE_W];
    in.down  = state[SDL_SCANCODE_S];
    in.left  = state[SDL_SCANCODE_A];
    in.right = state[SDL_SCANCODE_D];
}

// 保持兼容：只有退出检测
void pollEvents(bool &running) {
    InputState dummy;
    pollEvents(running, dummy);
}

void swapBuffers() {
    if (window) {
        SDL_GL_SwapWindow(window);
    }
}

void shutdown() {
    if (glContext) SDL_GL_DeleteContext(glContext);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

} // namespace Platform