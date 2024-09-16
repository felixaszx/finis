#include "vk_mesh.h"

IMPL_OBJ_NEW_DEFAULT(vk_prim)
{
    for (size_t i = 0; i < VK_PRIM_ATTRIB_COUNT; i++)
    {
        this->attrib_datas_[i] = -1;
    }
    return this;
}

IMPL_OBJ_DELETE(vk_prim)
{
}

size_t vk_prim_get_attrib_size(vk_prim* this, vk_prim_attrib_type attrib_type)
{
    const static size_t ATTRIB_SIZES[VK_PRIM_ATTRIB_COUNT] = {sizeof(uint32_t), sizeof(vec3), sizeof(vec3),
                                                              sizeof(vec4),     sizeof(vec2), sizeof(vec4),
                                                              sizeof(uvec4),    sizeof(vec4), sizeof(vk_material)};
    return ATTRIB_SIZES[attrib_type] * this->attrib_counts_[attrib_type];
}

void vk_mesh_free_staging(vk_mesh* this)
{
    if (this->staging_alloc_)
    {
        vmaDestroyBuffer(this->ctx_->allocator_, this->staging_, this->staging_alloc_);
    }
}

IMPL_OBJ_NEW(vk_mesh, vk_ctx* ctx, const char* name, VkDeviceSize mem_limit, uint32_t prim_limit)
{
    this->ctx_ = ctx;
    this->mem_limit_ = mem_limit;
    this->prim_limit_ = prim_limit;
    strcpy_s(this->name_, sizeof(this->name_), name);

    this->prims_ = alloc(vk_prim, prim_limit);
    for (size_t i = 0; i < prim_limit; i++)
    {
        construct_vk_prim(this->prims_ + i);
    }

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = mem_limit;
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |        //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateBuffer(ctx->allocator_, &buffer_info, &alloc_info, &this->buffer_, &this->alloc_, nullptr);

    buffer_info.size = mem_limit;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
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
}

vk_prim* vk_mesh_add_prim(vk_mesh* this, const char* name)
{
    if (this->prim_count_ >= this->prim_limit_)
    {
        return nullptr;
    }

    strcpy_s(this->prims_[this->prim_count_].name_, sizeof(this->prims_->name_), name);
    this->prim_count_++;
    return this->prims_ + this->prim_count_ - 1;
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
