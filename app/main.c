#include <stdio.h>
#include <stdlib.h>

#include "fi_vk.h"
#include "vk_mesh.h"
#include "vk_pipeline.h"

typedef struct render_thr_arg
{
    vk_ctx* ctx_;
    vk_swapchain* sc_;
    atomic_bool rendering_;
} render_thr_arg;

void* render_thr_func(void* arg)
{
    render_thr_arg* ctx_combo = arg;
    vk_ctx* ctx = ctx_combo->ctx_;
    vk_swapchain* sc = ctx_combo->sc_;
    atomic_bool* rendering = &ctx_combo->rendering_;

    VkFence frame_fence = {};
    VkFenceCreateInfo fence_cinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_cinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(ctx->device_, &fence_cinfo, nullptr, &frame_fence);

    VkSemaphore acquired = {};
    VkSemaphore submitted = {};
    VkSemaphoreCreateInfo sem_cinfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &acquired);
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &submitted);
    VkSemaphoreSubmitInfo waits[1] = {vk_get_sem_info(acquired, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT)};
    VkSemaphoreSubmitInfo signals[1] = {vk_get_sem_info(submitted, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT)};

    VkCommandPool cmd_pool = {};
    VkCommandPoolCreateInfo pool_cinfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_cinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_cinfo.queueFamilyIndex = ctx->queue_idx_;
    vkCreateCommandPool(ctx->device_, &pool_cinfo, nullptr, &cmd_pool);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(ctx->device_, &cmd_alloc, &cmd);
    VkCommandBufferSubmitInfo cmd_submits[1] = {};
    cmd_submits[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_submits[0].commandBuffer = cmd;

    vk_shader* vert = new (vk_shader, ctx, "res/shaders/0.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    vk_shader* frag = new (vk_shader, ctx, "res/shaders/0.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    vk_mesh* mesh = new (vk_mesh, ctx, "test_mesh", to_mb(10), 100);
    vec3 positions[3] = {{-0.5, 0, 0}, {-0.5, 0.5, 0}, {0.5, 0.5, 0}};
    uint32_t idx[3] = {0, 2, 1};
    vk_prim* prim = vk_mesh_add_prim(mesh);
    vk_mesh_add_prim_attrib(mesh, prim, INDEX, idx, 3);
    vk_mesh_add_prim_attrib(mesh, prim, POSITION, positions, 3);
    vk_mesh_alloc_device_mem(mesh, cmd_pool);

    while (atomic_load_explicit(rendering, memory_order_relaxed))
    {
        uint32_t image_idx = -1;

        vkWaitForFences(ctx->device_, 1, &frame_fence, true, UINT64_MAX);
        vk_swapchain_process(sc, cmd_pool, acquired, nullptr, &image_idx);
        vkResetFences(ctx->device_, 1, &frame_fence);

        VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkResetCommandBuffer(cmd, 0);
        vkBeginCommandBuffer(cmd, &begin);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submit.waitSemaphoreInfoCount = 1;
        submit.pWaitSemaphoreInfos = waits;
        submit.signalSemaphoreInfoCount = 1;
        submit.pSignalSemaphoreInfos = signals;
        submit.commandBufferInfoCount = 1;
        submit.pCommandBufferInfos = cmd_submits;
        vkQueueSubmit2(ctx->queue_, 1, &submit, frame_fence);

        VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &submitted;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &sc->swapchain_;
        present_info.pImageIndices = &image_idx;
        vkQueuePresentKHR(ctx->queue_, &present_info);
    }

    vkWaitForFences(ctx->device_, 1, &frame_fence, true, UINT64_MAX);

    vkDestroyFence(ctx->device_, frame_fence, nullptr);
    vkDestroySemaphore(ctx->device_, acquired, nullptr);
    vkDestroySemaphore(ctx->device_, submitted, nullptr);
    vkDestroyCommandPool(ctx->device_, cmd_pool, nullptr);

    delete (vk_shader, vert);
    delete (vk_shader, frag);
    delete (vk_mesh, mesh);
    return nullptr;
}

int main(int argc, char** argv)
{
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    vk_ctx* ctx = new (vk_ctx, WIDTH, HEIGHT, false);
    vk_swapchain* sc = new (vk_swapchain, ctx);
    render_thr_arg render_thr_args = {ctx, sc};
    atomic_init(&render_thr_args.rendering_, true);

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

    delete (vk_swapchain, sc);
    delete (vk_ctx, ctx);
    return 0;
}