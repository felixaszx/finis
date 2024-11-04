#include "vk_mesh.h"

IMPL_OBJ_NEW(vk_mesh, vk_ctx* ctx, const char* name, VkDeviceSize mem_limit, uint32_t prim_limit)
{
    cthis->ctx_ = ctx;
    cthis->mem_limit_ = mem_limit - prim_limit * (sizeof(*cthis->draw_calls_) + sizeof(*cthis->prims_));
    cthis->prim_limit_ = prim_limit;
    strncpy(cthis->name_, name, sizeof(cthis->name_));

    cthis->prims_ = fi_alloc(vk_prim, prim_limit);
    cthis->draw_calls_ = fi_alloc(VkDrawIndirectCommand, prim_limit);
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
    vmaCreateBuffer(ctx->allocator_, &buffer_info, &alloc_info, &cthis->staging_, &cthis->staging_alloc_, &allocated);
    cthis->mapping_ = allocated.pMappedData;
    return cthis;
}

IMPL_OBJ_DELETE(vk_mesh)
{
    vk_mesh_free_staging(cthis);
    if (cthis->alloc_)
    {
        vmaDestroyBuffer(cthis->ctx_->allocator_, cthis->buffer_, cthis->alloc_);
    }
    fi_free(cthis->prims_);
    fi_free(cthis->draw_calls_);
}

void vk_mesh_free_staging(vk_mesh* cthis)
{
    if (cthis->staging_alloc_)
    {
        vmaDestroyBuffer(cthis->ctx_->allocator_, cthis->staging_, cthis->staging_alloc_);
        cthis->mapping_ = fi_nullptr;
        cthis->staging_ = fi_nullptr;
        cthis->staging_alloc_ = fi_nullptr;
    }
}

void vk_mesh_alloc_device_mem(vk_mesh* cthis, VkCommandPool pool)
{
    if (cthis->buffer_)
    {
        return;
    }

    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = cthis->mem_size_ + cthis->prim_count_ * (sizeof(*cthis->draw_calls_) + sizeof(*cthis->prims_));
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |        //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | //
                        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |       //
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    vmaCreateBuffer(cthis->ctx_->allocator_, &buffer_info, &alloc_info, &cthis->buffer_, &cthis->alloc_, fi_nullptr);

    VkBufferDeviceAddressInfo address_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    address_info.buffer = cthis->buffer_;
    cthis->address_ = vkGetBufferDeviceAddress(cthis->ctx_->device_, &address_info);

    cthis->prim_offset_ = cthis->mem_size_;
    for (size_t p = 0; p < cthis->prim_count_; p++)
    {
        cthis->draw_calls_[p].instanceCount = 1;
        cthis->draw_calls_[p].vertexCount = cthis->prims_[p].attrib_counts_[VK_PRIM_ATTRIB_INDEX];
        memcpy(cthis->mapping_ + cthis->mem_size_, cthis->draw_calls_ + p, sizeof(*cthis->draw_calls_));
        cthis->mem_size_ += sizeof(*cthis->draw_calls_);

        for (int i = 0; i < VK_PRIM_ATTRIB_COUNT; i++)
        {
            if (cthis->prims_[p].attrib_counts_[i])
            {
                cthis->prims_[p].attrib_address_[i] += cthis->address_;
            }
        }
        memcpy(cthis->mapping_ + cthis->mem_size_, cthis->prims_ + p, sizeof(*cthis->prims_));
        cthis->mem_size_ += sizeof(*cthis->prims_);
    }

    // copy buffer
    vmaFlushAllocation(cthis->ctx_->allocator_, cthis->staging_alloc_, 0, cthis->mem_size_);

    VkFence fence = {};
    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(cthis->ctx_->device_, &fence_cinfo, fi_nullptr, &fence);
    vkResetFences(cthis->ctx_->device_, 1, &fence);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(cthis->ctx_->device_, &cmd_alloc, &cmd);

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy region = {0, 0, cthis->mem_size_};
    vkCmdCopyBuffer(cmd, cthis->staging_, cthis->buffer_, 1, &region);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(cthis->ctx_->queue_, 1, &submit, fence);
    vkWaitForFences(cthis->ctx_->device_, 1, &fence, true, UINT64_MAX);

    vkDestroyFence(cthis->ctx_->device_, fence, fi_nullptr);
    vkFreeCommandBuffers(cthis->ctx_->device_, pool, 1, &cmd);
    vk_mesh_free_staging(cthis);
    cthis->prim_limit_ = 0;
    cthis->mem_limit_ = 0;
}

void vk_mesh_draw_prims(vk_mesh* cthis, VkCommandBuffer cmd)
{
    vkCmdDrawIndirect(cmd, cthis->buffer_, cthis->prim_offset_, cthis->prim_count_, //
                      sizeof(*cthis->draw_calls_) + sizeof(*cthis->prims_));
}

vk_prim* vk_mesh_add_prim(vk_mesh* cthis)
{
    if (cthis->prim_count_ >= cthis->prim_limit_)
    {
        return fi_nullptr;
    }

    cthis->prim_count_++;
    return cthis->prims_ + cthis->prim_count_ - 1;
}

VkDeviceSize vk_mesh_add_memory(vk_mesh* cthis, T* src, VkDeviceSize size)
{
    if (!src)
    {
        return -1;
    }

    if (cthis->mem_size_ + size >= cthis->mem_limit_)
    {
        return -1;
    }

    size_t offset = cthis->mem_size_;
    memcpy(cthis->mapping_ + cthis->mem_size_, src, size);
    cthis->mem_size_ += size;
    cthis->padding_size_ += VK_MESH_PAD_MEMORY(cthis->mem_size_);
    cthis->mem_size_ += VK_MESH_PAD_MEMORY(cthis->mem_size_);
    return offset;
}

VkDeviceSize vk_mesh_add_prim_attrib(vk_mesh* cthis, vk_prim* prim, vk_prim_attrib attrib, T* data, size_t count)
{
    prim->attrib_counts_[attrib] = count;
    VkDeviceSize offset = vk_mesh_add_memory(cthis, data, vk_prim_get_attrib_size(prim, attrib));
    if (offset != -1)
    {
        prim->attrib_address_[attrib] = offset;
        return offset;
    }
    return -1;
}

void vk_mesh_add_prim_morph_attrib(vk_mesh* cthis, vk_morph* morph, vk_morph_attrib attrib, T* data, size_t count)
{
    morph->attrib_counts_[attrib] = count;
    VkDeviceSize offset = vk_mesh_add_memory(cthis, data, vk_morph_get_attrib_size(morph, attrib));
    if (offset != -1)
    {
        morph->attrib_offsets_[attrib] = offset;
    }
}

IMPL_OBJ_NEW(vk_tex_arr, vk_ctx* ctx, uint32_t tex_limit, uint32_t sampler_limit)
{
    cthis->ctx_ = ctx;

    cthis->tex_limit_ = tex_limit;
    cthis->texs_ = fi_alloc(VkImage, tex_limit);
    cthis->views_ = fi_alloc(VkImageView, tex_limit);
    cthis->allocs_ = fi_alloc(VmaAllocation, tex_limit);
    cthis->desc_infos_ = fi_alloc(VkDescriptorImageInfo, tex_limit);

    cthis->sampler_limit_ = sampler_limit;
    cthis->samplers_ = fi_alloc(VkSampler, sampler_limit);
    return cthis;
}

IMPL_OBJ_DELETE(vk_tex_arr)
{
    for (size_t i = 0; i < cthis->tex_count_; i++)
    {
        vkDestroyImageView(cthis->ctx_->device_, cthis->views_[i], fi_nullptr);
        vmaDestroyImage(cthis->ctx_->allocator_, cthis->texs_[i], cthis->allocs_[i]);
    }

    for (size_t i = 0; i < cthis->sampler_count_; i++)
    {
        vkDestroySampler(cthis->ctx_->device_, cthis->samplers_[i], fi_nullptr);
    }

    fi_free(cthis->texs_);
    fi_free(cthis->views_);
    fi_free(cthis->allocs_);
    fi_free(cthis->desc_infos_);
    fi_free(cthis->samplers_);
}

bool vk_tex_arr_add_sampler(vk_tex_arr* cthis, VkSamplerCreateInfo* sampler_info)
{
    if (cthis->sampler_count_ >= cthis->sampler_limit_)
    {
        return false;
    }

    vkCreateSampler(cthis->ctx_->device_, sampler_info, fi_nullptr, cthis->samplers_ + cthis->sampler_count_);

    cthis->sampler_count_++;
    return true;
}

bool vk_tex_arr_add_tex(vk_tex_arr* cthis,
                        VkCommandPool cmd_pool,
                        uint32_t sampler_idx,
                        byte* data,
                        size_t size,
                        const VkExtent3D* extent,
                        const VkImageSubresource* sub_res)
{
    if (cthis->tex_count_ >= cthis->tex_limit_)
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
    vmaCreateImage(cthis->ctx_->allocator_, &tex_info, &alloc_info, //
                   cthis->texs_ + cthis->tex_count_, cthis->allocs_ + cthis->tex_count_, fi_nullptr);

    VkImageViewCreateInfo view_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.image = cthis->texs_[cthis->tex_count_];
    view_info.format = tex_info.format;
    view_info.subresourceRange.aspectMask = sub_res->aspectMask;
    view_info.subresourceRange.layerCount = tex_info.arrayLayers;
    view_info.subresourceRange.levelCount = tex_info.mipLevels;
    vkCreateImageView(cthis->ctx_->device_, &view_info, fi_nullptr, cthis->views_ + cthis->tex_count_);

    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VmaAllocationInfo allocated = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    alloc_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    VkBuffer staging = {};
    VmaAllocation staging_alloc = {};
    vmaCreateBuffer(cthis->ctx_->allocator_, &buffer_info, &alloc_info, &staging, &staging_alloc, &allocated);
    memcpy(allocated.pMappedData, data, size);
    vmaFlushAllocation(cthis->ctx_->allocator_, staging_alloc, 0, size);

    // manipulate the imageVkFence fence = {};
    VkFence fence = {};
    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(cthis->ctx_->device_, &fence_cinfo, fi_nullptr, &fence);
    vkResetFences(cthis->ctx_->device_, 1, &fence);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(cthis->ctx_->device_, &cmd_alloc, &cmd);

    VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = cthis->texs_[cthis->tex_count_];
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
                         0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);
    vkCmdCopyBufferToImage(cmd, staging, cthis->texs_[cthis->tex_count_], barrier.newLayout, 1, &region);

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
                             0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);

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
                             0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);

        mip_w > 1 ? mip_w /= 2 : mip_w;
        mip_h > 1 ? mip_h /= 2 : mip_h;
    }
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.subresourceRange.baseMipLevel = sub_res->mipLevel - 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(cthis->ctx_->queue_, 1, &submit, fence);
    vkWaitForFences(cthis->ctx_->device_, 1, &fence, true, UINT64_MAX);

    vkDestroyFence(cthis->ctx_->device_, fence, fi_nullptr);
    vkFreeCommandBuffers(cthis->ctx_->device_, cmd_pool, 1, &cmd);

    VkDescriptorImageInfo* desc_info = cthis->desc_infos_ + cthis->tex_count_;
    desc_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc_info->imageView = cthis->views_[cthis->tex_count_];
    desc_info->sampler = cthis->samplers_[sampler_idx];
    vmaDestroyBuffer(cthis->ctx_->allocator_, staging, staging_alloc);

    cthis->tex_count_++;
    return true;
}

VkWriteDescriptorSet vk_tex_arr_get_write_info(vk_tex_arr* cthis, VkDescriptorSet set, uint32_t binding)
{
    return (VkWriteDescriptorSet){.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = set,
                                  .dstBinding = binding,
                                  .descriptorCount = cthis->tex_count_,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  .pImageInfo = cthis->desc_infos_};
}

IMPL_OBJ_NEW(vk_mesh_desc, vk_ctx* ctx, uint32_t node_count)
{
    cthis->ctx_ = ctx;
    cthis->node_count_ = node_count;
    cthis->nodes_ = fi_alloc(vk_mesh_node, node_count);
    cthis->output_ = fi_alloc(mat4, node_count);

    for (uint32_t i = 0; i < node_count; i++)
    {
        construct_vk_mesh_node(cthis->nodes_ + i);
    }
    glm_mat4_identity_array(cthis->output_, node_count);
    return cthis;
}

IMPL_OBJ_DELETE(vk_mesh_desc)
{
    if (cthis->buffer_)
    {
        vmaDestroyBuffer(cthis->ctx_->allocator_, cthis->buffer_, cthis->alloc_);
    }
    fi_free(cthis->nodes_);
    fi_free(cthis->output_);
}

void vk_mesh_desc_flush(vk_mesh_desc* cthis)
{
    memcpy(cthis->mapping_, cthis->output_, sizeof(*cthis->output_) * cthis->node_count_);
}

void vk_mesh_desc_update(vk_mesh_desc* cthis, mat4 root_trans)
{
    uint32_t iter = 0;
    while (iter < cthis->node_count_ && cthis->nodes_[iter].parent_idx_ == -1)
    {
        mat4* output = cthis->output_ + iter;
        glm_mat4_identity(*output);

        glm_mat4_mul(root_trans, *output, *output);
        glm_translate(*output, cthis->nodes_[iter].translation_);
        glm_quat_rotate(*output, cthis->nodes_[iter].rotation, *output);
        glm_scale(*output, cthis->nodes_[iter].scale_);
        iter++;
    }
    while (iter < cthis->node_count_)
    {
        mat4* output = cthis->output_ + iter;
        int a = cthis->nodes_[iter].parent_idx_;
        mat4* parent_transform = cthis->output_ + cthis->nodes_[iter].parent_idx_;
        glm_mat4_identity(*output);

        glm_mat4_mul(*parent_transform, *output, *output);
        glm_translate(*output, cthis->nodes_[iter].translation_);
        glm_quat_rotate(*output, cthis->nodes_[iter].rotation, *output);
        glm_scale(*output, cthis->nodes_[iter].scale_);
        iter++;
    }
    vk_mesh_desc_flush(cthis);
}

void vk_mesh_desc_alloc_device_mem(vk_mesh_desc* cthis)
{
    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = cthis->node_count_ * sizeof(*cthis->output_);
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | //
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
    alloc_info.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VmaAllocationInfo allocated = {};
    vmaCreateBuffer(cthis->ctx_->allocator_, &buffer_info, &alloc_info, &cthis->buffer_, &cthis->alloc_, &allocated);
    cthis->mapping_ = allocated.pMappedData;

    VkBufferDeviceAddressInfo address_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    address_info.buffer = cthis->buffer_;
    cthis->address_ = vkGetBufferDeviceAddress(cthis->ctx_->device_, &address_info);
}

IMPL_OBJ_NEW(vk_mesh_skin, vk_ctx* ctx, uint32_t joint_size)
{
    cthis->ctx_ = ctx;
    cthis->joint_count_ = joint_size;
    cthis->joints_ = fi_alloc(vk_mesh_joint, joint_size);

    for (uint32_t i = 0; i < joint_size; i++)
    {
        cthis->joints_[i].joint_ = -1;
        cthis->joints_[i].inv_binding_[0] = 1;
        cthis->joints_[i].inv_binding_[5] = 1;
        cthis->joints_[i].inv_binding_[10] = 1;
        cthis->joints_[i].inv_binding_[15] = 1;
    }
    return cthis;
}

IMPL_OBJ_DELETE(vk_mesh_skin)
{
    fi_free(cthis->joints_);
    if (cthis->buffer_)
    {
        vmaDestroyBuffer(cthis->ctx_->allocator_, cthis->buffer_, cthis->alloc_);
    }
}

void vk_mesh_skin_alloc_device_mem(vk_mesh_skin* cthis, VkCommandPool cmd_pool)
{
    VkBuffer staging = {};
    VmaAllocation staging_alloc = {};
    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = cthis->joint_count_ * sizeof(*cthis->joints_);
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | //
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
    alloc_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    VmaAllocationInfo allocated = {};
    vmaCreateBuffer(cthis->ctx_->allocator_, &buffer_info, &alloc_info, &staging, &staging_alloc, &allocated);
    memcpy(allocated.pMappedData, cthis->joints_, buffer_info.size);
    vmaFlushAllocation(cthis->ctx_->allocator_, staging_alloc, 0, buffer_info.size);

    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |   //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;
    alloc_info.requiredFlags = 0;
    vmaCreateBuffer(cthis->ctx_->allocator_, &buffer_info, &alloc_info, &cthis->buffer_, &cthis->alloc_, fi_nullptr);

    VkBufferDeviceAddressInfo address_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    address_info.buffer = cthis->buffer_;
    cthis->address_ = vkGetBufferDeviceAddress(cthis->ctx_->device_, &address_info);

    VkFence fence = {};
    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(cthis->ctx_->device_, &fence_cinfo, fi_nullptr, &fence);
    vkResetFences(cthis->ctx_->device_, 1, &fence);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(cthis->ctx_->device_, &cmd_alloc, &cmd);

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy region = {0, 0, buffer_info.size};
    vkCmdCopyBuffer(cmd, staging, cthis->buffer_, 1, &region);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(cthis->ctx_->queue_, 1, &submit, fence);
    vkWaitForFences(cthis->ctx_->device_, 1, &fence, true, UINT64_MAX);

    vkDestroyFence(cthis->ctx_->device_, fence, fi_nullptr);
    vkFreeCommandBuffers(cthis->ctx_->device_, cmd_pool, 1, &cmd);
    vmaDestroyBuffer(cthis->ctx_->allocator_, staging, staging_alloc);
}