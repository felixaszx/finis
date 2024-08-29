#include "graphics/prims.hpp"

fi::graphics::Primitives::Primitives(vk::DeviceSize data_size_limit, uint32_t prim_limit)
    : data_(0, data_size_limit),
      prims_(0, prim_limit)
{
    vk::BufferCreateInfo buffer_info{.size = data_.capacity_,                         //
                                     .usage = vk::BufferUsageFlagBits::eTransferDst | //
                                              vk::BufferUsageFlagBits::eStorageBuffer |
                                              vk::BufferUsageFlagBits::eShaderDeviceAddress};
    vma::AllocationCreateInfo alloc_info{.usage = vma::MemoryUsage::eAutoPreferDevice};
    auto allocated = allocator().createBuffer(buffer_info, alloc_info);
    data_.buffer_ = allocated.first;
    data_.alloc_ = allocated.second;
    vk::BufferDeviceAddressInfo address_info{.buffer = data_.buffer_};
    addresses_.data_buffer_ = device().getBufferAddress(address_info);

    buffer_info = vk::BufferCreateInfo();
    buffer_info = {.size = prims_.capacity_ * (sizeof(vk::DrawIndirectCommand) + sizeof(PrimInfo)), //
                   .usage = vk::BufferUsageFlagBits::eTransferDst |                                 //
                            vk::BufferUsageFlagBits::eStorageBuffer |                               //
                            vk::BufferUsageFlagBits::eShaderDeviceAddress |                         //
                            vk::BufferUsageFlagBits::eIndirectBuffer};
    allocated = allocator().createBuffer(buffer_info, alloc_info);
    prims_.buffer_ = allocated.first;
    prims_.alloc_ = allocated.second;
    address_info.buffer = data_.buffer_;
    addresses_.prim_buffer_ = device().getBufferAddress(address_info);
}

fi::graphics::Primitives::~Primitives()
{
    free_staging_buffer();
    allocator().destroyBuffer(data_.buffer_, data_.alloc_);
    allocator().destroyBuffer(prims_.buffer_, prims_.alloc_);
}

void fi::graphics::Primitives::generate_staging_buffer(vk::DeviceSize limit)
{
    free_staging_buffer();
    vk::BufferCreateInfo buffer_info{.size = limit, //
                                     .usage = vk::BufferUsageFlagBits::eTransferSrc |
                                              vk::BufferUsageFlagBits::eVertexBuffer};
    vma::AllocationCreateInfo alloc_info{.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
                                                  vma::AllocationCreateFlagBits::eMapped,
                                         .usage = vma::MemoryUsage::eAutoPreferHost,
                                         .requiredFlags = vk::MemoryPropertyFlagBits::eHostCached};
    vma::AllocationInfo alloc{};
    auto allocated = allocator().createBuffer(buffer_info, alloc_info, alloc);
    staging_buffer_ = allocated.first;
    staging_alloc_ = allocated.second;
    staging_span_.reference(alloc.pMappedData, limit);
}

void fi::graphics::Primitives::free_staging_buffer()
{
    if (staging_buffer_)
    {
        allocator().destroyBuffer(staging_buffer_, staging_alloc_);
        staging_buffer_ = nullptr;
        staging_alloc_ = nullptr;
        staging_span_.reference(nullptr, 0);
    }
}

bool fi::graphics::Primitives::allocate_staging_memory(std::byte* data, vk::DeviceSize size)
{
    static std::mutex lock;
    std::lock_guard guard(lock);
    return staging_span_.push_back(data, size);
}

bool fi::graphics::Primitives::allocate_data_memory(vk::CommandPool cmd_pool)
{
    static std::mutex lock;
    std::lock_guard guard(lock);
    data_.curr_size_ += staging_span_.front_block_size();
    if (data_.curr_size_ < data_.capacity_)
    {
        return true;
    }
    return false;
}
