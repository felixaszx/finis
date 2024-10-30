#include "proto/render.h"
#include "fi_ext.h"

#include <array>
#include <vector>

struct deffer
{
    vk_ctx* ctx_ = {};
    vk_swapchain* sc_ = {};
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

    std::array<VkImage, 6> images_ = {};
    std::array<VkImageView, 6> image_views_ = {};
    std::array<VmaAllocation, 6> image_allocs_ = {};
    std::array<VkImageLayout, 6> final_layouts_ = {};
    std::array<VkRenderingAttachmentInfo, 5> atchm_infos_ = {};
    std::array<VkDescriptorImageInfo, 1> image_infos_ = {};

    std::array<VkRenderingInfo, 1> rendering_infos_ = {};

    std::array<vk_desc_set_base, 1> desc_set_bases_ = {};
    std::array<VkDescriptorSetLayout, 1> desc_set_layouts_ = {};

    void setup(fi_pass_state* state, vk_ctx* ctx, vk_swapchain* sc)
    {
        ctx_ = ctx;
        sc_ = sc;
    }

    ~deffer()
    {
        for (auto& set_base : desc_set_bases_)
        {
            vkDestroyDescriptorSetLayout(ctx_->device_, set_base.layout_, nullptr);
            fi_free(set_base.bindings_);
        }

        for (size_t i = 0; i < sizeof(frame_fences_) / sizeof(frame_fences_[0]); i++)
        {
            vkDestroyFence(ctx_->device_, frame_fences_[i], nullptr);
        }

        for (size_t i = 0; i < sizeof(pls_) / sizeof(pls_[0]); i++)
        {
            vkDestroyPipeline(ctx_->device_, pls_[i], nullptr);
            vkDestroyPipelineLayout(ctx_->device_, pl_layouts_[i], nullptr);
        }

        vkDestroySemaphore(ctx_->device_, submitted_, nullptr);
        vkDestroyCommandPool(ctx_->device_, cmd_pool_, nullptr);

        for (size_t i = 0; i < sizeof(images_) / sizeof(images_[0]); i++)
        {
            vmaDestroyImage(ctx_->allocator_, images_[i], image_allocs_[i]);
            vkDestroyImageView(ctx_->device_, image_views_[i], nullptr);
        }
    }
};

static deffer deffer;

DLL_EXPORT void setup(fi_pass_state* state, vk_ctx* ctx, vk_swapchain* sc)
{
    deffer.setup(state, ctx, sc);
}

DLL_EXPORT void draw(fi_mesh_pkg* pkgs)
{
}
