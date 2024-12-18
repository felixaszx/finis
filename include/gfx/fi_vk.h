#ifndef INCLUDE_FI_VK_H
#define INCLUDE_FI_VK_H

#define GLFW_INCLUDE_VULKAN
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include <cglm/cglm.h>
#include <cglm/quat.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#ifdef __linux__
#include <vk_mem_alloc.h>
#else
#include <vma/vk_mem_alloc.h>
#endif

#include "../fi_tool.h"

#define QUICK_ENUMERATE(type, func, target, count, ptr) \
    type* ptr;                                          \
    func(target, count, fi_nullptr);                       \
    ptr = fi_alloc(type, *count);                          \
    func(target, count, ptr)
#define QUICK_GET(type, func, target, count, ptr) QUICK_ENUMERATE(type, func, target, count, ptr);

typedef versor quat;

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

    sem_t resize_done_;
    sem_t recreate_done_;
    int width_;
    int height_;
} vk_ctx;

DEFINE_OBJ(vk_ctx, uint32_t width, uint32_t height, bool full_screen);
bool vk_ctx_update(vk_ctx* ctx);

typedef struct vk_swapchain
{
    bool vsync_;
    vk_ctx* ctx_;
    VkSwapchainKHR swapchain_;
    uint32_t image_count_;
    VkImage* images_;
    VkFormat format_;
    VkExtent2D extent_;
    VkFence recreate_fence_;
    VkSurfaceFormatKHR surface_format_;
} vk_swapchain;

DEFINE_OBJ(vk_swapchain, vk_ctx* ctx);
bool vk_swapchain_recreate(vk_swapchain* cthis, VkCommandPool cmd_pool);
VkResult vk_swapchain_process(vk_swapchain* cthis,
                          VkCommandPool cmd_pool,
                          VkSemaphore signal,
                          VkFence fence,
                          uint32_t* image_idx);

VkSemaphoreSubmitInfo vk_get_sem_info(VkSemaphore sem, VkPipelineStageFlags2 stage);

#endif // INCLUDE_FI_VK_H