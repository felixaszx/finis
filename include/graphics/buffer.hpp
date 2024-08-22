/**
 * @file buffer.hpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
#ifndef GRAPHICS_BUFFER_HPP
#define GRAPHICS_BUFFER_HPP

#include "graphics.hpp"
#include "tools.hpp"

namespace fi
{
    struct BufferBase : public vk::Buffer, //
                        public vma::Allocation,
                        protected GraphicsObject
    {
      protected:
        bool presistant_ = false;
        vk::DeviceSize size_ = 0;
        std::byte* mapping_ = nullptr;

        void create_buffer(const vk::BufferCreateInfo& buffer_info,
                           const vma::AllocationCreateInfo& alloc_info);

      public:
        struct EmptyExtraInfo
        {
        };
        ~BufferBase();

        std::byte* map_memory();
        void unmap_memory();
        void flush_cache(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);
        void invilidate_cache(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);
        [[nodiscard]] bool valid() const;
        [[nodiscard]] std::byte* mapping() const;
        [[nodiscard]] vk::DeviceSize size() const;
    };

    using BufferConfigFunc = void(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void uniform(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void storage(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void index(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void vertex(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void indirect(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void seq_write(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void presistant(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void hosted(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void host_coherent(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void host_cached(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);
    void device_address(vk::BufferCreateInfo& buffer_info, vma::AllocationCreateInfo& alloc_info);

    enum Transfer : uint32_t
    {
        DST = bit_shift_left(0),
        SRC = bit_shift_left(1)
    };

    template <typename ExtraInfo, BufferConfigFunc... CF>
    struct Buffer : public BufferBase, //
                    public ExtraInfo
    {
      public:
        Buffer(vk::DeviceSize size, uint32_t transfer_flag = {});
        Buffer(Buffer&& target) noexcept;
    };

    template <typename ExtraInfo, BufferConfigFunc... CF>
    inline Buffer<ExtraInfo, CF...>::Buffer(vk::DeviceSize size, uint32_t transfer_flag)
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
        (CF(buffer_info, alloc_info), ...);

        create_buffer(buffer_info, alloc_info);
    }

    template <typename ExtraInfo, BufferConfigFunc... CF>
    inline Buffer<ExtraInfo, CF...>::Buffer(Buffer&& target) noexcept
    {
        memcpy(this, &target, sizeof(Buffer<ExtraInfo, CF...>));
        casts(vk::Buffer&, target) = nullptr;
    }
}; // namespace fi

#endif // GRAPHICS_BUFFER_HPP