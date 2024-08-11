#include "graphics/buffer.hpp"

void fi::BufferBase::create_buffer(const vk::BufferCreateInfo& buffer_info, const vma::AllocationCreateInfo& alloc_info)
{
    size_ = buffer_info.size;

    auto result = allocator().createBuffer(buffer_info, alloc_info);
    sset(*this, result.first);
    sset(*this, result.second);
}

fi::BufferBase::~BufferBase()
{
    unmap_memory();
    allocator().destroyBuffer(*this, *this);
}

std::byte* fi::BufferBase::map_memory()
{
    if (!mapping_)
    {
        mapping_ = (std::byte*)allocator().mapMemory(*this);
    }
    return mapping_;
}

void fi::BufferBase::unmap_memory()
{
    if (mapping_)
    {
        allocator().unmapMemory(*this);
        mapping_ = nullptr;
    }
}

void fi::BufferBase::flush_cache(vk::DeviceSize offset, vk::DeviceSize size)
{
    allocator().flushAllocation(*this, offset, size);
}

void fi::BufferBase::invilidate_cache(vk::DeviceSize offset, vk::DeviceSize size)
{
    allocator().invalidateAllocation(*this, offset, size);
}

bool fi::BufferBase::valid() const
{
    return casts(vk::Buffer, *this) && size_;
}

std::byte* fi::BufferBase::mapping() const
{
    return mapping_;
}

vk::DeviceSize fi::BufferBase::size() const
{
    return size_;
}

void fi::uniform(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
}

void fi::storage(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
}

void fi::index(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
}

void fi::vertex(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
}

void fi::indirect(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
}

void fi::seq_write(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    alloc_info.flags |= vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
}

void fi::host_coherent(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    alloc_info.preferredFlags |= vk::MemoryPropertyFlagBits::eHostCoherent;
}

void fi::host_cached(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    alloc_info.preferredFlags |= vk::MemoryPropertyFlagBits::eHostCached;
}
