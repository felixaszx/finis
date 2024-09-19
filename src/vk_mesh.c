#include "vk_mesh.h"

void vk_mesh_free_staging(vk_mesh* this)
{
    if (this->staging_alloc_)
    {
        vmaDestroyBuffer(this->ctx_->allocator_, this->staging_, this->staging_alloc_);
        this->mapping_ = nullptr;
        this->staging_ = nullptr;
        this->staging_alloc_ = nullptr;
    }
}

void vk_mesh_flush_staging(vk_mesh* this)
{
    vmaFlushAllocation(this->ctx_->allocator_, this->staging_alloc_, 0, this->mem_size_);
}

void vk_mesh_alloc_device_mem(vk_mesh* this, VkCommandPool pool)
{
    if (this->buffer_)
    {
        return;
    }

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = this->mem_size_ + this->prim_size_ * (sizeof(VkDrawIndirectCommand) + sizeof(vk_prim));
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |        //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateBuffer(this->ctx_->allocator_, &buffer_info, &alloc_info, &this->buffer_, &this->alloc_, nullptr);

    VkBufferDeviceAddressInfo address_info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    address_info.buffer = this->buffer_;
    this->address_ = vkGetBufferDeviceAddress(this->ctx_->device_, &address_info);

    this->dc_offset_ = this->mem_size_;
    for (size_t p = 0; p < this->prim_size_; p++)
    {
        this->draw_calls_[p].instanceCount = 1;
        this->draw_calls_[p].vertexCount = this->prims_[p].attrib_counts_[INDEX];
        memcpy(this->mapping_ + this->mem_size_, this->draw_calls_ + p, sizeof(VkDrawIndirectCommand));
        this->mem_size_ += sizeof(VkDrawIndirectCommand);
        memcpy(this->mapping_ + this->mem_size_, this->prims_ + p, sizeof(vk_prim));
        this->mem_size_ += sizeof(vk_prim);
    }

    // copy buffer
    vk_mesh_flush_staging(this);

    VkFence fence = {};
    VkFenceCreateInfo fence_cinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(this->ctx_->device_, &fence_cinfo, nullptr, &fence);
    vkResetFences(this->ctx_->device_, 1, &fence);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(this->ctx_->device_, &cmd_alloc, &cmd);

    VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy region = {0, 0, this->mem_size_};
    vkCmdCopyBuffer(cmd, this->staging_, this->buffer_, 1, &region);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(this->ctx_->queue_, 1, &submit, fence);
    vkWaitForFences(this->ctx_->device_, 1, &fence, true, UINT64_MAX);

    vkDestroyFence(this->ctx_->device_, fence, nullptr);
    vkFreeCommandBuffers(this->ctx_->device_, pool, 1, &cmd);
    vk_mesh_free_staging(this);
    this->prim_limit_ = 0;
    this->mem_limit_ = 0;
}

void vk_mesh_draw_prims(vk_mesh* this, VkCommandBuffer cmd)
{
    vkCmdDrawIndirect(cmd, this->buffer_, this->dc_offset_, this->prim_size_,
                      sizeof(VkDrawIndirectCommand) + sizeof(vk_prim));
}

IMPL_OBJ_NEW(vk_mesh, vk_ctx* ctx, const char* name, VkDeviceSize mem_limit, uint32_t prim_limit)
{
    this->ctx_ = ctx;
    this->mem_limit_ = mem_limit - prim_limit * (sizeof(VkDrawIndirectCommand) + sizeof(vk_prim));
    this->prim_limit_ = prim_limit;
    strcpy_s(this->name_, sizeof(this->name_), name);

    this->prims_ = alloc(vk_prim, prim_limit);
    this->draw_calls_ = alloc(VkDrawIndirectCommand, prim_limit);
    for (size_t i = 0; i < prim_limit; i++)
    {
        construct_vk_prim(this->prims_ + i);
    }

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = mem_limit;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | //
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
    alloc_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    VmaAllocationInfo allocated = {};
    vmaCreateBuffer(ctx->allocator_, &buffer_info, &alloc_info, &this->staging_, &this->staging_alloc_, &allocated);
    this->mapping_ = allocated.pMappedData;

    return this;
}

IMPL_OBJ_DELETE(vk_mesh)
{
    vk_mesh_free_staging(this);
    if (this->alloc_)
    {
        vmaDestroyBuffer(this->ctx_->allocator_, this->buffer_, this->alloc_);
    }
    ffree(this->prims_);
    ffree(this->draw_calls_);
}

vk_prim* vk_mesh_add_prim(vk_mesh* this)
{
    if (this->prim_size_ >= this->prim_limit_)
    {
        return nullptr;
    }

    this->prim_size_++;
    return this->prims_ + this->prim_size_ - 1;
}

void vk_mesh_add_prim_attrib(vk_mesh* this, vk_prim* prim, vk_prim_attrib attrib, void* data, size_t count)
{
    prim->attrib_counts_[attrib] = count;
    size_t data_size = vk_prim_get_attrib_size(prim, attrib);
    if (this->mem_size_ + data_size >= this->mem_limit_)
    {
        return;
    }

    prim->attrib_offsets_[attrib] = this->mem_size_;
    memcpy(this->mapping_ + this->mem_size_, data, data_size);
    this->mem_size_ += data_size;
}

IMPL_OBJ_NEW(vk_tex_arr, vk_ctx* ctx, uint32_t tex_limit, uint32_t sampler_limit)
{
    this->ctx_ = ctx;

    this->tex_limit_ = tex_limit;
    this->texs_ = alloc(VkImage, tex_limit);
    this->views_ = alloc(VkImageView, tex_limit);
    this->allocs_ = alloc(VmaAllocation, tex_limit);
    this->desc_infos_ = alloc(VkDescriptorImageInfo, tex_limit);

    this->sampler_limit_ = sampler_limit;
    this->samplers_ = alloc(VkSampler, sampler_limit);

    return this;
}

IMPL_OBJ_DELETE(vk_tex_arr)
{
    for (size_t i = 0; i < this->tex_size_; i++)
    {
        vkDestroyImageView(this->ctx_->device_, this->views_[i], nullptr);
        vmaDestroyImage(this->ctx_->allocator_, this->texs_[i], this->allocs_[i]);
    }

    for (size_t i = 0; i < this->sampler_size_; i++)
    {
        vkDestroySampler(this->ctx_->device_, this->samplers_[i], nullptr);
    }

    ffree(this->texs_);
    ffree(this->views_);
    ffree(this->allocs_);
    ffree(this->desc_infos_);
    ffree(this->samplers_);
}

bool vk_tex_arr_add_sampler(vk_tex_arr* this, VkSamplerCreateInfo* sampler_info)
{
    if (this->sampler_size_ >= this->sampler_limit_)
    {
        return false;
    }

    vkCreateSampler(this->ctx_->device_, sampler_info, nullptr, this->samplers_ + this->sampler_size_);

    this->sampler_size_++;
    return true;
}

bool vk_tex_arr_add_tex(vk_tex_arr* this,
                        VkCommandPool cmd_pool,
                        uint32_t sampler_idx,
                        byte* data,
                        size_t size,
                        const VkExtent3D* extent,
                        const VkImageSubresource* sub_res)
{
    if (this->tex_size_ >= this->tex_limit_)
    {
        return false;
    }

    VkImageCreateInfo tex_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    tex_info.imageType = VK_IMAGE_TYPE_2D;
    tex_info.arrayLayers = sub_res->arrayLayer;
    tex_info.mipLevels = sub_res->mipLevel;
    tex_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    tex_info.extent = *extent;
    tex_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateImage(this->ctx_->allocator_, &tex_info, &alloc_info, //
                   this->texs_ + this->tex_size_, this->allocs_ + this->tex_size_, nullptr);

    VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.image = this->texs_[this->tex_size_];
    view_info.format = tex_info.format;
    view_info.subresourceRange.aspectMask = sub_res->aspectMask;
    view_info.subresourceRange.layerCount = tex_info.arrayLayers;
    view_info.subresourceRange.levelCount = tex_info.mipLevels;
    vkCreateImageView(this->ctx_->device_, &view_info, nullptr, this->views_ + this->tex_size_);

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VmaAllocationInfo allocated = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    alloc_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    VkBuffer staging = {};
    VmaAllocation staging_alloc = {};
    vmaCreateBuffer(this->ctx_->allocator_, &buffer_info, &alloc_info, &staging, &staging_alloc, &allocated);
    memcpy(allocated.pMappedData, data, size);
    vmaFlushAllocation(this->ctx_->allocator_, staging_alloc, 0, size);

    // manipulate the imageVkFence fence = {};
    VkFence fence = {};
    VkFenceCreateInfo fence_cinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(this->ctx_->device_, &fence_cinfo, nullptr, &fence);
    vkResetFences(this->ctx_->device_, 1, &fence);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(this->ctx_->device_, &cmd_alloc, &cmd);

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = this->texs_[this->tex_size_];
    barrier.subresourceRange.aspectMask = sub_res->aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = sub_res->mipLevel;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = sub_res->arrayLayer;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = sub_res->aspectMask;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = *extent;

    VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkCmdCopyBufferToImage(cmd, staging, this->texs_[this->tex_size_], barrier.newLayout, 1, &region);

    uint32_t mip_w = extent->width;
    uint32_t mip_h = extent->height;
    for (size_t i = 1; i < sub_res->mipLevel; i++)
    {
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = barrier.newLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.subresourceRange.baseMipLevel = i - 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[1] = (VkOffset3D){mip_w, mip_h, 1};
        blit.srcSubresource.aspectMask = sub_res->aspectMask;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[1] = (VkOffset3D){mip_w > 1 ? mip_w / 2 : 1, //
                                          mip_h > 1 ? mip_h / 2 : 1, //
                                          1};
        blit.dstSubresource.aspectMask = sub_res->aspectMask;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        vkCmdBlitImage(cmd,                                                 //
                       barrier.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //
                       barrier.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //
                       1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = barrier.newLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.subresourceRange.baseMipLevel = i - 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        mip_w > 1 ? mip_w /= 2 : mip_w;
        mip_h > 1 ? mip_h /= 2 : mip_h;
    }
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.subresourceRange.baseMipLevel = sub_res->mipLevel - 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(this->ctx_->queue_, 1, &submit, fence);
    vkWaitForFences(this->ctx_->device_, 1, &fence, true, UINT64_MAX);

    vkDestroyFence(this->ctx_->device_, fence, nullptr);
    vkFreeCommandBuffers(this->ctx_->device_, cmd_pool, 1, &cmd);

    VkDescriptorImageInfo* desc_info = this->desc_infos_ + this->tex_size_;
    desc_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc_info->imageView = this->views_[this->tex_size_];
    desc_info->sampler = this->samplers_[sampler_idx];
    vmaDestroyBuffer(this->ctx_->allocator_, staging, staging_alloc);
    this->tex_size_++;
    return true;
}

IMPL_OBJ_NEW(vk_mesh_desc, uint32_t node_limit)
{
    return this;
}

IMPL_OBJ_DELETE(vk_mesh_desc)
{
    for (uint32_t i = 0; i < this->layer_count_; i++)
    {
        ffree(this->layers_[i]);
    }
    ffree(this->layer_sizes_);
    ffree(this->output_);
}

uint32_t vk_mesh_desc_get_node_size(vk_mesh_desc* this)
{
    uint32_t size = 0;
    for (uint32_t i = 0; i < this->layer_count_; i++)
    {
        size += this->layer_sizes_[i];
    }
    return size;
}