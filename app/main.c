#include <stdio.h>
#include <stdlib.h>

#include "fi_ext.h"
#include "gfx/res_loader.h"
#include "gfx/fi_vk.h"
#include "gfx/vk_mesh.h"
#include "gfx/vk_desc.h"
#include "gfx/vk_pipeline.h"

#include "renderer/default.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

typedef struct default_renderer_arg
{
    vk_swapchain* sc_;
    VkSemaphore acquired_;
    uint32_t image_idx_;
} default_renderer_arg;

void process_sc(default_renderer* renderer, T* data)
{
    default_renderer_arg* args = data;
    vk_swapchain_process(args->sc_, renderer->cmd_pool_, args->acquired_, nullptr, &args->image_idx_);
}

typedef struct render_thr_arg
{
    vk_ctx* ctx_;
    atomic_bool rendering_;
    atomic_long frame_time_;
} render_thr_arg;

T* render_thr_func(T* arg)
{
    render_thr_arg* ctx_combo = arg;
    vk_ctx* ctx = ctx_combo->ctx_;
    atomic_bool* rendering = &ctx_combo->rendering_;

    vk_swapchain* sc = new (vk_swapchain, ctx);

    VkSemaphore acquired = {};
    VkSemaphoreCreateInfo sem_cinfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &acquired);

    default_renderer_arg rrr_args = {.sc_ = sc, .acquired_ = acquired};

    dll_handle default_pl_dll = dlopen("./exts/dlls/default_pl.dll", RTLD_NOW);
    default_renderer* rrr = new (default_renderer, ctx, default_pl_dll, (VkExtent3D){WIDTH, HEIGHT, 1});
    rrr->frame_begin_cb_ = process_sc;
    rrr->sem_submits_[0] = vk_get_sem_info(acquired, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
    dlclose(default_pl_dll);

    while (atomic_load_explicit(rendering, memory_order_relaxed))
    {
        default_renderer_render(rrr, &rrr_args);

        VkPresentInfoKHR present_info = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &rrr->submitted_;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &sc->swapchain_;
        present_info.pImageIndices = &rrr_args.image_idx_;
        vkQueuePresentKHR(ctx->queue_, &present_info);
    }

    default_renderer_wait_idle(rrr);

    vkDestroySemaphore(ctx->device_, acquired, nullptr);

    delete (default_renderer, rrr);
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