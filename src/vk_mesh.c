#include "vk_mesh.h"

void init_vk_prim(vk_prim* prim)
{
    for (size_t i = 0; i < VK_PRIM_ATTRIB_COUNT; i++)
    {
        prim->attrib_datas_[i] = -1;
    }
}

size_t vk_prim_get_attrib_size(vk_prim* prim, vk_prim_attrib_type attrib_type)
{
    const static size_t ATTRIB_SIZES[VK_PRIM_ATTRIB_COUNT] = {sizeof(uint32_t), sizeof(vec3), sizeof(vec3),
                                                              sizeof(vec4),     sizeof(vec2), sizeof(vec4),
                                                              sizeof(uvec4),    sizeof(vec4), sizeof(vk_material)};
    return ATTRIB_SIZES[attrib_type] * prim->attrib_counts_[attrib_type];
}

vk_mesh* new_vk_mesh(vk_ctx* ctx, VkDeviceSize mem_limit)
{
    vk_mesh* m = alloc(vk_mesh);
    init_vk_mesh(m, ctx, mem_limit);
    return m;
}

void init_vk_mesh(vk_mesh* mesh, vk_ctx* ctx, VkDeviceSize mem_limit)
{
    mesh->ctx_ = ctx;
    mesh->mem_limit_ = mem_limit;

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = mem_limit;
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |        //
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | //
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateBuffer(ctx->allocator_, &buffer_info, &alloc_info, &mesh->buffer_, &mesh->alloc_, nullptr);
}

void release_vk_mesh(vk_mesh* mesh)
{
    if (mesh->alloc_)
    {
        vmaDestroyBuffer(mesh->ctx_->allocator_, mesh->buffer_, mesh->alloc_);
    }
    ffree(mesh->prims_);
}
