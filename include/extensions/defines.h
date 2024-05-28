#ifndef INCLUDE_EXT_DEFINES_H
#define INCLUDE_EXT_DEFINES_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "vma/vk_mem_alloc.h"

enum QueueType
{
    GRAPHICS_QUEUE_IDX = 0,
    COMPUTE_QUEUE_IDX = 1,
    TRANSFER_QUEUE_IDX = 2
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
    CreateBufferReturn (*create_buffer_)(const ObjectDetails* details, //
                                         const VkBufferCreateInfo* create_info,
                                         const VmaAllocationCreateInfo* alloc_info);
    void (*destory_buffer_)(const ObjectDetails* details, VkBuffer buffer, VmaAllocation alloc);
} BufferFunctions;

typedef struct
{
    VkDeviceSize size_;
    VkImage image_;
    VmaAllocation alloc_;
    VkDeviceMemory memory_;
} CreateImageReturn;

typedef struct
{
    CreateImageReturn (*create_image_)(const ObjectDetails* details,        //
                                       const VkImageCreateInfo* image_info, //
                                       const VmaAllocationCreateInfo* alloc_info);
    void (*destory_image_)(const ObjectDetails* details, VkImage image, VmaAllocation alloc);
} ImageFunctions;

enum AtchmIdx
{
    DEPTH_ATCHM_IDX = 0,
    STENCIL_ATCHM_IDX = 1
};

typedef struct PassChain
{
    uint32_t chain_id_;

    uint32_t image_count_;
    struct PassChain* prev_;
    struct PassChain* next_;

    // setup function must setup following info
    VkRect2D render_area_;
    uint32_t layer_count_;
    void* shared_info_; // lifetime manage by extension

    VkImage* images_;
    VkImageView* image_views_;
    VkRenderingAttachmentInfoKHR* atchm_info_;
    bool depth_atchm_;      // always idx 0
    bool stencil_atchm_;    // always idx 1
    bool get_swapchain_img; // always idx (image_count_ - 1)
} PassChain;

// This extension will connect to Graphics system and Scene management system
typedef struct
{
    uint32_t image_count_;
    void (*init_)(const ObjectDetails* details, PassChain* this_pass);
    void (*clear_)();
    void (*setup_)();
    void (*render_)(VkCommandBuffer cmd);
    void (*finish_)();
} PassStates;

typedef float GlmFloat;
typedef struct
{
    // all array of 3 floats, they are not relative, but absolute
    GlmFloat position_[3];
    GlmFloat rotation_[3];
    GlmFloat scale_[3];
    GlmFloat up_[3];

    GlmFloat right_[3];
    GlmFloat front_[3];
} WorldTransform;

typedef struct
{
    void (*init_)(const ObjectDetails* details, void** shared_data_ptr);
    void (*clear_)();
} SceneManagerStates;

typedef struct SceneNodeFunctions
{
    struct SceneNodeFunctions* parent_;

    void (*set_render_info_)(const ObjectDetails* details, struct SceneNodeFunctions* res);
    void (*set_physic_info_)(); // tbc
} SceneNodeFunctions;

#endif // INCLUDE_EXT_DEFINES_H
