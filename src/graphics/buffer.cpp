#include "graphics/buffer.hpp"

void BufferBase::create_buffer(const vk::BufferCreateInfo& buffer_info, const vma::AllocationCreateInfo& alloc_info)
{
    size_ = buffer_info.size;

    BufferAllocation buffer_alloc = allocator().createBuffer(buffer_info, alloc_info);
    casts(vk::Buffer&, *this) = buffer_alloc;
    casts(vma::Allocation&, *this) = buffer_alloc;
}

BufferBase::~BufferBase()
{
    unmap_memory();
    allocator().destroyBuffer(*this, *this);
}

std::byte* BufferBase::map_memory()
{
    if (!mapping_)
    {
        mapping_ = (std::byte*)allocator().mapMemory(*this);
    }
    return mapping_;
}

void BufferBase::unmap_memory()
{
    if (mapping_)
    {
        allocator().unmapMemory(*this);
        mapping_ = nullptr;
    }
}

void BufferBase::flush_cache(std::byte* memory, vk::DeviceSize offset, vk::DeviceSize size)
{
    memcpy(mapping_ + offset, memory, size);
    allocator().flushAllocation(*this, offset, size);
}

void BufferBase::invilidate_cache(vk::DeviceSize offset, vk::DeviceSize size)
{
    allocator().invalidateAllocation(*this, offset, size);
}

bool BufferBase::valid() const
{
    return casts(vk::Buffer, *this) && size_;
}

std::byte* BufferBase::mapping() const
{
    return mapping_;
}

vk::DeviceSize BufferBase::size() const
{
    return size_;
}

void Uniform::config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
    alloc_info.flags |= vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
    alloc_info.preferredFlags |= vk::MemoryPropertyFlagBits::eHostCoherent;
    alloc_info.usage = vma::MemoryUsage::eAutoPreferHost;
}

void Storage::config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
}

void Index::config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
}

void Vertex::config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
}

void Indirect::config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    buffer_info.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
}

void HostAccess::config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info)
{
    alloc_info.flags |= vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
    alloc_info.preferredFlags |= vk::MemoryPropertyFlagBits::eHostCoherent;
    alloc_info.usage = vma::MemoryUsage::eAutoPreferHost;
}
