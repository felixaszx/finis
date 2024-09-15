#ifndef INCLUDE_VK_H
#define INCLUDE_VK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define GLFW_INCLUDE_VULKAN
#include <cglm/cglm.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "tool.h"

typedef struct
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
} vk_ctx;

#endif // INCLUDE_VK_H