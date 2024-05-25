#ifndef INCLUDE_BUFFER_HPP
#define INCLUDE_BUFFER_HPP

#include "vk_base.hpp"

/**
 * @brief
 *
 * @tparam TypeID: each TypeID share same extension functions
 */
template <uint32_t TypeID>
class Buffer : public vk::Buffer, //
               private VkObject

{
  private:
    inline static BufferFunctions funcs_ = {};

    vk::DeviceSize size_ = 0;
    vk::DeviceMemory memory_ = nullptr;
    vma::Allocation allocation_ = nullptr;
    void* mapping_ = nullptr;

  public:
    static void load_funcs(const BufferFunctions& funcs);

    Buffer(const Buffer&) = delete;
    Buffer(const vk::BufferCreateInfo& buffer_info, //
           vma::AllocationCreateInfo& alloc_info);
    ~Buffer();

    void map_memory();
    void unmap_memory();
    void flush_cache(void* memory, vk::DeviceSize offset = 0, vk::DeviceSize size = 0);
    void invilidate_cache(vk::DeviceSize offset = 0, vk::DeviceSize size = 0);
    [[nodiscard]] bool valid() const;
    [[nodiscard]] void* mapping() const;
    [[nodiscard]] vk::DeviceSize size() const;
    [[nodiscard]] vma::Allocation allocation() const;
    [[nodiscard]] vk::DeviceMemory memory() const;
};

template <uint32_t TypeID>
inline void Buffer<TypeID>::load_funcs(const BufferFunctions& funcs)
{
    funcs_ = funcs;
}

template <uint32_t TypeID>
inline Buffer<TypeID>::Buffer(const vk::BufferCreateInfo& buffer_info, //
                              vma::AllocationCreateInfo& alloc_info)
    : size_(buffer_info.size)
{
    CreateBufferReturn r = funcs_.create_buffer_(details_ptr(), //
                                                 &static_cast<const VkBufferCreateInfo&>(buffer_info),
                                                 &static_cast<const VmaAllocationCreateInfo&>(alloc_info));
    static_cast<vk::Buffer&>(*this) = r.buffer_;
    memory_ = r.memory_;
    allocation_ = r.alloc_;
}

template <uint32_t TypeID>
inline Buffer<TypeID>::~Buffer()
{
    if (valid())
    {
        unmap_memory();
        funcs_.destory_buffer_(details_ptr(), *this, allocation_);
    }
}

template <uint32_t TypeID>
inline void Buffer<TypeID>::map_memory()
{
    if (!mapping_)
    {
        mapping_ = allocator().mapMemory(allocation_);
    }
}

template <uint32_t TypeID>
inline void Buffer<TypeID>::unmap_memory()
{
    if (mapping_)
    {
        allocator().unmapMemory(allocation_);
        mapping_ = nullptr;
    }
}

template <uint32_t TypeID>
inline void Buffer<TypeID>::flush_cache(void* memory, vk::DeviceSize offset, vk::DeviceSize size)
{
    if (!size)
    {
        size = size_;
    }

    memcpy((unsigned char*)(mapping_) + offset, memory, size);
    allocator().flushAllocation(allocation_, offset, size);
}

template <uint32_t TypeID>
inline void Buffer<TypeID>::invilidate_cache(vk::DeviceSize offset, vk::DeviceSize size)
{
    allocator().invalidateAllocation(allocation_, offset, size);
}

template <uint32_t TypeID>
inline bool Buffer<TypeID>::valid() const
{
    return size_ && static_cast<vk::Buffer>(*this);
}

template <uint32_t TypeID>
inline vma::Allocation Buffer<TypeID>::allocation() const
{
    return allocation_;
}

template <uint32_t TypeID>
inline void* Buffer<TypeID>::mapping() const
{
    return mapping_;
}

template <uint32_t TypeID>
inline vk::DeviceMemory Buffer<TypeID>::memory() const
{
    return memory_;
}

template <uint32_t TypeID>
inline vk::DeviceSize Buffer<TypeID>::size() const
{
    return size_;
}

#endif // INCLUDE_BUFFER_HPP
