#ifndef DEFFER_GBUFFER_H
#define DEFFER_GBUFFER_H

#include "meta.h"

typedef struct gbuffer_pass gbuffer_pass;
typedef void (*gbuffer_pass_cb)(gbuffer_pass* pass, deffer_renderer_state state, T* data);

typedef struct gbuffer_pass
{
    gbuffer_pass_cb cb_;

    vk_gfx_pl_desc pl_descs_[1];
    VkPushConstantRange pushed_[1];
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
} gbuffer_pass;

DEFINE_OBJ(gbuffer_pass, vk_ctx* ctx, dll_handle pl_dll, VkExtent3D atchm_size);

typedef struct deffer_renderer
{
    vk_ctx* ctx_;
    VkFence frame_fences_[1];

    gbuffer_pass* gbuffer_pass_;

    VkSemaphore submitted_;
    VkSemaphoreSubmitInfo sem_submit_infos_[1];

    uint32_t extra_sem_wait_count_;
    VkSemaphoreSubmitInfo* extra_sem_submit_info_;

    VkCommandPool cmd_pool_;
    VkCommandBuffer main_cmd_;
    VkCommandBufferSubmitInfo cmd_submit_info_[1];

    VkDescriptorPool desc_pool_;
    VkDescriptorSet* desc_sets_; // stb_ds::arr
} deffer_renderer;

DEFINE_OBJ(deffer_renderer, vk_ctx* ctx, dll_handle pl_dll, VkExtent3D atchm_size);

#endif // DEFFER_GBUFFER_H