#include "graphics/texture.hpp"
#include "tools.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

fi::Texture::operator vk::DescriptorImageInfo() const
{
    vk::DescriptorImageInfo info{};
    info.imageView = image_view_;
    info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    info.sampler = sampler_;
    return info;
}

fi::TextureMgr::TextureMgr()
{
    if (images_.contains("res/textures/blank.png"))
    {
        return;
    }

    std::ifstream buffer;
    buffer.open("res/textures/blank.png", std::ios::binary);
    buffer.seekg(0, std::ios::end);
    size_t file_size = buffer.tellg();
    buffer.seekg(0, std::ios::beg);

    gltf::StaticVector<std::uint8_t> bytes(file_size);
    buffer.read(castf(char*, bytes.data()), file_size);
    Texture tmp = load_texture("res/textures/blank.png");
}

fi::TextureMgr::~TextureMgr()
{
}

fi::Texture& fi::TextureMgr::load_texture(const std::string& name, bool mip_mapping)
{
    if (images_.contains(name))
    {
        return images_[name];
    }

    fi::TextureStorage& storage = images_[name];
    storage.name_ = name;
    int w = 0, h = 0, chan = 0;
    stbi_uc* pixels = stbi_load(name.c_str(), &w, &h, &chan, STBI_rgb_alpha);
    if (pixels == nullptr)
    {
        throw std::runtime_error(std::format("{} texture can not be loaded", name));
    }
    storage.extent_ = vk::Extent3D(w, h, 1);
    storage.levels_ = mip_mapping ? std::floor(std::log2(std::max(w, h))) + 1 : 1;

    vk::BufferCreateInfo buffer_info{};
    buffer_info.size = w * h * STBI_rgb_alpha;
    buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
    vma::AllocationCreateInfo alloc_info{};
    alloc_info.usage = vma::MemoryUsage::eAutoPreferHost;
    alloc_info.preferredFlags = vk::MemoryPropertyFlagBits::eHostCoherent;
    alloc_info.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
    auto staging = allocator().createBuffer(buffer_info, alloc_info);
    void* mapping = allocator().mapMemory(staging.second);
    memcpy(mapping, pixels, buffer_info.size);
    stbi_image_free(pixels);
    allocator().unmapMemory(staging.second);

    vk::ImageCreateInfo image_info{};
    image_info.imageType = vk::ImageType::e2D;
    image_info.extent = storage.extent_;
    image_info.extent.depth = 1;
    image_info.mipLevels = storage.levels_;
    image_info.arrayLayers = 1;
    image_info.format = vk::Format::eR8G8B8A8Srgb;
    image_info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc //
                       | vk::ImageUsageFlagBits::eSampled;
    image_info.samples = vk::SampleCountFlagBits::e1;
    alloc_info.usage = vma::MemoryUsage::eAutoPreferDevice;
    alloc_info.preferredFlags = {};
    alloc_info.flags = {};
    auto tex_alloca = allocator().createImage(image_info, alloc_info);
    storage.image_ = tex_alloca.first;
    storage.allocation_ = tex_alloca.second;

    vk::ImageViewCreateInfo view_info{};
    view_info.image = tex_alloca.first;
    view_info.viewType = vk::ImageViewType::e2D;
    view_info.format = vk::Format::eR8G8B8A8Srgb;
    view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = storage.levels_;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    storage.image_view_ = device().createImageView(view_info);

    vk::ImageMemoryBarrier barrier{};
    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex_alloca.first;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = storage.levels_;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    vk::BufferImageCopy region{};
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = storage.extent_;

    vk::CommandBuffer cmd = one_time_submit_cmd();
    begin_cmd(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                        {}, {}, {}, barrier);
    cmd.copyBufferToImage(staging.first, tex_alloca.first, vk::ImageLayout::eTransferDstOptimal, region);

    int32_t mip_w = w;
    int32_t mip_h = h;
    for (int i = 1; i < storage.levels_; i++)
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
        cmd.blitImage(tex_alloca.first, vk::ImageLayout::eTransferSrcOptimal, //
                      tex_alloca.first, vk::ImageLayout::eTransferDstOptimal, //
                      blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.subresourceRange.baseMipLevel = i - 1;
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, //
                            {}, {}, {}, barrier);

        mip_w > 1 ? mip_w /= 2 : mip_w;
        mip_h > 1 ? mip_h /= 2 : mip_h;
    }
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.subresourceRange.baseMipLevel = storage.levels_ - 1;
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, //
                        {}, {}, {}, barrier);
    cmd.end();
    submit_one_time_cmd(cmd);
    allocator().destroyBuffer(staging.first, staging.second);

    return storage;
}

fi::Texture& fi::TextureMgr::get_texture(const std::string& name)
{
    auto result = images_.find(name);
    if (result != images_.end())
    {
        return result->second;
    }
    return images_["res/textures/blank.png"];
}

void fi::TextureMgr::remove_image(const std::string& file_path)
{
    images_.erase(file_path);
}

fi::TextureStorage::~TextureStorage()
{
    device().destroyImageView(image_view_);
    allocator().destroyImage(image_, allocation_);
}