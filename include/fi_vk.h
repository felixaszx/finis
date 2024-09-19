#ifndef INCLUDE_VK_H
#define INCLUDE_VK_H

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include <cglm/cglm.h>
#include <cglm/quat.h>
#include <volk.h>
#include <GLFW/glfw3.h>
#include <vma/vk_mem_alloc.h>

#include "tool.h"

#define QUICK_ENUMERATE(type, func, target, count, ptr) \
    type* ptr;                                          \
    func(target, count, nullptr);                       \
    ptr = alloc(type, *count);                          \
    func(target, count, ptr)
#define QUICK_GET(type, func, target, count, ptr) QUICK_ENUMERATE(type, func, target, count, ptr);

typedef struct vk_ctx
{
    VkInstance instance_;
    VkSurfaceKHR surface_;
    VkDevice device_;
    VkPhysicalDevice physical_;
    VkPipelineCache pipeline_cache_;

    VkQueue queue_;
    uint32_t queue_idx_;
    GLFWwindow* win_;
    VmaAllocator allocator_;

    uint32_t width_;
    uint32_t height_;
} vk_ctx;

DEFINE_OBJ(vk_ctx, uint32_t width, uint32_t height, bool full_screen);
bool vk_ctx_update(vk_ctx* ctx);

typedef struct vk_swapchain
{
    VkDevice device_;
    VkSwapchainKHR swapchain_;
    uint32_t image_count_;
    VkImage* images_;
    VkFormat format_;
} vk_swapchain;

DEFINE_OBJ(vk_swapchain, vk_ctx* ctx);

VkSemaphoreSubmitInfo vk_get_sem_info(VkSemaphore sem, VkPipelineStageFlags2 stage);

#endif // INCLUDE_VK_H