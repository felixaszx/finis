#include <stdio.h>
#include <stdlib.h>

#include "fi_vk.h"
#include "vk_mesh.h"

int main(int argc, char** argv)
{
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    vk_ctx* ctx = new (vk_ctx, WIDTH, HEIGHT, false);
    vk_swapchain* sc = new (vk_swapchain, ctx);

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

    vk_mesh* mesh = new (vk_mesh, ctx, "test_mesh", to_mb(10), 100);
    vk_tex_arr* tex_arr = new (vk_tex_arr, ctx, 10, 10);

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
            continue;
        }

        uint32_t image_idx_ = -1;

        vkWaitForFences(ctx->device_, 1, &frame_fence, true, UINT64_MAX);
        vkAcquireNextImageKHR(ctx->device_, sc->swapchain_, UINT64_MAX, acquired, nullptr, &image_idx_);
        vkResetFences(ctx->device_, 1, &frame_fence);

        VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkResetCommandBuffer(cmd, 0);
        vkBeginCommandBuffer(cmd, &begin);
        vkEndCommandBuffer(cmd);

        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
        VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &acquired;
        submit.pWaitDstStageMask = wait_stages;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &submitted;
        vkQueueSubmit(ctx->queue_, 1, &submit, frame_fence);

        VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &submitted;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &sc->swapchain_;
        present_info.pImageIndices = &image_idx_;
        vkQueuePresentKHR(ctx->queue_, &present_info);
    }
    vkWaitForFences(ctx->device_, 1, &frame_fence, true, UINT64_MAX);

    vkDestroyFence(ctx->device_, frame_fence, nullptr);
    vkDestroySemaphore(ctx->device_, acquired, nullptr);
    vkDestroySemaphore(ctx->device_, submitted, nullptr);
    vkDestroyCommandPool(ctx->device_, cmd_pool, nullptr);
    delete (vk_mesh, mesh);
    delete (vk_tex_arr, tex_arr);
    delete (vk_swapchain, sc);
    delete (vk_ctx, ctx);
    return 0;
}