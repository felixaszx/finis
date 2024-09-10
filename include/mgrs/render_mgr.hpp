#ifndef MGRS_RENDER_MGR_HPP
#define MGRS_RENDER_MGR_HPP

#include "graphics/prim_res.hpp"
#include "graphics/shader.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/swapchain.hpp"
#include "extensions/cpp_defines.hpp"
#include "extensions/dll.hpp"

namespace fi::mgr
{
    struct render_pkg
    {
        gfx::primitives* prims_ = nullptr;
        gfx::prim_structure* structs_ = nullptr;
        gfx::prim_skins* skins_ = nullptr;
        gfx::tex_arr* tex_arr_ = nullptr;
    };

    struct pipeline
    {
        vk::Pipeline pipeline_ = nullptr;
        vk::PipelineLayout layout_ = nullptr;
        std::vector<size_t> pkg_idxs_{};
    };

    struct render : protected gfx::graphcis_obj
    {
        using func = std::function<void(const std::vector<vk::SemaphoreSubmitInfo>& waits,
                                        const std::vector<vk::SemaphoreSubmitInfo>& signals,
                                        const std::function<void()>& deffered)>;
        std::vector<render_pkg> pkgs_;
        std::vector<pipeline> pipelines_; // always excute in order

        virtual ~render() = default;

        virtual void construct() = 0;
        virtual func get_frame_func() = 0;
    };
}; // namespace fi::mgr

#endif // MGRS_RENDER_MGR_HPP