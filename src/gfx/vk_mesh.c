#include "vk_mesh.h"

IMPL_OBJ_NEW(vk_mesh, vk_ctx* ctx, const char* name, VkDeviceSize mem_limit, uint32_t prim_limit)
{
    this->ctx_ = ctx;
    this->mem_limit_ = mem_limit - prim_limit * (sizeof(*this->draw_calls_) + sizeof(*this->prims_));
    this->prim_limit_ = prim_limit;
    strncpy(this->name_, name, sizeof(this->name_));

    this->prims_ = alloc(vk_prim, prim_limit);
    this->draw_calls_ = alloc(VkDrawIndirectCommand, prim_limit);
    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
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

void vk_mesh_alloc_device_mem(vk_mesh* this, VkCommandPool pool)
{
    if (this->buffer_)
    {
        return;
    }

    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = this->mem_size_ + this->prim_count_ * (sizeof(*this->draw_calls_) + sizeof(*this->prims_));
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |        //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | //
                        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |       //
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    vmaCreateBuffer(this->ctx_->allocator_, &buffer_info, &alloc_info, &this->buffer_, &this->alloc_, nullptr);

    VkBufferDeviceAddressInfo address_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    address_info.buffer = this->buffer_;
    this->address_ = vkGetBufferDeviceAddress(this->ctx_->device_, &address_info);

    this->prim_offset_ = this->mem_size_;
    for (size_t p = 0; p < this->prim_count_; p++)
    {
        this->draw_calls_[p].instanceCount = 1;
        this->draw_calls_[p].vertexCount = this->prims_[p].attrib_counts_[VK_PRIM_ATTRIB_INDEX];
        memcpy(this->mapping_ + this->mem_size_, this->draw_calls_ + p, sizeof(*this->draw_calls_));
        this->mem_size_ += sizeof(*this->draw_calls_);

        for (int i = 0; i < VK_PRIM_ATTRIB_COUNT; i++)
        {
            if (this->prims_[p].attrib_counts_[i])
            {
                this->prims_[p].attrib_address_[i] += this->address_;
            }
        }
        memcpy(this->mapping_ + this->mem_size_, this->prims_ + p, sizeof(*this->prims_));
        this->mem_size_ += sizeof(*this->prims_);
    }

    // copy buffer
    vmaFlushAllocation(this->ctx_->allocator_, this->staging_alloc_, 0, this->mem_size_);

    VkFence fence = {};
    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(this->ctx_->device_, &fence_cinfo, nullptr, &fence);
    vkResetFences(this->ctx_->device_, 1, &fence);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(this->ctx_->device_, &cmd_alloc, &cmd);

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy region = {0, 0, this->mem_size_};
    vkCmdCopyBuffer(cmd, this->staging_, this->buffer_, 1, &region);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
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
    vkCmdDrawIndirect(cmd, this->buffer_, this->prim_offset_, this->prim_count_, //
                      sizeof(*this->draw_calls_) + sizeof(*this->prims_));
}

vk_prim* vk_mesh_add_prim(vk_mesh* this)
{
    if (this->prim_count_ >= this->prim_limit_)
    {
        return nullptr;
    }

    this->prim_count_++;
    return this->prims_ + this->prim_count_ - 1;
}

VkDeviceSize vk_mesh_add_memory(vk_mesh* this, T* src, VkDeviceSize size)
{
    if (!src)
    {
        return -1;
    }

    if (this->mem_size_ + size >= this->mem_limit_)
    {
        return -1;
    }

    size_t offset = this->mem_size_;
    memcpy(this->mapping_ + this->mem_size_, src, size);
    this->mem_size_ += size;
    this->padding_size_ += VK_MESH_PAD_MEMORY(this->mem_size_);
    this->mem_size_ += VK_MESH_PAD_MEMORY(this->mem_size_);
    return offset;
}

VkDeviceSize vk_mesh_add_prim_attrib(vk_mesh* this, vk_prim* prim, vk_prim_attrib attrib, T* data, size_t count)
{
    prim->attrib_counts_[attrib] = count;
    VkDeviceSize offset = vk_mesh_add_memory(this, data, vk_prim_get_attrib_size(prim, attrib));
    if (offset != -1)
    {
        prim->attrib_address_[attrib] = offset;
        return offset;
    }
    return -1;
}

void vk_mesh_add_prim_morph_attrib(vk_mesh* this, vk_morph* morph, vk_morph_attrib attrib, T* data, size_t count)
{
    morph->attrib_counts_[attrib] = count;
    VkDeviceSize offset = vk_mesh_add_memory(this, data, vk_morph_get_attrib_size(morph, attrib));
    if (offset != -1)
    {
        morph->attrib_offsets_[attrib] = offset;
    }
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
    for (size_t i = 0; i < this->tex_count_; i++)
    {
        vkDestroyImageView(this->ctx_->device_, this->views_[i], nullptr);
        vmaDestroyImage(this->ctx_->allocator_, this->texs_[i], this->allocs_[i]);
    }

    for (size_t i = 0; i < this->sampler_count_; i++)
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
    if (this->sampler_count_ >= this->sampler_limit_)
    {
        return false;
    }

    vkCreateSampler(this->ctx_->device_, sampler_info, nullptr, this->samplers_ + this->sampler_count_);

    this->sampler_count_++;
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
    if (this->tex_count_ >= this->tex_limit_)
    {
        return false;
    }

    VkImageCreateInfo tex_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    tex_info.imageType = VK_IMAGE_TYPE_2D;
    tex_info.arrayLayers = sub_res->arrayLayer;
    tex_info.mipLevels = sub_res->mipLevel;
    tex_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    tex_info.extent = *extent;
    tex_info.samples = VK_SAMPLE_COUNT_1_BIT;
    tex_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateImage(this->ctx_->allocator_, &tex_info, &alloc_info, //
                   this->texs_ + this->tex_count_, this->allocs_ + this->tex_count_, nullptr);

    VkImageViewCreateInfo view_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.image = this->texs_[this->tex_count_];
    view_info.format = tex_info.format;
    view_info.subresourceRange.aspectMask = sub_res->aspectMask;
    view_info.subresourceRange.layerCount = tex_info.arrayLayers;
    view_info.subresourceRange.levelCount = tex_info.mipLevels;
    vkCreateImageView(this->ctx_->device_, &view_info, nullptr, this->views_ + this->tex_count_);

    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
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
    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(this->ctx_->device_, &fence_cinfo, nullptr, &fence);
    vkResetFences(this->ctx_->device_, 1, &fence);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(this->ctx_->device_, &cmd_alloc, &cmd);

    VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = this->texs_[this->tex_count_];
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

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkCmdCopyBufferToImage(cmd, staging, this->texs_[this->tex_count_], barrier.newLayout, 1, &region);

    uint32_t mip_w = extent->width;
    uint32_t mip_h = extent->height;
    for (size_t i = 1; i < sub_res->mipLevel; i++)
    {
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
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

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
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

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(this->ctx_->queue_, 1, &submit, fence);
    vkWaitForFences(this->ctx_->device_, 1, &fence, true, UINT64_MAX);

    vkDestroyFence(this->ctx_->device_, fence, nullptr);
    vkFreeCommandBuffers(this->ctx_->device_, cmd_pool, 1, &cmd);

    VkDescriptorImageInfo* desc_info = this->desc_infos_ + this->tex_count_;
    desc_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc_info->imageView = this->views_[this->tex_count_];
    desc_info->sampler = this->samplers_[sampler_idx];
    vmaDestroyBuffer(this->ctx_->allocator_, staging, staging_alloc);

    this->tex_count_++;
    return true;
}

VkWriteDescriptorSet vk_tex_arr_get_write_info(vk_tex_arr* this, VkDescriptorSet set, uint32_t binding)
{
    return (VkWriteDescriptorSet){.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = set,
                                  .dstBinding = binding,
                                  .descriptorCount = this->tex_count_,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  .pImageInfo = this->desc_infos_};
}

IMPL_OBJ_NEW(vk_mesh_desc, vk_ctx* ctx, uint32_t node_count)
{
    this->ctx_ = ctx;
    this->node_count_ = node_count;
    this->nodes_ = alloc(vk_mesh_node, node_count);
    this->output_ = alloc(mat4, node_count);

    for (uint32_t i = 0; i < node_count; i++)
    {
        construct_vk_mesh_node(this->nodes_ + i);
    }
    glm_mat4_identity_array(this->output_, node_count);
    return this;
}

IMPL_OBJ_DELETE(vk_mesh_desc)
{
    if (this->buffer_)
    {
        vmaDestroyBuffer(this->ctx_->allocator_, this->buffer_, this->alloc_);
    }
    ffree(this->nodes_);
    ffree(this->output_);
}

void vk_mesh_desc_flush(vk_mesh_desc* this)
{
    memcpy(this->mapping_, this->output_, sizeof(*this->output_) * this->node_count_);
}

void vk_mesh_desc_update(vk_mesh_desc* this, mat4 root_trans)
{
    uint32_t iter = 0;
    while (iter < this->node_count_ && this->nodes_[iter].parent_idx_ == -1)
    {
        mat4* output = this->output_ + iter;
        glm_mat4_identity(*output);

        glm_mat4_mul(root_trans, *output, *output);
        glm_translate(*output, this->nodes_[iter].translation_);
        glm_quat_rotate(*output, this->nodes_[iter].rotation, *output);
        glm_scale(*output, this->nodes_[iter].scale_);
        iter++;
    }
    while (iter < this->node_count_)
    {
        mat4* output = this->output_ + iter;
        int a = this->nodes_[iter].parent_idx_;
        mat4* parent_transform = this->output_ + this->nodes_[iter].parent_idx_;
        glm_mat4_identity(*output);

        glm_mat4_mul(*parent_transform, *output, *output);
        glm_translate(*output, this->nodes_[iter].translation_);
        glm_quat_rotate(*output, this->nodes_[iter].rotation, *output);
        glm_scale(*output, this->nodes_[iter].scale_);
        iter++;
    }
    vk_mesh_desc_flush(this);
}

void vk_mesh_desc_alloc_device_mem(vk_mesh_desc* this)
{
    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = this->node_count_ * sizeof(*this->output_);
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | //
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
    alloc_info.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VmaAllocationInfo allocated = {};
    vmaCreateBuffer(this->ctx_->allocator_, &buffer_info, &alloc_info, &this->buffer_, &this->alloc_, &allocated);
    this->mapping_ = allocated.pMappedData;

    VkBufferDeviceAddressInfo address_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    address_info.buffer = this->buffer_;
    this->address_ = vkGetBufferDeviceAddress(this->ctx_->device_, &address_info);
}

IMPL_OBJ_NEW(vk_mesh_skin, vk_ctx* ctx, uint32_t joint_size)
{
    this->ctx_ = ctx;
    this->joint_count_ = joint_size;
    this->joints_ = alloc(vk_mesh_joint, joint_size);

    for (uint32_t i = 0; i < joint_size; i++)
    {
        this->joints_[i].joint_ = -1;
        this->joints_[i].inv_binding_[0] = 1;
        this->joints_[i].inv_binding_[5] = 1;
        this->joints_[i].inv_binding_[10] = 1;
        this->joints_[i].inv_binding_[15] = 1;
    }
    return this;
}

IMPL_OBJ_DELETE(vk_mesh_skin)
{
    ffree(this->joints_);
    if (this->buffer_)
    {
        vmaDestroyBuffer(this->ctx_->allocator_, this->buffer_, this->alloc_);
    }
}

void vk_mesh_skin_alloc_device_mem(vk_mesh_skin* this, VkCommandPool cmd_pool)
{
    VkBuffer staging = {};
    VmaAllocation staging_alloc = {};
    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = this->joint_count_ * sizeof(*this->joints_);
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | //
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
    alloc_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    VmaAllocationInfo allocated = {};
    vmaCreateBuffer(this->ctx_->allocator_, &buffer_info, &alloc_info, &staging, &staging_alloc, &allocated);
    memcpy(allocated.pMappedData, this->joints_, buffer_info.size);
    vmaFlushAllocation(this->ctx_->allocator_, staging_alloc, 0, buffer_info.size);

    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |   //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;
    alloc_info.requiredFlags = 0;
    vmaCreateBuffer(this->ctx_->allocator_, &buffer_info, &alloc_info, &this->buffer_, &this->alloc_, nullptr);

    VkBufferDeviceAddressInfo address_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    address_info.buffer = this->buffer_;
    this->address_ = vkGetBufferDeviceAddress(this->ctx_->device_, &address_info);

    VkFence fence = {};
    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(this->ctx_->device_, &fence_cinfo, nullptr, &fence);
    vkResetFences(this->ctx_->device_, 1, &fence);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(this->ctx_->device_, &cmd_alloc, &cmd);

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy region = {0, 0, buffer_info.size};
    vkCmdCopyBuffer(cmd, staging, this->buffer_, 1, &region);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(this->ctx_->queue_, 1, &submit, fence);
    vkWaitForFences(this->ctx_->device_, 1, &fence, true, UINT64_MAX);

    vkDestroyFence(this->ctx_->device_, fence, nullptr);
    vkFreeCommandBuffers(this->ctx_->device_, cmd_pool, 1, &cmd);
    vmaDestroyBuffer(this->ctx_->allocator_, staging, staging_alloc);
}