#ifndef RENDERER_GBUFFER_H
#define RENDERER_GBUFFER_H

#include "fi_ext.h"
#include "gfx/gfx.h"

typedef enum gbuffer_renderer_atchm_usage
{
    POSITION_ATCHM,
    COLOR_ATCHM,
    NORMAL_ATCHM,
    SPEC_ATCHM,
    DEPTH_STENCIL_ATCHM,
    LIGHT_ATCHM
} gbuffer_renderer_atchm_usage;

typedef struct gbuffer_renderer gbuffer_renderer;
typedef void (*gbuffer_render_cb)(gbuffer_renderer* renderer, T* data);

struct gbuffer_renderer
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

    vk_desc_set_base desc_set_bases_[1];
    VkDescriptorSetLayout desc_set_layouts_[1];

    gbuffer_render_cb cmd_begin_cb_;
    gbuffer_render_cb render_cb_;
    gbuffer_render_cb render_begin_cb_;
    gbuffer_render_cb render_end_cb_;
};

DEFINE_OBJ(gbuffer_renderer, vk_ctx* ctx, dll_handle pl_dll, VkExtent3D atchm_size);
void gbuffer_renderer_render(gbuffer_renderer* this, T* data);
void gbuffer_renderer_wait_idle(gbuffer_renderer* this);

#endif // RENDERER_GBUFFER_H