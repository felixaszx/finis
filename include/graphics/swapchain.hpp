/**
 * @file swapchain.hpp
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

namespace fi
{
    struct Swapchain : public vk::SwapchainKHR, //
                       private GraphicsObject
    {
      private:
        uint32_t curr_idx_ = 0;

      public:
        std::vector<vk::Image> images_{};
        std::vector<vk::ImageView> views_{};
        vk::Format image_format_{};

        operator vk::SwapchainKHR*() { return this; }

        void create();
        void create(const vk::Extent2D& extent);
        void destory();

        uint32_t aquire_next_image(vk::Semaphore sem = nullptr, vk::Fence fence = nullptr,
                                   uint64_t timeout = std::numeric_limits<uint64_t>::max());
        vk::Result present(const vk::ArrayProxyNoTemporaries<const vk::Semaphore> & wait_sems);
    };
}; // namespace fi

#endif // GRAPHICS_SWAPCHAIN_HPP
