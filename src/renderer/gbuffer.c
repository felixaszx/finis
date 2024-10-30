#include "gbuffer.h"

IMPL_OBJ_NEW(gbuffer_renderer, vk_ctx* ctx, dll_handle ext_dll, VkExtent3D atchm_size)
{
    cthis->ctx_ = ctx;

    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_cinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(ctx->device_, &fence_cinfo, nullptr, cthis->frame_fences_);

    VkSemaphoreCreateInfo sem_cinfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &cthis->submitted_);
    cthis->sem_submits_[1] = vk_get_sem_info(cthis->submitted_, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    VkCommandPoolCreateInfo pool_cinfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_cinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_cinfo.queueFamilyIndex = ctx->queue_idx_;
    vkCreateCommandPool(ctx->device_, &pool_cinfo, nullptr, &cthis->cmd_pool_);

    VkCommandBufferAllocateInfo cmd_alloc = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = cthis->cmd_pool_;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(ctx->device_, &cmd_alloc, &cthis->main_cmd_);

    cthis->cmd_submits_[0] = (VkCommandBufferSubmitInfo){.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cthis->cmd_submits_[0].commandBuffer = cthis->main_cmd_;

    cthis->pushed_[0].offset = 0;
    cthis->pushed_[0].size = 4 * sizeof(VkDeviceAddress) + 2 * sizeof(mat4);
    cthis->pushed_[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    cthis->desc_set_bases_[0].binding_count_ = 1;
    cthis->desc_set_bases_[0].bindings_ = fi_alloc(VkDescriptorSetLayoutBinding, 1);
    cthis->desc_set_bases_[0].bindings_[0].descriptorCount = 100;
    cthis->desc_set_bases_[0].bindings_[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cthis->desc_set_bases_[0].bindings_[0].binding = 0;
    cthis->desc_set_bases_[0].bindings_[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    vk_desc_set_base_create_layout(cthis->desc_set_bases_, ctx);
    cthis->desc_set_layouts_[0] = cthis->desc_set_bases_->layout_;

    construct_vk_gfx_pl_desc(cthis->pl_descs_, dlsym(ext_dll, "configurator"), dlsym(ext_dll, "cleaner"));
    cthis->pl_descs_[0].push_range_ = cthis->pushed_;
    cthis->pl_descs_[0].push_range_count_ = sizeof(cthis->pushed_) / sizeof(cthis->pushed_[0]);
    cthis->pl_descs_[0].push_range_->stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    cthis->pl_descs_[0].set_layout_count_ = 1;
    cthis->pl_descs_[0].set_layouts_ = cthis->desc_set_layouts_;
    cthis->pls_[0] = vk_gfx_pl_desc_build(cthis->pl_descs_, ctx, cthis->pl_layouts_);

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
                       cthis->images_ + i, cthis->image_allocs_ + i, nullptr);
    }

    atchm_cinfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
    atchm_cinfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | //
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info, //
                   cthis->images_ + DEPTH_STENCIL_ATCHM, cthis->image_allocs_ + DEPTH_STENCIL_ATCHM, nullptr);

    atchm_cinfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    atchm_cinfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | //
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info, //
                   cthis->images_ + LIGHT_ATCHM, cthis->image_allocs_ + LIGHT_ATCHM, nullptr);

    VkImageViewCreateInfo view_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.subresourceRange.layerCount = 1;
    view_info.subresourceRange.levelCount = 1;

    for (size_t i = 0; i < 4; i++)
    {
        view_info.image = cthis->images_[i];
        view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCreateImageView(ctx->device_, &view_info, nullptr, cthis->image_views_ + i);
    }

    view_info.image = cthis->images_[DEPTH_STENCIL_ATCHM];
    view_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    vkCreateImageView(ctx->device_, &view_info, nullptr, cthis->image_views_ + DEPTH_STENCIL_ATCHM);

    view_info.image = cthis->images_[LIGHT_ATCHM];
    view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCreateImageView(ctx->device_, &view_info, nullptr, cthis->image_views_ + LIGHT_ATCHM);

    for (size_t i = 0; i < 4; i++)
    {
        cthis->atchm_infos_[i] = (VkRenderingAttachmentInfo){.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        cthis->atchm_infos_[i].clearValue.color = (VkClearColorValue){{0, 0, 0, 1}};
        cthis->atchm_infos_[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        cthis->atchm_infos_[i].imageView = cthis->image_views_[i];
        cthis->atchm_infos_[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        cthis->atchm_infos_[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }

    cthis->atchm_infos_[4] = (VkRenderingAttachmentInfo){.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    cthis->atchm_infos_[4].clearValue.depthStencil = (VkClearDepthStencilValue){1.0f, 0};
    cthis->atchm_infos_[4].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    cthis->atchm_infos_[4].imageView = cthis->image_views_[DEPTH_STENCIL_ATCHM];
    cthis->atchm_infos_[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    cthis->atchm_infos_[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    cthis->image_infos_[0].imageView = cthis->image_views_[LIGHT_ATCHM];
    cthis->image_infos_[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    cthis->image_infos_[0].sampler = nullptr;

    cthis->rendering_infos_[0] = (VkRenderingInfo){.sType = VK_STRUCTURE_TYPE_RENDERING_INFO};
    cthis->rendering_infos_[0].colorAttachmentCount = 4;
    cthis->rendering_infos_[0].pColorAttachments = cthis->atchm_infos_;
    cthis->rendering_infos_[0].renderArea.extent.width = atchm_size.width;
    cthis->rendering_infos_[0].renderArea.extent.height = atchm_size.height;
    cthis->rendering_infos_[0].layerCount = 1;
    cthis->rendering_infos_[0].pDepthAttachment = cthis->atchm_infos_ + DEPTH_STENCIL_ATCHM;
    cthis->rendering_infos_[0].pStencilAttachment = cthis->atchm_infos_ + DEPTH_STENCIL_ATCHM;

    for (size_t i = 0; i < 4; i++)
    {
        cthis->final_layouts_[i] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    cthis->final_layouts_[4] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    cthis->final_layouts_[5] = VK_IMAGE_LAYOUT_GENERAL;

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
    vkBeginCommandBuffer(cthis->main_cmd_, &begin);

    for (size_t i = 0; i < 4; i++)
    {
        barrier.image = cthis->images_[i];
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCmdPipelineBarrier(cthis->main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    barrier.image = cthis->images_[DEPTH_STENCIL_ATCHM];
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    vkCmdPipelineBarrier(cthis->main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    barrier.image = cthis->images_[LIGHT_ATCHM];
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCmdPipelineBarrier(cthis->main_cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cthis->main_cmd_);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cthis->main_cmd_;

    vkResetFences(ctx->device_, 1, cthis->frame_fences_);
    vkQueueSubmit(ctx->queue_, 1, &submit, cthis->frame_fences_[0]);
    return cthis;
}

IMPL_OBJ_DELETE(gbuffer_renderer)
{
    for (size_t i = 0; i < sizeof(cthis->desc_set_bases_) / sizeof(cthis->desc_set_bases_[0]); i++)
    {
        vkDestroyDescriptorSetLayout(cthis->ctx_->device_, cthis->desc_set_bases_[i].layout_, nullptr);
        fi_free(cthis->desc_set_bases_[i].bindings_);
    }

    for (size_t i = 0; i < sizeof(cthis->frame_fences_) / sizeof(cthis->frame_fences_[0]); i++)
    {
        vkDestroyFence(cthis->ctx_->device_, cthis->frame_fences_[i], nullptr);
    }

    for (size_t i = 0; i < sizeof(cthis->pls_) / sizeof(cthis->pls_[0]); i++)
    {
        vkDestroyPipeline(cthis->ctx_->device_, cthis->pls_[i], nullptr);
        vkDestroyPipelineLayout(cthis->ctx_->device_, cthis->pl_layouts_[i], nullptr);
    }

    vkDestroySemaphore(cthis->ctx_->device_, cthis->submitted_, nullptr);
    vkDestroyCommandPool(cthis->ctx_->device_, cthis->cmd_pool_, nullptr);

    for (size_t i = 0; i < sizeof(cthis->images_) / sizeof(cthis->images_[0]); i++)
    {
        vmaDestroyImage(cthis->ctx_->allocator_, cthis->images_[i], cthis->image_allocs_[i]);
        vkDestroyImageView(cthis->ctx_->device_, cthis->image_views_[i], nullptr);
    }
}

void gbuffer_renderer_render(gbuffer_renderer* cthis, T* data)
{
    vkWaitForFences(cthis->ctx_->device_, 1, cthis->frame_fences_, true, UINT64_MAX);
    if (cthis->cmd_begin_cb_)
    {
        cthis->cmd_begin_cb_(cthis, data);
    }
    vkResetFences(cthis->ctx_->device_, 1, cthis->frame_fences_);

    vkResetCommandBuffer(cthis->main_cmd_, 0);
    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cthis->main_cmd_, &begin);

    vkCmdBeginRendering(cthis->main_cmd_, cthis->rendering_infos_);
    if (cthis->render_begin_cb_)
    {
        cthis->render_begin_cb_(cthis, data);
    }

    vkCmdBindPipeline(cthis->main_cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, cthis->pls_[0]);
    VkViewport viewport = {.width = cthis->rendering_infos_[0].renderArea.extent.width,
                           .height = cthis->rendering_infos_[0].renderArea.extent.height};
    VkRect2D scissor = {.extent = cthis->rendering_infos_[0].renderArea.extent};
    vkCmdSetViewport(cthis->main_cmd_, 0, 1, &viewport);
    vkCmdSetScissor(cthis->main_cmd_, 0, 1, &scissor);

    if (cthis->render_cb_)
    {
        cthis->render_cb_(cthis, data);
    }
    vkCmdEndRendering(cthis->main_cmd_);

    if (cthis->render_end_cb_)
    {
        cthis->render_end_cb_(cthis, data);
    }
    vkEndCommandBuffer(cthis->main_cmd_);

    VkSubmitInfo2 submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = cthis->sem_submits_;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = cthis->sem_submits_ + 1;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = cthis->cmd_submits_;
    vkQueueSubmit2(cthis->ctx_->queue_, 1, &submit, cthis->frame_fences_[0]);
}

void gbuffer_renderer_wait_idle(gbuffer_renderer* cthis)
{
    vkWaitForFences(cthis->ctx_->device_, 1, cthis->frame_fences_, true, UINT64_MAX);
}