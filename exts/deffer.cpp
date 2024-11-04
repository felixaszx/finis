#include "proto/render.h"
#include "fi_ext.h"

#include <array>
#include <string>
#include <vector>

enum struct ATCHM_USAGE : size_t
{
    POSITION_ATCHM = 0,
    COLOR_ATCHM,
    NORMAL_ATCHM,
    SPEC_ATCHM,
    DEPTH_STENCIL_ATCHM,
    LIGHT_ATCHM
};

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

    void setup(fi_pass_state* state, vk_ctx* ctx, vk_swapchain* sc, const VkExtent3D& atchm_size)
    {
        ctx_ = ctx;
        sc_ = sc;

        VkFenceCreateInfo fence_cinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_cinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(ctx->device_, &fence_cinfo, fi_nullptr, frame_fences_.data());

        VkSemaphoreCreateInfo sem_cinfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(ctx->device_, &sem_cinfo, fi_nullptr, &submitted_);
        sem_submits_[2] = vk_get_sem_info(submitted_, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

        VkCommandPoolCreateInfo pool_cinfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool_cinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_cinfo.queueFamilyIndex = ctx->queue_idx_;
        vkCreateCommandPool(ctx->device_, &pool_cinfo, fi_nullptr, &cmd_pool_);

        VkCommandBufferAllocateInfo cmd_alloc = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmd_alloc.commandPool = cmd_pool_;
        cmd_alloc.commandBufferCount = 1;
        vkAllocateCommandBuffers(ctx->device_, &cmd_alloc, &main_cmd_);

        cmd_submits_[0] = (VkCommandBufferSubmitInfo){VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        cmd_submits_[0].commandBuffer = main_cmd_;

        pushed_[0].offset = 0;
        pushed_[0].size = 4 * sizeof(VkDeviceAddress) + 2 * sizeof(mat4);
        pushed_[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        desc_set_bases_[0].binding_count_ = 1;
        desc_set_bases_[0].bindings_ = fi_alloc(VkDescriptorSetLayoutBinding, 1);
        desc_set_bases_[0].bindings_[0].descriptorCount = 100;
        desc_set_bases_[0].bindings_[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc_set_bases_[0].bindings_[0].binding = 0;
        desc_set_bases_[0].bindings_[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_desc_set_base_create_layout(desc_set_bases_.data(), ctx);
        desc_set_layouts_[0] = desc_set_bases_[0].layout_;

        std::string pl_dll_name = get_shared_lib_name("./ext_dlls/gbuffer_pl");
        dll_handle pl_dll = dlopen(pl_dll_name.c_str(), RTLD_NOW);
        construct_vk_gfx_pl_desc(pl_descs_.data(), //
                                 (vk_gfx_pl_configurator)dlsym(pl_dll, "configurator"),
                                 (vk_gfx_pl_cleaner)dlsym(pl_dll, "cleaner"));

        pl_descs_[0].push_range_ = pushed_.data();
        pl_descs_[0].push_range_count_ = sizeof(pushed_) / sizeof(pushed_[0]);
        pl_descs_[0].push_range_->stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pl_descs_[0].set_layout_count_ = 1;
        pl_descs_[0].set_layouts_ = desc_set_layouts_.data();
        pls_[0] = vk_gfx_pl_desc_build(pl_descs_.data(), ctx, pl_layouts_.data());
        dlclose(pl_dll);

        VkImageCreateInfo atchm_cinfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        atchm_cinfo.imageType = VK_IMAGE_TYPE_2D;
        atchm_cinfo.arrayLayers = 1;
        atchm_cinfo.mipLevels = 1;
        atchm_cinfo.samples = VK_SAMPLE_COUNT_1_BIT;
        atchm_cinfo.extent = atchm_size;
        atchm_cinfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        atchm_cinfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | //
                            VK_IMAGE_USAGE_STORAGE_BIT |          //
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo alloc_info = {.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE};

        for (size_t i = 0; i < 4; i++)
        {
            vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info, //
                           &images_[i], &image_allocs_[i], fi_nullptr);
        }

        atchm_cinfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
        atchm_cinfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | //
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info,                            //
                       &images_[static_cast<size_t>(ATCHM_USAGE::DEPTH_STENCIL_ATCHM)],       //
                       &image_allocs_[static_cast<size_t>(ATCHM_USAGE::DEPTH_STENCIL_ATCHM)], //
                       fi_nullptr);

        atchm_cinfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        atchm_cinfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | //
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info,                    //
                       &images_[static_cast<size_t>(ATCHM_USAGE::LIGHT_ATCHM)],       //
                       &image_allocs_[static_cast<size_t>(ATCHM_USAGE::LIGHT_ATCHM)], //
                       fi_nullptr);

        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.subresourceRange.layerCount = 1;
        view_info.subresourceRange.levelCount = 1;

        for (size_t i = 0; i < 4; i++)
        {
            view_info.image = images_[i];
            view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vkCreateImageView(ctx->device_, &view_info, fi_nullptr, &image_views_[i]);
        }

        view_info.image = images_[static_cast<size_t>(ATCHM_USAGE::DEPTH_STENCIL_ATCHM)];
        view_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        vkCreateImageView(ctx->device_, &view_info, fi_nullptr,
                          &image_views_[static_cast<size_t>(ATCHM_USAGE::DEPTH_STENCIL_ATCHM)]);

        view_info.image = images_[static_cast<size_t>(ATCHM_USAGE::LIGHT_ATCHM)];
        view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCreateImageView(ctx->device_, &view_info, fi_nullptr,
                          &image_views_[static_cast<size_t>(ATCHM_USAGE::LIGHT_ATCHM)]);

        for (size_t i = 0; i < 4; i++)
        {
            atchm_infos_[i] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            atchm_infos_[i].clearValue.color = (VkClearColorValue){{0, 0, 0, 1}};
            atchm_infos_[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            atchm_infos_[i].imageView = image_views_[i];
            atchm_infos_[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            atchm_infos_[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }

        atchm_infos_[4] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        atchm_infos_[4].clearValue.depthStencil = (VkClearDepthStencilValue){1.0f, 0};
        atchm_infos_[4].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        atchm_infos_[4].imageView = image_views_[static_cast<size_t>(ATCHM_USAGE::DEPTH_STENCIL_ATCHM)];
        atchm_infos_[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        atchm_infos_[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        image_infos_[0].imageView = image_views_[static_cast<size_t>(ATCHM_USAGE::LIGHT_ATCHM)];
        image_infos_[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_infos_[0].sampler = fi_nullptr;

        rendering_infos_[0] = (VkRenderingInfo){VK_STRUCTURE_TYPE_RENDERING_INFO};
        rendering_infos_[0].colorAttachmentCount = 4;
        rendering_infos_[0].pColorAttachments = atchm_infos_.data();
        rendering_infos_[0].renderArea.extent.width = atchm_size.width;
        rendering_infos_[0].renderArea.extent.height = atchm_size.height;
        rendering_infos_[0].layerCount = 1;
        rendering_infos_[0].pDepthAttachment = &atchm_infos_[static_cast<size_t>(ATCHM_USAGE::DEPTH_STENCIL_ATCHM)];
        rendering_infos_[0].pStencilAttachment = &atchm_infos_[static_cast<size_t>(ATCHM_USAGE::DEPTH_STENCIL_ATCHM)];

        for (size_t i = 0; i < 4; i++)
        {
            final_layouts_[i] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        final_layouts_[4] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        final_layouts_[5] = VK_IMAGE_LAYOUT_GENERAL;

        // layout transition

        VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(main_cmd_, &begin);

        for (size_t i = 0; i < 4; i++)
        {
            barrier.image = images_[i];
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vkCmdPipelineBarrier(main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                                 0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);
        }

        barrier.image = images_[static_cast<size_t>(ATCHM_USAGE::DEPTH_STENCIL_ATCHM)];
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        vkCmdPipelineBarrier(main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                             0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);

        barrier.image = images_[static_cast<size_t>(ATCHM_USAGE::LIGHT_ATCHM)];
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCmdPipelineBarrier(main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                             0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);

        vkEndCommandBuffer(main_cmd_);

        VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &main_cmd_;

        vkResetFences(ctx->device_, 1, frame_fences_.data());
        vkQueueSubmit(ctx->queue_, 1, &submit, frame_fences_[0]);
    }

    ~deffer()
    {
        for (auto& set_base : desc_set_bases_)
        {
            vkDestroyDescriptorSetLayout(ctx_->device_, set_base.layout_, fi_nullptr);
            fi_free(set_base.bindings_);
        }

        for (size_t i = 0; i < sizeof(frame_fences_) / sizeof(frame_fences_[0]); i++)
        {
            vkDestroyFence(ctx_->device_, frame_fences_[i], fi_nullptr);
        }

        for (size_t i = 0; i < sizeof(pls_) / sizeof(pls_[0]); i++)
        {
            vkDestroyPipeline(ctx_->device_, pls_[i], fi_nullptr);
            vkDestroyPipelineLayout(ctx_->device_, pl_layouts_[i], fi_nullptr);
        }

        vkDestroySemaphore(ctx_->device_, submitted_, fi_nullptr);
        vkDestroyCommandPool(ctx_->device_, cmd_pool_, fi_nullptr);

        for (size_t i = 0; i < sizeof(images_) / sizeof(images_[0]); i++)
        {
            vmaDestroyImage(ctx_->allocator_, images_[i], image_allocs_[i]);
            vkDestroyImageView(ctx_->device_, image_views_[i], fi_nullptr);
        }
    }
};

static deffer deffer;

FI_DLL_EXPORT void setup(fi_pass_state* state, vk_ctx* ctx, vk_swapchain* sc, VkExtent3D atchm_size)
{
    deffer.setup(state, ctx, sc, atchm_size);
}

FI_DLL_EXPORT void draw(fi_mesh_pkg* pkgs)
{
}
