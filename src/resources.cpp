#include "resources.hpp"

Buffer::Buffer(const vk::BufferCreateInfo& buffer_info, //
               vma::AllocationCreateInfo& alloc_info,   //
               vma::Pool pool)
    : size_(buffer_info.size)
{
    alloc_info.pool = pool;
    vma::AllocationInfo finish_info{};
    auto result = allocator().createBuffer(buffer_info, alloc_info, finish_info);
    static_cast<vk::Buffer&>(*this) = result.first;
    allocation_ = result.second;
    memory_ = finish_info.deviceMemory;
}

Buffer::~Buffer()
{
    if (valid())
    {
        unmap_memory();
        allocator().destroyBuffer(*this, allocation_);
    }
}

bool Buffer::valid() const
{
    return static_cast<vk::Buffer>(*this);
}

void Buffer::map_memory()
{
    if (!mapping_)
    {
        mapping_ = allocator().mapMemory(allocation_);
    }
}

void Buffer::unmap_memory()
{
    if (mapping_)
    {
        allocator().unmapMemory(allocation_);
        mapping_ = nullptr;
    }
}

void Buffer::copy_from(void* memory)
{
    map_memory();
    memcpy(mapping_, memory, size_);
}

void Buffer::flush_cache(void* memory, vk::DeviceSize offset, vk::DeviceSize size)
{
    if (!size)
    {
        size = size_;
    }

    memcpy((unsigned char*)(mapping_) + offset, memory, size);
    allocator().flushAllocation(allocation_, offset, size);
}

void Buffer::invilidate_cache(vk::DeviceSize offset, vk::DeviceSize size)
{
    allocator().invalidateAllocation(allocation_, offset, size);
}

vk::DeviceSize Buffer::size() const
{
    return size_;
}

void* Buffer::mapping() const
{
    return mapping_;
}
Image::Image(const vk::ImageCreateInfo& image_info, //
             vma::AllocationCreateInfo& alloc_info, //
             vma::Pool pool)
{
    alloc_info.pool = pool;
    vma::AllocationInfo finish_info{};
    auto result = allocator().createImage(image_info, alloc_info, finish_info);
    static_cast<vk::Image&>(*this) = result.first;
    allocation_ = result.second;
    memory_ = finish_info.deviceMemory;
}

Image::~Image()
{
    if (valid())
    {
        allocator().destroyImage(*this, allocation_);
    }
}

bool Image::valid() const
{
    return static_cast<vk::Image>(*this);
}
