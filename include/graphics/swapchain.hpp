/**
 * @file sw_chain.hpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
#ifndef GRAPHICS_SWAPCHAIN_HPP
#define GRAPHICS_SWAPCHAIN_HPP

#include "graphics.hpp"

namespace fi::gfx
{
    struct swapchain : public vk::SwapchainKHR, //
                       private graphcis_obj
    {
      private:
        uint32_t curr_idx_ = 0;

      public:
        swapchain() = default;
        swapchain(const swapchain&) = delete;
        swapchain(swapchain&&) = delete;
        swapchain& operator=(const swapchain&) = delete;
        swapchain& operator=(swapchain&&) = delete;
        std::vector<vk::Image> images_{};
        vk::Format image_format_{};

        operator vk::SwapchainKHR*() { return this; }

        void create();
        void create(const vk::Extent2D& extent);
        void destory();

        vk::Result aquire_next_image(uint32_t& image_idx,
                                     vk::Semaphore sem = nullptr,
                                     vk::Fence fence = nullptr,
                                     uint64_t timeout = std::numeric_limits<uint64_t>::max());
        vk::Result present(const vk::ArrayProxyNoTemporaries<const vk::Semaphore>& wait_sems);
    };
}; // namespace fi::gfx

#endif // GRAPHICS_SWAPCHAIN_HPP
