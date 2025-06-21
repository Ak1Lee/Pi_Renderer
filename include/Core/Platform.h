#pragma once

namespace Platform {

// 初始化窗口与 OpenGL 上下文，返回是否成功
bool initWindow(int width, int height);
// 轮询事件，修改 running 标志
void pollEvents(bool &running);
// 交换前后缓冲区，显示渲染结果
void swapBuffers();
// 清理 SDL 和 GL 资源
void shutdown();

} // namespace Platform