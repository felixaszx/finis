#include <stdio.h>
#include <stdlib.h>

#include "fi_ext.h"
#include "gfx/gfx.h"
#include "renderer/gbuffer.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

typedef struct render_thr_arg
{
    vk_ctx* ctx_;
    atomic_bool rendering_;
    atomic_long frame_time_;
} render_thr_arg;

typedef struct gbuffer_renderer_arg
{
    vk_swapchain* sc_;
    VkSemaphore acquired_;
    uint32_t image_idx_;
} gbuffer_renderer_arg;

void process_sc(gbuffer_renderer* renderer, T* data)
{
    gbuffer_renderer_arg* args = data;
    vk_swapchain_process(args->sc_, renderer->cmd_pool_, args->acquired_, nullptr, &args->image_idx_);
}

void gbuffer_draw(gbuffer_renderer* renderer, T* data)
{
    vkCmdDraw(renderer->main_cmd_, 3, 1, 0, 0);
}

T* render_thr_func(T* arg)
{
    render_thr_arg* ctx_combo = arg;
    vk_ctx* ctx = ctx_combo->ctx_;
    atomic_bool* rendering = &ctx_combo->rendering_;

    vk_swapchain* sc = new (vk_swapchain, ctx);

    VkSemaphore acquired = {};
    VkSemaphoreCreateInfo sem_cinfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &acquired);

    gbuffer_renderer_arg gbuffer_arg = {.sc_ = sc, .acquired_ = acquired};

    dll_handle default_pl_dll = dlopen("./exts/dlls/gbuffer_pl.dll", RTLD_NOW);
    gbuffer_renderer* gbuffer = new (gbuffer_renderer, ctx, default_pl_dll, (VkExtent3D){WIDTH, HEIGHT, 1});
    gbuffer->cmd_begin_cb_ = process_sc;
    gbuffer->render_cb_ = gbuffer_draw;
    gbuffer->sem_submits_[0] = vk_get_sem_info(acquired, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
    dlclose(default_pl_dll);

    while (atomic_load_explicit(rendering, memory_order_relaxed))
    {
        gbuffer_renderer_render(gbuffer, &gbuffer_arg);

        VkPresentInfoKHR present_info = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &gbuffer->submitted_;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &sc->swapchain_;
        present_info.pImageIndices = &gbuffer_arg.image_idx_;
        vkQueuePresentKHR(ctx->queue_, &present_info);
    }
    gbuffer_renderer_wait_idle(gbuffer);

    vkDestroySemaphore(ctx->device_, acquired, nullptr);
    delete (gbuffer_renderer, gbuffer);
    delete (vk_swapchain, sc);
    return nullptr;
}

int main(int argc, char** argv)
{
    vk_ctx* ctx = new (vk_ctx, WIDTH, HEIGHT, false);
    render_thr_arg render_thr_args = {ctx};
    atomic_init(&render_thr_args.rendering_, true);
    atomic_init(&render_thr_args.frame_time_, 0);

    pthread_t render_thr = {};
    pthread_create(&render_thr, nullptr, render_thr_func, &render_thr_args);

    while (vk_ctx_update(ctx))
    {
        if (glfwGetKey(ctx->win_, GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(ctx->win_, true);
            break;
        }

        if (glfwGetWindowAttrib(ctx->win_, GLFW_ICONIFIED))
        {
            ms_sleep(1);
        }
    }

    atomic_store(&render_thr_args.rendering_, false);
    pthread_join(render_thr, nullptr);

    delete (vk_ctx, ctx);
    return 0;
}