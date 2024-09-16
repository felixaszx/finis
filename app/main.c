#include <stdio.h>
#include <stdlib.h>

#include "vk.h"
#include "vk_mesh.h"

int main(int argc, char** argv)
{
    const uint32_t WIDTH = 1920;
    const uint32_t HEIGHT = 1080;

    vk_ctx ctx = {};
    init_vk_ctx(&ctx, WIDTH, HEIGHT);
    vk_swapchain sc = {};
    init_vk_swapchain(&sc, &ctx, (VkExtent2D){WIDTH, HEIGHT});

    VkFence frame_fence = {};
    VkFenceCreateInfo fence_cinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_cinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(ctx.device_, &fence_cinfo, nullptr, &frame_fence);

    VkSemaphore acquired = {};
    VkSemaphore submitted = {};
    VkSemaphoreCreateInfo sem_cinfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx.device_, &sem_cinfo, nullptr, &acquired);
    vkCreateSemaphore(ctx.device_, &sem_cinfo, nullptr, &submitted);
    VkSemaphoreSubmitInfo waits[1] = {get_vk_sem_info(acquired, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT)};
    VkSemaphoreSubmitInfo signals[1] = {get_vk_sem_info(submitted, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT)};

    VkCommandPool cmd_pool = {};
    VkCommandPoolCreateInfo pool_cinfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_cinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_cinfo.queueFamilyIndex = ctx.queue_idx_;
    vkCreateCommandPool(ctx.device_, &pool_cinfo, nullptr, &cmd_pool);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = cmd_pool;
    alloc_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(ctx.device_, &alloc_info, &cmd);
    VkCommandBufferSubmitInfo cmd_submits[1] = {};
    cmd_submits[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_submits[0].commandBuffer = cmd;

    vk_mesh* mesh = new (vk_mesh, &ctx, to_mb(10));

    while (vk_ctx_update(&ctx))
    {
        if (glfwGetWindowAttrib(ctx.win_, GLFW_ICONIFIED))
        {
            ms_sleep(1);
            continue;
        }

        uint32_t image_idx_ = -1;

        vkWaitForFences(ctx.device_, 1, &frame_fence, true, UINT64_MAX);
        vkAcquireNextImageKHR(ctx.device_, sc.swapchain_, UINT64_MAX, acquired, nullptr, &image_idx_);
        vkResetFences(ctx.device_, 1, &frame_fence);

        VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkResetCommandBuffer(cmd, 0);
        vkBeginCommandBuffer(cmd, &begin);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submit.commandBufferInfoCount = 1;
        submit.pCommandBufferInfos = cmd_submits;
        submit.waitSemaphoreInfoCount = 1;
        submit.pWaitSemaphoreInfos = waits;
        submit.signalSemaphoreInfoCount = 1;
        submit.pSignalSemaphoreInfos = signals;
        vkQueueSubmit2(ctx.queue_, 1, &submit, frame_fence);

        VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &submitted;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &sc.swapchain_;
        present_info.pImageIndices = &image_idx_;
        vkQueuePresentKHR(ctx.queue_, &present_info);
    }
    vkWaitForFences(ctx.device_, 1, &frame_fence, true, UINT64_MAX);

    vkDestroyFence(ctx.device_, frame_fence, nullptr);
    vkDestroySemaphore(ctx.device_, acquired, nullptr);
    vkDestroySemaphore(ctx.device_, submitted, nullptr);
    vkDestroyCommandPool(ctx.device_, cmd_pool, nullptr);
    release_vk_swapchain(&sc);
    release_vk_ctx(&ctx);
    return 0;
}