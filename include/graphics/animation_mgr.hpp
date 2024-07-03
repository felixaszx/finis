#ifndef GRAPHICS_ANIMATION_MGR_HPP
#define GRAPHICS_ANIMATION_MGR_HPP

#include "graphics.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"
#include "glms.hpp"
#include "render_mgr.hpp"

namespace fi
{
    class AnimationMgr : private GraphicsObject
    {
      private:
        bool locked_ = false;

      public:
        void upload_res(RenderMgr& render_mgr, gltf::Expected<gltf::Asset>& asset)
        {
            for (auto& animation : asset->animations)
            {
                std::cout << animation.name << std::endl;
            }
        }
    };
}; // namespace fi

#endif // GRAPHICS_ANIMATION_MGR_HPP
