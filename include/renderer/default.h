#ifndef RENDERER_DEFAULT_H
#define RENDERER_DEFAULT_H

#include "fi_vk.h"
#include "fi_ext.h"
#include "vk_pipeline.h"

typedef struct default_renderer default_renderer;
typedef void (*default_renderer_cb)(default_renderer* renderer, T* data);

struct default_renderer
{
    vk_ctx* ctx_;
    VkFence frame_fences_[1];

    VkSemaphore submitted_;
    VkSemaphoreSubmitInfo sem_submits_[2];

    VkCommandPool cmd_pool_;
    VkCommandBuffer main_cmd_;
    VkCommandBufferSubmitInfo cmd_submits_[1];

    VkPushConstantRange pushed_[1];
    vk_gfx_pl_desc pl_descs_[1];
    VkPipelineLayout pl_layouts_[1];
    VkPipeline pls_[1];

    VkImage images_[6];
    VkImageView image_views_[6];
    VmaAllocation image_allocs_[6];
    VkImageLayout final_layouts_[6];
    VkRenderingAttachmentInfo atchm_infos_[5];
    VkDescriptorImageInfo image_infos_[1];

    VkRenderingInfo rendering_infos_[1];

    default_renderer_cb frame_end_cb_;
    default_renderer_cb frame_begin_cb_;
    default_renderer_cb frame_draw_cb_;
};

DEFINE_OBJ(default_renderer, vk_ctx* ctx, dll_handle ext_dll, VkExtent3D atchm_size);

void default_renderer_render(default_renderer* this, T* data);
void default_renderer_wait_idle(default_renderer* this);

#endif // RENDERER_DEFAULT_H