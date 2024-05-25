#include "ext_defines.h"

CreateBufferReturn create_vertex_buffer(const ObjectDetails* details, //
                                        const VkBufferCreateInfo* create_info,
                                        const VmaAllocationCreateInfo* alloc_info)
{
    CreateBufferReturn r = {};
    VkBufferCreateInfo create_info_ = *create_info;
    VmaAllocationCreateInfo alloc_info_ = *alloc_info;
    create_info_.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    alloc_info_.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer(details->allocator_, &create_info_, &alloc_info_, &r.buffer_, &r.alloc_, NULL);
    return r;
}

void destory_vertex_buffer(const ObjectDetails* details, VkBuffer buffer, VmaAllocation alloc)
{
    vmaDestroyBuffer(details->allocator_, buffer, alloc);
};

BufferFunctions buffer_func_getter()
{
    BufferFunctions funcs;
    funcs.create_buffer_ = create_vertex_buffer;
    funcs.destory_buffer_ = destory_vertex_buffer;
    return funcs;
};