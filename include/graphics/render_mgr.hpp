#ifndef GRAPHICS_RENDER_TARGET_HPP
#define GRAPHICS_RENDER_TARGET_HPP

#include "graphics.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "glms.hpp"

namespace fi
{
    class RenderMgr
    {
      public:
        struct Vertex
        {
            glm::vec3 position_{};     // 0
            glm::vec3 normal_{};       // 1
            glm::vec2 tex_coord_{};    // 2
            glm::ivec4 bone_ids_{};    // 3
            glm::vec4 bone_weights_{}; // 4
        };

        struct MaterialPipeline
        {
            vk::Pipeline pipeline_;
            static vk::PipelineLayout layout_;
        };

        struct Material
        {
        };

      private:
      public:
    };

}; // namespace fi

#endif // GRAPHICS_RENDER_TARGET_HPP
