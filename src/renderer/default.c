#include "renderer/default.h"

typedef enum default_renderer_atchm_usage
{
    POSITION_ATCHM,
    COLOR_ATCHM,
    NORMAL_ATCHM,
    SPEC_ATCHM,
    LIGHT_ATCHM,
    DEPTH_STENCIL_ATCHM
} default_renderer_atchm_usage;

IMPL_OBJ_NEW(default_renderer, vk_ctx* ctx, dll_handle ext_dll, VkExtent3D atchm_size)
{
    this->ctx_ = ctx;

    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_cinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(ctx->device_, &fence_cinfo, nullptr, &this->frame_fence_);

    VkSemaphoreCreateInfo sem_cinfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &this->submitted_);
    this->sem_submits_[1] = vk_get_sem_info(this->submitted_, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    VkCommandPoolCreateInfo pool_cinfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_cinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_cinfo.queueFamilyIndex = ctx->queue_idx_;
    vkCreateCommandPool(ctx->device_, &pool_cinfo, nullptr, &this->cmd_pool_);

    VkCommandBufferAllocateInfo cmd_alloc = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = this->cmd_pool_;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(ctx->device_, &cmd_alloc, &this->main_cmd_);

    this->cmd_submits_[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    this->cmd_submits_[0].commandBuffer = this->main_cmd_;

    this->pushed_[0].size = 16;

    construct_vk_gfx_pl_desc(this->pl_descs_, dlsym(ext_dll, "configurator"), dlsym(ext_dll, "cleaner"));
    this->pl_descs_[0].push_range_ = this->pushed_;
    this->pl_descs_[0].push_range_count_ = sizeof(this->pushed_) / sizeof(this->pushed_[0]);
    this->pl_descs_[0].push_range_->stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    this->pls_[0] = vk_gfx_pl_desc_build(this->pl_descs_, ctx, this->pl_layouts_);

    VkImageCreateInfo atchm_cinfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
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
                       this->atchms_ + i, this->atchm_allocs_ + i, nullptr);
    }

    atchm_cinfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | //
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info, //
                   this->atchms_ + LIGHT_ATCHM, this->atchm_allocs_ + LIGHT_ATCHM, nullptr);

    atchm_cinfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
    atchm_cinfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | //
                        VK_IMAGE_USAGE_STORAGE_BIT |                  //
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info, //
                   this->atchms_ + DEPTH_STENCIL_ATCHM, this->atchm_allocs_ + DEPTH_STENCIL_ATCHM, nullptr);

    VkImageViewCreateInfo view_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = atchm_cinfo.format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.layerCount = atchm_cinfo.arrayLayers;
    view_info.subresourceRange.levelCount = atchm_cinfo.mipLevels;

    for (size_t i = 0; i < 5; i++)
    {
        view_info.image = this->atchms_[i];
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCreateImageView(ctx->device_, &view_info, nullptr, this->atchm_views_ + i);
    }
    view_info.image = this->atchms_[DEPTH_STENCIL_ATCHM];
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    vkCreateImageView(ctx->device_, &view_info, nullptr, this->atchm_views_ + DEPTH_STENCIL_ATCHM);

    return this;
}

IMPL_OBJ_DELETE(default_renderer)
{
}