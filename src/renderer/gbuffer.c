#include "renderer/gbuffer.h"

typedef enum default_renderer_atchm_usage
{
    POSITION_ATCHM,
    COLOR_ATCHM,
    NORMAL_ATCHM,
    SPEC_ATCHM,
    DEPTH_STENCIL_ATCHM,
    LIGHT_ATCHM
} default_renderer_atchm_usage;

IMPL_OBJ_NEW(default_renderer, vk_ctx* ctx, dll_handle ext_dll, VkExtent3D atchm_size)
{
    this->ctx_ = ctx;

    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_cinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(ctx->device_, &fence_cinfo, nullptr, this->frame_fences_);

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

    this->cmd_submits_[0] = (VkCommandBufferSubmitInfo){.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    this->cmd_submits_[0].commandBuffer = this->main_cmd_;

    this->pushed_[0].offset = 0;
    this->pushed_[0].size = 16;
    this->pushed_[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

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
                       this->images_ + i, this->image_allocs_ + i, nullptr);
    }

    atchm_cinfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
    atchm_cinfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | //
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info, //
                   this->images_ + DEPTH_STENCIL_ATCHM, this->image_allocs_ + DEPTH_STENCIL_ATCHM, nullptr);

    atchm_cinfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    atchm_cinfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | //
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info, //
                   this->images_ + LIGHT_ATCHM, this->image_allocs_ + LIGHT_ATCHM, nullptr);

    VkImageViewCreateInfo view_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.subresourceRange.layerCount = 1;
    view_info.subresourceRange.levelCount = 1;

    for (size_t i = 0; i < 4; i++)
    {
        view_info.image = this->images_[i];
        view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCreateImageView(ctx->device_, &view_info, nullptr, this->image_views_ + i);
    }

    view_info.image = this->images_[DEPTH_STENCIL_ATCHM];
    view_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    vkCreateImageView(ctx->device_, &view_info, nullptr, this->image_views_ + DEPTH_STENCIL_ATCHM);

    view_info.image = this->images_[LIGHT_ATCHM];
    view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCreateImageView(ctx->device_, &view_info, nullptr, this->image_views_ + LIGHT_ATCHM);

    for (size_t i = 0; i < 4; i++)
    {
        this->atchm_infos_[i] = (VkRenderingAttachmentInfo){.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        this->atchm_infos_[i].clearValue.color = (VkClearColorValue){{0, 0, 0, 1}};
        this->atchm_infos_[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        this->atchm_infos_[i].imageView = this->image_views_[i];
        this->atchm_infos_[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        this->atchm_infos_[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }

    this->atchm_infos_[4] = (VkRenderingAttachmentInfo){.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    this->atchm_infos_[4].clearValue.depthStencil = (VkClearDepthStencilValue){1.0f, 0};
    this->atchm_infos_[4].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    this->atchm_infos_[4].imageView = this->image_views_[DEPTH_STENCIL_ATCHM];
    this->atchm_infos_[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    this->atchm_infos_[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    this->image_infos_[0].imageView = this->image_views_[LIGHT_ATCHM];
    this->image_infos_[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    this->image_infos_[0].sampler = nullptr;

    this->rendering_infos_[0] = (VkRenderingInfo){.sType = VK_STRUCTURE_TYPE_RENDERING_INFO};
    this->rendering_infos_[0].colorAttachmentCount = 4;
    this->rendering_infos_[0].pColorAttachments = this->atchm_infos_;
    this->rendering_infos_[0].renderArea.extent.width = atchm_size.width;
    this->rendering_infos_[0].renderArea.extent.height = atchm_size.height;
    this->rendering_infos_[0].layerCount = 1;
    this->rendering_infos_[0].pDepthAttachment = this->atchm_infos_ + DEPTH_STENCIL_ATCHM;
    this->rendering_infos_[0].pStencilAttachment = this->atchm_infos_ + DEPTH_STENCIL_ATCHM;

    for (size_t i = 0; i < 4; i++)
    {
        this->final_layouts_[i] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    this->final_layouts_[4] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    this->final_layouts_[5] = VK_IMAGE_LAYOUT_GENERAL;

    // layout transition

    VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(this->main_cmd_, &begin);

    for (size_t i = 0; i < 4; i++)
    {
        barrier.image = this->images_[i];
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCmdPipelineBarrier(this->main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    barrier.image = this->images_[DEPTH_STENCIL_ATCHM];
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    vkCmdPipelineBarrier(this->main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    barrier.image = this->images_[LIGHT_ATCHM];
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCmdPipelineBarrier(this->main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(this->main_cmd_);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &this->main_cmd_;

    vkResetFences(ctx->device_, 1, this->frame_fences_);
    vkQueueSubmit(ctx->queue_, 1, &submit, this->frame_fences_[0]);
    return this;
}

IMPL_OBJ_DELETE(default_renderer)
{
    for (size_t i = 0; i < sizeof(this->frame_fences_) / sizeof(this->frame_fences_[0]); i++)
    {
        vkDestroyFence(this->ctx_->device_, this->frame_fences_[i], nullptr);
    }

    for (size_t i = 0; i < sizeof(this->pls_) / sizeof(this->pls_[0]); i++)
    {
        vkDestroyPipeline(this->ctx_->device_, this->pls_[i], nullptr);
        vkDestroyPipelineLayout(this->ctx_->device_, this->pl_layouts_[i], nullptr);
    }

    vkDestroySemaphore(this->ctx_->device_, this->submitted_, nullptr);
    vkDestroyCommandPool(this->ctx_->device_, this->cmd_pool_, nullptr);

    for (size_t i = 0; i < sizeof(this->images_) / sizeof(this->images_[0]); i++)
    {
        vmaDestroyImage(this->ctx_->allocator_, this->images_[i], this->image_allocs_[i]);
        vkDestroyImageView(this->ctx_->device_, this->image_views_[i], nullptr);
    }
}

void default_renderer_render(default_renderer* this, T* data)
{
    vkWaitForFences(this->ctx_->device_, 1, this->frame_fences_, true, UINT64_MAX);
    if (this->frame_begin_cb_)
    {
        this->frame_begin_cb_(this, data);
    }
    vkResetFences(this->ctx_->device_, 1, this->frame_fences_);

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkResetCommandBuffer(this->main_cmd_, 0);
    vkBeginCommandBuffer(this->main_cmd_, &begin);
    vkCmdBeginRendering(this->main_cmd_, this->rendering_infos_);

    vkCmdBindPipeline(this->main_cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pls_[0]);
    VkViewport viewport = {.width = this->rendering_infos_[0].renderArea.extent.width,
                           .height = this->rendering_infos_[0].renderArea.extent.height};
    VkRect2D scissor = {.extent = this->rendering_infos_[0].renderArea.extent};
    vkCmdSetViewport(this->main_cmd_, 0, 1, &viewport);
    vkCmdSetScissor(this->main_cmd_, 0, 1, &scissor);

    if (this->frame_draw_cb_)
    {
        this->frame_draw_cb_(this, data);
    }

    vkCmdEndRendering(this->main_cmd_);

    if (this->frame_end_cb_)
    {
        this->frame_begin_cb_(this, data);
    }
    vkEndCommandBuffer(this->main_cmd_);

    VkSubmitInfo2 submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = this->sem_submits_;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = this->sem_submits_ + 1;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = this->cmd_submits_;
    vkQueueSubmit2(this->ctx_->queue_, 1, &submit, this->frame_fences_[0]);
}

void default_renderer_wait_idle(default_renderer* this)
{
    vkWaitForFences(this->ctx_->device_, 1, this->frame_fences_, true, UINT64_MAX);
}
