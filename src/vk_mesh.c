#include "vk_mesh.h"

IMPL_OBJ_NEW_DEFAULT(vk_material)
{
    memcpy(&this->color_factor_, (float[4]){1, 1, 1, 1}, sizeof(float[4]));
    memcpy(&this->emissive_factor_, (float[4]){0, 0, 0, 1}, sizeof(float[4]));
    memcpy(&this->sheen_color_factor_, (float[4]){0, 0, 0, 0}, sizeof(float[4]));
    memcpy(&this->specular_color_factor_, (float[4]){1, 1, 1, 1}, sizeof(float[4]));

    this->alpha_cutoff_ = 0;
    this->metallic_factor_ = 1.0f;
    this->roughness_factor_ = 1.0f;

    this->color_ = -1;
    this->metallic_roughness_ = -1;
    this->normal_ = -1;
    this->emissive_ = -1;
    this->occlusion_ = -1;

    this->anisotropy_rotation_ = 0;
    this->anisotropy_strength_ = 0;
    this->anisotropy_ = -1;

    this->specular_ = -1;
    this->spec_color_ = -1;

    this->sheen_color_ = -1;
    this->sheen_roughness_ = -1;
    return this;
}

IMPL_OBJ_DELETE(vk_material)
{
}

IMPL_OBJ_NEW_DEFAULT(vk_prim)
{
    for (size_t i = 0; i < VK_PRIM_ATTRIB_COUNT; i++)
    {
        this->attrib_offsets_[i] = -1;
    }
    return this;
}

IMPL_OBJ_DELETE(vk_prim)
{
}

size_t vk_prim_get_attrib_size(vk_prim* this, vk_prim_attrib attrib_type)
{
    const static size_t ATTRIB_SIZES[VK_PRIM_ATTRIB_COUNT] = {
        sizeof(uint32_t), sizeof(float[3]),    sizeof(float[3]), sizeof(float[4]),   sizeof(float[2]),
        sizeof(float[4]), sizeof(uint32_t[4]), sizeof(float[4]), sizeof(vk_material)};
    return ATTRIB_SIZES[attrib_type] * this->attrib_counts_[attrib_type];
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

void vk_mesh_flush_staging(vk_mesh* this)
{
    vmaFlushAllocation(this->ctx_->allocator_, this->staging_alloc_, 0, this->mem_size_);
}

void vk_mesh_alloc_device_mem(vk_mesh* this, VkCommandPool pool)
{
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = this->mem_size_ + this->prim_count_ * (sizeof(VkDrawIndirectCommand) + sizeof(vk_prim));
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |        //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateBuffer(this->ctx_->allocator_, &buffer_info, &alloc_info, &this->buffer_, &this->alloc_, nullptr);

    this->dc_offset_ = this->mem_size_;
    for (size_t p = 0; p < this->prim_count_; p++)
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
}

void vk_mesh_draw_prims(vk_mesh* this, VkCommandBuffer cmd)
{
    vkCmdDrawIndirect(cmd, this->buffer_, this->dc_offset_, this->prim_count_,
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
    if (this->prim_count_ >= this->prim_limit_)
    {
        return nullptr;
    }

    this->prim_count_++;
    return this->prims_ + this->prim_count_ - 1;
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

IMPL_OBJ_NEW(vk_model, vk_ctx* ctx, const char* name, uint32_t mesh_limit)
{
    this->ctx_ = ctx;
    this->mesh_limit_ = mesh_limit;
    strcpy_s(this->name_, sizeof(this->name_), name);
    this->meshes_ = alloc(vk_mesh, mesh_limit);

    return this;
}

IMPL_OBJ_DELETE(vk_model)
{
    for (size_t m = 0; m < this->mesh_count_; m++)
    {
        destroy_vk_mesh(this->meshes_ + m);
    }
    ffree(this->meshes_);
}

vk_mesh* vk_model_add_mesh(vk_model* this, const char* name, VkDeviceSize mem_limit, uint32_t prim_limit)
{
    if (this->mesh_count_ >= this->mesh_limit_)
    {
        return nullptr;
    }
    construct_vk_mesh(this->meshes_ + this->mesh_count_, this->ctx_, name, mem_limit, prim_limit);
    this->mesh_count_++;

    return this->meshes_ + this->mesh_count_ - 1;
}