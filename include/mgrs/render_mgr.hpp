#ifndef MGRS_RENDER_MGR_HPP
#define MGRS_RENDER_MGR_HPP

#include "graphics/shader.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/swapchain.hpp"
#include "extensions/cpp_defines.hpp"
#include "extensions/dll.hpp"

namespace fi::mgr
{
    struct render : protected gfx::graphcis_obj
    {
        using func = std::function<void(const std::vector<vk::SemaphoreSubmitInfo>& waits,
                                        const std::vector<vk::SemaphoreSubmitInfo>& signals,
                                        const std::function<void()>& deffered)>;

        std::string name_ = "";
        func frame_func_{};
        std::vector<gfx::gfx_pipeline*> pipelines_;

        std::vector<vk::Image> images_{};
        std::vector<vk::ImageView> imag_views_{};
        std::vector<vma::Allocation> image_allocs_{};

        std::vector<vk::Buffer> buffers_{};
        std::vector<vma::Allocation> buffer_allocs_{};

        virtual ~render()
        {
            for (size_t i = 0; i < images_.size(); i++)
            {
                allocator().destroyImage(images_[i], image_allocs_[i]);
            }

            for (vk::ImageView view : imag_views_)
            {
                device().destroyImageView(view);
            }

            for (size_t i = 0; i < buffers_.size(); i++)
            {
                allocator().destroyBuffer(buffers_[i], buffer_allocs_[i]);
            }
        }

        virtual void construct() = 0;
        void draw_frame(
            const std::vector<vk::SemaphoreSubmitInfo>& waits,
            const std::vector<vk::SemaphoreSubmitInfo>& signals,
            const std::function<void()>& deffered = []() {})
        {
            frame_func_(waits, signals, deffered);
        }
    };
}; // namespace fi::mgr

#endif // MGRS_RENDER_MGR_HPP