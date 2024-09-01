/**
 * @file swapchain.cpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
#include "graphics/swapchain.hpp"

void fi::gfx::swapchain::create()
{
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window(), &w, &h);
    create(vk::Extent2D(w, h));
}

void fi::gfx::swapchain::create(const vk::Extent2D& extent)
{
    std::vector<vk::SurfaceFormatKHR> surface_formats = physical().getSurfaceFormatsKHR(surface());
    std::vector<vk::PresentModeKHR> present_modes = physical().getSurfacePresentModesKHR(surface());
    vk::SurfaceCapabilitiesKHR capabilities = physical().getSurfaceCapabilitiesKHR(surface());

    vk::SurfaceFormatKHR selected_format = surface_formats[0];
    vk::PresentModeKHR selected_present_mode = vk::PresentModeKHR::eFifo;

    for (vk::SurfaceFormatKHR& format : surface_formats)
    {
        if (format.format == vk::Format::eR8G8B8A8Srgb && //
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            selected_format = format;
            break;
        }
    }

    for (vk::PresentModeKHR present_mode : present_modes)
    {
        if (present_mode == vk::PresentModeKHR::eMailbox)
        {
            selected_present_mode = present_mode;
            break;
        }
    }

    uint32_t image_count = 3;
    vk::SwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.surface = surface();
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = selected_format.format;
    swapchain_create_info.imageColorSpace = selected_format.colorSpace;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | //
                                       vk::ImageUsageFlagBits::eInputAttachment;
    swapchain_create_info.preTransform = capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchain_create_info.presentMode = selected_present_mode;
    swapchain_create_info.clipped = true;
    swapchain_create_info.oldSwapchain = nullptr;

    static_cast<vk::SwapchainKHR&>(*this) = device().createSwapchainKHR(swapchain_create_info);
    images_ = device().getSwapchainImagesKHR(*this);
    images_.reserve(image_count);
    image_format_ = selected_format.format;

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.queueFamilyIndex = queue_indices(GRAPHICS);
    vk::CommandPool cmd_pool = device().createCommandPool(pool_info);

    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.commandPool = cmd_pool;
    alloc_info.level = vk::CommandBufferLevel::ePrimary;
    alloc_info.commandBufferCount = 1;
    vk::CommandBuffer cmd = device().allocateCommandBuffers(alloc_info)[0];

    vk::CommandBufferBeginInfo begin{};
    begin.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(begin);
    for (auto image : images_)
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.image = image;
        barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe, //
                            {}, {}, {}, barrier);
    }
    cmd.end();

    vk::SubmitInfo submit{};
    submit.setCommandBuffers(cmd);
    queues(GRAPHICS).submit(submit);
    queues(GRAPHICS).waitIdle();
    device().destroyCommandPool(cmd_pool);
}

void fi::gfx::swapchain::destory()
{
    device().destroySwapchainKHR(*this);
}

uint32_t fi::gfx::swapchain::aquire_next_image(vk::Semaphore sem, vk::Fence fence, uint64_t timeout)
{
    curr_idx_ = device().acquireNextImageKHR(*this, timeout, sem, fence).value;
    return curr_idx_;
}

vk::Result fi::gfx::swapchain::present(const vk::ArrayProxyNoTemporaries<const vk::Semaphore>& wait_sems)
{
    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphores(wait_sems);
    present_info.swapchainCount = 1;
    present_info.pSwapchains = this;
    present_info.pImageIndices = &curr_idx_;

    return queues(GRAPHICS).presentKHR(present_info);
}