#ifndef GRAPHICS_BUFFER_HPP
#define GRAPHICS_BUFFER_HPP

#include "vk_base.hpp"
#include "tools.hpp"

struct BufferBase : public vk::Buffer,      //
                    public vma::Allocation, //
                    protected VkObject
{
  protected:
    vk::DeviceSize size_ = 0;
    std::byte* mapping_ = nullptr;

    void create_buffer(const vk::BufferCreateInfo& buffer_info, const vma::AllocationCreateInfo& alloc_info);

  public:
    ~BufferBase();

    std::byte* map_memory();
    void unmap_memory();
    void flush_cache(std::byte* memory, vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);
    void invilidate_cache(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);
    [[nodiscard]] bool valid() const;
    [[nodiscard]] std::byte* mapping() const;
    [[nodiscard]] vk::DeviceSize size() const;
};

template <typename TypeBase>
concept BufferType                                                                                      //
    = requires(TypeBase type, vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info) //
{ type.config_buffer(buffer_info, alloc_info); };

struct Uniform
{
    static void config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
};

struct Storage
{
    static void config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
};

struct Index
{
    static void config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
};

struct Vertex
{
    static void config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
};

struct Indirect
{
    static void config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
};

struct HostAccess
{
    static void config_buffer(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
};

enum Transfer : uint32_t
{
    DST = bit_shift_left(0),
    SRC = bit_shift_left(1)
};

template <BufferType... BT>
struct Buffer : public BufferBase, //
                private BT...
{
  public:
    Buffer(vk::DeviceSize size, uint32_t transfer_flag = {});
};

template <BufferType... BT>
inline Buffer<BT...>::Buffer(vk::DeviceSize size, uint32_t transfer_flag)
{
    vk::BufferCreateInfo buffer_info{};
    vma::AllocationCreateInfo alloc_info{};

    if (transfer_flag & DST)
    {
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
    }
    if (transfer_flag & SRC)
    {
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferSrc;
    }

    buffer_info.size = size;
    alloc_info.usage = vma::MemoryUsage::eAutoPreferDevice;
    (BT::config_buffer(buffer_info, alloc_info), ...);
    create_buffer(buffer_info, alloc_info);
}

#endif // GRAPHICS_BUFFER_HPP
