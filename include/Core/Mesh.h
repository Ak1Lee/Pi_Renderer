#pragma once

#ifdef USE_DESKTOP_GL
#include <glad/glad.h>
#else

#include <GLES2/gl2.h>

#endif
#include <vector>
#include <cstddef> // For std::size_t

namespace Core
{
class Mesh {
public:
    virtual void draw() = 0;
    virtual ~Mesh() {}
    // 可以加一些通用属性和方法
};
} // namespace core
