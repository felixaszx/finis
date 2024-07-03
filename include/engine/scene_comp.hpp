#ifndef ENGINE_SCENE_COMP_HPP
#define ENGINE_SCENE_COMP_HPP

#include <string>
#include <vector>

namespace fi
{
    class RenderMgr;
};

namespace fi::comp
{
    struct Script
    {
    };

    struct RenderTarget
    {
        bool rendering_ = true;
        size_t data_idx_ = ~0;
        std::vector<size_t> rendered_mesh_idxs_{};
        RenderMgr* render_mgr_ptr_ = nullptr;
    };

    struct PhysicalTarget
    {
    };

    struct AudioPlayer
    {
    };

    struct AudioListener
    {
    };
}; // namespace fi::comp

#endif // ENGINE_SCENE_COMP_HPP