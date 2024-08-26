#include "graphics/prims.hpp"

fi::Primitives::Primitives(vk::DeviceSize max_data_size, uint32_t max_prim_count)
    : max_size_(max_data_size),
      max_prims_(max_prim_count)
{
    setter_.prims_ = this;

    vk::BufferCreateInfo data_buffer_info{.size = max_data_size,
                                          .usage = vk::BufferUsageFlagBits::eTransferDst |
                                                   vk::BufferUsageFlagBits::eStorageBuffer |
                                                   vk::BufferUsageFlagBits::eShaderDeviceAddress};
    vma::AllocationCreateInfo data_alloc_info{.usage = vma::MemoryUsage::eAutoPreferDevice};
    auto allocated = allocator().createBuffer(data_buffer_info, data_alloc_info);
    data_buffer_ = allocated.first;
    data_alloc_ = allocated.second;
    vk::BufferDeviceAddressInfo address_info{.buffer = data_buffer_};
    addresses_.data_buffer_ = device().getBufferAddress(address_info);

    vk::BufferCreateInfo prim_buffer_info{.size = max_prim_count * (sizeof(vk::DrawIndexedIndirectCommand) //
                                                                    + sizeof(PrimInfo)),
                                          .usage = vk::BufferUsageFlagBits::eTransferDst      //
                                                   | vk::BufferUsageFlagBits::eIndirectBuffer //
                                                   | vk::BufferUsageFlagBits::eStorageBuffer  //
                                                   | vk::BufferUsageFlagBits::eShaderDeviceAddress};
    vma::AllocationCreateInfo prim_alloc_info{.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
                                                       vma::AllocationCreateFlagBits::eMapped,
                                              .usage = vma::MemoryUsage::eAutoPreferDevice};
    vma::AllocationInfo alloc_info{};
    draw_call_offset_ = prim_buffer_info.size / 2;
    allocated = allocator().createBuffer(prim_buffer_info, prim_alloc_info, alloc_info);
    prim_buffer_ = allocated.first;
    prim_alloc_ = allocated.second;
    prim_mapping_ = (std::byte*)alloc_info.pMappedData;
    address_info.buffer = prim_buffer_;
    addresses_.prim_buffer_ = device().getBufferAddress(address_info);
}

fi::Primitives::~Primitives()
{
    allocator().destroyBuffer(data_buffer_, data_alloc_);
    allocator().destroyBuffer(prim_buffer_, prim_alloc_);
}

fi::Primitives::AttribSetter& fi::Primitives::add_primitives(
    const std::vector<vk::DrawIndexedIndirectCommand>& draw_calls)
{
    memcpy(prim_mapping_ + draw_call_offset_, draw_calls.data(), sizeof_arr(draw_calls));
    setter_.staging_.resize(draw_calls.size());
    return setter_;
}

vk::DeviceSize fi::Primitives::AttribSetter::alloc_mem(vk::DeviceSize required)
{
    std::lock_guard lk(alloc_lock_);
    while (prims_->curr_size_ % 16)
    {
        prims_->curr_size_ += 4;
    }

    prims_->curr_size_ += required;
    if (prims_->curr_size_ > prims_->max_size_)
    {
        throw std::runtime_error("Not enougth memory");
    }
    return prims_->curr_size_ - required;
}

void fi::Primitives::AttribSetter::copy_buffer(vk::CommandPool submission_pool,
                                               vk::Buffer src,
                                               vk::Buffer dst,
                                               vk::BufferCopy region)
{
    std::lock_guard lk(copy_lock_);
    vk::CommandBufferAllocateInfo cmd_info{.commandPool = submission_pool, //
                                           .commandBufferCount = 1};

    vk::CommandBuffer cmd = device().allocateCommandBuffers(cmd_info)[0];
    vk::CommandBufferBeginInfo begin{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    cmd.begin(begin);
    cmd.copyBuffer(src, dst, region);
    cmd.end();

    Fence fence;
    device().resetFences(fence);

    vk::SubmitInfo submit{};
    submit.setCommandBuffers(cmd);
    queues(GRAPHICS).submit(submit, fence);
    auto r = device().waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
}

void fi::Primitives::AttribSetter::update_gpu() { free_stl_container(staging_); }
