#include "proto/render.h"
#include "fi_ext.h"

#include <array>
#include <vector>

struct deffer
{
    vk_ctx* ctx_ = {};
    std::array<VkFence, 1> frame_fences_ = {};

    VkSemaphore submitted_ = {};
    std::array<VkSemaphoreSubmitInfo, 2> sem_submits_ = {};

    VkCommandPool cmd_pool_ = {};
    VkCommandBuffer main_cmd_ = {};
    std::array<VkCommandBufferSubmitInfo, 1> cmd_submits_ = {};

    std::array<VkPushConstantRange, 1> pushed_ = {};
    std::array<vk_gfx_pl_desc, 1> pl_descs_ = {};
    std::array<VkPipelineLayout, 1> pl_layouts_ = {};
    std::array<VkPipeline, 1> pls_ = {};

    std::array<VkImage, 6> images_[6];
    std::array<VkImageView, 6> image_views_ = {};
    std::array<VmaAllocation, 6> image_allocs_ = {};
    std::array<VkImageLayout, 6> final_layouts_ = {};
    std::array<VkRenderingAttachmentInfo, 5> atchm_infos_ = {};
    std::array<VkDescriptorImageInfo, 1> image_infos_ = {};

    std::array<VkRenderingInfo,1> rendering_infos_;

    vk_desc_set_base desc_set_bases_[1];
    VkDescriptorSetLayout desc_set_layouts_[1];
    ~deffer();
};

DLL_EXPORT void draw(fi_mesh_pkg* pkgs)
{
}

DLL_EXPORT void setup(fi_pass_state* state, vk_ctx* ctx, vk_swapchain* sc)
{
}