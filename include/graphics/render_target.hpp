#ifndef GRAPHICS_RENDER_TARGET_HPP
#define GRAPHICS_RENDER_TARGET_HPP

#include "graphics.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "glms.hpp"

namespace fi
{
    struct SceneResources : private GraphicsObject
    {
        std::vector<vk::Sampler> samplers_{};

        ~SceneResources();
    };

    struct RenderableScene
    {
        std::string name_ = "";
        std::shared_ptr<SceneResources> res_{};
    };

    struct GltfLoader : private GraphicsObject
    {
        struct Vertex
        {
            glm::vec3 position_{};
            glm::vec3 normal_{};
            glm::vec2 tex_coord_{};
            glm::ivec4 bone_ids_{};
            glm::vec4 bone_weight_{};
        };

        static std::array<vk::VertexInputBindingDescription, 2> vertex_bindings();
        static std::array<vk::VertexInputAttributeDescription, 9> vertex_attributes();

        std::vector<RenderableScene> from_file(const std::filesystem::path& path, ImageMgr& img_mgr);
    };

}; // namespace fi

#endif // GRAPHICS_RENDER_TARGET_HPP
