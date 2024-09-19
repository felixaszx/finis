#include "fi_gl.h"

IMPL_OBJ_NEW(gl_ctx, uint32_t width, uint32_t height, bool full_screen)
{
    this->width_ = width;
    this->height_ = height;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    if (full_screen)
    {
        this->win_ = glfwCreateWindow(width, height, "", glfwGetPrimaryMonitor(), nullptr);
    }
    else
    {
        this->win_ = glfwCreateWindow(width, height, "", nullptr, nullptr);
    }
    glfwMakeContextCurrent(this->win_);
    gladLoadGLLoader(((GLADloadproc)glfwGetProcAddress));
    glViewport(0, 0, 800, 600);

    return this;
}

IMPL_OBJ_DELETE(gl_ctx)
{
    glfwTerminate();
}

bool gl_ctx_update(gl_ctx* ctx)
{
    glfwPollEvents();
    glfwSwapBuffers(ctx->win_);
    return !glfwWindowShouldClose(ctx->win_);
}