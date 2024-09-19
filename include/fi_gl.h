#ifndef INCLUDE_GL_H
#define INCLUDE_GL_H

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include <cglm/cglm.h>
#include "third_party/glad.h"
#include <GLFW/glfw3.h>

#include "tool.h"

typedef struct gl_ctx
{
    GLFWwindow* win_;
    uint32_t width_;
    uint32_t height_;
} gl_ctx;

DEFINE_OBJ(gl_ctx, uint32_t width, uint32_t height, bool full_screen);
DEFINE_OBJ_DELETE(gl_ctx);
bool gl_ctx_update(gl_ctx* ctx);

#endif // INCLUDE_GL_H