#include "swapchain.hpp"

void Swapchain::create()
{
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window(), &w, &h);
    create(vk::Extent2D(w, h));
}

void Swapchain::create(const vk::Extent2D& extent)
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

    views_.resize(images_.size());
    for (int i = 0; i < images_.size(); i++)
    {
        vk::ImageViewCreateInfo create_info{};
        create_info.image = images_[i];
        create_info.viewType = vk::ImageViewType::e2D;
        create_info.format = image_format_;
        create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        views_[i] = device().createImageView(create_info);
    }
}

void Swapchain::destory()
{
    for (auto view : views_)
    {
        device().destroyImageView(view);
        view = nullptr;
    }
    device().destroySwapchainKHR(*this);
}

uint32_t Swapchain::aquire_next_image(vk::Semaphore sem, vk::Fence fence, uint64_t timeout)
{
    return device().acquireNextImageKHR(*this, timeout, sem, fence).value;
}
