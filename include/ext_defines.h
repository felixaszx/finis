#ifndef INCLUDE_EXT_DEFINES_H
#define INCLUDE_EXT_DEFINES_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "vma/vk_mem_alloc.h"

enum QueueType
{
    GRAPHICS = 0,
    COMPUTE = 1,
    TRANSFER = 2
};

typedef struct
{
    VkInstance instance_;
    VkSurfaceKHR surface_;
    VkDebugUtilsMessengerEXT messenger_;

    VkDevice device_;
    VkPhysicalDevice physical_;
    VkPipelineCache pipeline_cache_;

    VkQueue queues_[3];
    uint32_t queue_indices_[3];

    VmaAllocator allocator_;
    GLFWwindow* window_;
} ObjectDetails;

typedef struct
{
    VkBuffer buffer_;
    VmaAllocation alloc_;
    VkDeviceMemory memory_;
} CreateBufferReturn;

typedef struct
{
    CreateBufferReturn (*create_buffer)(ObjectDetails* details, //
                                        const VkBufferCreateInfo* create_info,
                                        const VmaAllocationCreateInfo* alloc_info);
    void (*destory_buffer)(ObjectDetails* details, VkBuffer buffer, VmaAllocation alloc);
} BufferFunctions;

typedef struct
{
    VkImage buffer_;
    VmaAllocation alloc_;
    VkDeviceMemory memory_;
} CreateImageReturn;

typedef struct
{
    CreateBufferReturn (*create_image)(ObjectDetails* details);
    void (*destory_image)(ObjectDetails* details);
} ImageFunctions;

#endif // INCLUDE_EXT_DEFINES_H
