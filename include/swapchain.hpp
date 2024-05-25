#ifndef INCLUDE_SWAPCHAIN_HPP
#define INCLUDE_SWAPCHAIN_HPP

#include "vk_base.hpp"

struct Swapchain : public vk::SwapchainKHR, //
                   private VkObject

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
    vk::Result present(const std::vector<vk::Semaphore>& wait_sems);
};

#endif // INCLUDE_SWAPCHAIN_HPP
