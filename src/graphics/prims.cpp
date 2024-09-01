#include "graphics/prims.hpp"

fi::gfx::primitives::primitives(vk::DeviceSize data_size_limit, uint32_t prim_limit)
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
    buffer_info = {.size = prims_.capacity_ * (sizeof(vk::DrawIndirectCommand) + sizeof(prim_info)), //
                   .usage = vk::BufferUsageFlagBits::eTransferDst |                                 //
                            vk::BufferUsageFlagBits::eStorageBuffer |                               //
                            vk::BufferUsageFlagBits::eShaderDeviceAddress |                         //
                            vk::BufferUsageFlagBits::eIndirectBuffer};
    allocated = allocator().createBuffer(buffer_info, alloc_info);
    prims_.buffer_ = allocated.first;
    prims_.alloc_ = allocated.second;
    address_info.buffer = data_.buffer_;
    addresses_.prim_buffer_ = device().getBufferAddress(address_info);
    draw_calls_.reserve(prim_limit);
    prim_infos_.reserve(prim_limit);
}

fi::gfx::primitives::~primitives()
{
    free_staging_buffer();
    allocator().destroyBuffer(data_.buffer_, data_.alloc_);
    allocator().destroyBuffer(prims_.buffer_, prims_.alloc_);
}

void fi::gfx::primitives::generate_staging_buffer(vk::DeviceSize limit)
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

void fi::gfx::primitives::flush_staging_memory(vk::CommandPool pool)
{
    if (staging_span_.empty())
    {
        return;
    }

    Fence fence;
    device().resetFences(fence);
    vk::CommandBufferAllocateInfo cmd_alloc{.commandPool = pool, //
                                            .commandBufferCount = 1};
    vk::CommandBuffer cmd = device().allocateCommandBuffers(cmd_alloc)[0];
    vk::CommandBufferBeginInfo begin{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    cmd.begin(begin);
    while (!staging_span_.empty())
    {
        vk::DeviceSize dst_offset = staging_queue_.front();
        auto blocks = staging_span_.front_block_region();
        vk::BufferCopy region{.srcOffset = staging_span_.offset(blocks[0]), //
                              .dstOffset = dst_offset,
                              .size = blocks[0].size_};
        cmd.copyBuffer(staging_buffer_, data_.buffer_, region);
        if (blocks[1].size_)
        {
            region.srcOffset = staging_span_.offset(blocks[1]);
            region.dstOffset = dst_offset + blocks[0].size_;
            region.size = blocks[1].size_;
            cmd.copyBuffer(staging_buffer_, data_.buffer_, region);
        }
        staging_span_.pop_front();
        staging_queue_.pop();
    }
    cmd.end();

    vk::CommandBufferSubmitInfo cmd_submit{.commandBuffer = cmd};
    vk::SubmitInfo2 submit2{};
    submit2.setCommandBufferInfos(cmd_submit);
    queues(GRAPHICS).submit2(submit2, fence);
    staging_span_.reset();
    if (device().waitForFences(fence, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Memory copy over time.");
    }
    device().freeCommandBuffers(pool, cmd);
}

void fi::gfx::primitives::free_staging_buffer()
{
    if (staging_buffer_)
    {
        allocator().destroyBuffer(staging_buffer_, staging_alloc_);
        staging_buffer_ = nullptr;
        staging_alloc_ = nullptr;
        staging_span_.reference(nullptr, 0);
    }
}

uint32_t fi::gfx::primitives::add_primitives(const std::vector<vk::DrawIndirectCommand>& draw_calls)
{
    curr_prim_ = prims_.count_;
    if (prims_.capacity_ - prims_.count_ < draw_calls.size())
    {
        return prims_.capacity_ - prims_.count_;
    }
    prims_.count_ += draw_calls.size();
    prim_infos_.resize(prims_.count_);
    draw_calls_.insert(draw_calls_.end(), draw_calls.begin(), draw_calls.end());
    return EMPTY;
}

void fi::gfx::primitives::reload_draw_calls(vk::CommandPool pool)
{
    flush_staging_memory(pool);
    staging_span_.push_back(castr(const std::byte*, prim_infos_.data()), sizeof_arr(prim_infos_));
    staging_span_.push_back(castr(const std::byte*, draw_calls_.data()), sizeof_arr(draw_calls_));

    Fence fence;
    device().resetFences(fence);
    vk::CommandBufferAllocateInfo cmd_alloc{.commandPool = pool, //
                                            .commandBufferCount = 1};
    vk::CommandBuffer cmd = device().allocateCommandBuffers(cmd_alloc)[0];
    vk::CommandBufferBeginInfo begin{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    cmd.begin(begin);
    vk::DeviceSize dst_offset = 0;
    while (!staging_span_.empty())
    {
        auto blocks = staging_span_.front_block_region();
        vk::BufferCopy region{.srcOffset = staging_span_.offset(blocks[0]), //
                              .dstOffset = dst_offset,
                              .size = blocks[0].size_};
        cmd.copyBuffer(staging_buffer_, prims_.buffer_, region);
        if (blocks[1].size_)
        {
            region.srcOffset = staging_span_.offset(blocks[1]);
            region.dstOffset = dst_offset + blocks[0].size_;
            region.size = blocks[1].size_;
            cmd.copyBuffer(staging_buffer_, prims_.buffer_, region);
        }
        dst_offset += sizeof_arr(prim_infos_);
        staging_span_.pop_front();
    }
    cmd.end();

    vk::CommandBufferSubmitInfo cmd_submit{.commandBuffer = cmd};
    vk::SubmitInfo2 submit2{};
    submit2.setCommandBufferInfos(cmd_submit);
    queues(GRAPHICS).submit2(submit2, fence);
    staging_span_.reset();
    if (device().waitForFences(fence, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Memory copy over time.");
    }
    device().freeCommandBuffers(pool, cmd);
}

vk::DeviceSize fi::gfx::primitives::load_staging_memory(const std::byte* data, vk::DeviceSize size)
{
    data_.curr_size_ += size;
    if (data_.curr_size_ > data_.capacity_ || size > staging_span_.capacity())
    {
        throw std::runtime_error("Not enought reserved memory");
    }

    if (staging_span_.remainning() < size)
    {
        return EMPTY_L;
    }
    staging_span_.push_back(data, size);
    staging_queue_.push(data_.curr_size_ - size);
    return data_.curr_size_ - size;
}