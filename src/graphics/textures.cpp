#include "graphics/textures.hpp"

fi::gfx::tex_arr::tex_arr()
{
    vk::FenceCreateInfo fence_info{};
    fence_ = device().createFence(fence_info);
}

fi::gfx::tex_arr::~tex_arr()
{
    for (vk::Sampler sampler : samplers_)
    {
        device().destroySampler(sampler);
    }

    for (size_t i = 0; i < images_.size(); i++)
    {
        allocator().destroyImage(images_[i], allocs_[i]);
        device().destroyImageView(views_[i]);
    }

    device().destroyFence(fence_);
}

void fi::gfx::tex_arr::add_sampler(const vk::SamplerCreateInfo& sampler_info)
{
    samplers_.push_back(device().createSampler(sampler_info));
}

void fi::gfx::tex_arr::add_tex(vk::CommandPool cmd_pool,
                               uint32_t sampler_idx,
                               const std::vector<std::byte>& data,
                               vk::Extent3D extent,
                               uint32_t levels)
{
    vk::ImageCreateInfo image_info{};
    image_info.imageType = vk::ImageType::e2D;
    image_info.extent = extent;
    image_info.mipLevels = levels;
    image_info.arrayLayers = 1;
    image_info.format = vk::Format::eR8G8B8A8Srgb;
    image_info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc //
                       | vk::ImageUsageFlagBits::eSampled;
    image_info.samples = vk::SampleCountFlagBits::e1;
    vma::AllocationCreateInfo alloc_info{.usage = vma::MemoryUsage::eAutoPreferDevice};
    auto image = allocator().createImage(image_info, alloc_info);
    images_.push_back(image.first);
    allocs_.push_back(image.second);
    desc_infos_.push_back(vk::DescriptorImageInfo{.sampler = samplers_[sampler_idx],
                                                  .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal});

    vk::ImageViewCreateInfo view_info{};
    view_info.image = image.first;
    view_info.viewType = vk::ImageViewType::e2D;
    view_info.format = vk::Format::eR8G8B8A8Srgb;
    view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    views_.push_back(device().createImageView(view_info));

    vk::BufferCreateInfo buffer_info{.size = data.size(),
                                     .usage = vk::BufferUsageFlagBits::eTransferSrc | //
                                              vk::BufferUsageFlagBits::eVertexBuffer};
    alloc_info = {.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
                           vma::AllocationCreateFlagBits::eMapped,
                  .usage = vma::MemoryUsage::eAutoPreferHost,
                  .requiredFlags = vk::MemoryPropertyFlagBits::eHostCached};
    vma::AllocationInfo alloc_result{};
    auto buffer = allocator().createBuffer(buffer_info, alloc_info, alloc_result);
    memcpy(alloc_result.pMappedData, data.data(), util::sizeof_arr(data));

    // process the image

    vk::CommandBufferAllocateInfo cmd_alloc{.commandPool = cmd_pool, //
                                            .commandBufferCount = 1};
    vk::CommandBuffer cmd = device().allocateCommandBuffers(cmd_alloc)[0];
    vk::CommandBufferBeginInfo begin{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    vk::ImageMemoryBarrier barrier{};
    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.first;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    vk::BufferImageCopy region{};
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = extent;

    cmd.begin(begin);
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                        {}, {}, {}, barrier);
    cmd.copyBufferToImage(buffer.first, barrier.image, vk::ImageLayout::eTransferDstOptimal, region);

    int32_t mip_w = extent.width;
    int32_t mip_h = extent.height;
    for (int i = 1; i < levels; i++)
    {
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.subresourceRange.baseMipLevel = i - 1;
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, //
                            {}, {}, {}, barrier);

        vk::ImageBlit blit{};
        blit.srcOffsets[1] = vk::Offset3D(mip_w, mip_h, 1);
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[1] = vk::Offset3D(mip_w > 1 ? mip_w / 2 : 1, //
                                          mip_h > 1 ? mip_h / 2 : 1, //
                                          1);
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        cmd.blitImage(barrier.image, vk::ImageLayout::eTransferSrcOptimal, //
                      barrier.image, vk::ImageLayout::eTransferDstOptimal, //
                      blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.subresourceRange.baseMipLevel = i - 1;
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, //
                            {}, {}, {}, barrier);

        mip_w > 1 ? mip_w /= 2 : mip_w;
        mip_h > 1 ? mip_h /= 2 : mip_h;
    }
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.subresourceRange.baseMipLevel = levels - 1;
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, //
                        {}, {}, {}, barrier);
    cmd.end();

    vk::SubmitInfo submit_info{};
    submit_info.setCommandBuffers(cmd);

    device().resetFences(fence_);
    queues().submit(submit_info, fence_);
    if (device().waitForFences(fence_, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("texture add fail");
    }

    allocator().destroyBuffer(buffer.first, buffer.second);
}