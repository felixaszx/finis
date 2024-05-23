#ifndef INCLUDE_SWAPCHAIN_HPP
#define INCLUDE_SWAPCHAIN_HPP

#include "vk_base.hpp"

struct Swapchain : private VkObject, //
                   public vk::SwapchainKHR

{
    std::vector<vk::Image> images_{};
    std::vector<vk::ImageView> views_{};
    vk::Format image_format_{};

    operator vk::SwapchainKHR*() { return this; }

    void create();
    void create(const vk::Extent2D& extent);
    void destory();

    uint32_t aquire_next_image(vk::Semaphore sem = nullptr, vk::Fence fence = nullptr,
                               uint64_t timeout = std::numeric_limits<uint64_t>::max());
};

#endif // INCLUDE_SWAPCHAIN_HPP
