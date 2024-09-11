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
        std::vector<gfx::gfx_pipeline*> pipelines_;

        virtual ~render() = default;

        virtual void construct() = 0;
        virtual func get_frame_func() = 0;
    };
}; // namespace fi::mgr

#endif // MGRS_RENDER_MGR_HPP